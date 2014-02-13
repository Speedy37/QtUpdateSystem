#ifndef UPDATER_FILEMANAGER_H
#define UPDATER_FILEMANAGER_H

#include <QObject>
#include <QSharedPointer>

class Operation;

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = 0);

signals:
    void operationPrepared(QSharedPointer<Operation> operation);
    void operationApplied(QSharedPointer<Operation> operation);
    void applyFinished();

public slots:
    void prepareOperation(QSharedPointer<Operation> operation);
    void applyOperation(QSharedPointer<Operation> operation);
    void downloadFinished();
};

#endif // UPDATER_FILEMANAGER_H
