#include "removedirectoryoperation.h"

#include <QLoggingCategory>
#include <QFileInfo>
#include <QDir>

Q_LOGGING_CATEGORY(LOG_RMDIROP, "updatesystem.common.rmdir")

const QString RemoveDirectoryOperation::Action = QStringLiteral("rmdir");

void RemoveDirectoryOperation::create(const QString &path)
{
    setPath(path);
}

Operation::Status RemoveDirectoryOperation::localDataStatus()
{
    QFileInfo dirInfo(localFilename());
    if(!dirInfo.isDir())
    {
        qCDebug(LOG_RMDIROP) << "Directory " << path() << " was already removed";
        return Valid;
    }

    return ApplyRequired;
}

void RemoveDirectoryOperation::applyData()
{
    QFileInfo dirInfo(localFilename());
    if(dirInfo.isDir() && !QDir().rmdir(localFilename()))
    {
        throwWarning(QObject::tr("Failed to remove directory"));
    }
    else
    {
        qCDebug(LOG_RMDIROP) << "Directory removed" << path();
    }
}

QString RemoveDirectoryOperation::type() const
{
    return Action;
}
