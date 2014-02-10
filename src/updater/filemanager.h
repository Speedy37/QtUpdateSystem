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
    void operationPrepared(Operation * operation);
    void operationApplied(Operation * operation);
    void applyFinished();

public slots:
    void prepareOperation(Operation * operation);
    void applyOperation(Operation * operation);
    void downloadFinished();
};

#endif // UPDATER_FILEMANAGER_H
