#include "packager.h"
#include "packager/compressfiletask.h"
#include "packager/patchfiletask.h"

#include <qtlog.h>
#include <QCryptographicHash>
#include <QProcess>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QRunnable>
#include <QThreadPool>
#include <QTemporaryDir>

Packager::Packager(QObject *parent) : QObject(parent)
{

}

/**
 * @brief Generate a new patch from old source to new source
 * @return
 */
Packager::GenerationError Packager::generate()
{
    try
    {
        m_lastException = Exception();
        threadpool = NULL;

        LOG_TRACE(tr("Checking preconditions"));

        if(newDirectoryPath().isEmpty())
            throw Exception(NewDirectoryPathIsEmpty, tr("New directory path is empty"));

        if(newRevisionName().isEmpty())
            throw Exception(NewRevisionNameIsEmpty, tr("New revision name is empty"));

        QDir oldDir(oldDirectoryPath());
        if(!oldDir.exists() && !oldDirectoryPath().isEmpty())
            throw Exception(OldDirectoryInvalid, tr("Old directory doesn't exists"));

        QDir newDir(newDirectoryPath());
        if(!newDir.exists())
            throw Exception(NewDirectoryInvalid, tr("New directory doesn't exists"));

        QFile deltaFile(deltaFilename());
        if(deltaFile.exists())
            throw Exception(DeltaFileAlreadyExists, tr("Delta file already exists"));
        if(!deltaFile.open(QFile::WriteOnly))
            throw Exception(DeltaFileOpenFailed, tr("Unable to create new delta file"));

        QFile metadataFile(deltaMetaDataFilename());
        if(metadataFile.exists())
            throw Exception(DeltaMetaFileAlreadyExists, tr("Delta metadata file already exists"));
        if(!metadataFile.open(QFile::WriteOnly | QFile::Text))
            throw Exception(DeltaMetaFileOpenFailed, tr("Unable to create new delta metadata file"));

        LOG_TRACE(tr("Preconditions passed"));

        QFileInfoList newFiles = dirList(newDir);
        QFileInfoList oldFiles = oldDirectoryPath().isNull() ? QFileInfoList() : dirList(oldDir);

        {
            QScopedPointer<QThreadPool> scopedThreadPool(threadpool = new QThreadPool());
            QScopedPointer<QTemporaryDir> tmpDirectory(tmpDirectoryPath().isEmpty() ? new QTemporaryDir : new QTemporaryDir(tmpDirectoryPath()));
            if (!tmpDirectory->isValid())
                throw Exception(TmpDirInvalid, tr("Unable to create temporary directory"));
            m_currentTmpDirectoryPath = tmpDirectory->path();
            m_tmpFileCounter = 0;

            generate_recursion(QStringLiteral(""), newFiles, oldFiles);
            scopedThreadPool->waitForDone();
        }

        for(int i = 0; i < latentTaskInfos.size(); ++i)
        {
            TaskInfo * task = latentTaskInfos[i];

            if(task->exception.errorType != NoError)
                throw task->exception;

            if(!task->description.isEmpty())
            {
                appendFileContent(deltaFile, task);
                operations.append(task->description);
            }

            delete task;
            latentTaskInfos[i] = NULL;
        }
        latentTaskInfos.clear();

        // Writing metadata header
        QJsonObject json;
        json.insert(QStringLiteral("version"), QStringLiteral("1"));
        json.insert(QStringLiteral("operations"), operations);
        json.insert(QStringLiteral("revision"), newRevisionName());
        json.insert(QStringLiteral("size"), QString::number(deltaFile.size()));
        if(!oldRevisionName().isEmpty())
            json.insert(QStringLiteral("patchfor"), oldRevisionName());

        metadataFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));

        LOG_INFO(tr("Delta creation succeded"));
        return NoError;
    }
    catch (const Exception & reason)
    {
        for(int i = 0; i < latentTaskInfos.size(); ++i)
        {
            TaskInfo * task = latentTaskInfos[i];
            if(task != NULL)
                delete task;
        }
        latentTaskInfos.clear();

        LOG_INFO(tr("Delta creation failed %1").arg(reason.errorString));
        m_lastException = reason;
        return reason.errorType;
    }
}

QFileInfoList Packager::dirList(const QDir & dir)
{
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);
    for (int i = list.size()-1; i >= 0; --i)
    {
        if (list.at(i).fileName() == QLatin1String(".git"))
            list.removeAt(i);
    }
    return list;
}

QString Packager::newTmpFilename()
{
    return m_currentTmpDirectoryPath + "/" + QString::number(++m_tmpFileCounter);
}

void Packager::appendFileContent(QFile &file, TaskInfo *task)
{
    task->description.insert(QStringLiteral("dataOffset"), QString::number(file.pos()));

    char buffer[4096];
    qint64 read;
    QFile finalFile(task->destinationFileName);

    if(!finalFile.open(QFile::ReadOnly))
        throw Exception(AppendFailed, tr("Unable to open file %1").arg(finalFile.fileName()));

    while((read = finalFile.read(buffer, 4096)) > 0)
    {
        if(file.write(buffer, read) != read)
            throw Exception(AppendFailed, tr("Failed to write to %1").arg(file.fileName()));
    }
}

