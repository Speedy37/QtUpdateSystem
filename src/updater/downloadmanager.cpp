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

    FileManager *filemanager = new FileManager();

    // Create the download thread
    {
        OneObjectThread * thread = new OneObjectThread(updater); //< QThread will be garbage collected by QObject parent/childs system
        thread->manage(this); //< Move itself to new thread and enable safe blocking garbage collection
        connect(thread, &QThread::started, this, &DownloadManager::update); //< Autostart the update once thread started
        thread->start();
    }
    {
        OneObjectThread * thread = new OneObjectThread(updater);
        thread->manage(filemanager);

        connect(this, &DownloadManager::operationLoaded, filemanager, &FileManager::prepareOperation);
        connect(filemanager, &FileManager::operationPrepared, this, &DownloadManager::operationPrepared);

        connect(this, &DownloadManager::operationReadyToApply, filemanager, &FileManager::applyOperation);
        connect(filemanager, &FileManager::operationApplied, this, &DownloadManager::operationApplied);

        connect(this, &DownloadManager::downloadFinished, filemanager, &FileManager::downloadFinished);
        connect(filemanager, &FileManager::applyFinished, this, &DownloadManager::applyFinished);

        // Safe blocking abort
        thread->start();
    }

    connect(this, &DownloadManager::finished, filemanager, &FileManager::deleteLater);
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
            else
            {
                fixingPath = QString();
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

            if(!fixingPath.isEmpty())
            {
                downloadPathPos = 0;
                const Package & package = downloadPath.at(downloadPathPos);
                metadata.setPackage(package);
                metadataRequest = get(package.metadataUrl());
                connect(metadataRequest, &QNetworkReply::finished, this, &DownloadManager::updatePackageMetadataFinished);
                LOG_INFO(tr("Downloading metadata for %1").arg(fixingPath));
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

        preparedOperationIndex = -1;
        operationIndex = 0;
        operation = metadata.operation(0);
        dataRequest = nullptr;

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
                    preparedOperationIndex = operationIndex;
                    updateDataStartDownload(operation->offset() + operation->size());
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

void DownloadManager::readyToApply(QSharedPointer<Operation> readyOperation)
{
    if(readyOperation->status() == Operation::DownloadRequired || !fixingPath.isEmpty())
    {
        LOG_TRACE(tr("Operation ready to apply (%1), already downloaded").arg(readyOperation->path()));
        if(QFile(readyOperation->dataDownloadFilename()).rename(readyOperation->dataFilename()))
            emit operationReadyToApply(readyOperation);
        else
            failure(readyOperation->path(), DownloadRenameFailed);
    }
    else if(readyOperation->status() == Operation::ApplyRequired)
    {
        QFile::remove(readyOperation->dataDownloadFilename());
        LOG_TRACE(tr("Operation ready to apply (%1), already downloaded was ready to apply").arg(readyOperation->path()));
        emit operationReadyToApply(readyOperation);
    }
}

void DownloadManager::failure(const QString &path, Failure reason)
{
    LOG_WARN(tr("Operation failed %1 (%2)").arg(path).arg(reason));
    failures.insert(path, reason);
}

/**
   \brief Find the next operation
   - dataRequest can be null if no download is in progress
   - if dataRequest != nullptr, downloadSeek is the amount of download that will be wasted
 */
void DownloadManager::nextOperation()
{
    operation = metadata.operation(++operationIndex);

    if(dataRequest == nullptr)
    {
        // We must find the next operation that is prepared and requires download
        while(!operation.isNull() && operationIndex <= preparedOperationIndex)
        {
            if(operation->status() == Operation::DownloadRequired)
            {
                // Restart the download
                updateDataStartDownload();
                break;
            }
            else if(operation->status() == Operation::ApplyRequired)
            {
                readyToApply(operation);
            }
            operation = metadata.operation(++operationIndex);
        }
    }
    else
    {
        // We must find the next operation to download
        // First we skip all the nullsized operations
        while(!operation.isNull() && operation->size() == 0)
        {
            if(operationIndex <= preparedOperationIndex)
            {
                readyToApply(operation);
            }
            operation = metadata.operation(++operationIndex);
        }

        // 2nd we compute the amount of data that can be skipped from download
        while(!operation.isNull() && operationIndex <= preparedOperationIndex && operation->status() != Operation::DownloadRequired)
        {
            downloadSeek += operation->size();
            readyToApply(operation);
            operation = metadata.operation(++operationIndex);
        }

        if(operation.isNull())
        {
            updateDataStopDownload();
        }
        else if(isSkipDownloadUseful(downloadSeek))
        {
            if(operationIndex <= preparedOperationIndex)
            {
                Q_ASSERT(operation->status() == Operation::DownloadRequired);
                // Restart the download
                updateDataStartDownload();
            }
        }
        else
        {
            file.setFileName(operation->dataDownloadFilename());
            if(!file.open(QFile::WriteOnly | QFile::Truncate))
                throw(tr("Unable to open datafile for writing : %1").arg(file.fileName()));

            offset = 0;
        }
    }
    if(operation.isNull() && preparedOperationIndex + 1 == metadata.operationCount())
    {
        LOG_TRACE(tr("downloadFinished, cause %1 %2 + 1 == %3").arg(operationIndex).arg(preparedOperationIndex).arg(metadata.operationCount()));
        emit downloadFinished();
    }
}

/**
   \brief Filemanager has done is pre-work about this operation
   If the operation download isn't necessary, determine if skipping it is useful
 */
void DownloadManager::operationPrepared(QSharedPointer<Operation> preparedOperation)
{
    ++preparedOperationIndex;
    Q_ASSERT(preparedOperation == metadata.operation(preparedOperationIndex));

    LOG_TRACE(tr("Operation prepared %1").arg(preparedOperation->path()));
    LOG_TRACE(tr("opIndex = %1, prepIndex = %2, op = %3").arg(operationIndex).arg(preparedOperationIndex).arg(operation.isNull() ? QString("NULL") : operation->path()));

    // [--DL--][--Prepared--][--Apply--]
    if(preparedOperationIndex < operationIndex)
    {
        Q_ASSERT(preparedOperation != operation);
        readyToApply(preparedOperation);
        if(operationIndex == metadata.operationCount() && preparedOperationIndex + 1 == operationIndex)
        {
            LOG_TRACE(tr("downloadFinished, cause %1 == preparedOperationIndex + 1 == operationIndex == metadata.operationCount()").arg(operationIndex));
            emit downloadFinished();
        }
    }
    // [--DL--------------]
    //   [--Prepared--]    [--Apply--] if DownloadRequired
    //   [--Prepared--][--Apply--]     if !DownloadRequired
    else if(preparedOperationIndex == operationIndex)
    {
        Q_ASSERT(preparedOperation == operation);
        if(dataRequest == nullptr)
        {
            if(preparedOperation->status() == Operation::DownloadRequired)
            {
                // No download is in progress, start a new one
                updateDataStartDownload();
            }
            else
            {
                readyToApply(preparedOperation);
                nextOperation();
            }
        }
        else if(preparedOperation->status() != Operation::DownloadRequired)
        {
            // There is a download in progress and preparedOperation doesn't require to be downloaded
            // We may be able to optimise the download time

            // Queue apply the current operation if its possible
            readyToApply(preparedOperation);

            downloadSeek += preparedOperation->size() - offset;
            nextOperation();
        }
        //else
        //{
        //    preparedOperation->status() == Operation::DownloadRequired
        //    Let the updateDataReadyRead finish to handle operation
        //}
    }
    // [--Prepared--][--DL--][--Apply--]
    //else if(preparedOperationIndex > operationIndex)
    //{
    //    Nothing can be done, because previous operations aren't downloaded
    //}

    if(preparedOperation->status() == Operation::LocalFileInvalid)
    {
        failure(preparedOperation->path(), LocalFileInvalid);
    }
}

void DownloadManager::updateDataReadyRead()
{
    Q_ASSERT(dataRequest != nullptr);
    Q_ASSERT(operation != nullptr);
    Q_ASSERT(operation->size() > 0);

    // Read all available data
    qint64 size;
    qint64 availableSize = dataRequest->bytesAvailable();

    if(downloadSeek > 0)
    {
        if(availableSize <= downloadSeek)
        {
            downloadSeek -= availableSize;
            return;
        }
        dataRequest->read(downloadSeek);
        availableSize -= downloadSeek;
        downloadSeek = 0;
    }

    while(availableSize > 0)
    {
        size = operation->size() - offset;
        if(size <= availableSize)
        {
            file.write(dataRequest->read(size));
            operationDownloaded();
            if(dataRequest == nullptr)
                return; //< Futures download aborted
            availableSize -= size;
        }
        else
        {
            file.write(dataRequest->read(availableSize));
            offset += availableSize;
            availableSize = 0;
        }
    }
}

void DownloadManager::operationDownloaded()
{
    LOG_TRACE(tr("Operation downloaded %1").arg(operation->path()));

    Q_ASSERT(file.isOpen());
    Q_ASSERT(file.size() == operation->size());

    file.close();

    if(!fixingPath.isEmpty())
    {
        updateDataStopDownload();
        readyToApply(operation);
        LOG_TRACE(tr("downloadFinished, cause fixingPath == %1").arg(fixingPath));
        emit downloadFinished();
        return;
    }

    // [--Prepared--]
    //        [--DL--][--Apply--]
    if(preparedOperationIndex >= operationIndex)
    {
        readyToApply(operation);
    }

    // Find what to do next
    nextOperation();
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

void DownloadManager::failure(const QString &reason)
{

    LOG_TRACE(reason);
    emit updateFailed(reason);
    emit finished();
}

bool DownloadManager::isSkipDownloadUseful(qint64 skippableSize)
{
    return skippableSize > 1024*1024; // 1MB
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
    while(i <= preparedOperationIndex)
    {
        // Find the next skip position
        while(i <= preparedOperationIndex && metadata.operation(i)->status() == Operation::DownloadRequired)
            ++i;

        // Compute how much data can be skipped
        while(i <= preparedOperationIndex && metadata.operation(i)->status() != Operation::DownloadRequired)
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

    updateDataStartDownload(endOffset);
}

void DownloadManager::updateDataStartDownload(qint64 endOffset)
{
    Q_ASSERT(operation == metadata.operation(operationIndex));

    file.setFileName(operation->dataDownloadFilename());
    if(!file.open(QFile::WriteOnly | QFile::Truncate))
        throw(tr("Unable to open datafile for writing : %1").arg(file.fileName()));

    offset = 0;
    downloadSeek = 0;
    dataRequest = get(metadata.dataUrl(), operation->offset(), endOffset);
    connect(dataRequest, &QNetworkReply::readyRead, this, &DownloadManager::updateDataReadyRead);
    connect(dataRequest, &QNetworkReply::finished, this, &DownloadManager::updateDataFinished);
}

void DownloadManager::updateDataStopDownload()
{
    disconnect(dataRequest, &QNetworkReply::readyRead, this, &DownloadManager::updateDataReadyRead);
    disconnect(dataRequest, &QNetworkReply::finished, this, &DownloadManager::updateDataFinished);

    file.close();

    downloadSeek = 0;
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
