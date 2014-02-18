#include "downloadmanager.h"
#include "filemanager.h"
#include "../common/packages.h"
#include "../common/jsonutil.h"
#include "../common/packagemetadata.h"
#include "../operations/operation.h"

#include <qtlog.h>
#include <QNetworkReply>
#include <QAuthenticator>

Q_DECLARE_METATYPE(QSharedPointer<Operation>)

DownloadManager::DownloadManager(Updater *updater) : QObject()
{
    qRegisterMetaType< QSharedPointer<Operation> >();

    // Make a safe copy of important properties
    // This is "required" because this class if run in it's own thread and allow it to be independant at almost no cost
    m_updateDirectory = updater->localRepository();
    m_updateTmpDirectory = updater->tmpDirectory();
    m_updateUrl = updater->remoteRepository();
    m_localRevision = updater->localRevision();
    m_remoteRevision = updater->updateRevision();
    m_username = updater->username();
    m_password = updater->password();

    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &DownloadManager::authenticationRequired);

    // Create the file manager and assign it to a new thread
    m_filemanager = new FileManager();

    // Create the download thread
    {
        OneObjectThread * thread = new OneObjectThread(updater); //< QThread will be garbage collected by QObject parent/childs system
        thread->manage(this); //< Move itself to new thread and enable safe blocking garbage collection
        connect(thread, &QThread::started, this, &DownloadManager::update); //< Autostart the update once thread started
        thread->start();
    }
    {
        OneObjectThread * thread = new OneObjectThread(updater);
        thread->manage(m_filemanager);

        connect(this, &DownloadManager::operationLoaded, m_filemanager, &FileManager::prepareOperation);
        connect(m_filemanager, &FileManager::operationPrepared, this, &DownloadManager::operationPrepared);

        connect(this, &DownloadManager::operationReadyToApply, m_filemanager, &FileManager::applyOperation);
        connect(m_filemanager, &FileManager::operationApplied, this, &DownloadManager::operationApplied);

        connect(this, &DownloadManager::downloadFinished, m_filemanager, &FileManager::downloadFinished);
        connect(m_filemanager, &FileManager::applyFinished, this, &DownloadManager::applyFinished);

        // Safe blocking abort
        thread->start();
    }

    connect(this, &DownloadManager::finished, m_filemanager, &FileManager::deleteLater);
    connect(this, &DownloadManager::finished, this, &DownloadManager::deleteLater);
}

DownloadManager::~DownloadManager()
{

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
    LOG_INFO(tr("update"));
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

        m_packages.fromJsonObject(JsonUtil::fromJson(packagesListRequest->readAll()));

        LOG_INFO(tr("Remote informations analyzed"));

        downloadPath = m_packages.findBestPath(m_localRevision, m_remoteRevision);
        if(downloadPath.isEmpty())
            throw tr("No download path found from %1 to %2").arg(m_localRevision, m_remoteRevision);
        downloadPathPos = 0;
        downloadGlobalOffset = 0;
        downloadGlobalSize = 0;
        foreach(const Package &package, downloadPath)
        {
            downloadGlobalSize += package.size;
        }

        updatePackageLoop();
    }
    catch(const QString & msg)
    {
        LOG_ERROR(tr("Update failed %1").arg(msg));
        failure(msg);
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
        if(failures.size() > 0)
        {
            if(fixingPath.isEmpty())
            {
                // First time we get in this loop
                LOG_INFO(tr("Some parts of the update have gone wrong, fixing..."));
                downloadPath = m_packages.findBestPath("", m_remoteRevision);
                if(downloadPath.isEmpty())
                    failure(tr("No download path found %1").arg(m_remoteRevision));
            }

            int fixedFailures = 0;
            QMap<QString, Failure>::iterator it = failures.begin();
            while(it != failures.end())
            {
                if(it.value() == Fixed)
                {
                    ++fixedFailures;
                }
                else if(it.value() != NonRecoverable && it.value() != FixInProgress)
                {
                    fixingPath = it.key();
                    it.value() = FixInProgress;
                    break;
                }
                ++it;
            }

            if(it != failures.end())
            {
                downloadPathPos = 0;
                const Package & package = downloadPath.at(downloadPathPos);
                metadata.setPackage(package);
                metadataRequest = get(package.metadataUrl());
                connect(metadataRequest, &QNetworkReply::finished, this, &DownloadManager::updatePackageMetadataFinished);
                LOG_INFO(tr("Downloading metadata for %1").arg(package.url()));
            }
            else
            {
                LOG_INFO(tr("Update finished with %1/%2 fixed errors").arg(fixedFailures).arg(failures.size()));
                if(fixedFailures == failures.size())
                    emit updateSucceeded();
                else
                    emit updateFailed(tr("Unable to fixes all errors (%1/%2 fixed)").arg(fixedFailures).arg(failures.size()));
                emit finished();
            }
        }
        else
        {
            LOG_INFO(tr("Update finished without error"));
            emit updateSucceeded();
            emit finished();
        }
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

        if(metadata.operationCount() == 0)
        {
            ++downloadPathPos;
            updatePackageLoop();
            return;
        }

        operationIndex = 0;
        operation.clear();
        preparedOperationCount = 0;

        if(fixingPath.isEmpty())
        {
            foreach(QSharedPointer<Operation> op, metadata.operations())
            {
                // Ask the file manager to take care of pre-apply job
                // Jobs are automatically queued by Qt signals & slots
                emit operationLoaded(op);
            }
        }
        else
        {
            while(operationIndex < metadata.operationCount())
            {
                operation = metadata.operation(operationIndex);
                if(operation->path() == fixingPath)
                {
                    preparedOperationCount = operationIndex + 1;
                    updateDataSetupOperationFile();

                    dataRequest = get(metadata.dataUrl(), operation->offset(), operation->offset() + operation->size());
                    connect(dataRequest, &QNetworkReply::readyRead, this, &DownloadManager::updateDataReadyRead);
                    connect(dataRequest, &QNetworkReply::finished, this, &DownloadManager::updateDataFinished);
                    break;
                }
                ++operationIndex;
            }
        }
    }
    catch(const QString & msg)
    {
        LOG_ERROR(tr("Update failed %1").arg(msg));
        failure(msg);
    }
}

