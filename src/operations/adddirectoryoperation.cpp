#include "adddirectoryoperation.h"

#include <QLoggingCategory>
#include <QFileInfo>
#include <QDir>

Q_LOGGING_CATEGORY(LOG_MKDIROP, "updatesystem.adddirectoryoperation")

const QString AddDirectoryOperation::Action = QStringLiteral("mkdir");

void AddDirectoryOperation::create(const QString &path)
{
    setPath(path);
}

Operation::FileType AddDirectoryOperation::fileType() const
{
    return Folder;
}

Operation::Status AddDirectoryOperation::localDataStatus()
{
    QFileInfo dirInfo(localFilename());
    if(dirInfo.exists() && dirInfo.isDir())
    {
        qCDebug(LOG_MKDIROP) << "Directory " << path() << " already exists";
        return Valid;
    }

    return ApplyRequired;
}

void AddDirectoryOperation::applyData()
{
    QFileInfo dirInfo(localFilename());
    if(dirInfo.exists() && !dirInfo.isDir())
    {        
        throwWarning(QObject::tr("The update was supposed to add a directory, but a file was found"));
        if(!QFile::remove(localFilename()))            
            throw QObject::tr("The update failed to remove the file %1").arg(path());
    }

    if(!QDir().mkpath(localFilename()))
        throw QObject::tr("Failed to create directory %1").arg(path());
}

QString AddDirectoryOperation::type() const
{
    return Action;
}
