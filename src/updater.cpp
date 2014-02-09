#include "updater.h"
#include <qtlog.h>
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

/*! \fn bool Updater::isIdle() const
    Returns \c true if no action is in progress; otherwise returns \c false.

    \sa isUpdateRequired()
*/

/*! \fn bool Updater::isUpdateAvailable() const
    Returns \c true if an update is available; otherwise returns \c false.

    \sa isIdle()
*/

/*! \fn QString Updater::currentVersion() const
    Returns the local current version.

    \sa latestVersion(), versions()
*/

/*! \fn QString Updater::latestVersion() const
    Returns the latest remote version.

    \sa currentVersion(), versions()
*/

/*! \fn QVector<Version> Updater::versions()
    Returns the remote version list.

    \sa currentVersion(), latestVersion()
*/

/*! \fn QString Updater::updateDirectory() const
    Returns the directory to update.

    \sa setUpdateDirectory(), updateTmpDirectory(), setUpdateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn void Updater::setUpdateDirectory(const QString &updateDirectory)
    Set the directory to update

    \sa updateDirectory(), updateTmpDirectory(), setUpdateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn QString Updater::updateTmpDirectory() const
    Returns the directory to use for storing temporary files required for doing the update.

    \sa updateDirectory(), setUpdateDirectory(), setUpdateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn void Updater::setUpdateTmpDirectory(const QString &updateTmpDirectory)
    Set the directory to use for storing temporary files required for doing the update.

    \sa updateDirectory(), setUpdateDirectory(), updateTmpDirectory(), updateUrl(), setUpdateUrl()
*/

/*! \fn QString Updater::updateUrl() const
    Returns the remote update url.

    \sa setUpdateUrl(), setCredential()
*/

/*! \fn void Updater::setUpdateUrl(const QString &updateUrl)
    Set the remote update url.

    \sa updateUrl(), setCredential()
*/

/*! \fn void Updater::setCredential(const QString &username, const QString &password)
    Set the username and password for remote url basic authentification.

    \sa updateUrl(), setUpdateUrl()
*/

/*! \fn Updater::State Updater::state() const
    Returns this object state.

    \sa Updater::State
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

/**
 * @brief Name of the repository packages file
 */
const QString UpdateUrlPackages = QStringLiteral("packages");



Updater::Updater(QObject *parent) : QObject(parent)
{
    m_state = Idle;
    m_createApplyManifest = false;
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &Updater::authenticationRequired);
}

Updater::~Updater()
{
}

QString Updater::iniCurrentVersion() const
{
    QSettings settings(updateDirectory()+QStringLiteral("status.ini"), QSettings::IniFormat);
    return (settings.value(Revision).toString());
}

void Updater::checkForUpdates()
{
    if(isIdle())
    {
        setState(DownloadingInformations);
        clearError();
        setCurrentVersion(iniCurrentVersion());
        info = get(UpdateUrlInfo);
        connect(metadata, &QNetworkReply::finished, this, &Updater::onInfoFinished);
    }
    else
    {
        LOG_WARN("called while not Idle");
    }
}

void Updater::update()
{
    if(isUpdateAvailable())
    {
        setState(Updating);
        clearError();

        LOG_TRACE(tr("Getting package list"));
        info = get(UpdateUrlPackages);
        connect(metadata, &QNetworkReply::finished, this, &Updater::onPackagesFinished);


        if(currentVersion().isEmpty())
        {
            metadata = get(QStringLiteral("complete_%1.metadata").arg(latestVersion()));
        }
        else
        {
            metadata = get(QStringLiteral("patch%1_%2.metadata").arg(currentVersion(), latestVersion()));
        }

        connect(metadata, &QNetworkReply::finished, this, &Updater::onMetadataFinished);
    }
    else
    {
        LOG_WARN("called without an available update");
    }
}

void Updater::applyLocally(const QString &localFolder)
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
        connect(downloader, &DownloadManager::updateFinished, this, &Updater::onApplyFinished);
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

QNetworkReply* Updater::get(const QString & what)
{
    QNetworkRequest request(QUrl(updateUrl().arg(what)));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    return reply;
}

