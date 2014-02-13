#include "downloadmanager.h"
#include "filemanager.h"
#include "../common/packages.h"
#include "../common/jsonutil.h"
#include "../common/packagemetadata.h"
#include "../operations/operation.h"

#include <qtlog.h>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QThread>

DownloadManager::DownloadManager(Updater *updater)
    : QObject()
{
    // Make a safe copy of important properties
    // This is "required" because this class if run in it's own thread and allow it to be independant at almost no cost
    m_updateDirectory = updater->updateDirectory();
    m_updateTmpDirectory = updater->updateTmpDirectory();
    m_updateUrl = updater->updateUrl();
    m_localRevision = updater->localRevision();
    m_remoteRevision = updater->remoteRevision();
    m_username = updater->username();
    m_password = updater->password();

    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &DownloadManager::authenticationRequired);

    // Create the file manager and assign it to a new thread
    m_filemanager = new FileManager();
    QThread * thread = new QThread(this);
    m_filemanager->moveToThread(thread);

    connect(this, &DownloadManager::operationLoaded, m_filemanager, &FileManager::prepareOperation);
    connect(m_filemanager, &FileManager::operationPrepared, this, &DownloadManager::operationPrepared);

    connect(this, &DownloadManager::operationDownloaded, m_filemanager, &FileManager::applyOperation);
    connect(m_filemanager, &FileManager::operationApplied, this, &DownloadManager::operationApplied);

    connect(this, &DownloadManager::downloadFinished, m_filemanager, &FileManager::downloadFinished);
    connect(m_filemanager, &FileManager::applyFinished, this, &DownloadManager::applyFinished);

    connect(this, &DownloadManager::destroyed, m_filemanager, &FileManager::deleteLater);
    connect(thread, &QThread::destroyed, m_filemanager, &FileManager::deleteLater);
    thread->start();
}

/**
   \brief Start the update sequence
   update() : Get packages list
    -> updatePackagesListRequestFinished() : Compute shortest path
    -> updatePackageLoop() : iterate over package path
        -> updatePackageMetadataFinished() : load metadata & start package download
        -> updateDataFinished() : finish package application
   \sa updatePackagesListRequestFinished()
 */
void DownloadManager::update()
{
    packagesListRequest = get(QStringLiteral("packages"));
    connect(packagesListRequest, &QNetworkReply::finished, this, &DownloadManager::updatePackagesListRequestFinished);
}

/**
   \brief Handle the downloaded packages list
   Parse the packages list
   Compute the shortest path
   Start the update loop (updatePackageLoop())
   \sa updatePackageLoop()
 */
void DownloadManager::updatePackagesListRequestFinished()
{
    try
    {
        if(packagesListRequest->error() != QNetworkReply::NoError)
            throw packagesListRequest->errorString();

        LOG_INFO(tr("Packages list downloaded"));

        Packages packages;
        packages.fromJsonObject(JsonUtil::fromJson(packagesListRequest->readAll()));

        LOG_INFO(tr("Remote informations analyzed"));

        downloadPath = packages.findBestPath(m_localRevision, m_remoteRevision);
        downloadPathPos = 0;

        updatePackageLoop();
    }
    catch(const QString & msg)
    {
        LOG_ERROR(tr("Update failed %1").arg(msg));
        emit failure(msg);
    }
}

/**
   \brief Iterate over the packages path to reach RemoteRevision
   Find the next package to download & apply
   Start the download of the package metadata
   \sa updatePackageMetadataFinished()
 */
void DownloadManager::updatePackageLoop()
{
    if(downloadPathPos < downloadPath.size())
    {
        const Package & package = downloadPath.at(downloadPathPos);
        metadata.setPackage(package);
        metadataRequest = get(package.metadataUrl());
        connect(metadataRequest, &QNetworkReply::finished, this, &DownloadManager::updatePackageMetadataFinished);
        LOG_INFO(tr("Downloading metadata for %1").arg(package.url()));
    }
    else
    {
        LOG_INFO(tr("Main update finished, checking for errors"));
    }
}

/**
   \brief Handle the download package metadata
   Parse package metadata's
   Ask the file manager to take care of pre-apply job
   Start the package download
 */
void DownloadManager::updatePackageMetadataFinished()
{
    try
    {
        if(metadataRequest->error() != QNetworkReply::NoError)
            throw metadataRequest->errorString();

        LOG_INFO(tr("Metadata downloaded"));

        metadata.fromJsonObject(JsonUtil::fromJson(metadataRequest->readAll()));
        metadata.setup(m_updateDirectory, m_updateTmpDirectory);

        operationIndex = 0;
        operation = nullptr;
        preparedOperationCount = 0;
        foreach(Operation * op, metadata.operations())
        {
            // Ask the file manager to take care of pre-apply job
            // Jobs are automatically queued by Qt signals & slots
            emit operationLoaded(op);
        }

    }
    catch(const QString & msg)
    {
        LOG_ERROR(tr("Update failed %1").arg(msg));
        emit failure(msg);
    }
}

/**
   \brief Filemanager has done is pre-work about this operation
   If the operation download isn't necessary, determine if skipping it is worthless
 */
