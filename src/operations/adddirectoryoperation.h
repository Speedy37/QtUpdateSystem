#ifndef UPDATER_AddDirectoryOperation_H
#define UPDATER_AddDirectoryOperation_H

#include "operation.h"

class AddDirectoryOperation : public Operation
{
public:
    static const QString Action;
    virtual FileType fileType() const Q_DECL_OVERRIDE;
    void create(const QString &path);
protected:
    virtual Status localDataStatus() Q_DECL_OVERRIDE;
    virtual void applyData() Q_DECL_OVERRIDE;
    virtual QString type() const Q_DECL_OVERRIDE;
};

#endif // UPDATER_AddDirectoryOperation_H
