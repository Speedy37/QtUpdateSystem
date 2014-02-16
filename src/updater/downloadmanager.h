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
    void updateDataReadyRead();
    void updateDataFinished();
    void operationPrepared(QSharedPointer<Operation> preparedOperation);
    void operationApplied(QSharedPointer<Operation> appliedOperation);
    void applyFinished();

signals:
    void operationLoaded(QSharedPointer<Operation> operation);
    void operationReadyToApply(QSharedPointer<Operation> operation);
    void downloadFinished();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void failure(const QString &reason);
    void updateSucceeded();
    void finished();

private:
    void updateDataSetupOperationFile();
    void updatePackageLoop();
    void updateDataReadAll();
    void updateDataStartDownload();
    void updateDataStopDownload();
    bool tryContinueDownload(qint64 skippableSize);
    bool isSkipDownloadUseful(qint64 skippableSize);
    QNetworkReply *get(const QString &what, qint64 startPosition = 0, qint64 endPosition = 0);

private:
    enum Failure
    {
        DownloadFailed,
        LocalFileInvalid,
        ApplyFailed
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
    QMap<QString, Failure> failures; ///< Map<Path, Reason> of failures
    qint64 downloadGlobalOffset, downloadGlobalSize;

    // Package download/application
    PackageMetadata metadata; ///< Informations about the package currently downloaded
    QSharedPointer<Operation> operation; ///< Current operation in download
    QFile file;
    int preparedOperationCount;
    int appliedOperationCount;
    int failedOperationCount;
    int operationIndex; ///< Index in metadata of the current operation
    qint64 downloadSeek; ///< If seeking the download can't be done server side (ie, it's a file)
    qint64 offset; ///< Current download offset relative to operation->offset()
    qint64 downloadSpeed; ///< Current download speed in bits/s

    // Disables the use of copy constructors and assignment operators
    Q_DISABLE_COPY(DownloadManager)
};

#endif // DOWNLOADMANAGER_H
