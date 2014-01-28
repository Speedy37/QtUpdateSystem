#include "remoteupdate.h"
#include "remoteupdate_p.h"
#include "log.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QSettings>
#include <QMutexLocker>
#include <QAbstractEventDispatcher>
#include <QAuthenticator>


/*!
    \class RemoteUpdate
    \brief Provide remote update functionality

    \reentrant

*/

/*! \fn bool RemoteUpdate::isIdle() const
    Returns \c true if no action is in progress; otherwise returns \c false.

    \sa isUpdateRequired()
*/

/*! \fn bool RemoteUpdate::isUpdateAvailable() const
    Returns \c true if an update is available; otherwise returns \c false.

    \sa isIdle()
*/

/*! \fn QString RemoteUpdate::currentVersion() const
    Returns the local current version.

    \sa latestVersion(), versions()
*/

/*! \fn QString RemoteUpdate::latestVersion() const
    Returns the latest remote version.

    \sa currentVersion(), versions()
*/

/*! \fn QVector<Version> RemoteUpdate::versions()
    Returns the remote version list.

    \sa currentVersion(), latestVersion()
*/

/*! \fn QString RemoteUpdate::updateDirectory() const
    Returns the directory to update.

    \sa setUpdateDirectory(), updateTmpDirectory(), setUpdateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn void RemoteUpdate::setUpdateDirectory(const QString &updateDirectory)
    Set the directory to update

    \sa updateDirectory(), updateTmpDirectory(), setUpdateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn QString RemoteUpdate::updateTmpDirectory() const
    Returns the directory to use for storing temporary files required for doing the update.

    \sa updateDirectory(), setUpdateDirectory(), setUpdateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn void RemoteUpdate::setUpdateTmpDirectory(const QString &updateTmpDirectory)
    Set the directory to use for storing temporary files required for doing the update.

    \sa updateDirectory(), setUpdateDirectory(), updateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn QString RemoteUpdate::updateUrl() const
    Returns the remote update url.

    \sa setUpdateUrl(), setCredential()
*/

/*! \fn void RemoteUpdate::setUpdateUrl(const QString &updateUrl)
    Set the remote update url.

    \sa updateUrl(), setCredential()
*/

/*! \fn void RemoteUpdate::setCredential(const QString &username, const QString &password)
    Set the username and password for remote url basic authentification.

    \sa updateUrl(), setUpdateUrl()
*/

/*! \fn RemoteUpdate::State RemoteUpdate::state() const
    Returns this object state.

    \sa RemoteUpdate::State
*/

/**
 * @brief Config name of the key that hold the current Revision of the working repository
 */
const QString Revision = QStringLiteral("Revision");

/**
 * @brief Config name of the key that hold the name of the in update file
 */
const QString UpdateFile = QStringLiteral("UpdateFile");

/**
 * @brief Name of the repository information file
 */
const QString UpdateUrlInfo = QStringLiteral("info");



RemoteUpdate::RemoteUpdate(QObject *parent) : QObject(parent)
{
    m_state = Idle;
    m_createApplyManifest = false;
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &RemoteUpdate::authenticationRequired);
}

RemoteUpdate::~RemoteUpdate()
{
}

QString RemoteUpdate::iniCurrentVersion() const
{
    QSettings settings(updateDirectory()+QStringLiteral("status.ini"), QSettings::IniFormat);
    return (settings.value(Revision).toString());
}

void RemoteUpdate::checkForUpdates()
{
    Q_ASSERT(isIdle());
    setState(DownloadingInformations);
    clearError();
    setCurrentVersion(iniCurrentVersion());
    info = get(UpdateUrlInfo);
    connect(metadata, &QNetworkReply::finished, this, &RemoteUpdate::onInfoFinished);
}

/**
 * @brief Start the update in it's own thread
 * TODO : Ensure safety of public methods
 */
void RemoteUpdate::update()
{
    Q_ASSERT(isUpdateAvailable());
    setState(Updating);
    clearError();
    metadata = get(m_metaDataBaseUrl+QStringLiteral(".metadata"));
    connect(metadata, &QNetworkReply::finished, this, &RemoteUpdate::onMetadataFinished);
}

