#include "compressfiletask.h"

CompressFileTask::CompressFileTask(QObject *parent) :
    QRunnable(parent)
{
}


void CompressFileTask::run()
{
    compress(true);
}

void CompressFileTask::compress(bool isSourceFinalFile)
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
