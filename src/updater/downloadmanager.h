#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "../updater.h"
#include "../common/package.h"
#include "../common/packagemetadata.h"
#include <QFile>
#include <QObject>
#include <QMap>

class QNetworkReply;
class QNetworkAccessManager;
class Operation;
class FileManager;

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager(Updater *updater);

public slots:
    void update();

private slots:
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);
    void updatePackagesListRequestFinished();
    void updatePackageMetadataFinished();
    void updateDataDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateDataFinished();
    void operationPrepared(Operation *);
    void operationApplied(Operation *);
    void applyFinished();

signals:
    void operationLoaded(Operation * operation);
    void operationDownloaded(Operation * operation);
    void downloadFinished();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void failure(const QString &reason);
    void finished();

private:
    void updateDataSetupOperationFile();
    void updatePackageLoop();
    void updateDataReadAll();
    QNetworkReply *get(const QString &what, qint64 startPosition = 0);

private:
    enum Failure
    {
        DownloadFailed
    };

    // Configuration
    bool m_createApplyManifest;
    QString m_updateDirectory, m_updateTmpDirectory;
    QString m_localRevision, m_remoteRevision;
    QString m_updateUrl, m_username, m_password;

    // Network
    QNetworkAccessManager *m_manager;
    QNetworkReply *packagesListRequest, *metadataRequest, *dataRequest;

    // Packages
    int downloadPathPos;
    QVector<Package> downloadPath;

    // Data download/application
    PackageMetadata metadata;
    QMap<QString, Failure> failures;
    Operation *operation;
    int operationIndex;
    qint64 offset;

    FileManager *m_filemanager;
};

#endif // DOWNLOADMANAGER_H