/**
   \brief Filemanager has done is pre-work about this operation
   If the operation download isn't necessary, determine if skipping it is useful
 */
#include <QDebug>
void DownloadManager::operationPrepared(QSharedPointer<Operation> preparedOperation)
{
    Q_ASSERT(preparedOperation == metadata.operation(preparedOperationCount));
    LOG_TRACE(tr("Operation prepared %1").arg(preparedOperation->path()));
    ++preparedOperationCount;
    qDebug() << operationIndex << preparedOperationCount << (operation.isNull() ? QString("NULL") : operation->path());

    if(operationIndex >= preparedOperationCount)
    {
        // preparedOperation have already been downloaded
        if(preparedOperation->status() == Operation::DownloadRequired)
        {
            LOG_TRACE(tr("Operation ready to apply (%1), already downloaded").arg(preparedOperation->path()));
            if(QFile(operation->dataDownloadFilename()).rename(operation->dataFilename()))
                emit operationReadyToApply(preparedOperation);
            else
                failure(operation->path(), DownloadRenameFailed);
        }
        else if(preparedOperation->status() == Operation::ApplyRequired)
        {
            QFile::remove(operation->dataDownloadFilename());
            LOG_TRACE(tr("Operation ready to apply (%1), already downloaded was ready to apply").arg(preparedOperation->path()));
            emit operationReadyToApply(preparedOperation);
        }
    }
    else if(preparedOperation->status() == Operation::DownloadRequired)
    {
        if(operation == nullptr)
        {
            // No download is in progress, start a new one

            Q_ASSERT(operationIndex + 1 == preparedOperationCount);

            // Setup the current operation
            operation = metadata.operation(operationIndex);
            updateDataSetupOperationFile();

            // Start the download
            updateDataStartDownload();
        }
    }
    else if(operation != nullptr)
    {
        // There is a download in progress and preparedOperation doesn't require to be downloaded

        if(operationIndex < preparedOperationCount && operation->status() != Operation::DownloadRequired)
        {
            // We may be able to optimise the download time

            // Queue apply the current operation if it's possible
            if(operation == preparedOperation && operation->status() == Operation::ApplyRequired)
            {
                LOG_TRACE(tr("Operation ready to apply (%1), download aborted").arg(operation->path()));
                emit operationReadyToApply(operation);
            }

            // preparedOperation can influence the current download
            // test if it's worth continuing the current download
            tryContinueDownload(operation->size() - offset);
        }
    }
    else
    {
        // No download is in progress, and no one is yet required

        Q_ASSERT(operationIndex + 1 == preparedOperationCount);

        // Queue apply of the current operatin if it's possible
        if(preparedOperation->status() == Operation::ApplyRequired)
        {
            LOG_TRACE(tr("Operation ready to apply (%1), no download required").arg(preparedOperation->path()));
            emit operationReadyToApply(preparedOperation);
        }

        nextOperation();
    }

    if(preparedOperation->status() == Operation::LocalFileInvalid)
    {
        failure(preparedOperation->path(), LocalFileInvalid);
    }
}

void DownloadManager::nextOperation()
{
    operation = metadata.operation(++operationIndex);
    if(operationIndex == metadata.operationCount())
    {
        LOG_TRACE(tr("downloadFinished, cause operationIndex == metadata.operationCount() == %1").arg(operationIndex));
        emit downloadFinished();
    }
}