void RemoteUpdate::applyLocally(const QString &localFolder)
{
    Q_ASSERT(isIdle());
    setState(ApplyingLocally);
    clearError();

    try
    {
        QFile file(updateDirectory()+QStringLiteral("manifest.json"));

        if(!file.open(QFile::ReadOnly))
            throw tr("Unable to open file %1").arg(file.errorString());

        QJsonParseError e;
        QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &e);

        if(e.error != QJsonParseError::NoError)
            throw(tr("Unable to parse json from manisfest : %1").arg(e.errorString()));

        if(!json.isObject())
            throw(tr("Json metadata : Expecting an object"));

        DownloadManager *downloader = new DownloadManager(this);
        downloader->applyLocally(localFolder);
        downloader->loadMetadata(json);
        Log::info(tr("Metadata information analyzed"));

        QThread * thread = new QThread(this);
        downloader->moveToThread(thread);
        connect(downloader, SIGNAL(actionFailed(QString)), this, SLOT(actionFailed(QString)));
        connect(downloader, &DownloadManager::updateFinished, this, &RemoteUpdate::onApplyFinished);
        connect(downloader, SIGNAL(applyProgress(qint64,qint64)), this, SLOT(onApplyProgress(qint64,qint64)));
        connect(downloader, SIGNAL(updateFinished()), thread, SLOT(quit()));
        connect(downloader, SIGNAL(updateFinished()), downloader, SLOT(deleteLater()));
        connect(thread, SIGNAL(started()), downloader, SLOT(run()));
        connect(thread, SIGNAL(destroyed()), downloader, SLOT(deleteLater()));
        thread->start();
    }
    catch(const QString & msg)
    {
        actionFailed(msg);
    }
}

QNetworkReply* RemoteUpdate::get(const QString & what)
{
    QNetworkRequest request(QUrl(updateUrl().arg(what)));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    return reply;
}

void RemoteUpdate::onInfoFinished()
{
    try
    {
        if(info->error() != QNetworkReply::NoError)
            throw info->errorString();

        QJsonParseError jsonError;
        QJsonDocument json = QJsonDocument::fromJson(info->readAll(), &jsonError);

        Log::info(tr("Remote information downloaded"));

        if(jsonError.error != QJsonParseError::NoError)
            throw(tr("Unable to parse json from info : %1").arg(jsonError.errorString()));

        if(!json.isObject())
            throw(tr("Json info : Expecting an object"));

        loadVersions(json);

        Log::info(tr("Remote information analyzed"));

        if(currentVersion() == latestVersion())
        {
            Log::info(tr("Already at the latest version"));
            setState(AlreadyUptodate);
        }
        else
        {
            if(currentVersion().isEmpty())
            {
                Log::info(tr("Install required to %1").arg(latestVersion()));
                m_metaDataBaseUrl = QStringLiteral("complete_%1").arg(latestVersion());
            }
            else
            {
                Log::info(tr("Update required from %1 to %2").arg(currentVersion(), latestVersion()));
                m_metaDataBaseUrl = QStringLiteral("patch_%1_%2").arg(currentVersion(), latestVersion());
            }

            setState(UpdateRequired);
            emit updateRequired();
        }
    }
    catch(const QString & msg)
    {
        actionFailed(msg);
    }

    emit checkForUpdatesFinished();
}

void RemoteUpdate::onMetadataFinished()
{
    if(metadata->error() == QNetworkReply::ContentNotFoundError && m_metaDataBaseUrl.startsWith("patch_"))
    {
        m_metaDataBaseUrl = QStringLiteral("complete_%1").arg(latestVersion());
        metadata = get(m_metaDataBaseUrl+QStringLiteral(".metadata"));
        connect(metadata, SIGNAL(finished()), SLOT(onMetadataFinished()));
        return;
    }

    try
    {
        if(metadata->error() != QNetworkReply::NoError)
            throw(metadata->errorString());

        Log::info(tr("Metadata information downloaded"));

        QJsonParseError e;
        QJsonDocument json = QJsonDocument::fromJson(metadata->readAll(), &e);

        if(e.error != QJsonParseError::NoError)
            throw(tr("Unable to parse json from metadata : %1").arg(e.errorString()));

        if(!json.isObject())
            throw(tr("Json metadata : Expecting an object"));

        DownloadManager *downloader = new DownloadManager(this);
        downloader->loadMetadata(json);
        Log::info(tr("Metadata information analyzed"));

        QThread * thread = new QThread(this);
        downloader->moveToThread(thread);
        connect(downloader, &DownloadManager::actionFailed, this, &RemoteUpdate::actionFailed);
        connect(downloader, &DownloadManager::updateFinished, this, &RemoteUpdate::onUpdateFinished);
        connect(downloader, &DownloadManager::downloadProgress, this, &RemoteUpdate::onDownloadProgress);
        connect(downloader, &DownloadManager::applyProgress, this, &RemoteUpdate::onApplyProgress);
        connect(downloader, &DownloadManager::finished, thread, &QThread::quit);
        connect(downloader, &DownloadManager::finished, downloader, &DownloadManager::deleteLater);
        connect(thread, &QThread::started, downloader, &DownloadManager::run);
        connect(thread, &QThread::destroyed, downloader, &DownloadManager::deleteLater);
        thread->start();
    }
    catch(const QString & msg)
    {
        actionFailed(msg);
    }
}

