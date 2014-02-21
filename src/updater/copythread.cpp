#include "copythread.h"
#include "localrepository.h"
#include "../common/utils.h"
#include <qtlog.h>
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

        int i = 0;
        QFileInfo destFileInfo;
        QDir dir;
        QString src, dest;

        foreach(const QString &filename, sourcefiles)
        {
            src = m_sourceRepository.directory() + filename;
            dest = m_destinationDir + filename;

            destFileInfo.setFile(dest);
            if(!dir.mkpath(destFileInfo.absolutePath()))
                throw tr("Unable to create path %1").arg(destFileInfo.absolutePath());

            if(QFile::exists(dest) && !QFile::remove(dest))
                throw tr("Unable to remove %1").arg(dest);

            if(!QFile::copy(src, dest))
                throw tr("Unable to copy %1 to %2").arg(src, dest);

            emit progression(++i, sourcefiles.size());
        }

        QSet<QString> diffFiles = destRepository.fileList().toSet() - sourcefiles;
        foreach(const QString &filename, diffFiles)
        {
            dest = m_destinationDir + filename;
            if(QFile::exists(dest) && !QFile::remove(dest))
                LOG_WARN(tr("Unable to remove %1").arg(dest));
        }

        destRepository.setFileList(m_sourceRepository.fileList());
        destRepository.setRevision(m_sourceRepository.revision());
        destRepository.setUpdateInProgress(m_sourceRepository.updateInProgress());
        destRepository.save();
    }
    catch(const QString &msg)
    {
        LOG_ERROR(msg);
    }
}
