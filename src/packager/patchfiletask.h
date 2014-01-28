#ifndef PATCHFILETASK_H
#define PATCHFILETASK_H

#include "compressfiletask.h"

class PatchFileTask : public CompressFileTask
{
public:
    PatchFileTask(TaskInfo * info, const QString &sourceFileName, const QString &oldFilename, const QString &tmpFilename) : CompressFileTask(info, sourceFileName)
    {
        this->tmpFilename = tmpFilename;
        this->oldFilename = oldFilename;
    }

    void run();
protected:
    QString tmpFilename, oldFilename;
};

#endif // PATCHFILETASK_H