void RemoteUpdate::onApplyFinished(bool success)
{
    Log::info(tr("Apply finished"));
    emit applyFinished(success);
}

void RemoteUpdate::onUpdateFinished(bool success)
{
    if(success)
    {
        QSettings settings(updateDirectory()+QStringLiteral("status.ini"), QSettings::IniFormat);
        settings.setValue(RemoteUpdate::String::Revision, latestVersion());
        setCurrentVersion(latestVersion());
        setState(Uptodate);
    }
    Log::info(tr("Update finished"));
    emit updateFinished(success);
}



void RemoteUpdate::actionFailed(const QString &msg)
{
    m_lastError = msg;
    Log::error(msg);
    m_state = Idle;
    emit error(msg);
}

void RemoteUpdate::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void RemoteUpdate::onApplyProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit applyProgress(bytesReceived, bytesTotal);
}

void RemoteUpdate::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password() != m_password)
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user() != m_username)
        authenticator->setUser(m_username);
}

bool RemoteUpdate::createApplyManifest() const
{
    return m_createApplyManifest;
}

void RemoteUpdate::setCreateApplyManifest(bool createApplyManifest)
{
    isIdle();
    m_createApplyManifest = createApplyManifest;
}

void RemoteUpdate::loadVersions(const QJsonDocument & json)
{
    QJsonObject object = json.object();
    QJsonValue versionJsonValue = object.value(QStringLiteral("version"));

    if(!versionJsonValue.isString())
        throw(tr("Json info : Unable to find 'version' as a string"));

    QString version = versionJsonValue.toString();
    if(version == "1")
        loadVersions1(object);
    else
        throw(tr("Json info : Unsupported version %1").arg(version));
}

void RemoteUpdate::loadVersions1(const QJsonObject & object)
{
    const QJsonArray history = object.value(QStringLiteral("history")).toArray();
    if(history.isEmpty())
        throw(tr("loadVersions1 : 'history' is empty"));

    m_versions.resize(history.size());
    for(int i = 0; i < history.size(); ++i)
    {
        const QJsonValue & jsonVersionValue = history[i];
        if(!jsonVersionValue.isObject())
            throw(tr("loadVersions1 : 'jsonVersion' is not an object"));
        QJsonObject jsonVersion = jsonVersionValue.toObject();

        Version & version = m_versions[i];
        version.revision = jsonVersion.value(QStringLiteral("revision")).toString();
        version.description = jsonVersion.value(QStringLiteral("description")).toString();

        if(version.revision.isEmpty())
            throw(tr("loadVersions1 : version[%1].name is empty").arg(i));
    }
}


bool OperationThread::isEmpty()
{
    QMutexLocker locker(&m_queueMutex);
    return m_queue.isEmpty();
}

void OperationThread::apply(Operation *operation)
{
    m_queueMutex.lock();
    m_queue.enqueue(operation);
    m_queueMutex.unlock();
    if(!isRunning())
        start();
}

void OperationThread::run()
{
    while (!isEmpty())
    {
        m_queueMutex.lock();
        Operation * operation = m_queue.dequeue();
        m_queueMutex.unlock();
        try
        {
            if(m_applyLocally.isEmpty())
                operation->run();
            else
                operation->applyLocally(m_applyLocally);
            Log::trace(tr("%1 %2 finished").arg(operation->actionString(), operation->name()));
            emit operationFinished(operation);
        }
        catch(const QString &msg)
        {
            Log::error(tr("%1 %2 failed : %3").arg(operation->actionString(),operation->name()).arg(msg));
            operation->setError(msg);
            emit operationFailed(operation);
        }
        catch(...)
        {
            Log::error(tr("%1 %2 failed : Unknown reason").arg(operation->actionString(),operation->name()));
            operation->setError(tr("Unknown reason"));
            emit operationFailed(operation);
        }
    }
}