void Updater::onInfoFinished()
{
    try
    {
        if(info->error() != QNetworkReply::NoError)
            throw info->errorString();

        LOG_INFO(tr("Remote informations downloaded"));

        QJsonParseError jsonError;
        QJsonDocument json = QJsonDocument::fromJson(info->readAll(), &jsonError);

        if(jsonError.error != QJsonParseError::NoError)
            throw(tr("Unable to parse json from info : %1").arg(jsonError.errorString()));

        if(!json.isObject())
            throw(tr("Json info : Expecting an object"));

        loadVersions(json);

        LOG_INFO(tr("Remote informations analyzed"));

        if(currentVersion() == latestVersion())
        {
            LOG_INFO(tr("Already at the latest version"));
            setState(AlreadyUptodate);
        }
        else
        {
            if(currentVersion().isEmpty())
            {
                LOG_INFO(tr("Install required to %1").arg(latestVersion()));
            }
            else
            {
                LOG_INFO(tr("Update required from %1 to %2").arg(currentVersion(), latestVersion()));
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

void Updater::onPackagesFinished()
{
    try
    {
        if(info->error() != QNetworkReply::NoError)
            throw info->errorString();

        LOG_INFO(tr("Package list downloaded"));

        QJsonParseError jsonError;
        QJsonDocument json = QJsonDocument::fromJson(info->readAll(), &jsonError);

        if(jsonError.error != QJsonParseError::NoError)
            throw(tr("Unable to parse json from info : %1").arg(jsonError.errorString()));

        if(!json.isObject())
            throw(tr("Json info : Expecting an object"));

        loadVersions(json);

        LOG_INFO(tr("Remote informations analyzed"));

        if(currentVersion() == latestVersion())
        {
            LOG_INFO(tr("Already at the latest version"));
            setState(AlreadyUptodate);
        }
        else
        {
            if(currentVersion().isEmpty())
            {
                LOG_INFO(tr("Install required to %1").arg(latestVersion()));
            }
            else
            {
                LOG_INFO(tr("Update required from %1 to %2").arg(currentVersion(), latestVersion()));
            }

            setState(UpdateRequired);
            emit updateRequired();
        }
    }
    catch(const QString & msg)
    {
        actionFailed(msg);
    }

    emit updateFinished();
}

void Updater::onMetadataFinished()
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
        connect(downloader, &DownloadManager::actionFailed, this, &Updater::actionFailed);
        connect(downloader, &DownloadManager::updateFinished, this, &Updater::onUpdateFinished);
        connect(downloader, &DownloadManager::downloadProgress, this, &Updater::onDownloadProgress);
        connect(downloader, &DownloadManager::applyProgress, this, &Updater::onApplyProgress);
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

void Updater::onApplyFinished(bool success)
{
    Log::info(tr("Apply finished"));
    emit applyFinished(success);
}

void Updater::onUpdateFinished(bool success)
{
    if(success)
    {
        QSettings settings(updateDirectory()+QStringLiteral("status.ini"), QSettings::IniFormat);
        settings.setValue(Updater::String::Revision, latestVersion());
        setCurrentVersion(latestVersion());
        setState(Uptodate);
    }
    Log::info(tr("Update finished"));
    emit updateFinished(success);
}



void Updater::actionFailed(const QString &msg)
{
    m_lastError = msg;
    Log::error(msg);
    m_state = Idle;
    emit error(msg);
}

void Updater::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void Updater::onApplyProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit applyProgress(bytesReceived, bytesTotal);
}

void Updater::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password() != m_password)
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user() != m_username)
        authenticator->setUser(m_username);
}

bool Updater::createApplyManifest() const
{
    return m_createApplyManifest;
}

void Updater::setCreateApplyManifest(bool createApplyManifest)
{
    isIdle();
    m_createApplyManifest = createApplyManifest;
}

void Updater::loadVersions1(const QJsonObject & object)
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

void Updater::loadVersions(const QJsonDocument & json)
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

void Updater::loadVersions1(const QJsonObject & object)
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


