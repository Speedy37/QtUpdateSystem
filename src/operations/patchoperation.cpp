#include "patchoperation.h"
#include "../common/jsonutil.h"
#include "../common/utils.h"
#include "../tools/brotli.h"
#include "../tools/lzma.h"
#include "../tools/xdelta3.h"
#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QJsonDocument>

Q_LOGGING_CATEGORY(LOG_PATCHOP, "updatesystem.patchoperation")

const QString PatchOperation::Action = QStringLiteral("patch");

const QString PatchOperation::LocalSize = QStringLiteral("localSize");
const QString PatchOperation::LocalSha1 = QStringLiteral("localSha1");
const QString PatchOperation::PathType = QStringLiteral("patchType");

const QString COMPRESSION_LZMA = QStringLiteral("lzma");
const QString COMPRESSION_BROTLI = QStringLiteral("brotli");
const QString COMPRESSION_NONE = QStringLiteral("none");

const QString PATCHTYPE_XDELTA = QStringLiteral("xdelta");

PatchOperation::PatchOperation() : AddOperation()
{
    m_localSize = 0;
}

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
                qCDebug(LOG_PATCHOP) << "File is already at the right version" << path();
                return Valid;
            }
            else if(hash == m_localSha1)
            {
                qCDebug(LOG_PATCHOP) << "File is as expected for patching" << path();

                file.setFileName(dataFilename());
                if(file.exists())
                {
                    // Check downloaded data file content
                    if(file.size() == size() && sha1(&file) == sha1())
                    {
                        qCDebug(LOG_PATCHOP) << "File data is valid" << path();
                        return ApplyRequired;
                    }
                    throwWarning(QObject::tr("File data is invalid and will be downloaded again"));
                }
                return DownloadRequired;
            }
        }

        throwWarning(QObject::tr("File content is invalid"));
    }
    else
    {
        throwWarning(QObject::tr("File doesn't exists and can't be patched, complete file download will happen"));
    }

    return LocalFileInvalid;
}

void PatchOperation::applyData()
{
    if(m_patchtype == PATCHTYPE_XDELTA)
    {
        qCDebug(LOG_PATCHOP) << "Decompressing " << dataFilename() << " to " << path() << " by " << m_compression << " and " << m_patchtype;

        QString patchedFilename = dataFilename()+".patched";
        QFile file(patchedFilename);
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw QObject::tr("Unable to open file %1 for writing").arg(file.fileName());

        QFile baseFile(localFilename());
        if (!baseFile.open(QFile::ReadOnly))
            throw QObject::tr("Unable to open file %1 for writing").arg(baseFile.fileName());

        QFile dataFile(dataFilename());
        if (!dataFile.open(QFile::ReadOnly))
            throw QObject::tr("Unable to open file %1 for reading").arg(dataFile.fileName());

        QScopedPointer<QIODevice> source;
        if(m_compression == COMPRESSION_NONE)
            source.reset(&dataFile);
        else if (m_compression == COMPRESSION_BROTLI)
            source.reset(BrotliDecompressor(&dataFile));
        else if (m_compression == COMPRESSION_LZMA)
            source.reset(LZMADecompressor(&dataFile));
        else
            throw QObject::tr("Unsupported compression %1").arg(m_compression);
        QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
        QScopedPointer<QIODevice> xd3(XDelta3(source.data(), &baseFile, false));
        readAll(xd3.data(), &file, &sha1Hash);

        if(QString(sha1Hash.result().toHex()) != m_finalSha1)
            throw QObject::tr("Final sha1 file signature doesn't match");

        if(!file.flush())
            throw QObject::tr("Unable to flush all extracted data");

        baseFile.close();
        file.close();
        dataFile.close();

        Q_ASSERT(sha1(&file) == m_finalSha1);

        qCDebug(LOG_PATCHOP) << "Patch succeeded" << path();

        if(!baseFile.remove())
            throw QObject::tr("Unable to remove local file %1").arg(path());

        if(!file.rename(localFilename()))
            throw QObject::tr("Unable to rename file %1 to %2").arg(file.fileName(), path());

        cleanup();
    }
    else
    {
        throw QObject::tr("Patchtype %1 unknown").arg(m_patchtype);
    }
}

