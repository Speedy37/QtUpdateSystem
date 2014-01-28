#include "patchoperation.h"

#include <log.h>
#include <QFileInfo>
#include <QProcess>

void PatchOperation::run()
{
    if(state() == Applied)
        return;

    QFileInfo fileInfo(localpath());
    if(!fileInfo.exists())
    {
        if(fileInfo.size() == finalSize)
        {
            QFile file(localpath());
            file.open(QFile::ReadOnly);
            QString hash = fileHash(&file, file.size(), finalHashType);
            if(hash == finalHash)
            {
                Log::info(QObject::tr("File was already at the right version %1").arg(path));
                return;
            }
        }
        throw tr("The update was supposed to create a new file %1, another one is already there").arg(path);
    }

    QFile dataFile(filename());

    if(dataFile.size() != dataLength)
        throw tr("Data file size is unexpected");

    QDir filedir = fileInfo.dir();
    if(!filedir.exists() && !filedir.mkpath(filedir.absolutePath()))
        throw tr("Unable to create directory %1 to contains %2").arg(filedir.path(), path);

    if(dataCompression == Uncompressed)
    {
        if(!fileInfo.exists())
            Log::warn(tr("The file was supposed to already exists %1").arg(path));
        else if(!QFile(localpath()).remove())
            throw tr("Unable to remove file %1").arg(path);
        else if(!dataFile.rename(localpath()))
            throw tr("Unable to rename file %1 to %2").arg(filename(), path);
        Log::info(tr("Rename succeeded %1").arg(path));
        return;
    }
    else if(dataCompression == XdeltaLzma)
    {
        Log::trace(tr("Xdelta3+lzma decompression %1 to %2").arg(filename(), path));

        if(!fileInfo.exists())
            throw tr("The original file %1 is required for the decompression").arg(path);

        QFile file(filename()+QStringLiteral(".final"));
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw tr("Unable to open file %1 for writing").arg(file.fileName());

        QProcess lzma, xdelta;
        QStringList lzmaArguments, xdeltaArguments;
        lzma.setStandardOutputProcess(&xdelta);
#ifdef Q_OS_WIN
        lzmaArguments << "d" << dataFile.fileName() << "-so";
        xdeltaArguments << "-d" << "-c" << "-s" << localpath();
        xdelta.start(QStringLiteral("xdelta3.exe"), xdeltaArguments);
        lzma.start(QStringLiteral("lzma.exe"), lzmaArguments);
#endif
        if(!xdelta.waitForStarted())
            throw tr("Unable to start xdelta3");
        if(!lzma.waitForStarted())
            throw tr("Unable to start lzma");

        QCryptographicHash hashMaker(finalHashType);
        char buffer[4096];
        qint64 read;
        while(xdelta.waitForReadyRead())
        {
            while((read = xdelta.read(buffer, 4096)) > 0)
            {
                hashMaker.addData(buffer, read);
                if(file.write(buffer, read) != read)
                {
                    lzma.kill();
                    xdelta.kill();
                    throw tr("Failed to write to %1").arg(file.fileName());
                }
            }
        }

        if(!waitForFinished(lzma))
        {
            Log::error(QString(lzma.readAllStandardError()));
            throw tr("Extraction failed, lzma failed");
        }

        if(!waitForFinished(xdelta))
        {
            Log::error(QString(xdelta.readAllStandardError()));
            throw tr("Extraction failed, xdelta3 failed");
        }

        if(QString(hashMaker.result().toHex()) != finalHash)
            throw tr("Extraction failed, the result was unexpected");

        if(!QFile(localpath()).remove())
            throw tr("Unable to remove file %1").arg(path);

        if(!file.rename(localpath()))
            throw tr("Unable to rename file %1 to %2").arg(file.fileName(), path);

        Log::info(tr("Extraction succeeded %1").arg(path));
        if(!QFile(filename()).remove())
            Log::warn(tr("Unable to remove temporary file %1").arg(filename()));
    }
    else
    {
        throw tr("Unsupported dataCompression %1").arg(dataCompression);
    }
    setState(Applied);
}

void PatchOperation::save1(QJsonObject &object)
{
    AddOperation::save1(object);

    object.insert(QStringLiteral("currentSize"), QString::number(currentSize));
    object.insert(QStringLiteral("currentHashType"), getHashType(currentHashType));
    object.insert(QStringLiteral("currentHash"), currentHash);
}

void PatchOperation::load1(const QJsonObject &object)
{
    AddOperation::load1(object);

    loadInt64(object.value(QStringLiteral("currentSize")), &currentSize);
    loadHashType(object.value(QStringLiteral("currentHashType")), &currentHashType);
    currentHash = object.value(QStringLiteral("currentHash")).toString();
}
