#include "addoperation.h"
#include "../common/jsonutil.h"
#include "../common/utils.h"
#include <qtlog.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>

const QString AddOperation::Action = QStringLiteral("add");

const QString AddOperation::DataOffset = QStringLiteral("dataOffset");
const QString AddOperation::DataSize = QStringLiteral("dataSize");
const QString AddOperation::DataSha1 = QStringLiteral("dataSha1");
const QString AddOperation::DataCompression = QStringLiteral("dataCompression");
const QString AddOperation::FinalSize = QStringLiteral("finalSize");
const QString AddOperation::FinalSha1 = QStringLiteral("finalSha1");

const QString COMPRESSION_LZMA = QStringLiteral("lzma");
const QString COMPRESSION_NONE = QStringLiteral("none");

AddOperation::AddOperation() : Operation()
{
    m_finalSize = 0;
}

void AddOperation::fromJsonObjectV1(const QJsonObject &object)
{
    Operation::fromJsonObjectV1(object);

    m_offset = JsonUtil::asInt64String(object, DataOffset);
    m_size = JsonUtil::asInt64String(object, DataSize);
    m_sha1 = JsonUtil::asString(object, DataSha1);
    m_compression = JsonUtil::asString(object, DataCompression);
    m_finalSize = JsonUtil::asInt64String(object, FinalSize);
    m_finalSha1 = JsonUtil::asString(object, FinalSha1);
}

