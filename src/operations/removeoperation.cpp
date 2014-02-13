#include "removeoperation.h"

#include <qtlog.h>
#include <QFileInfo>
#include <QDir>

const QString RemoveOperation::Action = QStringLiteral("rm");

void RemoveOperation::create(const QString &path, const QString &oldFilename, const QString &newFilename, const QString &tmpDirectory)
{

}

Operation::Status RemoveOperation::localDataStatus()
{
    QFileInfo fileInfo(localFilename());
    if(!fileInfo.exists())
    {
        LOG_INFO(QObject::tr("File %1 was already removed").arg(path()));
        return Valid;
    }

    return ApplyRequired;
}

void RemoveOperation::applyData()
{
    QFileInfo dirInfo(localFilename());
    if(dirInfo.isDir())
    {
        LOG_WARN(QObject::tr("The update was supposed to remove a file %1, but a directory was found").arg(path()));
        if(!QDir().rmdir(localFilename()))
            throw QObject::tr("Failed to remove directory %1").arg(path());
        LOG_INFO(QObject::tr("Successfully removed directory %1 that was supposed to be a file").arg(path()));
    }
    else
    {
        if(!QFile::remove(localFilename()))
            throw QObject::tr("The update failed to remove the file %1").arg(path());
        LOG_INFO(QObject::tr("File %1 removed").arg(path()));
    }
}

QString RemoveOperation::action()
{
    return Action;
}
