#include "removeoperation.h"

#include <log.h>
#include <QFileInfo>

void RemoveOperation::run()
{
    QFileInfo fileInfo(localpath());
    if(!fileInfo.exists())
    {
        Log::warn(tr("The update was supposed to remove this file %1, but it was already not there anymore").arg(path));
        return;
    }

    if(!fileInfo.isFile())
    {
        Log::warn(tr("The update was supposed to remove a file %1").arg(path));
    }

    if(!QFile::remove(localpath()))
        throw tr("The update failed to remove the file %1").arg(path);

    Log::info(tr("Successfully removed file %1").arg(path));
}

void RemoveOperation::applyLocally(const QString &localFolder)
{
    QString tmpUpdateDir = updateDir;
    updateDir = localFolder;
    run();
    updateDir = tmpUpdateDir;
}
