#include "patchfiletask.h"

PatchFileTask::PatchFileTask()
{
}


void PatchFileTask::run()
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
