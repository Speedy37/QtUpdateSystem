#include "removedirectoryoperation.h"

#include <QLoggingCategory>
#include <QFileInfo>
#include <QDir>

Q_LOGGING_CATEGORY(LOG_RMDIROP, "updatesystem.common.rmdir")

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
        qCDebug(LOG_RMDIROP) << "Directory " << path() << " was already removed";
        return Valid;
    }

    return ApplyRequired;
}

void RemoveDirectoryOperation::applyData()
{
    QFileInfo dirInfo(localFilename());
    if(!dirInfo.isDir())
    {
        qCWarning(LOG_RMDIROP) << "The update was supposed to remove a directory, but a file was found" << path();
        if(!QFile::remove(localFilename()))
            throw QObject::tr("The update failed to remove the file %1").arg(path());
    }
    else
    {
        if(!QDir().rmdir(localFilename()))
            throw QObject::tr("Failed to remove directory %1").arg(path());
        qCDebug(LOG_RMDIROP) << "Directory removed" << path();
    }
}

QString RemoveDirectoryOperation::type() const
{
    return Action;
}
