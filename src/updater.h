#ifndef REMOTEUPDATE_H
#define REMOTEUPDATE_H

#include "qtupdatesystem_global.h"
#include "common/version.h"

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QAuthenticator;

class QTUPDATESYSTEMSHARED_EXPORT Updater : public QObject
{
    Q_OBJECT
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

    Updater(QObject * parent = 0);
    ~Updater();

    bool isIdle() const;
    bool isUpdateAvailable() const;

    QString localRevision() const;
    QString remoteRevision() const;
    Version remoteVersion() const;

    QString updateDirectory() const;
    void setUpdateDirectory(const QString &updateDirectory);

    QString updateTmpDirectory() const;
    void setUpdateTmpDirectory(const QString &updateTmpDirectory);

    QString updateUrl() const;
    void setUpdateUrl(const QString &updateUrl);

    QString username() const;
    QString password() const;
    void setCredential(const QString &username, const QString &password);

    QString errorString() const;
    State state() const;

    QString iniCurrentVersion() const;

public slots:
    void checkForUpdates();
    void update();
private slots:
    void onInfoFinished();
signals:
    void updateRequired();
    void checkForUpdatesFinished();

public slots:
signals:
    void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateApplyProgress(qint64 bytesApplied, qint64 bytesTotal);
    void updateFinished();

private slots:
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);
    void actionFailed(const QString &msg);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void applyProgress(qint64 bytesReceived, qint64 bytesTotal);
    void applyFinished(bool success);

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
    QString m_updateDirectory, m_updateTmpDirectory;
    QString m_updateUrl;
    QString m_username, m_password;

    // Informations
    QString m_localRevision;
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
    return m_localRevision;
}

inline QString Updater::remoteRevision() const
{
    return m_remoteRevision.revision;
}

inline Version Updater::remoteVersion() const
{
    return m_remoteRevision;
}

inline QString Updater::updateDirectory() const
{
    return m_updateDirectory;
}

inline QString Updater::updateTmpDirectory() const
{
    return m_updateTmpDirectory;
}

inline QString Updater::updateUrl() const
{
    return m_updateUrl;
}

inline void Updater::setUpdateUrl(const QString &updateUrl)
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

inline void Updater::setCredential(const QString &username, const QString &password)
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
