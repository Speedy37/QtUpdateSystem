#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "remoteupdate.h"
#include <QFile>
#include <QObject>

class QNetworkReply;
class QNetworkAccessManager;
class Operation;

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager(RemoteUpdate *_update);
    ~DownloadManager();
    void loadMetadata(const QJsonDocument &json);
    void applyLocally(const QString &localFolder);
    QString updateDirectory() const { return m_updateDirectory; }
    QString updateTmpDirectory() const { return m_updateTmpDirectory; }
    QString updateUrl() const { return m_updateUrl; }
public slots:
    void run();

signals:
    void operationDownloaded(Operation * operation);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onDataFinished();
    void onDataDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void operationFinishedDirect(Operation *);
    void operationFinished(Operation *);
    void operationFailed(Operation *);
    void authenticationRequired(QNetworkReply *, QAuthenticator * authenticator);

private:
    QNetworkReply *get(const QString &what, qint64 startPosition = 0);
    void onDataDownloadProgress(bool final, qint64 bytesReceived, qint64 bytesTotal);
    void nextOperation(bool continuous);
    void loadMetadata1(const QJsonObject &object);

private:
    bool m_createApplyManifest;
    QString m_updateDirectory, m_updateTmpDirectory, m_updateUrl, m_metaDataBaseUrl, m_applyLocally, m_username, m_password;
    QNetworkAccessManager *m_manager;
    QNetworkReply *info, *metadata, *data;

    // Data download/application
    QFile m_dlFile;
    QVector<Operation*> m_operations;
    int m_dlOperationIdx, m_appliedSize, m_appliedCount, m_failedCount;
    qint64 m_dlReaded, m_dlBaseOffset, m_dlTotalSize, m_dlOperationLength;
    OperationThread *m_operationThread;
};

#endif // DOWNLOADMANAGER_H
