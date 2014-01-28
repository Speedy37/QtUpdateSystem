#ifndef UPDATER_PATCHOPERATION_H
#define UPDATER_PATCHOPERATION_H

#include "addoperation.h"

class PatchOperation : public AddOperation
{
    Q_OBJECT
public:
    PatchOperation(DownloadManager * _update) : AddOperation(_update) {}
    virtual void load1(const QJsonObject &object);
    virtual QString actionString() Q_DECL_OVERRIDE  { return QStringLiteral("PATCH"); }
    virtual void run() Q_DECL_OVERRIDE;
protected:
    virtual void save1(QJsonObject & object);
    qint64 currentSize;
    QCryptographicHash::Algorithm currentHashType;
    QString currentHash;
};

#endif // UPDATER_PATCHOPERATION_H
