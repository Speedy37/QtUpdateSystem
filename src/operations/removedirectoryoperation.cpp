#include "removedirectoryoperation.h"

#include <qtlog.h>
#include <QFileInfo>
#include <QDir>

const QString RemoveDirectoryOperation::Action = QStringLiteral("rmdir");

void RemoveDirectoryOperation::create(const QString &path)
{
    m_path = path;
}

Operation::Status RemoveDirectoryOperation::localDataStatus()
{
    QFileInfo dirInfo(localFilename());
    if(!dirInfo.exists())
    {
        LOG_INFO(QObject::tr("Directory %1 was already removed").arg(path()));
        return Valid;
    }

    return ApplyRequired;
}

void RemoveDirectoryOperation::applyData()
{
    QFileInfo dirInfo(localFilename());
    if(!dirInfo.isDir())
    {
        LOG_WARN(QObject::tr("The update was supposed to remove the directory %1, but a file was found").arg(path()));
        if(!QFile::remove(localFilename()))
            throw QObject::tr("The update failed to remove the file %1").arg(path());
        LOG_INFO(QObject::tr("Successfully removed file %1 that was supposed to be a directory").arg(path()));
    }
    else
    {
        if(!QDir().rmdir(localFilename()))
            throw QObject::tr("Failed to remove directory %1").arg(path());
        LOG_INFO(QObject::tr("Directory %1 removed").arg(path()));
    }
}

QString RemoveDirectoryOperation::action() const
{
    return Action;
}