void PatchOperation::fillJsonObjectV1(QJsonObject &object)
{
    AddOperation::fillJsonObjectV1(object);

    object.insert(LocalSize, QString::number(m_localSize));
    object.insert(LocalSha1, m_localSha1);
    object.insert(PathType, m_patchtype);
}

void PatchOperation::fromJsonObjectV1(const QJsonObject &object)
{
    AddOperation::fromJsonObjectV1(object);

    m_localSize = JsonUtil::asInt64String(object, LocalSize);
    m_localSha1 = JsonUtil::asString(object, LocalSha1);
    m_patchtype = JsonUtil::asString(object, PathType);
}

void PatchOperation::create(const QString &filepath, const QString &oldFilename, const QString &newFilename, const QString &tmpDirectory)
{
    setPath(filepath);

    QFile newFile(newFilename);
    QFile oldFile(oldFilename);

    // New file informations
    {
        if(!newFile.exists(newFilename))
            throw QObject::tr("File %1 doesn't exists").arg(newFilename);

        m_finalSha1 = sha1(&newFile);
        m_finalSize = newFile.size();
    }

    // Old file informations
    {
        if(!oldFile.exists(oldFilename))
            throw QObject::tr("File %1 doesn't exists").arg(oldFilename);

        m_localSha1 = sha1(&oldFile);
        m_localSize = oldFile.size();
    }

    if(m_localSize == m_finalSize && m_localSha1 == m_finalSha1)
    {
        return;
    }

    setDataFilename(tmpDirectory + "patch_" + m_finalSha1 + "_" + m_localSha1);
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
                m_patchtype = JsonUtil::asString(object, PathType);
                return;
            }
            catch(...) { }
            metadataFile.close();
        }
    }


    if (!newFile.open(QFile::ReadOnly))
        throw QObject::tr("Unable to open file %1 for reading").arg(newFile.fileName());

    if (!oldFile.open(QFile::ReadOnly))
        throw QObject::tr("Unable to open file %1 for reading").arg(oldFile.fileName());

    if(!dataFile.open(QFile::WriteOnly | QFile::Truncate))
        throw QObject::tr("Unable to open file %1 for writing").arg(dataFile.fileName());

    if (!metadataFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
        throw QObject::tr("Unable to open file %1 for writing : %2").arg(metadataFile.fileName(), metadataFile.errorString());

    m_compression = COMPRESSION_BROTLI;
    m_patchtype = PATCHTYPE_XDELTA;
    
    QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
    QScopedPointer<QIODevice> xd3(XDelta3(&newFile, &oldFile, true));
    QScopedPointer<QIODevice> compressor(BrotliCompressor(xd3.data()));
    readAll(compressor.data(), &dataFile, &sha1Hash);

    m_sha1 = QString(sha1Hash.result().toHex());
    m_size = dataFile.size();

    if(!dataFile.flush())
        throw QObject::tr("Unable to flush all compressed data");

    dataFile.close();

    Q_ASSERT(sha1(&dataFile) == m_sha1);

    // Writing metadata
    {
        QJsonObject object;
        object.insert(Path, path());
        object.insert(DataSize, QString::number(m_size));
        object.insert(DataSha1, m_sha1);
        object.insert(DataCompression, m_compression);
        object.insert(PathType, m_patchtype);

        if(metadataFile.write(QJsonDocument(object).toJson()) == -1)
            throw QObject::tr("Unable to write metadata");

        if(!metadataFile.flush())
            throw QObject::tr("Unable to flush metadata");

        metadataFile.close();
    }
}

QString PatchOperation::type() const
{
    return Action;
}
