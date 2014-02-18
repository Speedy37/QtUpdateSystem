#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "../updater.h"
#include "../common/package.h"
#include "../common/packages.h"
#include "../common/packagemetadata.h"
#include <QFile>
#include <QObject>
#include <QMap>
#include <QThread>

class QNetworkReply;
class QNetworkAccessManager;
class Operation;
class FileManager;


class OneObjectThread : public QThread
{
    Q_OBJECT
public:
    OneObjectThread(QObject * parent = 0) : QThread(parent)
    {
        m_time = 200;
    }

    void manage(QObject *object)
    {
        object->moveToThread(this);
        connect(this, &OneObjectThread::destroying, object, &QObject::deleteLater);
        connect(object, &QObject::destroyed, [this]() {
            quit();
        });
        connect(this, &QThread::finished, this, &QThread::deleteLater);
    }

    void setMaxWaitTime(unsigned long time)
    {
        m_time = time;
    }

    ~OneObjectThread()
    {
        emit destroying();
        wait(m_time);
    }
signals:
    void destroying();
private:
    unsigned long m_time;
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager(Updater *updater);
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
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateFailed(const QString &reason);
    void updateSucceeded();
    void finished();

private:
    void failure(const QString &reason);
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
        ApplyFailed,
        FixInProgress,
        Fixed,
        NonRecoverable
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
    Packages m_packages;
    int downloadPathPos;
    QVector<Package> downloadPath;
    QMap<QString, Failure> failures; ///< Map<Path, Reason> of failures
    qint64 downloadGlobalOffset, downloadGlobalSize;

    // Package download/application
    PackageMetadata metadata; ///< Informations about the package currently downloaded
    QSharedPointer<Operation> operation; ///< Current operation in download
    QString fixingPath;
    QFile file;
    int preparedOperationCount;
    int operationIndex; ///< Index in metadata of the current operation
    qint64 downloadSeek; ///< If seeking the download can't be done server side (ie, it's a file)
    qint64 offset; ///< Current download offset relative to operation->offset()
    qint64 downloadSpeed; ///< Current download speed in bits/s

    // Disables the use of copy constructors and assignment operators
    Q_DISABLE_COPY(DownloadManager)
};

#endif // DOWNLOADMANAGER_H
