#ifndef PACKAGERTASK_H
#define PACKAGERTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>
#include <QSharedPointer>

class Operation;

class PackagerTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    enum Type
    {
        Add,
        Patch,
        RemoveDir,
        RemoveFile,
        AddDir
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
signals:
    void done();
private:
    Q_DISABLE_COPY(PackagerTask)
};

#endif // PACKAGERTASK_H
