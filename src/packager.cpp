#include "packager.h"
#include "common/utils.h"
#include "common/package.h"
#include "operations/operation.h"

#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QProcess>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRunnable>
#include <QThreadPool>
#include <QTemporaryDir>
#include <QScopedPointer>
#include <QElapsedTimer>
#include <QJsonArray>

Q_LOGGING_CATEGORY(LOG_PACKAGER, "updatesystem.packager")

Packager::Packager(QObject *parent) : QObject(parent), m_temporaryDir(nullptr)
{

}

Packager::~Packager()
{
    delete m_temporaryDir;
}

/**
   \brief Generate a new patch from old source to new source for the given repository
   You don't need to setup deltaFilename and deltaMetaDataFilename
   \param repositoryPath Directory of the repository
 */
PackageMetadata Packager::generateForRepository(const QString &repositoryPath)
{
    setDeltaFilename(Utils::cleanPath(repositoryPath) + Package::repositoryPackageName(oldRevisionName(), newRevisionName()));
    setDeltaMetadataFilename(QString());
    return generate();
}

/**
   \brief Generate a new patch from old source to new source
   The generation is made of 5 sequentials steps :
   \list 1
    \li Check packager configuration
    \li Compare directories
    \li Construct operations (use a thread pool to speed up creation time)
    \li Construct the final package
    \li Save package metadata
   \endlist
 */
