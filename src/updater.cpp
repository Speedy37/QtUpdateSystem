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
   Creates a Updater object with parent object \c parent.
*/
Updater::Updater(QObject *parent)
    : QObject(parent)
{
    // Init
    m_state = Idle;

    // Network
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &Updater::authenticationRequired);
}

/*! \fn void Updater::warning(const QString &message)
    This signal is emitted when an operation encounter an unexpected but non fatal error.
    \param message A human-readable description of warning.
*/

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

/*!
    \brief Check the remote repository for updates
    \sa checkForUpdatesFinished()
*/
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
/*! \fn void Updater::checkForUpdatesFinished(bool success)
    This signal is emitted once checkForUpdates() has finished is work
    \param success True if check for updates succeeded, false otherwise.
                   See errorString() for details about the last error.
    \sa checkForUpdates()
*/

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

/*!
    \brief Update the local repository
    \pre A check for remote updates must have been done.
    \sa updateFinished()
*/
void Updater::update()
{
    if(state() == UpdateRequired || state() == AlreadyUptodate)
    {
        setState(Updating);
        clearError();
        m_localRepository.setUpdateInProgress(true);
        m_localRepository.save();

        DownloadManager * downloader = new DownloadManager(m_localRepository, this);
        connect(downloader, &DownloadManager::checkProgress, this, &Updater::updateCheckProgress);
        connect(downloader, &DownloadManager::downloadProgress, this, &Updater::updateDownloadProgress);
        connect(downloader, &DownloadManager::applyProgress, this, &Updater::updateApplyProgress);
        connect(downloader, &DownloadManager::progress, this, &Updater::updateProgress);
        connect(downloader, &DownloadManager::finished, this, &Updater::updateFinished);
        connect(downloader, &DownloadManager::updateSucceeded, this, &Updater::updateSucceeded);
        connect(downloader, &DownloadManager::updateFailed, this, &Updater::updateFailed);
    }
    else
    {
        qCWarning(LOG_UPDATER) << "Called without an available update";
    }
}
/*! \fn void Updater::updateFinished(bool success)
    This signal is emitted once update() has finished is work
    \param success True if the update operation has succeeded, false otherwise.
                   See errorString() for details about the last error.
    \sa update()
*/
/*! \fn void Updater::updateProgress(qint64 bytesProcessed, qint64 bytesTotal)
    This signal is emitted once the update progression has changed
    \param bytesProcessed The number of bytes processed
    \param bytesTotal The total number of bytes beeing processed
    \sa update()
*/
/*! \fn void Updater::updateCheckProgress(qint64 bytesChecked, qint64 bytesTotal)
    This signal is emitted once the check step of the update has progressed
    \param bytesChecked The number of bytes processed
    \param bytesTotal The total number of bytes beeing processed
    \sa update()
*/
/*! \fn void Updater::updateDownloadProgress(qint64 bytesDownloaded, qint64 bytesTotal)
    This signal is emitted once the download step of the update has progressed
    \param bytesDownloaded The number of bytes processed
    \param bytesTotal The total number of bytes beeing processed
    \sa update()
*/
/*! \fn void Updater::updateApplyProgress(qint64 bytesApplied, qint64 bytesTotal)
    This signal is emitted once the apply step of the update has progressed
    \param bytesApplied The number of bytes processed
    \param bytesTotal The total number of bytes beeing processed
    \sa update()
*/

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
