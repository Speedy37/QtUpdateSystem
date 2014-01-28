#ifndef UPDATER_REMOVEOPERATION_H
#define UPDATER_REMOVEOPERATION_H

#include "operation.h"

class RemoveOperation : public Operation
{
    Q_OBJECT
public:
    RemoveOperation(DownloadManager * _update) : Operation(_update) {}
    virtual QString actionString() Q_DECL_OVERRIDE  { return QStringLiteral("RM"); }
    virtual void run() Q_DECL_OVERRIDE;
    virtual void applyLocally(const QString &localFolder) Q_DECL_OVERRIDE;
};

#endif // UPDATER_REMOVEOPERATION_H
