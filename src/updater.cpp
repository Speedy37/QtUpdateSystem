#include "updater.h"
#include "common/jsonutil.h"
#include "common/utils.h"
#include "updater/downloadmanager.h"
#include "updater/copythread.h"

#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QLoggingCategory>
#include <QSettings>
#include <QAuthenticator>

Q_LOGGING_CATEGORY(LOG_UPDATER, "updatesystem.updater")

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

Updater::Updater(const QString &updateDirectory, const QString &remoteRepository, QObject *parent)
    : QObject(parent),
      m_updateUrl(remoteRepository),
      m_localRepository(updateDirectory)
{
    // Init
    m_state = Idle;

    // Network
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &Updater::authenticationRequired);
}

Updater::~Updater()
{

}

QNetworkReply* Updater::get(const QString & what)
{
    QNetworkRequest request(QUrl(remoteRepository() + what));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    return reply;
}

void Updater::checkForUpdates()
{
    if(isIdle())
    {
        setState(DownloadingInformations);
        clearError();

        m_currentRequest = get(QStringLiteral("current"));
        connect(m_currentRequest, &QNetworkReply::finished, this, &Updater::onInfoFinished);
    }
    else
    {
        qCWarning(LOG_UPDATER) << "Called while not Idle";
    }
}

void Updater::onInfoFinished()
{
    try
    {
        if(m_currentRequest->error() != QNetworkReply::NoError)
            throw m_currentRequest->errorString();

        qCDebug(LOG_UPDATER) << "Remote informations downloaded";

        m_remoteRevision.fromJsonObject(JsonUtil::fromJson(m_currentRequest->readAll()));

        qCDebug(LOG_UPDATER) << "Remote informations analyzed";

        if(!m_localRepository.isConsistent())
        {
            qCDebug(LOG_UPDATER) << "An update was in progress";
            setState(UpdateRequired);
            emit updateRequired();
        }
        else if(localRevision() == remoteRevision())
        {
            qCDebug(LOG_UPDATER) << "Already at the latest version";
            setState(AlreadyUptodate);
        }
        else
        {
            if(localRevision().isEmpty())
            {
                qCDebug(LOG_UPDATER) << "Install required to" << remoteRevision();
            }
            else
            {
                qCDebug(LOG_UPDATER) << "Install required from" << localRevision() << "to" << remoteRevision();
            }

            setState(UpdateRequired);
            emit updateRequired();
        }
    }
    catch(const QString & msg)
    {
        failure(msg);
        setState(Idle);
    }

    emit checkForUpdatesFinished();
}

void Updater::update()
{
    if(isUpdateAvailable())
    {
        setState(Updating);
        clearError();
        m_localRepository.setUpdateInProgress(true);
        m_localRepository.save();

        DownloadManager * downloader = new DownloadManager(m_localRepository, this);
        connect(downloader, &DownloadManager::finished, this, &Updater::updateFinished);
        connect(downloader, &DownloadManager::updateSucceeded, this, &Updater::updateSucceeded);
        connect(downloader, &DownloadManager::updateFailed, this, &Updater::updateFailed);
    }
    else
    {
        qCWarning(LOG_UPDATER) << "Called without an available update";
    }
}

void Updater::copy(const QString &copyDirectory)
{
    CopyThread *copier = new CopyThread(m_localRepository, copyDirectory, this);

    connect(copier, &CopyThread::progression, this, &Updater::copyProgress);
    connect(copier, &CopyThread::finished, this, &Updater::copyFinished);
    connect(copier, &CopyThread::finished, copier, &CopyThread::deleteLater);
    copier->start();
}

void Updater::updateSucceeded()
{
    m_localRepository.load();
    setState(Uptodate);
}

void Updater::updateFailed(const QString &reason)
{
    setState(UpdateRequired);
}

void Updater::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password() != m_password)
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user() != m_username)
        authenticator->setUser(m_username);
}

void Updater::setTmpDirectory(const QString &updateTmpDirectory)
{
    Q_ASSERT(isIdle());
    m_updateTmpDirectory = Utils::cleanPath(updateTmpDirectory);
}
