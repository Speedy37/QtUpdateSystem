#include "packagertask.h"
#include "../operations/operation.h"
#include "../operations/addoperation.h"
#include "../operations/patchoperation.h"
#include "../operations/removeoperation.h"
#include "../operations/removedirectoryoperation.h"

PackagerTask::PackagerTask(PackagerTask::Type operationType, QString path, QString oldFilename, QString newFilename)
    : QRunnable()
{
    this->operationType = operationType;
    this->path = path;
    this->oldFilename = oldFilename;
    this->newFilename = newFilename;
    this->operation = nullptr;
    setAutoDelete(false);
}

PackagerTask::~PackagerTask() : QRunnable::~QRunnable()
{
    delete operation;
}

void PackagerTask::run()
{
    switch (operationType) {
    case Add:
        operation = new AddOperation();
        break;
    case Patch:
        operation = new PatchOperation();
        break;
    case RemoveFile:
        operation = new RemoveOperation();
        break;
    case RemoveDir:
        operation = new RemoveDirectoryOperation();
        break;
    }

    operation->create(path, oldFilename, newFilename, tmpDirectory);
}