void DownloadManager::operationApplied(QSharedPointer<Operation> appliedOperation)
{
    LOG_TRACE(tr("Operation applied %1").arg(appliedOperation->path()));
    if(appliedOperation->status() != Operation::Valid)
    {
        failure(appliedOperation->path(), fixingPath.isEmpty() ? ApplyFailed : NonRecoverable);
    }
    else if(!fixingPath.isEmpty())
    {
        failure(appliedOperation->path(), Fixed);
    }
}

void DownloadManager::applyFinished()
{
    ++downloadPathPos;
    LOG_TRACE(tr("Apply %1/%2 finished").arg(downloadPathPos).arg(downloadPath.size()));
    updatePackageLoop();
}

void DownloadManager::failure(const QString &path, Failure reason)
{
    LOG_WARN(tr("Operation failed %1 (%2)").arg(path).arg(reason));
    failures.insert(path, reason);
}

void DownloadManager::failure(const QString &reason)
{
    emit updateFailed(reason);
    emit finished();
}

bool DownloadManager::isSkipDownloadUseful(qint64 skippableSize)
{
    return skippableSize > 1024*1024; // 1MB
}

/**
   \brief Test if it's worth continuing the current download
   If abort the current download result in a gain of time, the download is aborted
   If a prepared operation that require download is found, download is restarted at that position
   Otherwise, preparedOperation will take care of restarting the download if needed

   \warning Returns false even if the download has been restarted,
            you can test if download is started by checking if operation != nullptr
   \param skippableSize Amount of data that can be skipped off the current operation
   \return True if the current download has been stopped, false otherwise
 */
bool DownloadManager::tryContinueDownload(qint64 skippableSize)
{
    Q_ASSERT(operation->status() != Operation::DownloadRequired);
    Q_ASSERT(operationIndex < metadata.operationCount());

    // Compute the amount of data that can be skipped
    int i = operationIndex + 1;
    if(i == metadata.operationCount())
    {
        // It was the last download, we can for sure stop the download here
        updateDataStopDownload();
        nextOperation();
        return false;//< Download has been stopped
    }
    while(i < preparedOperationCount && metadata.operation(i)->status() != Operation::DownloadRequired)
    {
        skippableSize += metadata.operation(i)->size();
        ++i;
    }

    // test if it's worth skipping the download of that part
    if(isSkipDownloadUseful(skippableSize))
    {
        // Stopping the current download is worth it

        // Stop the download here
        updateDataStopDownload();

        // Find the next download start point
        nextOperation();
        while(operationIndex < preparedOperationCount)
        {
            if(operation->status() == Operation::DownloadRequired)
            {
                // Restart the download
                updateDataSetupOperationFile();
                updateDataStartDownload();
                break;
            }
            else if(operation->status() == Operation::ApplyRequired)
            {
                LOG_TRACE(tr("Operation ready to apply (%1), no download required").arg(operation->path()));
                emit operationReadyToApply(operation);
            }
            nextOperation();
        }

        return false;//< Download has been stopped
    }

    return true; //< Download continue
}

void DownloadManager::updateDataSetupOperationFile()
{
    offset = 0;

    if(operation != nullptr)
    {
        file.setFileName(operation->dataDownloadFilename());
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw(tr("Unable to open datafile for writing : %1").arg(file.fileName()));
    }
}

