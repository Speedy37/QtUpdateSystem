#ifndef UPDATER_REMOVEOPERATION_H
#define UPDATER_REMOVEOPERATION_H

#include "operation.h"

class RemoveOperation : public Operation
{
public:
    static const QString Action;
    void create(const QString &path);
protected:
    virtual Status localDataStatus() Q_DECL_OVERRIDE;
    virtual void applyData() Q_DECL_OVERRIDE;
    virtual QString action() Q_DECL_OVERRIDE;
};

#endif // UPDATER_REMOVEOPERATION_H
