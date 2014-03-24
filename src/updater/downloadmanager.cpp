#include "downloadmanager.h"
#include "oneobjectthread.h"
#include "filemanager.h"
#include "../common/packages.h"
#include "../common/jsonutil.h"
#include "../common/packagemetadata.h"
#include "../operations/operation.h"

#include <QLoggingCategory>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QDir>
#include <QTemporaryDir>

Q_LOGGING_CATEGORY(LOG_DLMANAGER, "updatesystem.updater.dlmanager")

Q_DECLARE_METATYPE(QSharedPointer<Operation>)
int OperationPointerMetaType = qMetaTypeId< QSharedPointer<Operation> >();

DownloadManager::DownloadManager(const LocalRepository &sourceRepository, Updater *updater) :
    QObject(), m_localRepository(sourceRepository), m_temporaryDir(nullptr)
{

    // Make a safe copy of important properties
    // This is "required" because this class if run in it's own thread and allow it to be independant at almost no cost
    m_updateDirectory = updater->localRepository();
    m_updateTmpDirectory = updater->tmpDirectory();
    m_updateUrl = updater->remoteRepository();
    m_localRevision = updater->localRevision();
    m_remoteRevision = updater->remoteRevision();
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

        connect(filemanager, &FileManager::warning, this, &DownloadManager::warning);

        // Safe blocking abort
        thread->start();
    }

    connect(this, &DownloadManager::finished, filemanager, &FileManager::deleteLater);
    connect(this, &DownloadManager::finished, this, &DownloadManager::deleteLater);
}

DownloadManager::~DownloadManager()
{
    delete m_temporaryDir;
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
    if(m_updateTmpDirectory.isEmpty())
    {
        m_temporaryDir = new QTemporaryDir;
        if(!m_temporaryDir->isValid())
        {
            emit finished(tr("Unable to create a temporary directory"));
            return;
        }
        m_updateTmpDirectory = Utils::cleanPath(m_temporaryDir->path());
    }

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

        qCDebug(LOG_DLMANAGER) << "Packages list downloaded";

        m_packages.fromJsonObject(JsonUtil::fromJson(packagesListRequest->readAll()));

        qCDebug(LOG_DLMANAGER) << "Remote informations analyzed";

        if(m_localRevision != m_remoteRevision)
        {
            downloadPath = m_packages.findBestPath(m_localRevision, m_remoteRevision);
            if(downloadPath.isEmpty())
                throw tr("No download path found from %1 to %2").arg(m_localRevision, m_remoteRevision);
        }
        else
        {
            downloadPath = m_packages.findBestPath(QString(), m_remoteRevision);
            if(downloadPath.isEmpty())
                throw tr("No download path found to %1").arg(m_remoteRevision);
            downloadPath.remove(0, downloadPath.count() - 1);
        }
        downloadPathPos = 0;

        downloadSize = 0;
        checkPosition = 0;
        downloadPosition = 0;
        applyPosition = 0;
        foreach(const Package &package, downloadPath)
        {
            downloadSize += package.size;
        }

        updatePackageLoop();
    }
    catch(const QString & msg)
    {
        emit finished(msg);
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
        loadPackageMetadata();
    }
    else
    {
        if(failures.size() > 0)
        {
            if(!isFixingError())
            {
                // First time we get in this loop
                qCDebug(LOG_DLMANAGER) << "Some parts of the update have gone wrong, fixing...";
                downloadPath = m_packages.findBestPath("", m_remoteRevision);
                if(downloadPath.isEmpty())
                    emit finished(tr("No download path found %1").arg(m_remoteRevision));
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
                else if(it.value() < NonRecoverable)
                {
                    fixingPath = it.key();
                    it.value() = FixInProgress;
                    break;
                }
                ++it;
            }

            if(isFixingError())
            {
                downloadPathPos = 0;
                loadPackageMetadata();
            }
            else
            {
                qCDebug(LOG_DLMANAGER) << "Update finished with " << fixedFailures << "/" << failures.size() << "fixed errors";
                if(fixedFailures == failures.size())
                {
                    success();
                }
                else
                {
                    emit finished(tr("Unable to fixes all errors (%1/%2 fixed)").arg(fixedFailures).arg(failures.size()));
                }
            }
        }
        else
        {
            qCDebug(LOG_DLMANAGER) << "Update finished without error";
            success();
        }
    }
}

