#include "updater.h"
#include "exceptions.h"
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
#include <QDir>

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
        qWarning("Updater::checkForUpdates : Called while not Idle");
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
            THROW(RequestFailed, m_currentRequest->errorString());

        qCDebug(LOG_UPDATER) << "Remote informations downloaded";

        m_remoteRevision.fromJsonObject(JsonUtil::fromJson(m_currentRequest->readAll()));

        qCDebug(LOG_UPDATER) << "Remote informations analyzed";

        if(!m_localRepository.isConsistent())
        {
            qCDebug(LOG_UPDATER) << "An update was in progress";
            setState(UpdateRequired);
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
        }
        emit checkForUpdatesFinished(true);
    }
    catch(std::exception & msg)
    {
        setErrorString(QString(msg.what()));
        setState(Idle);
        emit checkForUpdatesFinished(false);
    }
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
        connect(downloader, &DownloadManager::finished, this, &Updater::onUpdateFinished);
        connect(downloader, &DownloadManager::warning, this, &Updater::warning);
    }
    else
    {
        qWarning("Updater::update : Called without an available update");
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

void Updater::onUpdateFinished(const QString &errorString)
{
    if(errorString.isEmpty())
    {
        m_localRepository.load();
        setState(Uptodate);
        emit updateFinished(true);
    }
    else
    {
        setErrorString(errorString);
        setState(UpdateRequired);
        emit updateFinished(false);
    }
}

/*!
    \brief Copy the local repository to another directory.
    If the copy destination is a repository, the copy operation update it.
    \sa copyFinished()
*/
void Updater::copy(const QString &copyDirectory)
{
    if(isIdle())
    {
        m_beforeCopyState = state();
        setState(Copying);
        clearError();

        CopyThread *copier = new CopyThread(m_localRepository, copyDirectory, this);

        connect(copier, &CopyThread::progression, this, &Updater::copyProgress);
        connect(copier, &CopyThread::copyFinished, this, &Updater::onCopyFinished);
        connect(copier, &CopyThread::warning, this, &Updater::warning);
        connect(copier, &CopyThread::finished, copier, &CopyThread::deleteLater);
        copier->start();
    }
    else
    {
        qWarning("Updater::copy : Called while not Idle");
    }
}
/*! \fn void Updater::copyFinished(bool success)
    This signal is emitted once copy() operation has finished is work
    \param success True if the copy operation has succeeded, false otherwise.
                   See errorString() for details about the last error.
    \sa copy()
*/
/*! \fn void Updater::copyProgress(int copiedFileCount, int totalFileCount)
    This signal is emitted once the copy progressed
    \param copiedFileCount The number of files copied
    \param totalFileCount The number of files beeing copied
    \sa copy()
*/

void Updater::onCopyFinished(const QString &errorString)
{
    setErrorString(errorString);
    setState(m_beforeCopyState);
    emit copyFinished(errorString.isEmpty());
}

/*!
    \brief Search in the managed directory for non managed files
    \param testFunction Called for each file/dir with the file/dir as parameter.
                        If the function return true, the file is deleted.
*/
void Updater::removeOtherFiles(std::function<bool(QFileInfo)> testFunction)
{
    if(isIdle())
    {
        removeOtherFiles(m_localRepository.directory(), testFunction);
    }
    else
    {
        qWarning("Updater::removeOtherFiles : Called while not Idle");
    }
}

void Updater::removeOtherFiles(const QString &directory, std::function<bool(QFileInfo)> testFunction)
{
    QDir dir(directory);
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);
    foreach(const QFileInfo &file, list)
    {
        if(!isManaged(file))
        {
            if(!testFunction || testFunction(file))
            {
                if(file.isDir())
                {
                    if(!dir.rmpath(file.absoluteFilePath()))
                    {
                        qCWarning(LOG_UPDATER) << "Unable to remove dir" << file.path();
                    }
                }
                else
                {
                    if(!QFile::remove(file.absoluteFilePath()))
                    {
                        qCWarning(LOG_UPDATER) << "Unable to remove file" << file.path();
                    }
                }
            }
        }
        else if(file.isDir())
        {
            removeOtherFiles(file.absoluteFilePath(), testFunction);
        }
    }
}

void Updater::authenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    if(!m_password.isEmpty() && authenticator->password() != m_password)
        authenticator->setPassword(m_password);
    if(!m_username.isEmpty() && authenticator->user() != m_username)
        authenticator->setUser(m_username);
}
