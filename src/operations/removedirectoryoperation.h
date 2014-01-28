#ifndef UPDATER_REMOVEDIRECTORYOPERATION_H
#define UPDATER_REMOVEDIRECTORYOPERATION_H

#include "operation.h"

class RemoveDirectoryOperation : public Operation
{
    Q_OBJECT
public:
    RemoveDirectoryOperation(DownloadManager * _update) : Operation(_update) {}
    virtual void run() Q_DECL_OVERRIDE;
    virtual void applyLocally(const QString &localFolder) Q_DECL_OVERRIDE;
    virtual QString actionString() Q_DECL_OVERRIDE { return QStringLiteral("RMDIR"); }
};

#endif // UPDATER_REMOVEDIRECTORYOPERATION_H
