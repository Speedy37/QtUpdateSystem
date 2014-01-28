#ifndef TASKINFO_H
#define TASKINFO_H

#include "../common/exception.h"
#include <QJsonObject>

struct TaskInfo
{
    Exception exception;
    QJsonObject description;
    QString destinationFileName;
};

#endif // TASKINFO_H