PackageMetadata Packager::generate()
{
    QElapsedTimer globalTimer, stepTimer;
    globalTimer.start();
    stepTimer = globalTimer;

    qCDebug(LOG_PACKAGER) << "Checking packager configuration...";

    if(newDirectoryPath().isEmpty())
        throw tr("New directory path is empty");

    if(newRevisionName().isEmpty())
        throw tr("New revision name is empty");

    QDir oldDir(oldDirectoryPath());
    if(!oldDir.exists() && !oldDirectoryPath().isEmpty())
        throw tr("Old directory doesn't exists");

    QDir newDir(newDirectoryPath());
    if(!newDir.exists())
        throw tr("New directory doesn't exists");

    QFile deltaFile(deltaFilename());
    if(deltaFile.exists())
        throw tr("Delta file already exists");

    QFile metadataFile(deltaMetadataFilename());
    if(metadataFile.exists())
        throw tr("Delta metadata file already exists");

    qCDebug(LOG_PACKAGER) << "Packager configuration checked in" << Utils::formatMs(stepTimer.restart());

    qCDebug(LOG_PACKAGER) << "Comparing directories...";
    {
        QFileInfoList newFiles = dirList(newDir);
        QFileInfoList oldFiles = oldDirectoryPath().isNull() ? QFileInfoList() : dirList(oldDir);
        m_tasks.clear();
        compareDirectories(QString(), newFiles, oldFiles);
    }
    qCDebug(LOG_PACKAGER) << "Directory comparison done in" << Utils::formatMs(stepTimer.restart());

    qCDebug(LOG_PACKAGER) << "Creating operations...";
    {
        if(tmpDirectoryPath().isEmpty())
        {
            m_temporaryDir = new QTemporaryDir;
            if(!m_temporaryDir->isValid())
                throw tr("Unable to create a temporary directory");
            setTmpDirectoryPath(m_temporaryDir->path());
        }

        QThreadPool threadPool;
        for(size_t i = 0; i < m_tasks.size(); ++i)
        {
            PackagerTask & task = m_tasks[i];
            task.tmpDirectory = tmpDirectoryPath();
            if(task.isRunSlow())
                threadPool.start(&task);
            else
                task.run();
        }
        threadPool.waitForDone();
    }
    qCDebug(LOG_PACKAGER) << "Operations created in" << Utils::formatMs(stepTimer.restart());

    qCDebug(LOG_PACKAGER) << "Creating final delta file...";
    PackageMetadata metadata;
    {
        if(!deltaFile.open(QFile::WriteOnly))
            throw tr("Unable to create new delta file");
        qint64 totalSize = 0;
        qint64 read;
        char buffer[8096];
        QFile operationFile;
        for(size_t i = 0; i < m_tasks.size(); ++i)
        {
            PackagerTask & task = m_tasks[i];

            if(!task.errorString.isNull())
                throw task.errorString;

            if(task.operation->size() > 0)
            {
                operationFile.setFileName(task.operation->dataFilename());
                if(!operationFile.open(QFile::ReadOnly))
                    throw tr("Unable to open %1").arg(operationFile.fileName());

                while((read = operationFile.read(buffer, sizeof(buffer))) > 0)
                {
                    if(deltaFile.write(buffer, read) != read)
                         throw tr("Unable to write %1").arg(deltaFile.fileName());
                }

                operationFile.close();
                task.operation->setOffset(totalSize);
                totalSize += task.operation->size();
            }
            metadata.addOperation(task.operation);
        }
        metadata.setPackage(Package(newRevisionName(), oldRevisionName(), totalSize));

        if(!deltaFile.flush())
             throw tr("Unable to flush %1").arg(deltaFile.fileName());

        deltaFile.close();
    }
    qCDebug(LOG_PACKAGER) << "Final delta file created in" << Utils::formatMs(stepTimer.restart());

    qCDebug(LOG_PACKAGER) << "Writing metadata";
    if(!metadataFile.open(QFile::WriteOnly | QFile::Text))
        throw tr("Unable to create new delta metadata file");
    metadataFile.write(QJsonDocument(metadata.toJsonObject()).toJson(QJsonDocument::Indented));
    metadataFile.close();
    qCDebug(LOG_PACKAGER) << "Metadata written in" << Utils::formatMs(stepTimer.restart());

    qCDebug(LOG_PACKAGER) << "Delta creation succeded in" << Utils::formatMs(globalTimer.elapsed());

    return metadata;
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


void Packager::addRemoveDirTask(QString path, QFileInfo &pathInfo)
{
    QFileInfoList files = dirList(QDir(pathInfo.absoluteFilePath()));
    for(int pos = 0, len = files.size(); pos < len; ++pos)
    {
        QFileInfo & file = files[pos];
        if(file.isDir())
        {
            addRemoveDirTask(path+QLatin1Char('/')+file.fileName(), file);
        }
        else
        {
            addTask(PackagerTask::RemoveFile, path+QLatin1Char('/')+file.fileName(), QString(), file.filePath());
        }
    }
    addTask(PackagerTask::RemoveDir, path);
}

void Packager::compareDirectories(QString path, const QFileInfoList & newFiles, const QFileInfoList & oldFiles)
{
    int newPos = 0, newLen = newFiles.size();
    int oldPos = 0, oldLen = oldFiles.size();

    if(!path.isEmpty())
    {
        addTask(PackagerTask::AddDir, path);
        path += QLatin1Char('/');
    }
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

        if(diff < 0)
        {
            if(newFile.isFile())
            {
                // Add newFile
                addTask(PackagerTask::Add, path+newFile.fileName(), newFile.filePath());
            }
            else if(newFile.isDir())
            {
                compareDirectories(path + newFile.fileName(),
                                   dirList(QDir(newFile.filePath())), QFileInfoList());
            }
            ++newPos;
        }
        else if(diff > 0)
        {
            // Del oldFile
            if(oldFile.isDir())
                addRemoveDirTask(path + oldFile.fileName(), oldFile);
            else
                addTask(PackagerTask::RemoveFile, path + oldFile.fileName(), QString(), oldFile.filePath());
            ++oldPos;
        }
        else // diff == 0
        {
            if(newFile.isFile())
            {
                if(!oldFile.isFile())
                {
                    // RMD + ADD
                    addRemoveDirTask(path + oldFile.fileName(), oldFile);
                    //m_tasks.emplace_back(PackagerTask(PackagerTask::RemoveDir, path + newFile.fileName(), QString(), newFile.filePath()));
                    addTask(PackagerTask::Add, path + newFile.fileName(), newFile.filePath());
                }
                else
                {
                    // Make diff
                    addTask(PackagerTask::Patch, path + newFile.fileName(), newFile.filePath(), oldFile.filePath());
                }
            }
            else if(newFile.isDir())
            {
                if(!oldFile.isDir())
                {
                    addTask(PackagerTask::RemoveFile, path + oldFile.fileName(), QString(), oldFile.filePath());
                    compareDirectories(path + newFile.fileName(),
                                       dirList(QDir(newFile.filePath())),
                                       QFileInfoList());
                }
                else
                {
                    compareDirectories(path + newFile.fileName(),
                                       dirList(QDir(newFile.filePath())),
                                       dirList(QDir(oldFile.filePath())));
                }
            }

            ++newPos;
            ++oldPos;
        }

    }
}

void Packager::addTask(PackagerTask::Type operationType, QString path, QString newFilename, QString oldFilename)
{
    m_tasks.push_back(PackagerTask(operationType, path, oldFilename, newFilename));
}

