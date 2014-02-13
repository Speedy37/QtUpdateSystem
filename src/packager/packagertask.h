#ifndef PACKAGERTASK_H
#define PACKAGERTASK_H

#include <QRunnable>
#include <QString>
#include <QSharedPointer>

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
    Type operationType;
    QString path;
    QString oldFilename;
    QString newFilename;
    QString tmpDirectory;
    QString errorString;
    QSharedPointer<Operation> operation;
    bool isRunSlow() const;
    virtual void run() Q_DECL_OVERRIDE;
};

#endif // PACKAGERTASK_H
