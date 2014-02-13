#include "filemanager.h"
#include "../operations/operation.h"

FileManager::FileManager(QObject *parent) :
    QObject(parent)
{
}

void FileManager::prepareOperation(Operation *operation)
{
    operation->checkLocalData();
    emit operationPrepared(operation);
}

void FileManager::applyOperation(Operation *operation)
{
    if(operation->status() == Operation::DownloadRequired || operation->status() == Operation::ApplyRequired)
        operation->apply();
}

void FileManager::downloadFinished()
{
    emit applyFinished();
}
