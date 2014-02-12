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
    m_updateDirectory = updater->updateDirectory();
    m_updateTmpDirectory = updater->updateTmpDirectory();
    m_updateUrl = updater->updateUrl();
    m_localRevision = updater->localRevision();
    m_remoteRevision = updater->remoteRevision();
    m_username = updater->username();
    m_password = updater->password();

    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &DownloadManager::authenticationRequired);

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

void DownloadManager::update()
{
    packagesListRequest = get(QStringLiteral("packages"));
    connect(packagesListRequest, &QNetworkReply::finished, this, &DownloadManager::updatePackagesListRequestFinished);
}

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

void DownloadManager::updatePackageMetadataFinished()
{
    try
    {
        if(metadataRequest->error() != QNetworkReply::NoError)
            throw metadataRequest->errorString();

        LOG_INFO(tr("Metadata downloaded"));

        metadata.fromJsonObject(JsonUtil::fromJson(metadataRequest->readAll()));
        metadata.setup(m_updateDirectory, m_updateTmpDirectory);

        foreach(Operation * op, metadata.operations())
        {
            emit operationLoaded(op);
        }

        LOG_INFO(tr("Metadata analyzed and prepared"));

        operationIndex = 0;
        updateDataSetupOperationFile();

        dataRequest = get(metadata.dataUrl(), 0);
        connect(dataRequest, SIGNAL(downloadProgress(qint64,qint64)), SLOT(updateDataDownloadProgress(qint64,qint64)));
        connect(dataRequest, SIGNAL(finished()), SLOT(updateDataFinished()));

        LOG_INFO(tr("Downloading data"));
    }
    catch(const QString & msg)
    {
        LOG_ERROR(tr("Update failed %1").arg(msg));
        emit failure(msg);
    }
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
    operation = metadata.operation(operationIndex);
    if(operation != nullptr)
    {
        offset = 0;
        if(!operation->dataFile()->open(QFile::WriteOnly | QFile::Truncate))
            throw(tr("Unable to open datafile for writing : %1").arg(operation->dataFilename()));
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
            operation->dataFile()->write(data, writeSize);
            offset += writeSize;
            data += writeSize;
            size -= writeSize;
        }
        if(offset == operation->size())
        {
            emit operationDownloaded(operation);
            ++operationIndex;
            updateDataSetupOperationFile();
        }
    }
}


QNetworkReply *DownloadManager::get(const QString &what, qint64 startPosition)
{
    QNetworkRequest request(QUrl(m_updateUrl.arg(what)));
    if(startPosition > 0)
    {
        request.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(startPosition).toLatin1());
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
