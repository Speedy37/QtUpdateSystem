#include "removedirectoryoperation.h"

void RemoveDirectoryOperation::run()
{
    QFileInfo dirInfo(localpath());
    if(!dirInfo.exists())
    {
        Log::warn(tr("The update was supposed to remove this directory %1, but it was already not there anymore").arg(path));
        return;
    }

    if(!dirInfo.isDir())
    {
        Log::warn(tr("The update was supposed to remove the directory %1, but a file was found").arg(path));
        if(!QFile::remove(localpath()))
            throw tr("The update failed to remove the file %1").arg(path);
        Log::info(tr("Successfully removed file %1 that was supposed to be a directory").arg(path));
        return;
    }
    if(!QDir().rmdir(localpath()))
        throw tr("Failed to remove the directory %1").arg(path);
    Log::info(tr("Successfully removed directory %1").arg(path));
}

void RemoveDirectoryOperation::applyLocally(const QString &localFolder)
{
    QString tmpUpdateDir = updateDir;
    updateDir = localFolder;
    run();
    updateDir = tmpUpdateDir;
}
