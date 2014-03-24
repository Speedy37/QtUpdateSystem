#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "../errors/warning.h"
#include "../updater.h"
#include "../common/package.h"
#include "../common/packages.h"
#include "../common/packagemetadata.h"
#include <QFile>
#include <QObject>
#include <QMap>
#include <QSet>

class QNetworkReply;
class QNetworkAccessManager;
class QTemporaryDir;
class Operation;

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager(const LocalRepository &sourceRepository, Updater *updater);
    ~DownloadManager();
private slots:
    void update();
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
    void progress(qint64 bytesProgressed, qint64 bytesTotal);
    void checkProgress(qint64 bytesChecked, qint64 bytesTotal);
    void applyProgress(qint64 bytesApplied, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(const QString &errorString);
    void warning(const Warning &warning);

private:
    enum Failure
    {
        DownloadFailed,
        LocalFileInvalid,
        ApplyFailed,
        Fixed,
        NonRecoverable,
        FixInProgress,
        DownloadRenameFailed
    };
    void success();
    void failure(const QString &path, Failure reason);
    void updatePackageLoop();
    void loadPackageMetadata();
    void packageMetadataReady();
    void updateDataReadAll();
    void updateDataStartDownload();
    void updateDataStartDownload(qint64 endOffset);
    void updateDataStopDownload();
    void nextOperation();
    void operationDownloaded();
    void readyToApply(QSharedPointer<Operation> readyOperation);
    bool isLastPackage();
    bool isFixingError();
    bool tryContinueDownload(qint64 skippableSize);
    bool isSkipDownloadUseful(qint64 skippableSize);
    QNetworkReply *get(const QString &what, qint64 startPosition = 0, qint64 endPosition = 0);
    void incrementCheckPosition(qint64 size);
    void incrementDownloadPosition(qint64 size);
    void incrementApplyPosition(qint64 size);

private:
    // Configuration
    LocalRepository m_localRepository;
    QString m_updateDirectory, m_updateTmpDirectory;
    QString m_localRevision, m_remoteRevision;
    QString m_updateUrl, m_username, m_password;
    QSet<QString> m_fileListAfterUpdate, m_dirListAfterUpdate;

    // Network
    QNetworkAccessManager *m_manager;
    QNetworkReply *packagesListRequest, *metadataRequest, *dataRequest;

    // Internal
    QMap<QString, PackageMetadata> m_cachedMetadata;
    QTemporaryDir * m_temporaryDir;

    // Packages
    Packages m_packages;
    int downloadPathPos;
    QVector<Package> downloadPath;
    QMap<QString, Failure> failures; ///< Map<Path, Reason> of failures

    // Progression
    qint64 downloadSize, checkPosition, downloadPosition, applyPosition;

    // Package download/application
    PackageMetadata metadata; ///< Informations about the package currently downloaded
    QSharedPointer<Operation> operation; ///< Current operation in download
    QString fixingPath;
    QFile file;
    int preparedOperationIndex;
    int operationIndex;
    qint64 downloadSeek; ///< If seeking the download can't be done server side (ie, it's a file)
    qint64 offset; ///< Current download offset relative to operation->offset()

    // Disables the use of copy constructors and assignment operators
    Q_DISABLE_COPY(DownloadManager)
};

inline bool DownloadManager::isLastPackage()
{
    return downloadPathPos + 1 == downloadPath.size();
}

inline bool DownloadManager::isFixingError()
{
    return !fixingPath.isNull();
}

#endif // DOWNLOADMANAGER_H
