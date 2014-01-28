#include "downloadmanager.h"

DownloadManager::DownloadManager(RemoteUpdate *_update)
    : QObject()
{
    m_updateDirectory = _update->updateDirectory();
    m_updateTmpDirectory = _update->updateTmpDirectory();
    m_updateUrl = _update->updateUrl();
    m_createApplyManifest = _update->createApplyManifest();
    m_metaDataBaseUrl = _update->m_metaDataBaseUrl;
    m_username = _update->m_username;
    m_password = _update->m_password;

    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &DownloadManager::authenticationRequired);
    m_operationThread = new OperationThread(this);
    connect(m_operationThread, SIGNAL(operationFinished(Operation*)), SLOT(operationFinishedDirect(Operation*)), Qt::DirectConnection);
    connect(m_operationThread, &OperationThread::operationFinished, this, &DownloadManager::operationFinished);

    m_dlBaseOffset = 0;
    m_dlReaded = 0;
    m_dlOperationIdx = 0;
    m_appliedSize = 0;
    m_appliedCount = 0;
    m_failedCount = 0;
}

DownloadManager::~DownloadManager()
{
    for(int i = 0; i < m_operations.size(); ++i)
    {
        delete m_operations[i];
    }
}

void DownloadManager::loadMetadata(const QJsonDocument & json)
{
    QJsonObject object = json.object();
    QJsonValue versionJsonValue = object.value(QStringLiteral("version"));

    if(!versionJsonValue.isString())
        throw(tr("Json metadata : Unable to find 'version' as a string"));

    QString version = versionJsonValue.toString();
    if(version == "1")
        loadMetadata1(object);
    else
        throw(tr("Json metadata : Unsupported version %1").arg(version));

    if(m_createApplyManifest)
    {
        QFile saveFile(updateDirectory()+QStringLiteral("manifest.json"));

        if (!saveFile.open(QIODevice::WriteOnly))
            throw tr("Couldn't open save file.");

        saveFile.write(json.toJson());
    }
}

void DownloadManager::loadMetadata1(const QJsonObject & object)
{
    int i;
    const QJsonArray operations = object.value(QStringLiteral("operations")).toArray();
    if(operations.isEmpty())
        throw(tr("loadMetadata1 : 'operations' is empty"));

    QJsonValue sizeValue = object.value(QStringLiteral("size"));
    if(!sizeValue.isString())
        throw(tr("loadMetadata1 : 'size' isn't a string"));

    bool ok;
    m_dlTotalSize = sizeValue.toString().toLongLong(&ok);
    if(!ok)
        throw(tr("loadMetadata1 : 'size' isn't a valid qint64 string"));

    for(i = 0; i < m_operations.size(); ++i)
    {
        delete m_operations[i];
    }
    m_operations.resize(operations.size());

    try
    {
        for(i = 0; i < operations.size(); ++i)
        {
            const QJsonValue & jsonOperationValue = operations[i];
            if(!jsonOperationValue.isObject())
                throw(tr("loadMetadata1 : 'jsonVersion' is not an object"));
            QJsonObject jsonOperation = jsonOperationValue.toObject();

            QString action = jsonOperation.value(QStringLiteral("action")).toString();
            Operation * op;
            if(action == QLatin1String("RM"))
                op = new RemoveOperation(this);
            else if(action == QLatin1String("RMDIR"))
                op = new RemoveDirectoryOperation(this);
            else if(action == QLatin1String("ADD"))
                op = new AddOperation(this);
            else if(action == QLatin1String("PATCH"))
                op = new PatchOperation(this);
            else
                throw(tr("loadMetadata1 : 'action'==\"%1\" is not supported").arg(action));

            try
            {
                op->load1(jsonOperation);
            }
            catch(...)
            {
                delete op;
                throw;
            }

            op->setFilename(updateTmpDirectory()+QStringLiteral("Operation")+QString::number(i));
            op->setPosition(i);
            m_operations[i] = op;
        }
    }
    catch(...)
    {
        while(i > 0)
        {
            delete m_operations[--i];
        }
        m_operations.resize(0);
        throw;
    }
}

void DownloadManager::applyLocally(const QString &localFolder)
{
    m_applyLocally = localFolder;
    if(!m_applyLocally.endsWith(QDir::separator()))
        m_applyLocally += QDir::separator();
    m_operationThread->applyLocally(m_applyLocally);
}

void DownloadManager::run()
{
    if(m_applyLocally.isEmpty())
    {
        // Start Download
        QString name = m_metaDataBaseUrl;
        nextOperation(false);

        if(m_dlTotalSize > 0 && m_dlOperationIdx < m_operations.size())
        {

            // Open the next
            m_dlFile.setFileName(m_operations[m_dlOperationIdx]->filename());
            if(!m_dlFile.open(QFile::WriteOnly | QFile::Truncate))
                throw(tr("Unable to open datafile for writing : %1").arg(m_dlFile.fileName()));

            data = get(name, 0);
            Log::info(tr("Starting downloading data"));
            connect(data, SIGNAL(downloadProgress(qint64,qint64)), SLOT(onDataDownloadProgress(qint64,qint64)));
            connect(data, SIGNAL(finished()), SLOT(onDataFinished()));
        }
        else
        {
            data = NULL;
            Log::info(tr("Data is already downloaded"));
            emit downloadProgress(m_dlTotalSize, m_dlTotalSize);
            onDataFinished();
        }
    }
    else
    {
        for(int i = 0; i < m_operations.size(); ++i)
        {
            Operation * operation = m_operations[i];
            m_operationThread->apply(operation);
        }
    }
}

