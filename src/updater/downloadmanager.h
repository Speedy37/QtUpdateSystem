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
    void operationPrepared(Operation *op);
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
    QNetworkReply *get(const QString &what, qint64 startPosition = 0, qint64 endPosition = 0);

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

    // Apply
    FileManager *m_filemanager;

    // Packages
    int downloadPathPos;
    QVector<Package> downloadPath;

    // Package download/application
    PackageMetadata metadata; ///< Informations about the package currently downloaded
    QMap<QString, Failure> failures; ///< Map<Path, Reason> of failures
    Operation *operation; ///< Current operation in download
    QFile file;
    int preparedOperationCount;
    int operationIndex; ///< Index in metadata of the current operation
    qint64 offset; ///< Current download offset relative to operation->offset()
    qint64 downloadSpeed; ///< Current download speed in bits/s

    // Disables the use of copy constructors and assignment operators
    Q_DISABLE_COPY(DownloadManager)
};

#endif // DOWNLOADMANAGER_H
