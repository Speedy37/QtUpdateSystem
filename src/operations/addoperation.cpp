#include "addoperation.h"
#include <log.h>
#include <QFile>
#include <QFileInfo>

const QString DATAOFFSET = QStringLiteral("dataOffset");
const QString DATALENGTH = QStringLiteral("dataLength");
const QString DATAHASHTYPE = QStringLiteral("dataHashType");
const QString DATAHASH = QStringLiteral("dataHash");
const QString FINALSIZE = QStringLiteral("finalSize");
const QString FINALHASH = QStringLiteral("finalHash");
const QString FINALHASHTYPE = QStringLiteral("finalHashType");
const QString DATACOMPRESSION = QStringLiteral("dataCompression");


void AddOperation::load1(const QJsonObject &object)
{
    Operation::load1(object);

    loadInt64(object.value(DATAOFFSET), &dataOffset);
    loadInt64(object.value(DATALENGTH), &dataLength);
    loadHashType(object.value(DATAHASHTYPE), &dataHashType);
    loadCompression(object.value(DATACOMPRESSION), &dataCompression);
    loadInt64(object.value(FINALSIZE), &finalSize);
    loadHashType(object.value(FINALHASHTYPE), &finalHashType);
    finalHash = object.value(FINALHASH).toString();
    dataHash = object.value(DATAHASH).toString();
}

QString AddOperation::fileHash(QFile *file, qint64 length, QCryptographicHash::Algorithm algorithm)
{
    QCryptographicHash hashMaker(algorithm);
    if(!file->isOpen())
    {
        Log::error(QObject::tr("Cannot create hash from a non open file"));
        return QString();
    }

    char buffer[4096];
    qint64 read, size = 4096;
    while(size > 0 && (read = file->read(buffer, size)) > 0)
    {
        hashMaker.addData(buffer, read);
        length -= read;
        if(length < size)
            size = length;
    }

    if(length != 0)
    {
        Log::error(QObject::tr("Cannot create hash, the amount of data read is unexpected"));
        return QString();
    }

    return QString(hashMaker.result().toHex());
}

void AddOperation::loadInt64(const QJsonValue &value, qint64 *dest)
{
    if(!value.isString())
        throw(tr("loadInt64 : Value is not a string, cannot convert to qint64"));

    bool ok;
    *dest = value.toString().toLongLong(&ok, 10);

    if(!ok)
        throw(tr("loadInt64 : Convertion to int64 failed"));
}

QString AddOperation::getHashType(QCryptographicHash::Algorithm algo)
{
    switch(algo)
    {
    case QCryptographicHash::Sha1:
        return QStringLiteral("Sha1");
    case QCryptographicHash::Sha256:
        return QStringLiteral("Sha256");
    case QCryptographicHash::Md5:
        return QStringLiteral("Md5");
    default:
        throw(tr("getHashType : Unrecognized algorithm %1").arg(algo));
    }
}

void AddOperation::loadHashType(const QJsonValue &value, QCryptographicHash::Algorithm *dest)
{
    if(!value.isString())
        throw(tr("loadHashType : Value is not a string, cannot convert to QCryptographicHash::Algorithm"));

    QString str = value.toString();
    if(str == QLatin1String("Sha1"))
        *dest = QCryptographicHash::Sha1;
    else if(str == QLatin1String("Sha256"))
        *dest = QCryptographicHash::Sha256;
    else if(str == QLatin1String("Md5"))
        *dest = QCryptographicHash::Md5;
    else
       throw(tr("loadHashType : Unrecognized algorithm %1").arg(str));
}

QString AddOperation::getCompression(AddOperation::Compression algo)
{
    switch(algo)
    {
    case Lzma:
        return QStringLiteral("lzma");
    case Xdelta:
        return QStringLiteral("xdelta3");
    case XdeltaLzma:
        return QStringLiteral("xdelta3+lzma");
    default:
        throw(tr("getCompression : Unrecognized algorithm %1").arg(algo));
    }
}

void AddOperation::loadCompression(const QJsonValue &value, AddOperation::Compression *dest)
{
    if(!value.isString())
        throw(tr("loadCompression : Value is not a string, cannot convert to AddOperation::Compression"));

    QString str = value.toString();
    if(str == QLatin1String("lzma"))
        *dest = Lzma;
    else if(str == QLatin1String("xdelta3"))
        *dest = Xdelta;
    else if(str == QLatin1String("xdelta3+lzma"))
        *dest = XdeltaLzma;
    else
        *dest = Uncompressed;
}