QNetworkReply *DownloadManager::get(const QString &what, qint64 startPosition)
{
    QNetworkRequest request(QUrl(updateUrl().arg(what)));
    if(startPosition > 0)
    {
        request.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(startPosition).toLatin1());
    }

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    return reply;
}

void DownloadManager::nextOperation(bool continuous)
{
    for(; m_dlOperationIdx < m_operations.size(); ++m_dlOperationIdx)
    {
        Operation * operation = m_operations[m_dlOperationIdx];
        if(operation->offset() != -1)
        {
            if(continuous || !operation->isDataValid())
            {
                m_dlBaseOffset = operation->offset();
                m_dlOperationLength = operation->size();
                break;
            }
            emit downloadProgress(operation->offset() + operation->size(), m_dlTotalSize);
        }
        m_operationThread->apply(operation);
    }
}

void DownloadManager::operationFinishedDirect(Operation * operation)
{
    Log::trace(tr("Operation finished received %1").arg(operation->position()));
    emit applyProgress(m_appliedSize += operation->size(), m_dlTotalSize);
}

void DownloadManager::operationFinished(Operation *)
{
    ++m_appliedCount;
    if(m_appliedCount + m_failedCount == m_operations.size())
    {
        emit updateFinished(m_failedCount == 0);
        emit finished();
    }
}

void DownloadManager::operationFailed(Operation *)
{
    ++m_failedCount;
    if(m_appliedCount + m_failedCount == m_operations.size())
    {
        emit updateFinished(m_failedCount == 0);
        emit finished();
    }
}

void DownloadManager::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password().isEmpty())
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user().isEmpty())
        authenticator->setUser(m_username);
}

void DownloadManager::onDataFinished()
{
    try
    {
        Log::info(tr("Data download finished"));
        if(data != NULL)
        {
            data->deleteLater();

            onDataDownloadProgress(true, m_dlTotalSize, m_dlTotalSize);

            if(data->error() != QNetworkReply::NoError)
                throw(tr("Download failed : %1").arg(data->errorString()));

            data = NULL;
        }

        Log::info(tr("Data download saved"));

        //Finish apply operations
        for(; m_dlOperationIdx < m_operations.size(); ++m_dlOperationIdx)
        {
            Operation * operation = m_operations[m_dlOperationIdx];
            if(operation->offset() != -1)
                throw(tr("Download failed : %1").arg(m_dlOperationIdx));
            else
                m_operationThread->apply(operation);
        }
    }
    catch(const QString & msg)
    {
        actionFailed(msg);
    }
}

void DownloadManager::onDataDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    onDataDownloadProgress(false, m_dlBaseOffset+bytesReceived, m_dlBaseOffset+bytesTotal);
}

void DownloadManager::onDataDownloadProgress(bool final, qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
    while(data->error() == QNetworkReply::NoError && m_dlOperationIdx < m_operations.size() && m_dlFile.isOpen())
    {
        char buffer[4096];
        qint64 read, size = 4096;

        if(m_dlReaded < m_dlBaseOffset)
        {
            if(size > m_dlBaseOffset-m_dlReaded)
                size = m_dlBaseOffset-m_dlReaded;
            while(size > 0 && (read = data->read(buffer, size)) > 0)
            {
                m_dlReaded += read;
                if(size > m_dlBaseOffset-m_dlReaded)
                    size = m_dlBaseOffset-m_dlReaded;
            }
            size = 4096;
        }

        if(size > m_dlOperationLength)
            size = m_dlOperationLength;
        while(size > 0 && (read = data->read(buffer, size)) > 0)
        {
            m_dlFile.write(buffer, read);
            m_dlOperationLength -= read;
            m_dlReaded += read;
            if(size > m_dlOperationLength)
                size = m_dlOperationLength;
        }
        if(m_dlOperationLength == 0)
        {
            // Operation download is done
            m_dlFile.close();
            {
                Operation * operation = m_operations[m_dlOperationIdx];
                m_operationThread->apply(operation);
            }
            ++m_dlOperationIdx;
            nextOperation(true);
            if(m_dlOperationIdx < m_operations.size())
            {
                m_dlFile.setFileName(m_operations[m_dlOperationIdx]->filename());
                if(!m_dlFile.open(QFile::WriteOnly | QFile::Truncate))
                {
                    Log::error(tr("Unable to open datafile for writing : %1").arg(m_dlFile.fileName()));
                }
            }
            if(final)
                continue;
        }
        else if(m_dlOperationLength < 0)
        {
            // Download failed
            Log::error(tr("m_dlOperationLength < 0 : %1").arg(m_dlFile.fileName()));
        }
        break;
    }
}
