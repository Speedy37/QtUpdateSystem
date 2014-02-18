#include "filemanager.h"
#include "../operations/operation.h"

FileManager::FileManager(QObject *parent) :
    QObject(parent)
{
}

FileManager::~FileManager()
{

}

void FileManager::prepareOperation(QSharedPointer<Operation> operation)
{
    operation->checkLocalData();
    emit operationPrepared(operation);
}

void FileManager::applyOperation(QSharedPointer<Operation> operation)
{
    operation->apply();
}

void FileManager::downloadFinished()
{
    emit applyFinished();
}