void AddOperation::run()
{
    if(state() == Applied)
        return;

    QFileInfo fileInfo(localpath());
    if(fileInfo.exists())
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
        if(!QFile::remove(localpath()))
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
        if(!dataFile.rename(localpath()))
            throw tr("Unable to rename file %1 to %2").arg(filename(), path);
        Log::info(tr("Rename succeeded %1").arg(path));
        return;
    }
    else if(dataCompression == Lzma)
    {
        Log::trace(tr("Decompressing %1 to %2").arg(filename(), path));

        QFile file(localpath());
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            throw tr("Unable to open file %1 for writing").arg(file.fileName());

        QProcess lzma;
        QStringList arguments;
#ifdef Q_OS_WIN
        arguments << "d" << dataFile.fileName() << "-so";
        lzma.start(QStringLiteral("lzma.exe"), arguments);
#endif
        if(!lzma.waitForStarted())
            throw tr("Unable to start lzma");

        QCryptographicHash hashMaker(finalHashType);
        char buffer[4096];
        qint64 read;
        while(lzma.waitForReadyRead())
        {
            while((read = lzma.read(buffer, 4096)) > 0)
            {
                hashMaker.addData(buffer, read);
                if(file.write(buffer, read) != read)
                {
                    lzma.kill();
                    throw tr("Failed to write to %1").arg(file.fileName());
                }
            }
        }

        if(!waitForFinished(lzma))
        {
            Log::error(QString(lzma.readAllStandardError()));
            throw tr("Extraction failed, lzma failed");
        }

        if(QString(hashMaker.result().toHex()) != finalHash)
            throw tr("Extraction failed, the result was unexpected");

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

void AddOperation::applyLocally(const QString &localFolder)
{
    QString localFilename = localFolder+path;

    if(!QFile::exists(localpath()))
        throw tr("Unable to apply update, source file doesn't exists");

    if(QFile::exists(localFilename) && !QFile::remove(localFilename))
        throw tr("Unable to remove destination file to replace it");

    QFileInfo fileInfo(localFilename);
    if(!fileInfo.dir().exists() && !QDir().mkpath(fileInfo.dir().absolutePath()))
        throw tr("Unable to create directory to copy file");

    if(!QFile::copy(localpath(), localFilename))
        throw tr("Unable to apply update, copy failed");

    Log::info(tr("File copied from %1 to %2").arg(localpath(), localFilename));
}

bool AddOperation::isDataValid()
{
    QFile file(filename());
    if(!file.exists())
    {
        file.setFileName(localpath());
        if(!file.exists() || file.size() != finalSize)
            return false;
        QCryptographicHash hashMaker(finalHashType);
        if(!file.open(QFile::ReadOnly))
        {
            Log::error(tr("Unable to open file : %1").arg(localpath()));
            return false;
        }
        hashMaker.addData(&file);
        if( QString(hashMaker.result().toHex()) == finalHash )
        {
            setState(Applied);
            return true;
        }
    }
    else
    {
        if(file.size() != dataLength)
            return false;
        QCryptographicHash hashMaker(dataHashType);
        if(!file.open(QFile::ReadOnly))
        {
            Log::error(tr("Unable to open file : %1").arg(filename()));
            return false;
        }
        hashMaker.addData(&file);
        if(dataHash == QString(hashMaker.result().toHex()))
        {
            setHash(dataHash);
            setState(Downloaded);
            return true;
        }
    }

    return false;
}

void AddOperation::save1(QJsonObject &object)
{
    Operation::save1(object);

    object.insert(DATAOFFSET, QString::number(dataOffset));
    object.insert(DATALENGTH, QString::number(dataLength));
    object.insert(DATAHASHTYPE, getHashType(dataHashType));
    object.insert(DATACOMPRESSION, getCompression(dataCompression));
    object.insert(FINALSIZE, QString::number(finalSize));
    object.insert(FINALHASHTYPE, getHashType(dataCompression));
    object.insert(FINALHASH, dataHash);
    object.insert(DATAHASH, dataHash);
}
