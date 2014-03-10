#ifndef REMOTEUPDATE_H
#define REMOTEUPDATE_H

#include "qtupdatesystem_global.h"
#include "common/version.h"
#include "updater/localrepository.h"

#include <QObject>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;
class QAuthenticator;
class QSettings;

class QTUPDATESYSTEMSHARED_EXPORT Updater : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    enum State
    {
        Idle, ///< Nothing done.
        UpdateRequired, ///< An update is required.
        AlreadyUptodate, ///< Local repository is already uptodate.
        Uptodate, ///< Local repository is new uptodate.
        DownloadingInformations, ///< Downloading informations about the remote versions.
        Updating ///< Updating in progress
    };

    Updater(const QString &updateDirectory, const QString &remoteRepository = QString(), QObject * parent = 0);
    ~Updater();

    bool isIdle() const;
    bool isUpdateAvailable() const;

    QString localRevision() const;
    QString remoteRevision() const;
    Version remoteVersion() const;

    QString localRepository() const;

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
    void updateRequired();
    void checkForUpdatesFinished();
    void copyProgress(int copiedFileCount, int totalFileCount);
    void copyFinished();
    void updateProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateCheckProgress(qint64 bytesApplied, qint64 bytesTotal);
    void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateApplyProgress(qint64 bytesApplied, qint64 bytesTotal);
    void updateFinished();

private slots:
    void onInfoFinished();
    void updateSucceeded();
    void updateFailed(const QString &reason);
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);

private:
    QNetworkReply *get(const QString &what);
    void clearError();
    void failure(const QString & msg);
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
    State m_state;
};

inline bool Updater::isIdle() const
{
    return state() < DownloadingInformations;
}

inline bool Updater::isUpdateAvailable() const
{
    return state() == UpdateRequired;
}

inline QString Updater::localRevision() const
{
    return m_localRepository.revision();
}

inline QString Updater::remoteRevision() const
{
    return m_remoteRevision.revision;
}

inline Version Updater::remoteVersion() const
{
    return m_remoteRevision;
}

inline QString Updater::localRepository() const
{
    return m_localRepository.directory();
}

inline QString Updater::tmpDirectory() const
{
    return m_updateTmpDirectory;
}

inline QString Updater::remoteRepository() const
{
    return m_updateUrl;
}

inline void Updater::setRemoteRepository(const QString &updateUrl)
{
    Q_ASSERT(isIdle());
    m_updateUrl = updateUrl;
}

inline QString Updater::username() const
{
    return m_username;
}

inline QString Updater::password() const
{
    return m_password;
}

inline void Updater::setCredentials(const QString &username, const QString &password)
{
    Q_ASSERT(isIdle());
    m_username = username;
    m_password = password;
}

inline QString Updater::errorString() const
{
    return m_errorString;
}

inline Updater::State Updater::state() const
{
    return m_state;
}

inline void Updater::clearError()
{
    m_errorString = QString();
}

inline void Updater::failure(const QString &msg)
{
    m_errorString = msg;
}

inline void Updater::setState(Updater::State newState)
{
    m_state = newState;
}

#endif // REMOTEUPDATE_H