void DownloadManager::updateDataReadyRead()
{
    Q_ASSERT(operation != nullptr);

    // Read all available data
    QByteArray bytes = dataRequest->readAll();
    const char * data = bytes.constData();
    qint64 writeSize, size = bytes.size();

    if(downloadSeek > 0)
    {
        if(size <= downloadSeek)
        {
            downloadSeek -= size;
            return;
        }
        data += downloadSeek;
        size -= downloadSeek;
        downloadSeek = 0;
    }

    downloadGlobalOffset += size;
    emit downloadProgress(downloadGlobalOffset, downloadGlobalSize);

    while(size > 0)
    {
        writeSize = qMin(size, operation->size() - offset);
        if(writeSize > 0)
        {
            if(operationIndex >= preparedOperationCount || operation->status() == Operation::DownloadRequired)
            {
                file.write(data, writeSize);
            }
            offset += writeSize;
            data += writeSize;
            size -= writeSize;
        }

        if(offset == operation->size())
        {
            // Operation downloaded
            LOG_TRACE(tr("Operation downloaded %1").arg(operation->path()));

            if(file.isOpen())
            {
                // Close the file
                if(!file.flush())
                    throw(tr("Unable to flush all data %1").arg(file.fileName()));
                file.close();
            }
            Q_ASSERT(file.size() == operation->size());

            if(!fixingPath.isEmpty())
            {
                LOG_TRACE(tr("Operation ready to apply (%1), download done (fixing)").arg(operation->path()));
                if(file.rename(operation->dataFilename()))
                    emit operationReadyToApply(operation);
                else
                    failure(operation->path(), DownloadRenameFailed);
                LOG_TRACE(tr("downloadFinished, cause fixingPath == %1").arg(fixingPath));
                emit downloadFinished();
                return;
            }

            if(operationIndex < preparedOperationCount && operation->status() == Operation::DownloadRequired)
            {
                LOG_TRACE(tr("Operation ready to apply (%1, %2), download done").arg(operation->path(), operation->dataFilename()));
                if(file.rename(operation->dataFilename()))
                    emit operationReadyToApply(operation);
                else
                    failure(operation->path(), DownloadRenameFailed);
            }

            // Find what to do next
            nextOperation();
            while(operationIndex < preparedOperationCount)
            {
                if(operation->status() == Operation::DownloadRequired)
                {
                    // Setup the current operation
                    updateDataSetupOperationFile();
                    break;
                }
                else if(operation->status() == Operation::ApplyRequired)
                {
                    LOG_TRACE(tr("Operation ready to apply (%1), no download required").arg(operation->path()));
                    emit operationReadyToApply(operation);
                }

                if(operation->size() > 0)
                {
                    // operation->status() != Operation::DownloadRequired
                    if(operation->size() < size)
                    {
                        // Consume downloaded data
                        size -= operation->size();
                        data += operation->size();
                    }
                    else
                    {
                        // Test if it's worth waiting for more data
                        if(!tryContinueDownload(operation->size() - size))
                        {
                            // Download has been stopped, let's do the same
                            return;
                        }
                    }
                }
                nextOperation();
            }

            if(operationIndex >= preparedOperationCount)
            {
                // We don't know yet if it's worth continuing the download
                // operationPrepared with preparedOperationCount <= operationIndex will occur
                // Setup the current operation anyway
                updateDataSetupOperationFile();
            }
        }
    }
}

/**
   \brief Current download reply has finished
 */
void DownloadManager::updateDataFinished()
{
    if(operationIndex < metadata.operationCount())
    {
        // This is never expected, even in the case the download chunk was limited,
        // because the updateDataReadyRead function should have aborted the reply
        // once all expected bytes are received

        if(dataRequest->error() != QNetworkReply::NoError)
        {
            // TODO : Recover in case of connection lost
        }
        else
        {
            // Package file is probably corrupted or a bug occured
        }

        // Restart download
        //updateDataStopDownload();
        //updateDataStartDownload();

    }
}

void DownloadManager::updateDataStartDownload()
{
    Q_ASSERT(operation == metadata.operation(operationIndex));

    qint64 skippableSize = 0;
    qint64 endOffset = 0;
    int i = operationIndex + 1;
    while(i < preparedOperationCount)
    {
        // Find the next skip position
        while(i < preparedOperationCount && metadata.operation(i)->status() == Operation::DownloadRequired)
            ++i;

        // Compute how much data can be skipped
        while(i < preparedOperationCount && metadata.operation(i)->status() != Operation::DownloadRequired)
        {
            skippableSize += metadata.operation(i)->size();
            ++i;
        }

        if(isSkipDownloadUseful(skippableSize))
        {
            // Skipping those data is useful
            endOffset = metadata.operation(i)->offset();
            break;
        }
    }

    dataRequest = get(metadata.dataUrl(), operation->offset(), endOffset);
    connect(dataRequest, &QNetworkReply::readyRead, this, &DownloadManager::updateDataReadyRead);
    connect(dataRequest, &QNetworkReply::finished, this, &DownloadManager::updateDataFinished);
}

void DownloadManager::updateDataStopDownload()
{
    disconnect(dataRequest, &QNetworkReply::readyRead, this, &DownloadManager::updateDataReadyRead);
    disconnect(dataRequest, &QNetworkReply::finished, this, &DownloadManager::updateDataFinished);

    file.close();

    dataRequest->abort();
    dataRequest->deleteLater();
    dataRequest = nullptr;
}

QNetworkReply *DownloadManager::get(const QString &what, qint64 startPosition, qint64 endPosition)
{
    QUrl url(m_updateUrl + what);
    QNetworkRequest request(url);

    if(startPosition > 0)
    {
        if(endPosition > 0 && endPosition > startPosition)
        {
            request.setRawHeader("Range", QStringLiteral("bytes=%1-%2").arg(startPosition).arg(endPosition).toLatin1());
        }
        else
        {
            request.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(startPosition).toLatin1());
        }
    }

    LOG_TRACE(tr("get(%1,%2,%3)").arg(what).arg(startPosition).arg(endPosition));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    downloadSeek = startPosition > 0 && url.isLocalFile() ? startPosition : 0;

    return reply;
}

void DownloadManager::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password().isEmpty())
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user().isEmpty())
        authenticator->setUser(m_username);
}
