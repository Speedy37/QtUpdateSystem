#include "patchoperation.h"

#include "../common/jsonutil.h"
#include <qtlog.h>
#include <QFileInfo>
#include <QProcess>

const QString PatchOperation::Action = QStringLiteral("patch");

const QString LOCALSIZE = QStringLiteral("localSize");
const QString LOCALSHA1 = QStringLiteral("localSha1");
const QString PATCHTYPE = QStringLiteral("patchType");

const QString COMPRESSION_LZMA = QStringLiteral("lzma");
const QString COMPRESSION_NONE = QStringLiteral("none");

const QString PATCHTYPE_XDELTA = QStringLiteral("xdelta");


Operation::Status PatchOperation::localDataStatus()
{
    QFile file(localFilename());

    if(file.exists())
    {
        if(file.size() == m_finalSize || file.size() == m_localSize)
        {
            QString hash = sha1(&file);

            if(hash == m_finalSha1)
            {
                LOG_INFO(QObject::tr("File %1 is already at the right version").arg(path()));
                return Valid;
            }
            else if(hash == m_localSha1)
            {
                LOG_INFO(QObject::tr("File %1 is as expected for patching").arg(path()));

                file.setFileName(dataFilename());
                if(file.exists())
                {
                    // Check downloaded data file content
                    if(file.size() == size() && sha1(&file) == sha1())
                    {
                        LOG_INFO(QObject::tr("File %1 data is valid").arg(path()));
                        return ApplyRequired;
                    }

                    LOG_WARN(QObject::tr("File %1 data is invalid and will be downloaded again").arg(path()));
                }
                return DownloadRequired;
            }
        }

        LOG_WARN(QObject::tr("File %1 content is invalid").arg(path()));
    }
    else
    {
        LOG_WARN(QObject::tr("File %1 doesn't exists and can't be patched, complete file download will happen").arg(path()));
    }

    return LocalFileInvalid;
}

void PatchOperation::applyData()
{
    QFile dataFile(dataFilename());

    if(m_patchtype == PATCHTYPE_XDELTA)
    {
        LOG_TRACE(QObject::tr("Decompressing %1 to %2 by %3+%4").arg(dataFilename(), path(), m_compression, m_patchtype));

        QString patchedFilename = dataFilename()+".patched";
        QFile file(patchedFilename);
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw QObject::tr("Unable to open file %1 for writing").arg(file.fileName());

        QProcess decompressor, xdelta;
        QStringList xdeltaArguments;
        bool hasCompression;

        xdeltaArguments << "-d" << "-c" << "-s" << localFilename();
        if(m_compression == COMPRESSION_NONE)
        {
            xdeltaArguments << dataFilename();
            hasCompression = false;
        }
        else if(m_compression == COMPRESSION_LZMA)
        {
            QStringList decompressorArguments;
            decompressor.setStandardOutputProcess(&xdelta);
#ifdef Q_OS_WIN
            decompressorArguments << "d" << dataFilename() << "-so";
            decompressor.start(QStringLiteral("lzma.exe"), decompressorArguments);
#endif
            hasCompression = true;
        }
        else
        {
            throw QObject::tr("Unsupported compression %1").arg(m_compression);
        }
        xdelta.start(QStringLiteral("xdelta3.exe"), xdeltaArguments);

        if(hasCompression && !decompressor.waitForStarted())
            throw QObject::tr("Unable to start %1").arg(decompressor.program());

        if(!xdelta.waitForStarted())
            throw QObject::tr("Unable to start %1").arg(xdelta.program());

        QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
        char buffer[8192];
        qint64 read;
        while(xdelta.waitForReadyRead())
        {
            while((read = xdelta.read(buffer, sizeof(buffer))) > 0)
            {
                sha1Hash.addData(buffer, read);
                if(file.write(buffer, read) != read)
                    throw QObject::tr("Failed to write to %1").arg(file.fileName());
            }
        }

        if(hasCompression && !waitForFinished(decompressor))
        {
            LOG_ERROR(QString(decompressor.readAllStandardError()));
            throw QObject::tr("Decompression by %1 failed").arg(decompressor.program());
        }

        if(!waitForFinished(xdelta))
        {
            LOG_ERROR(QString(xdelta.readAllStandardError()));
            throw QObject::tr("%1 failed").arg(xdelta.program());
        }

        if(QString(sha1Hash.result().toHex()) != m_finalSha1)
            throw QObject::tr("Final sha1 file signature doesn't match");

        if(!file.flush())
            throw QObject::tr("Unable to flush all extracted data");

        file.close();

        Q_ASSERT(sha1(&file) == m_finalSha1);

        LOG_INFO(QObject::tr("Patch succeeded %1").arg(path()));

        if(!QFile(localFilename()).remove())
            throw QObject::tr("Unable to remove local file %1").arg(path());

        if(!file.rename(localFilename()))
            throw QObject::tr("Unable to rename file %1 to %2").arg(file.fileName(), path());

        if(!QFile(dataFilename()).remove())
            LOG_WARN(QObject::tr("Unable to remove temporary file %1").arg(dataFilename()));
    }
    else
    {
        throw QObject::tr("Patchtype %1 unknown").arg(m_patchtype);
    }
}

void PatchOperation::save1(QJsonObject &object)
{
    AddOperation::save1(object);

    object.insert(LOCALSIZE, QString::number(m_localSize));
    object.insert(LOCALSHA1, m_localSha1);
    object.insert(PATCHTYPE, m_patchtype);
}

void PatchOperation::load1(const QJsonObject &object)
{
    AddOperation::load1(object);

    m_localSize = JsonUtil::asInt64String(object, LOCALSHA1);
    m_localSha1 = JsonUtil::asString(object, LOCALSHA1);
    m_patchtype = JsonUtil::asString(object, PATCHTYPE);
}

QString PatchOperation::action()
{
    return Action;
}
