#include "packager.h"
#include "log.h"

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

        Log::info(tr("Delta creation succeded"));
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

        Log::info(tr("Delta creation failed %1").arg(reason.errorString));
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

    if(Log::isTraceEnabled())
        Log::trace(QString("generate_recursion : ") +
              QString::number(newLen) + ", " +
              QString::number(oldLen) + ", " +
              path );
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

        if(Log::isTraceEnabled())
            Log::trace(QString("step : ") + QString::number(diff) + ", "+ newFile.filePath() + ", " + oldFile.filePath() );

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

void Packager::CompressFileTask::run()
{
    compress(true);
}

void Packager::CompressFileTask::compress(bool isSourceFinalFile)
{
    try
    {
        QFile file(info->destinationFileName);
        if(!file.open(QFile::ReadOnly))
            throw Exception(CompressFailed, tr("Unable to open file %1").arg(file.fileName()));

        qint64 read, filesize = 0;
        char buffer[4096];
        QProcess lzma;
        QStringList arguments;
        QString finalFileHash, dataFileHash;
        QString compression;

        if(QFileInfo(sourceFilename).size() > 4096)
        {
            QCryptographicHash dataHashMaker(QCryptographicHash::Sha1);
            lzma.setProcessChannelMode(QProcess::ForwardedErrorChannel);
#if defined(Q_OS_LINUX)
            arguments << "e" << "-so" << sourceFilename;// Encode + Write to stdout
            lzma.start(QStringLiteral("./lzma"), arguments, QIODevice::ReadWrite | QIODevice::Unbuffered);
#endif

            if(!lzma.waitForStarted())
                throw Exception(CompressFailed, tr("Unable to start lzma"));

            while(lzma.waitForReadyRead())
            {
                while((read = lzma.read(buffer, 4096)) > 0)
                {
                    filesize += read;
                    dataHashMaker.addData(buffer, read);
                    if(file.write(buffer, read) != read)
                    {
                        lzma.kill();
                        throw Exception(PatchFailed, tr("Failed to write to %1").arg(file.fileName()));
                    }
                }
            }

            if(lzma.state() == QProcess::Running && !lzma.waitForFinished())
                throw Exception(CompressFailed, tr("Unable to finish lzma"));

            if(lzma.exitCode() != 0)
                throw Exception(CompressFailed, tr("lzma exitcode != 0"));

            if(!file.flush())
                throw Exception(CompressFailed, tr("Unable to write everything to compressed file"));

            dataFileHash = QString(dataHashMaker.result().toHex());
            compression = QStringLiteral("lzma");
        }
        else
        {
            QCryptographicHash dataHashMaker(QCryptographicHash::Sha1);
            QFile finalFile(sourceFilename);

            if(!finalFile.open(QFile::ReadOnly))
                throw Exception(CompressFailed, tr("Unable to open file %1").arg(finalFile.fileName()));

            while((read = finalFile.read(buffer, 4096)) > 0)
            {
                filesize += read;
                dataHashMaker.addData(buffer, read);
                if(file.write(buffer, read) != read)
                    throw Exception(CompressFailed, tr("Failed to write to %1").arg(file.fileName()));
            }
            dataFileHash = finalFileHash = QString(dataHashMaker.result().toHex());
            compression = QStringLiteral("");
        }

        QJsonObject &operation = info->description;
        if(isSourceFinalFile)
        {
            if(finalFileHash.isEmpty())
                finalFileHash = generate_hash(sourceFilename);
            operation.insert(QStringLiteral("finalSize"), QString::number(QFileInfo(sourceFilename).size()));
            operation.insert(QStringLiteral("finalHash"), finalFileHash);
            operation.insert(QStringLiteral("finalHashType"), QStringLiteral("Sha1"));
        }

        operation.insert(QStringLiteral("dataLength"), QString::number(filesize));
        operation.insert(QStringLiteral("dataHash"), dataFileHash);
        operation.insert(QStringLiteral("dataHashType"), QStringLiteral("Sha1"));
        operation.insert(QStringLiteral("dataCompression"), compression);
    }
    catch(const Exception &reason)
    {
        info->exception = reason;
    }
}

void Packager::PatchFileTask::run()
{
    try
    {
        QString finalFileHash = generate_hash(sourceFilename);
        QString currentFileHash = generate_hash(oldFilename);

        if(finalFileHash == currentFileHash) // No changes (info->isEmpty() == true)
            return;

        QFile file(tmpFilename);
        if(!file.open(QFile::ReadOnly))
            throw Exception(PatchFailed, tr("Unable to open file %1").arg(file.fileName()));

        qint64 read, filesize = 0;
        char buffer[4096];
        QProcess xdelta3;
        QStringList xdelta3Arguments;
        QCryptographicHash dataHashMaker(QCryptographicHash::Sha1);

        xdelta3.setProcessChannelMode(QProcess::ForwardedErrorChannel);
#if defined(Q_OS_LINUX)
        xdelta3Arguments << "-0" << "-e" << "-s" << oldFilename << sourceFilename;
        xdelta3.start(QStringLiteral("xdelta3"), xdelta3Arguments);
#endif

        if(!xdelta3.waitForStarted())
            throw Exception(PatchFailed, tr("Unable to start xdelta3"));

        while(xdelta3.waitForReadyRead())
        {
            while((read = xdelta3.read(buffer, 4096)) > 0)
            {
                filesize += read;
                dataHashMaker.addData(buffer, read);
                if(file.write(buffer, read) != read)
                {
                    xdelta3.kill();
                    throw Exception(PatchFailed, tr("Failed to write to %1").arg(tmpFilename));
                }
            }
        }

        if(xdelta3.state() == QProcess::Running && !xdelta3.waitForFinished())
            throw Exception(PatchFailed, tr("Unable to finish xdelta3"));

        if(xdelta3.exitCode() != 0)
            throw Exception(PatchFailed, tr("xdelta3 exitcode != 0"));

        if(!file.flush())
            throw Exception(PatchFailed, tr("Unable to write everything to patch file"));

        QJsonObject &operation = info->description;
        operation.insert(QStringLiteral("currentSize"), QString::number(QFileInfo(oldFilename).size()));
        operation.insert(QStringLiteral("currentHash"), currentFileHash);
        operation.insert(QStringLiteral("currentHashType"), QStringLiteral("Sha1"));

        operation.insert(QStringLiteral("finalSize"), QString::number(QFileInfo(sourceFilename).size()));
        operation.insert(QStringLiteral("finalHash"), finalFileHash);
        operation.insert(QStringLiteral("finalHashType"), QStringLiteral("Sha1"));

        operation.insert(QStringLiteral("patchLength"), QString::number(filesize));
        operation.insert(QStringLiteral("patchHash"), QString(dataHashMaker.result().toHex()));
        operation.insert(QStringLiteral("patchHashType"), QStringLiteral("Sha1"));

        sourceFilename = tmpFilename;
        compress(false);

        if(!QFile::remove(tmpFilename))
            Log::warn(tr("Unable to remove temporary file : %1").arg(tmpFilename));
    }
    catch(const Exception &reason)
    {
        info->exception = reason;
    }
}
