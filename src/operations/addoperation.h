#ifndef UPDATER_ADDOPERATION_H
#define UPDATER_ADDOPERATION_H

#include "operation.h"
#include <QCryptographicHash>

class DownloadManager;
class QFile;

class AddOperation : public Operation
{
public:
    static const QString Action;
    virtual void fromJsonObjectV1(const QJsonObject &object) Q_DECL_OVERRIDE;
    void create(const QString &path, const QString &newFilename, const QString &tmpDirectory);
protected:
    virtual Status localDataStatus() Q_DECL_OVERRIDE;
    virtual void applyData() Q_DECL_OVERRIDE;
    virtual QString action() Q_DECL_OVERRIDE;
    virtual void fillJsonObjectV1(QJsonObject & object) Q_DECL_OVERRIDE;

    QString m_compression, m_finalSha1;
    qint64 m_finalSize;

};

#endif // UPDATER_ADDOPERATION_H