void DownloadManager::loadPackageMetadata()
{
    const Package & package = downloadPath.at(downloadPathPos);
    metadata.setPackage(package);
    auto it = m_cachedMetadata.find(package.url());
    if(it != m_cachedMetadata.end())
    {
        metadata = it.value();
        packageMetadataReady();
    }
    else
    {
        metadataRequest = get(package.metadataUrl());
        connect(metadataRequest, &QNetworkReply::finished, this, &DownloadManager::updatePackageMetadataFinished);
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

        metadata.fromJsonObject(JsonUtil::fromJson(metadataRequest->readAll()));
        m_cachedMetadata.insert(metadata.package().url(), metadata);
        packageMetadataReady();
    }
    catch(const QString & msg)
    {
        emit finished(msg);
    }
}

void DownloadManager::packageMetadataReady()
{
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

    if(!isFixingError())
    {
        Q_ASSERT(!isLastPackage() || m_fileListAfterUpdate.size() == 0);
        Q_ASSERT(!isLastPackage() || m_dirListAfterUpdate.size() == 0);
        foreach(QSharedPointer<Operation> op, metadata.operations())
        {
            if(isLastPackage())
            {
                switch (op->fileType()) {
                case Operation::File:
                    m_fileListAfterUpdate.insert(op->path());
                    break;
                case Operation::Folder:
                    m_dirListAfterUpdate.insert(op->path());
                    break;
                default:
                    break;
                }
            }

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
                return;
            }
            ++operationIndex;
        }
        if(isLastPackage())
            failure(fixingPath, Fixed);
        updatePackageLoop();
    }
}

void DownloadManager::readyToApply(QSharedPointer<Operation> readyOperation)
{
    if(readyOperation->status() == Operation::DownloadRequired || isFixingError())
    {
        if( (!QFile::exists(readyOperation->dataFilename()) || QFile::remove(readyOperation->dataFilename()))
            && QFile(readyOperation->dataDownloadFilename()).rename(readyOperation->dataFilename())
           )
        {
            emit operationReadyToApply(readyOperation);
        }
        else
        {
            EMIT_WARNING(OperationDownload, tr("Unable to rename downloaded filename"), readyOperation);
            failure(readyOperation->path(), DownloadRenameFailed);
        }
    }
    else if(readyOperation->status() == Operation::ApplyRequired)
    {
        QFile::remove(readyOperation->dataDownloadFilename());
        emit operationReadyToApply(readyOperation);
    }
    else
    {
        incrementApplyPosition(readyOperation->size());
    }
}

void DownloadManager::failure(const QString &path, Failure reason)
{
    if(reason != Fixed)
        qCDebug(LOG_DLMANAGER) << "Operation failed " << reason << path;
    else
        qCDebug(LOG_DLMANAGER) << "Operation fixed " << path;

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
            updateDataStopDownload();
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
        qCDebug(LOG_DLMANAGER) << "DownloadFinished, cause " << preparedOperationIndex << "+ 1 == " << metadata.operationCount();
        emit downloadFinished();
    }
}

/**
   \brief Filemanager has done is pre-work about this operation
   If the operation download isn't necessary, determine if skipping it is useful
 */
void DownloadManager::operationPrepared(QSharedPointer<Operation> preparedOperation)
{
    if(preparedOperation->size() > 0)
        incrementCheckPosition(preparedOperation->size());
    ++preparedOperationIndex;
    Q_ASSERT(preparedOperation == metadata.operation(preparedOperationIndex));

    qCDebug(LOG_DLMANAGER) << "Operation prepared" << preparedOperation->path();

    // Download of prepared operation has already occured
    // [--DL--][--Prepared--][--Apply--]
    if(preparedOperationIndex < operationIndex)
    {
        Q_ASSERT(preparedOperation != operation);
        readyToApply(preparedOperation);
        if(operationIndex == metadata.operationCount() && preparedOperationIndex + 1 == operationIndex)
        {
            qCDebug(LOG_DLMANAGER) << "DownloadFinished, cause " << operationIndex << "== preparedOperationIndex + 1 == operationIndex == operationCount";
            emit downloadFinished();
        }
    }
    // Download of prepared operation is in progress
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
                incrementDownloadPosition(preparedOperation->size());
                readyToApply(preparedOperation);
                nextOperation();
            }
        }
        else if(preparedOperation->status() != Operation::DownloadRequired)
        {
            // There is a download in progress and preparedOperation doesn't require to be downloaded
            // We may be able to optimise the download time

            // Queue apply the current operation if its possible
            incrementDownloadPosition(preparedOperation->size() - offset);
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

    while(availableSize > 0)
    {
        if(downloadSeek > 0)
        {
            if(availableSize <= downloadSeek)
            {
                dataRequest->read(availableSize);
                downloadSeek -= availableSize;
                return;
            }
            dataRequest->read(downloadSeek);
            availableSize -= downloadSeek;
            downloadSeek = 0;
        }

        size = operation->size() - offset;
        Q_ASSERT(size >= 0);
        if(size <= availableSize)
        {
            file.write(dataRequest->read(size));
            incrementDownloadPosition(size);
            operationDownloaded();
            if(dataRequest == nullptr)
                return; //< Futures download aborted
            availableSize -= size;
        }
        else
        {
            file.write(dataRequest->read(availableSize));
            incrementDownloadPosition(availableSize);
            offset += availableSize;
            availableSize = 0;
        }
    }
}

