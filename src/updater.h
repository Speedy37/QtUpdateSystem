#ifndef REMOTEUPDATE_H
#define REMOTEUPDATE_H

#include "qtupdatesystem_global.h"
#include "common/version.h"
#include <QThread>
#include <QVector>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QEvent>

class QNetworkReply;
class Operation;
class QSettings;
class OperationThread;
class DownloadManager;

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

    QString updateDirectory() const;
    void setUpdateDirectory(const QString &updateDirectory);

    QString updateTmpDirectory() const;
    void setUpdateTmpDirectory(const QString &updateTmpDirectory);

    QString updateUrl() const;
    void setUpdateUrl(const QString &updateUrl);

    QString username() const;
    QString password() const;
    void setCredential(const QString &username, const QString &password);

    bool createApplyManifest() const;
    void setCreateApplyManifest(bool createApplyManifest);

    QString errorString() const;
    State state() const;

    QString iniCurrentVersion() const;

public slots:
    void checkForUpdates();
private slots:
    void onInfoFinished();
signals:
    void updateRequired();
    void checkForUpdatesFinished();

public slots:
    void update();
signals:
    void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateApplyProgress(qint64 bytesApplied, qint64 bytesTotal);
    void updateFinished();

private slots:
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);
    void onMetadataFinished();
    void onUpdateFinished(bool success);
    void onApplyFinished(bool success);
    void actionFailed(const QString &msg);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void applyProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateFinished(bool success);
    void applyFinished(bool success);

    void error(const QString & msg);

private:
    void clearError();
    QNetworkReply *get(const QString &what);
    void loadVersions(const QJsonDocument &json);
    void loadVersions1(const QJsonObject &object);
    void setState(State newState);
    void setCurrentVersion(const QString & versionName);

private:
    // Network
    QNetworkAccessManager *m_manager;
    QNetworkReply *info, *metadata;

    // Config
    bool m_createApplyManifest;
    QString m_updateDirectory, m_updateTmpDirectory, m_updateUrl, m_username, m_password;

    // Informations
    QString m_localRevision, m_remoteRevision;
    QString m_errorString;
    State m_state;
    QVector<Version> m_versions;
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
    return m_remoteRevision;
}

inline QString Updater::updateDirectory() const
{
    return m_updateDirectory;
}

inline void Updater::setUpdateDirectory(const QString &updateDirectory)
{
    Q_ASSERT(isIdle());
    m_updateDirectory = updateDirectory;
    if(!m_updateDirectory.endsWith(QDir::separator()))
        m_updateDirectory += QDir::separator();
}

inline QString Updater::updateTmpDirectory() const
{
    return m_updateTmpDirectory;
}

inline void Updater::setUpdateTmpDirectory(const QString &updateTmpDirectory)
{
    Q_ASSERT(isIdle());
    m_updateTmpDirectory = updateTmpDirectory;
    if(!m_updateTmpDirectory.endsWith(QDir::separator()))
        m_updateTmpDirectory += QDir::separator();
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

inline void Updater::setState(Updater::State newState)
{
    m_state = newState;
}

inline void Updater::setCurrentVersion(const QString &versionName)
{
    Q_ASSERT(isIdle());
    m_currentVersion = versionName;
}

#endif // REMOTEUPDATE_H
