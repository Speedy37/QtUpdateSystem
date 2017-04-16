#ifndef UPDATER_ADDOPERATION_H
#define UPDATER_ADDOPERATION_H

#include "operation.h"
#include <QCryptographicHash>

class DownloadManager;
class QFile;

class AddOperation : public Operation
{
public:
    AddOperation();
    static const QString Action;
    virtual FileType fileType() const Q_DECL_OVERRIDE;
    virtual void fromJsonObjectV1(const QJsonObject &object) Q_DECL_OVERRIDE;
    void create(const QString &filepath, const QString &newFilename, const QString &tmpDirectory);
protected:
    static const QString DataOffset;
    static const QString DataSize;
    static const QString DataSha1;
    static const QString DataCompression;
    static const QString FinalSize;
    static const QString FinalSha1;
    virtual Status localDataStatus() Q_DECL_OVERRIDE;
    virtual void applyData() Q_DECL_OVERRIDE;
    virtual QString type() const Q_DECL_OVERRIDE;
    virtual void fillJsonObjectV1(QJsonObject & object) Q_DECL_OVERRIDE;
    void readAll(QIODevice *from, QIODevice *to, QCryptographicHash *hash);

    QString m_compression, m_finalSha1;
    qint64 m_finalSize;
};

#endif // UPDATER_ADDOPERATION_H