void DownloadManager::operationDownloaded()
{
    qCDebug(LOG_DLMANAGER) << "Operation downloaded" << operation->path();

    Q_ASSERT(file.isOpen());
    Q_ASSERT(file.size() == operation->size());

    file.close();

    if(isFixingError())
    {
        updateDataStopDownload();
        readyToApply(operation);
        qCDebug(LOG_DLMANAGER) << "DownloadFinished, cause isFixingError()" << fixingPath;
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
    qCDebug(LOG_DLMANAGER) << "Operation applied" << appliedOperation->path();
    if(appliedOperation->status() != Operation::Valid)
    {
        if(isFixingError())
        {
            Q_ASSERT(appliedOperation->path() == fixingPath);
            failure(fixingPath, NonRecoverable);
            downloadPathPos = downloadPath.size(); // stopPackageLoop
        }
        else
        {
            EMIT_WARNING(OperationApply, appliedOperation->errorString(), appliedOperation);
            failure(appliedOperation->path(), ApplyFailed);
        }
    }
    else if(failures.contains(appliedOperation->path()))
    {
        incrementApplyPosition(appliedOperation->size());
        Q_ASSERT(!isFixingError() || appliedOperation->path() == fixingPath);
        failure(appliedOperation->path(), Fixed);
    }
    else
    {
        incrementApplyPosition(appliedOperation->size());
    }
}

void DownloadManager::applyFinished()
{
    ++downloadPathPos;
    qCDebug(LOG_DLMANAGER) << "Apply " << downloadPathPos << "/" << downloadPath.size() << "finished";
    updatePackageLoop();
}

void DownloadManager::success()
{
    QSet<QString> diffFileList = m_localRepository.fileList().toSet() - m_fileListAfterUpdate - m_dirListAfterUpdate;
    QSet<QString> diffDirList = m_localRepository.dirList().toSet() - m_fileListAfterUpdate - m_dirListAfterUpdate;
    foreach(const QString &fileToRemove, diffFileList)
    {
        QFile file(m_updateDirectory + fileToRemove);
        if(file.exists() && !file.remove())
            EMIT_WARNING(RemoveFiles, tr("Unable to remove file %1").arg(fileToRemove), fileToRemove);
    }

    foreach(const QString &dirToRemove, diffDirList)
    {
        QDir dir(m_updateDirectory + dirToRemove);
        if(dir.exists())
            dir.rmdir(dir.path());
    }

    m_localRepository.setFileList(QStringList(m_fileListAfterUpdate.toList()));
    m_localRepository.setDirList(QStringList(m_dirListAfterUpdate.toList()));
    m_localRepository.setRevision(m_remoteRevision);
    m_localRepository.setUpdateInProgress(false);
    m_localRepository.save();

    emit finished(QString());
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
    qCDebug(LOG_DLMANAGER) << "GET STOPPED";
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

    qCDebug(LOG_DLMANAGER) << "GET(" << what << "," << startPosition << "-" << endPosition << ")";
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    downloadSeek = startPosition > 0 && url.isLocalFile() ? startPosition : 0;

    return reply;
}

void DownloadManager::incrementCheckPosition(qint64 size)
{
    checkPosition += size;
    emit checkProgress(checkPosition, downloadSize);
    emit progress(downloadPosition + applyPosition + checkPosition, downloadSize * 3);
}

void DownloadManager::incrementDownloadPosition(qint64 size)
{
    downloadPosition += size;
    emit downloadProgress(downloadPosition, downloadSize);
    emit progress(downloadPosition + applyPosition + checkPosition, downloadSize * 3);
}

void DownloadManager::incrementApplyPosition(qint64 size)
{
    applyPosition += size;
    emit applyProgress(applyPosition, downloadSize);
    emit progress(downloadPosition + applyPosition + checkPosition, downloadSize * 3);
}

void DownloadManager::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password().isEmpty())
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user().isEmpty())
        authenticator->setUser(m_username);
}
