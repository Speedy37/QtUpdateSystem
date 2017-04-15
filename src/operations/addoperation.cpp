#include "addoperation.h"
#include "../common/jsonutil.h"
#include "../common/utils.h"
#include "../tools/brotli.h"
#include "../tools/lzma.h"
#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>

Q_LOGGING_CATEGORY(LOG_ADDOP, "updatesystem.addoperation")

const QString AddOperation::Action = QStringLiteral("add");

const QString AddOperation::DataOffset = QStringLiteral("dataOffset");
const QString AddOperation::DataSize = QStringLiteral("dataSize");
const QString AddOperation::DataSha1 = QStringLiteral("dataSha1");
const QString AddOperation::DataCompression = QStringLiteral("dataCompression");
const QString AddOperation::FinalSize = QStringLiteral("finalSize");
const QString AddOperation::FinalSha1 = QStringLiteral("finalSha1");

const QString COMPRESSION_LZMA = QStringLiteral("lzma");
const QString COMPRESSION_BROTLI = QStringLiteral("brotli");
const QString COMPRESSION_NONE = QStringLiteral("none");

AddOperation::AddOperation() : Operation()
{
    m_finalSize = 0;
}

Operation::FileType AddOperation::fileType() const
{
    return File;
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

void AddOperation::readAll(QIODevice *from, QIODevice *to, QCryptographicHash *hash)
{
    char buffer[65536];
    qint64 read;
    while ((read = from->read(buffer, sizeof(buffer))) > 0)
    {
        hash->addData(buffer, read);
        if (to->write(buffer, read) != read)
            throw QObject::tr("Write failed: %1").arg(to->errorString());
    }
    if (read == -1)
        throw QObject::tr("Read failed: %1").arg(from->errorString());
}

void AddOperation::create(const QString &filepath, const QString &newFilename, const QString &tmpDirectory)
{
	// Final file informations
    QFile file(newFilename);
	if (!file.exists(newFilename))
		throw QObject::tr("File %1 doesn't exists").arg(newFilename);
	m_finalSha1 = sha1(&file);
	m_finalSize = file.size();
    setPath(filepath);
    setDataFilename(tmpDirectory + "add_" + m_finalSha1);

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
            catch(...) { }
            metadataFile.close();
        }
	}

	if (!file.open(QFile::ReadOnly))
		throw QObject::tr("Unable to open file %1 for writing").arg(dataFile.fileName());

    if(!dataFile.open(QFile::WriteOnly | QFile::Truncate))
        throw QObject::tr("Unable to open file %1 for writing").arg(dataFile.fileName());

    if (!metadataFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
        throw QObject::tr("Unable to open file %1 for writing : %2").arg(metadataFile.fileName(), metadataFile.errorString());

    QScopedPointer<QIODevice> compressor(BrotliCompressor(&file));
    m_compression = COMPRESSION_BROTLI;
	
    QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
    readAll(compressor.data(), &dataFile, &sha1Hash);

	m_sha1 = QString(sha1Hash.result().toHex());
    m_size = dataFile.size();

    if(!dataFile.flush())
        throw QObject::tr("Unable to flush all compressed data");

	dataFile.close();
	file.close();

    // Writing metadata
    {
        QJsonObject object;
        object.insert(Path, path());
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
            qCDebug(LOG_ADDOP) << "File is already at the right version" << path();
            return Valid;
        }
    }

    file.setFileName(dataFilename());
    if(file.exists())
    {
        // Check downloaded data file content
        if(file.size() == size() && sha1(&file) == sha1())
        {
            qCDebug(LOG_ADDOP) << "File data is valid" << path();
            return ApplyRequired;
        }
        QString s1 = sha1(&file);
        throwWarning(QObject::tr("File data is invalid and will be downloaded again"));
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
        qCDebug(LOG_ADDOP) << "Rename succeeded" << path();
        return;
    }
    else if(m_compression == COMPRESSION_BROTLI || m_compression == COMPRESSION_LZMA)
    {
        qCDebug(LOG_ADDOP) << "Decompressing" << dataFilename() << "to" << path() << "by" << m_compression;

        QFile file(localFilename());
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw QObject::tr("Unable to open file %1 for writing").arg(file.fileName());

        QFile dataFile(dataFilename());
		if (!dataFile.open(QFile::ReadOnly))
			throw QObject::tr("Unable to open file %1 for reading").arg(dataFile.fileName());

        QScopedPointer<QIODevice> decompressor(m_compression == COMPRESSION_BROTLI ? BrotliDecompressor(&dataFile) : LZMADecompressor(&dataFile));
        QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
        readAll(decompressor.data(), &file, &sha1Hash);

        if(QString(sha1Hash.result().toHex()) != m_finalSha1)
            throw QObject::tr("Final sha1 file signature doesn't match");

        if(!file.flush())
            throw QObject::tr("Unable to flush all extracted data");

        file.close();
        dataFile.close();

        Q_ASSERT(sha1(&file) == m_finalSha1);

        qCDebug(LOG_ADDOP) << "Extraction succeeded" << path();

        cleanup();
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

QString AddOperation::type() const
{
    return Action;
}
