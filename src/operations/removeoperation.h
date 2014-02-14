#ifndef UPDATER_REMOVEOPERATION_H
#define UPDATER_REMOVEOPERATION_H

#include "operation.h"

class RemoveOperation : public Operation
{
public:
    RemoveOperation();
    static const QString Action;
    void create(const QString &path, const QString &oldFilename);
    virtual void fromJsonObjectV1(const QJsonObject &object);
protected:
    virtual void fillJsonObjectV1(QJsonObject & object) Q_DECL_OVERRIDE;
    virtual Status localDataStatus() Q_DECL_OVERRIDE;
    virtual void applyData() Q_DECL_OVERRIDE;
    virtual QString action() const Q_DECL_OVERRIDE;

    QString m_localSha1;
    qint64 m_localSize;
};

#endif // UPDATER_REMOVEOPERATION_H
