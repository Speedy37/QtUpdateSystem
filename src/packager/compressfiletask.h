#ifndef COMPRESSFILETASK_H
#define COMPRESSFILETASK_H

#include "taskinfo.h"
#include <QRunnable>

class CompressFileTask : public QRunnable
{
public:
    CompressFileTask(TaskInfo * info, const QString &sourceFileName)
    {
        this->info = info;
        this->sourceFilename = sourceFileName;
    }

    void run();
    void compress(bool isSourceFinalFile);
protected:
    TaskInfo * info;
    QString sourceFilename;
};

#endif // COMPRESSFILETASK_H
