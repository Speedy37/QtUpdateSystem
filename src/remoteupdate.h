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

using namespace Updater;

class QNetworkReply;
class Operation;
class QSettings;
class OperationThread;
class DownloadManager;

class QTUPDATESYSTEMSHARED_EXPORT RemoteUpdate : public QObject
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

    RemoteUpdate(QObject * parent = 0);
    ~RemoteUpdate();

    bool isIdle() const;
    bool isUpdateAvailable() const;

    QString currentVersion() const;
    QString latestVersion() const;
    QVector<Version> versions() const;

    QString updateDirectory() const;
    void setUpdateDirectory(const QString &updateDirectory);

    QString updateTmpDirectory() const;
    void setUpdateTmpDirectory(const QString &updateTmpDirectory);

    QString updateUrl() const;
    void setUpdateUrl(const QString &updateUrl);
    void setCredential(const QString &username, const QString &password);

    bool createApplyManifest() const;
    void setCreateApplyManifest(bool createApplyManifest);

    QString lastError() const;
    State state() const;

    QString iniCurrentVersion() const;

public slots:
    void checkForUpdates();
    void update();

private slots:
    void onInfoFinished();
    void onMetadataFinished();
    void onUpdateFinished(bool success);
    void onApplyFinished(bool success);
    void actionFailed(const QString &msg);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onApplyProgress(qint64 bytesReceived, qint64 bytesTotal);
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);

signals:
    void currentVersionChanged(const QString &version);
    void updateRequired();
    void checkForUpdatesFinished();

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
    QString m_currentVersion, m_metaDataBaseUrl, m_lastError;
    State m_state;
    QVector<Version> m_versions;
};

inline bool RemoteUpdate::isIdle() const
{
    return state() < DownloadingInformations;
}

inline bool RemoteUpdate::isUpdateAvailable() const
{
    return state() == UpdateRequired;
}

inline QString RemoteUpdate::currentVersion() const
{
    return m_currentVersion;
}

inline QString RemoteUpdate::latestVersion() const
{
    return m_versions.last().revision;
}

inline QVector<Version> RemoteUpdate::versions() const
{
    return m_versions;
}

inline QString RemoteUpdate::updateDirectory() const
{
    return m_updateDirectory;
}

inline void RemoteUpdate::setUpdateDirectory(const QString &updateDirectory)
{
    Q_ASSERT(isIdle());
    m_updateDirectory = updateDirectory;
    if(!m_updateDirectory.endsWith(QDir::separator()))
        m_updateDirectory += QDir::separator();
}

inline QString RemoteUpdate::updateTmpDirectory() const
{
    return m_updateTmpDirectory;
}

inline void RemoteUpdate::setUpdateTmpDirectory(const QString &updateTmpDirectory)
{
    Q_ASSERT(isIdle());
    m_updateTmpDirectory = updateTmpDirectory;
    if(!m_updateTmpDirectory.endsWith(QDir::separator()))
        m_updateTmpDirectory += QDir::separator();
}

inline QString RemoteUpdate::updateUrl() const
{
    return m_updateUrl;
}

inline void RemoteUpdate::setUpdateUrl(const QString &updateUrl)
{
    Q_ASSERT(isIdle());
    m_updateUrl = updateUrl;
}

inline void RemoteUpdate::setCredential(const QString &username, const QString &password)
{
    Q_ASSERT(isIdle());
    m_username = username;
    m_password = password;
}

inline QString RemoteUpdate::lastError() const
{
    return m_lastError;
}

inline RemoteUpdate::State RemoteUpdate::state() const
{
    return m_state;
}

inline void RemoteUpdate::clearError()
{
    m_lastError = QString();
}

inline void RemoteUpdate::setState(RemoteUpdate::State newState)
{
    m_state = newState;
}

inline void RemoteUpdate::setCurrentVersion(const QString &versionName)
{
    Q_ASSERT(isIdle());
    m_currentVersion = versionName;
    emit currentVersionChanged(versionName.isEmpty() ? tr("Unknown") : versionName);
}

#endif // REMOTEUPDATE_H