void Packager::generate_addfile(const QString &path, QFileInfo & newFile)
{
    TaskInfo * info = new TaskInfo;
    info->description.insert(QStringLiteral("action"), QStringLiteral("ADD"));
    info->description.insert(QStringLiteral("path"), path);
    info->destinationFileName = newTmpFilename();
    latentTaskInfos.append(info);
    threadpool->start(new CompressFileTask(info, newFile.absoluteFilePath()));
}

void Packager::generate_patchfile(QString path, QFileInfo & newFileInfo, QFileInfo & oldFileInfo)
{
    TaskInfo * info = new TaskInfo;
    info->description.insert(QStringLiteral("action"), QStringLiteral("PATCH"));
    info->description.insert(QStringLiteral("path"), path);
    info->destinationFileName = newTmpFilename();
    latentTaskInfos.append(info);
    threadpool->start(new PatchFileTask(info,
                                        newFileInfo.absoluteFilePath(),
                                        oldFileInfo.absoluteFilePath(),
                                        newTmpFilename()));
}

void Packager::generate_rmdir(QString path, QFileInfo &pathInfo)
{
    QFileInfoList files = dirList(QDir(pathInfo.absoluteFilePath()));
    for(int pos = 0, len = files.size(); pos < len; ++pos)
    {
        QFileInfo & file = files[pos];
        if(file.isDir())
        {
            generate_rmdir(path+QLatin1Char('/')+file.fileName(), file);
        }
        else
        {
            generate_rm(path+QLatin1Char('/')+file.fileName());
        }
    }
    QJsonObject operation;
    operation.insert(QStringLiteral("action"), QStringLiteral("RMDIR"));
    operation.insert(QStringLiteral("path"), path);
    operations.append(operation);
}

void Packager::generate_rm(const QString &path)
{
    QJsonObject operation;
    operation.insert(QStringLiteral("action"), QStringLiteral("RM"));
    operation.insert(QStringLiteral("path"), path);
    operations.append(operation);
}

QString Packager::generate_hash(const QString & srcFilename)
{
    QCryptographicHash hashMaker(QCryptographicHash::Sha1);
    QFile srcFile(srcFilename);
    if(!srcFile.open(QFile::ReadOnly))
        throw Exception(FileSignatureFailed, tr("Unable to open file : %1").arg(srcFilename));
    hashMaker.addData(&srcFile);
    return QString(hashMaker.result().toHex());
}

void Packager::generate_recursion(QString path, const QFileInfoList & newFiles, const QFileInfoList & oldFiles)
{
    int newPos = 0, newLen = newFiles.size();
    int oldPos = 0, oldLen = oldFiles.size();

    LOG_TRACE(tr("path = %1, newFiles.size() = %2, oldFiles.size() = %3").arg(path).arg(newLen).arg(oldLen));

    while(newPos < newLen || oldPos < oldLen)
    {
        QFileInfo newFile, oldFile;
        int diff;
        if(newPos < newLen)
            newFile = newFiles[newPos];
        if(oldPos < oldLen)
            oldFile = oldFiles[oldPos];

        if(newPos < newLen && oldPos < oldLen)
            diff = QString::compare(newFile.fileName(), oldFile.fileName());
        else
            diff = newPos < newLen ? -1 : 1;

        LOG_TRACE(tr("diff = %1, newFile = %2, oldFile = %3").arg(diff).arg(newFile.filePath()).arg(oldFile.filePath()));

        if(diff < 0)
        {
            if(newFile.isFile())
            {
                // Add newFile
                generate_addfile(path+newFile.fileName(), newFile);
            }
            else if(newFile.isDir())
            {
                generate_recursion(path + newFile.fileName() + QLatin1Char('/'),
                                   dirList(QDir(newFile.filePath())), QFileInfoList());
            }
            ++newPos;
        }
        else if(diff > 0)
        {
            // Del oldFile
            if(oldFile.isDir())
                generate_rmdir(path + oldFile.fileName(), oldFile);
            else
                generate_rm(path + oldFile.fileName());
            ++oldPos;
        }
        else // diff == 0
        {
            if(newFile.isFile())
            {
                if(!oldFile.isFile())
                {
                    // RMD + ADD
                    generate_rmdir(path + oldFile.fileName(), oldFile);
                    generate_addfile(path + newFile.fileName(), newFile);
                }
                else
                {
                    // Make diff
                    generate_patchfile(path + newFile.fileName(), newFile, oldFile);
                }
            }
            else if(newFile.isDir())
            {
                if(!oldFile.isDir())
                {
                    generate_rm(path + oldFile.fileName());
                    generate_recursion(path + newFile.fileName() + QLatin1Char('/'),
                                       dirList(QDir(newFile.filePath())),
                                       QFileInfoList());
                }
                else
                {
                    generate_recursion(path + newFile.fileName() + QLatin1Char('/'),
                                       dirList(QDir(newFile.filePath())),
                                       dirList(QDir(oldFile.filePath())));
                }
            }

            ++newPos;
            ++oldPos;
        }

    }

    if(Log::isTraceEnabled())
        Log::trace(QString("generate_recursion done"));
}

