#ifndef PACKAGERTASK_H
#define PACKAGERTASK_H

#include <QRunnable>

class Operation;

class PackagerTask : public QRunnable
{
public:
    enum Type
    {
        Add,
        Patch,
        RemoveDir,
        RemoveFile
    };

    PackagerTask(Type operationType, QString path, QString oldFilename = QString(), QString newFilename = QString());
    ~PackagerTask();
    Type operationType;
    QString path;
    QString oldFilename;
    QString newFilename;
    QString tmpDirectory;
    QString errorString;
    Operation *operation;
    virtual void run() Q_DECL_OVERRIDE;
private:
    Q_DISABLE_COPY(PackagerTask)
};

#endif // PACKAGERTASK_H
