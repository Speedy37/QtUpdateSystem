#ifndef UPDATER_FILEMANAGER_H
#define UPDATER_FILEMANAGER_H

#include <QObject>

class Operation;

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = 0);

signals:
    void operationPrepared(Operation * operation, bool downloadRequired);
    void operationApplied(Operation * operation);

public slots:
    void prepareOperation(Operation * operation);
    void applyOperation(Operation * operation);
};

#endif // UPDATER_FILEMANAGER_H