void AddOperation::create(const QString &path, const QString &newFilename, const QString &tmpDirectory)
{
    m_path = path;

    // Final file informations
    {
        QFile file(newFilename);
        if(!file.exists(newFilename))
            throw QObject::tr("File %1 doesn't exists").arg(newFilename);

        m_finalSha1 = sha1(&file);
        m_finalSize = file.size();
    }

    setDataFilename(tmpDirectory+"add_"+m_finalSha1);
    QFile dataFile(dataFilename());
    QFile metadataFile(dataFilename() + ".metadata");
    if(dataFile.exists() && metadataFile.exists())
    {
        if (metadataFile.open(QFile::ReadOnly | QFile::Text))
        {
            try
            {
                QJsonObject object = JsonUtil::fromJson(metadataFile.readAll());
                m_size = JsonUtil::asInt64String(object, DataSize);
                m_sha1 = JsonUtil::asString(object, DataSha1);
                m_compression = JsonUtil::asString(object, DataCompression);
                return;
            }
            catch(const QString &msg)
            {
                Q_UNUSED(msg);
            }
            metadataFile.close();
        }
    }

    if(!dataFile.open(QFile::WriteOnly | QFile::Truncate))
        throw QObject::tr("Unable to open file %1 for writing").arg(dataFile.fileName());

    if (!metadataFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
        throw QObject::tr("Unable to open file %1 for writing : %2").arg(metadataFile.fileName(), metadataFile.errorString());

    QProcess compressor;
    QStringList arguments;
    arguments << "e" << "-so" << newFilename;
    m_compression = COMPRESSION_LZMA;
    compressor.start(Utils::lzmaProgram(), arguments);

    if(!compressor.waitForStarted())
        throw QObject::tr("Unable to start %1").arg(compressor.program());

    QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
    char buffer[8192];
    qint64 read;
    while(compressor.waitForReadyRead())
    {
        while((read = compressor.read(buffer, sizeof(buffer))) > 0)
        {
            sha1Hash.addData(buffer, read);
            if(dataFile.write(buffer, read) != read)
                throw QObject::tr("Failed to write to %1").arg(dataFile.fileName());
        }
    }

    if(!waitForFinished(compressor))
    {
        LOG_ERROR(QString(compressor.readAllStandardError()));
        throw QObject::tr("%1 failed").arg(compressor.program());
    }

    m_sha1 = QString(sha1Hash.result().toHex());
    m_size = dataFile.size();

    if(!dataFile.flush())
        throw QObject::tr("Unable to flush all compressed data");

    dataFile.close();

    // Writing metadata
    {
        QJsonObject object;
        object.insert(Path, m_path);
        object.insert(DataSize, QString::number(m_size));
        object.insert(DataSha1, m_sha1);
        object.insert(DataCompression, m_compression);
        if(metadataFile.write(QJsonDocument(object).toJson()) == -1)
            throw QObject::tr("Unable to write metadata");

        if(!metadataFile.flush())
            throw QObject::tr("Unable to flush metadata");

        metadataFile.close();
    }
}

Operation::Status AddOperation::localDataStatus()
{
    QFile file(localFilename());

    if(file.exists())
    {
        // Check final file content
        if(file.size() == m_finalSize && sha1(&file) == m_finalSha1)
        {
            LOG_INFO(QObject::tr("File %1 is already at the right version").arg(path()));
            return Valid;
        }

        LOG_WARN(QObject::tr("File %1 already exists and will be overwrited").arg(path()));
    }

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

void AddOperation::applyData()
{
    // Ensure file directory exists
    {
        QFileInfo fileInfo(localFilename());
        QDir filedir = fileInfo.dir();
        if(!filedir.exists() && !filedir.mkpath(filedir.absolutePath()))
            throw QObject::tr("Unable to create directory %1 for %2").arg(filedir.path(), path());
    }

    if(m_compression == COMPRESSION_NONE)
    {
        QFile dataFile(dataFilename());
        if(!dataFile.rename(localFilename()))
            throw QObject::tr("Unable to rename file %1 to %2").arg(dataFilename(), path());
        LOG_INFO(QObject::tr("Rename succeeded %1").arg(path()));
        return;
    }
    else if(m_compression == COMPRESSION_LZMA)
    {
        LOG_TRACE(QObject::tr("Decompressing %1 to %2 by lzma").arg(dataFilename(), path()));

        QFile file(localFilename());
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw QObject::tr("Unable to open file %1 for writing").arg(file.fileName());

        QProcess decompressor;
        QStringList arguments;
#ifdef Q_OS_WIN
        arguments << "d" << dataFilename() << "-so";
        decompressor.start(Utils::lzmaProgram(), arguments);
#endif

        if(!decompressor.waitForStarted())
            throw QObject::tr("Unable to start %1").arg(decompressor.program());

        QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
        char buffer[8192];
        qint64 read;
        while(decompressor.waitForReadyRead())
        {
            while((read = decompressor.read(buffer, sizeof(buffer))) > 0)
            {
                sha1Hash.addData(buffer, read);
                if(file.write(buffer, read) != read)
                    throw QObject::tr("Failed to write to %1").arg(file.fileName());
            }
        }

        if(!waitForFinished(decompressor))
        {
            LOG_ERROR(QString(decompressor.readAllStandardError()));
            throw QObject::tr("%1 failed").arg(decompressor.program());
        }

        if(QString(sha1Hash.result().toHex()) != m_finalSha1)
            throw QObject::tr("Final sha1 file signature doesn't match");

        if(!file.flush())
            throw QObject::tr("Unable to flush all extracted data");

        file.close();

        Q_ASSERT(sha1(&file) == m_finalSha1);

        LOG_INFO(QObject::tr("Extraction succeeded %1").arg(path()));

        if(!QFile(dataFilename()).remove())
            LOG_WARN(QObject::tr("Unable to remove temporary file %1").arg(dataFilename()));
    }
    else
    {
        throw QObject::tr("Compression %1 unknown").arg(m_compression);
    }
}

void AddOperation::fillJsonObjectV1(QJsonObject &object)
{
    Operation::fillJsonObjectV1(object);

    object.insert(DataOffset, QString::number(offset()));
    object.insert(DataSize, QString::number(size()));
    object.insert(DataSha1, sha1());
    object.insert(DataCompression, m_compression);
    object.insert(FinalSize, QString::number(m_finalSize));
    object.insert(FinalSha1, m_finalSha1);
}

QString AddOperation::action() const
{
    return Action;
}
