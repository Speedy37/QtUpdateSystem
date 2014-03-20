#ifndef REMOTEUPDATE_H
#define REMOTEUPDATE_H

#include "qtupdatesystem_global.h"
#include "common/version.h"
#include "updater/localrepository.h"
#include "errors/warning.h"

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QAuthenticator;
class QSettings;

/*!
    \brief Provide remote update functionality

    \reentrant
*/
class QTUPDATESYSTEMSHARED_EXPORT Updater : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    /*!
       \brief Current state of the updater
     */
    enum State
    {
        Idle, ///< Nothing done.
        UpdateRequired, ///< An update is required.
        AlreadyUptodate, ///< Local repository is already uptodate.
        Uptodate, ///< Local repository is new uptodate.
        DownloadingInformations, ///< Downloading informations about the remote versions.
        Updating, ///< Updating in progress
        Copying ///< Copy in progress
    };

    Updater(QObject * parent = 0);
    ~Updater();

    bool isIdle() const;
    bool isUpdateAvailable() const;

    QString localRevision() const;
    QString remoteRevision() const;
    Version remoteVersion() const;

    QString localRepository() const;
    void setLocalRepository(const QString &localRepository);

    QString tmpDirectory() const;
    void setTmpDirectory(const QString &tmpDirectory);

    QString remoteRepository() const;
    void setRemoteRepository(const QString &remoteRepository);

    QString username() const;
    QString password() const;
    void setCredentials(const QString &username, const QString &password);

    QString errorString() const;
    State state() const;

public slots:
    void checkForUpdates();
    void update();
    void copy(const QString& copyDirectory);

signals:
    void checkForUpdatesFinished(bool success);
    void copyProgress(int copiedFileCount, int totalFileCount);
    void copyFinished(bool success);
    void updateProgress(qint64 bytesProcessed, qint64 bytesTotal);
    void updateCheckProgress(qint64 bytesChecked, qint64 bytesTotal);
    void updateDownloadProgress(qint64 bytesDownloaded, qint64 bytesTotal);
    void updateApplyProgress(qint64 bytesApplied, qint64 bytesTotal);
    void updateFinished(bool success);
    void warning(const Warning &warning);

private slots:
    void onInfoFinished();
    void onUpdateFinished(const QString &errorString);
    void onCopyFinished(const QString &errorString);
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);

private:
    QNetworkReply *get(const QString &what);
    void clearError();
    void setErrorString(const QString & msg);
    void setState(State newState);

private:
    // Network
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_currentRequest, *metadata;

    // Config
    QString m_updateTmpDirectory;
    QString m_updateUrl;
    QString m_username, m_password;

    // Informations
    LocalRepository m_localRepository;
    Version m_remoteRevision;
    QString m_errorString;
    State m_state, m_beforeCopyState;
};

/*!
    Returns \c true if no action is in progress; otherwise returns \c false.
    \sa isUpdateAvailable()
*/
inline bool Updater::isIdle() const
{
    return state() < DownloadingInformations;
}

/*!
    Returns \c true if an update is available; otherwise returns \c false.
    \sa isIdle()
*/
inline bool Updater::isUpdateAvailable() const
{
    return state() == UpdateRequired;
}

/*!
    Returns the local current version.
*/
inline QString Updater::localRevision() const
{
    return m_localRepository.revision();
}

/*!
    Returns the latest remote version name.
*/
inline QString Updater::remoteRevision() const
{
    return m_remoteRevision.revision;
}

/*!
    Returns the latest remote version.
*/
inline Version Updater::remoteVersion() const
{
    return m_remoteRevision;
}

/*!
    Returns the directory to update.
*/
inline QString Updater::localRepository() const
{
    return m_localRepository.directory();
}

/*!
    Set the directory to update.
*/
inline void Updater::setLocalRepository(const QString &localRepository)
{
    Q_ASSERT(isIdle());
    m_localRepository.setDirectory(localRepository);
}

/*!
    Returns the directory to use for storing temporary files required for doing the update.
*/
inline QString Updater::tmpDirectory() const
{
    return m_updateTmpDirectory;
}

/*!
    Set the directory to use for storing temporary files required for doing the update.
*/
inline void Updater::setTmpDirectory(const QString &updateTmpDirectory)
{
    Q_ASSERT(isIdle());
    m_updateTmpDirectory = Utils::cleanPath(updateTmpDirectory);
}

/*!
    Returns the remote update url.
*/
inline QString Updater::remoteRepository() const
{
    return m_updateUrl;
}

/*!
    Set the remote update url.
*/
inline void Updater::setRemoteRepository(const QString &updateUrl)
{
    Q_ASSERT(isIdle());
    m_updateUrl = updateUrl;
}


/*!
    username for remote url basic authentification.
    \sa password(), setCredentials()
*/
inline QString Updater::username() const
{
    return m_username;
}

/*!
    password for remote url basic authentification.
    \sa username(), setCredentials()
*/
inline QString Updater::password() const
{
    return m_password;
}

/*!
    Set the username and password for remote url basic authentification.
    \sa setRemoteRepository(), remoteRepository(), username(), password()
*/
inline void Updater::setCredentials(const QString &username, const QString &password)
{
    Q_ASSERT(isIdle());
    m_username = username;
    m_password = password;
}

/*!
   Returns a human-readable description of the last error that occurred.
 */
inline QString Updater::errorString() const
{
    return m_errorString;
}

/*!
    Returns this object state.
    \sa Updater::State
*/
inline Updater::State Updater::state() const
{
    return m_state;
}

inline void Updater::clearError()
{
    m_errorString = QString();
}

inline void Updater::setErrorString(const QString &msg)
{
    m_errorString = msg;
}

inline void Updater::setState(Updater::State newState)
{
    m_state = newState;
}

#endif // REMOTEUPDATE_H