void DownloadManager::operationPrepared(Operation *preparedOperation)
{
    Q_ASSERT(preparedOperation == metadata.operation(preparedOperationCount));
    ++preparedOperationCount;
    if(preparedOperation->status() == Operation::DownloadRequired)
    {
        if(operation == nullptr)
        {
            // No download is in progress, start a new one
            Q_ASSERT(operationIndex + 1 == preparedOperationCount);

            operation = metadata.operation(operationIndex);
            int i = operationIndex + 1;
            while(i < preparedOperationCount && metadata.operation(i)->status() == Operation::DownloadRequired)
                ++i;
            qint64 endOffset = -1;
            if(i < preparedOperationCount && metadata.operation(i)->status() != Operation::DownloadRequired)
                endOffset = metadata.operation(i)->offset();

            dataRequest = get(metadata.dataUrl(), operation->offset(), endOffset);
            connect(dataRequest, SIGNAL(downloadProgress(qint64,qint64)), SLOT(updateDataDownloadProgress(qint64,qint64)));
            connect(dataRequest, SIGNAL(finished()), SLOT(updateDataFinished()));
        }
    }
    else if(operation != nullptr)
    {
        if(operationIndex < preparedOperationCount && operation->status() != Operation::DownloadRequired)
        {
            // Compute the amount of data that can be skipped from download
            qint64 notRequiredSize = operation->size() - offset;
            int i = operationIndex + 1;
            while(i < preparedOperationCount && metadata.operation(i)->status() != Operation::DownloadRequired)
            {
                notRequiredSize += metadata.operation(i)->size();
                ++i;
            }
            if(worthless(notRequiredSize))
            {
                if(preparedOperation->status() == Operation::ApplyRequired)
                    emit operationDownloaded(preparedOperation);

                // Stop the download here
                dataRequest->abort();

                // Find the next download start point
                updateDataNextOperation();
            }
        }
    }
}

void DownloadManager::updateDataNextOperation()
{
    ++operationIndex;
    int i = operationIndex;
    qint64 notRequiredSize = 0;
    while(i < preparedOperationCount && metadata.operation(i)->status() != Operation::DownloadRequired)
    {
        notRequiredSize += metadata.operation(i)->size();
        ++i;
    }
    if(worthless(notRequiredSize))
    {
        dataRequest->abort();
        updateDataSetupOperationFile();

        dataRequest = get(metadata.dataUrl(), 0);
        connect(dataRequest, SIGNAL(downloadProgress(qint64,qint64)), SLOT(updateDataDownloadProgress(qint64,qint64)));
        connect(dataRequest, SIGNAL(finished()), SLOT(updateDataFinished()));
        // Stop the download here
        // Restart the download at i if i < preparedOperationCount
        // Otherwise wait for the fileManager to
        operationIndex = i;
    }
    operation = metadata.operation(operationIndex);
    updateDataSetupOperationFile();
}

void DownloadManager::updateDataDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);

    Q_ASSERT(bytesTotal == metadata.size());

    if(dataRequest->error() != QNetworkReply::NoError)
        return;

    updateDataReadAll();
}

void DownloadManager::updateDataFinished()
{
    try
    {
        if(dataRequest->error() != QNetworkReply::NoError)
        {
            // TODO : Recover in case of connection lost
            throw dataRequest->errorString();
        }

        emit downloadProgress(metadata.size(), metadata.size());

        updateDataReadAll();

        LOG_INFO(tr("Data downloaded"));

        while(operation != nullptr)
        {
            LOG_ERROR(tr("Some data are missing for %1").arg(operationIndex));
            failures.insert(operation->path(), DownloadFailed);
            operation = metadata.operation(++operationIndex);
        }

        emit downloadFinished();
    }
    catch(const QString & msg)
    {
        LOG_ERROR(tr("Update failed %1").arg(msg));
        emit failure(msg);
    }
}


void DownloadManager::updateDataSetupOperationFile()
{
    if(file.isOpen())
    {
        if(!file.flush())
            throw(tr("Unable to flush all data %1").arg(file.fileName()));
        file.close();
    }

    offset = 0;

    if(operation != nullptr)
    {
        file.setFileName(operation->dataFilename());
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw(tr("Unable to open datafile for writing : %1").arg(file.fileName()));
    }
}

void DownloadManager::updateDataReadAll()
{
    QByteArray bytes = dataRequest->readAll();
    const char * data = bytes.constData();
    qint64 writeSize, size = bytes.size();
    while(size > 0)
    {
        if(operation == nullptr)
        {
            LOG_ERROR(tr("No more operation while there is still data to read"));
            return;
        }
        writeSize = qMin(size, operation->size() - offset);
        if(writeSize > 0)
        {
            file.write(data, writeSize);
            offset += writeSize;
            data += writeSize;
            size -= writeSize;
        }
        if(offset == operation->size())
        {
            emit operationDownloaded(operation);
            updateDataNextOperation();
        }
    }
}

QNetworkReply *DownloadManager::get(const QString &what, qint64 startPosition, qint64 endPosition)
{
    QNetworkRequest request(QUrl(m_updateUrl.arg(what)));

    if(startPosition > 0)
    {
        if(endPosition > 0)
        {
            request.setRawHeader("Range", QStringLiteral("bytes=%1-%2").arg(startPosition).arg(endPosition).toLatin1());
        }
        else
        {
            request.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(startPosition).toLatin1());
        }
    }

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    return reply;
}

void DownloadManager::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password().isEmpty())
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user().isEmpty())
        authenticator->setUser(m_username);
}
