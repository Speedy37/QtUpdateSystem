#include "copythread.h"
#include "localrepository.h"
#include "../common/utils.h"
#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QDir>

CopyThread::CopyThread(const LocalRepository &sourceRepository, const QString &destinationDir, QObject *parent) :
    QThread(parent), m_sourceRepository(sourceRepository)
{
    m_destinationDir = Utils::cleanPath(destinationDir);
}

void CopyThread::run()
{
    try
    {
        LocalRepository destRepository(m_destinationDir);
        QSet<QString> sourcefiles = m_sourceRepository.fileList().toSet();
        QSet<QString> dirfiles = m_sourceRepository.dirList().toSet();

        int i = 0;
        QFileInfo destFileInfo;
        QDir dir;
        QString src, dest;

        foreach(const QString &dirname, dirfiles)
        {
            dest = m_destinationDir + dirname;
            if(!dir.mkpath(dest))
                EMIT_WARNING(CopyMkPath, tr("Unable to create path %1").arg(dest), dirname);
        }

        foreach(const QString &filename, sourcefiles)
        {
            src = m_sourceRepository.directory() + filename;
            dest = m_destinationDir + filename;

            destFileInfo.setFile(dest);
            if(!dir.mkpath(destFileInfo.absolutePath()))
                EMIT_WARNING(CopyMkPath, tr("Unable to create path %1").arg(destFileInfo.absolutePath()), filename);

            if(QFile::exists(dest) && !QFile::remove(dest))
                EMIT_WARNING(CopyRemove, tr("Unable to remove %1").arg(dest), filename);

            if(!QFile::copy(src, dest))
                EMIT_WARNING(Copy, tr("Unable to copy %1 to %2").arg(src, dest), filename);

            emit progression(++i, sourcefiles.size());
        }

        QSet<QString> diffFiles = destRepository.fileList().toSet() - sourcefiles - dirfiles;
        foreach(const QString &filename, diffFiles)
        {
            dest = m_destinationDir + filename;
            if(QFile::exists(dest) && !QFile::remove(dest))
                EMIT_WARNING(CopyRemove, tr("Unable to remove %1").arg(dest), filename);
        }

        QSet<QString> diffDirList = destRepository.dirList().toSet() - sourcefiles - dirfiles;
        foreach(const QString &dirToRemove, diffDirList)
        {
            QDir dir(m_destinationDir + dirToRemove);
            if(dir.exists())
                dir.rmdir(dir.path());
        }

        destRepository.setFileList(m_sourceRepository.fileList());
        destRepository.setDirList(m_sourceRepository.dirList());
        destRepository.setRevision(m_sourceRepository.revision());
        destRepository.setUpdateInProgress(m_sourceRepository.updateInProgress());
        destRepository.save();

        emit copyFinished(QString());
    }
    catch(const QString &msg)
    {
        emit copyFinished(msg);
    }
}
