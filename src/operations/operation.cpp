#include "operation.h"
#include <QLoggingCategory>
#include <QFile>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QDebug>

Q_LOGGING_CATEGORY(LOG_OP, "updatesystem.operation")

const QString Operation::Path = QStringLiteral("path");

Operation::Operation()
{
    m_offset = 0;
    m_size = 0;
    m_status = Unknown;
}

QString Operation::sha1(QFile * file) const
{
    if(!file->open(QIODevice::ReadOnly))
    {
        qCCritical(LOG_OP) << "Unable to open " << file->fileName();
        return QString();
    }

    QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
    sha1Hash.addData(file);

    file->close();

    return QString(sha1Hash.result().toHex());
}

void Operation::setup(const QString &updateDir, const QString &tmpUpdateDir, int uniqueid)
{
    m_status = Unknown;
    setDataFilename(tmpUpdateDir + "Operation" + QString::number(uniqueid));
    setUpdateDirectory(updateDir);
}

void Operation::checkLocalData()
{
    m_status = localDataStatus();
}

void Operation::apply()
{
    try
    {
        qCDebug(LOG_OP) << "Apply on [" << type() << "] " << path();
        m_errorString = QString();
        Q_ASSERT_X(localDataStatus() == ApplyRequired, __FUNCTION__, (m_path + m_dataFilename).toLocal8Bit().data());
        applyData();
        Q_ASSERT(localDataStatus() == Valid);
        m_status = Valid;
        qCDebug(LOG_OP) << "Apply succeeded on [" << type() << "] " << path();
    }
    catch(const QString &msg)
    {
        cleanup();
        qCDebug(LOG_OP) << msg;
        m_errorString = msg;
        m_status = ApplyFailed;
        qCDebug(LOG_OP) << "Apply failed on [" << type() << "] " << path() << ": " << msg;
    }
}

void Operation::cleanup()
{
    if(fileType() == File)
    {
        if(!QFile(dataFilename()).remove())
            throwWarning(QObject::tr("Unable to remove temporary file %1").arg(dataFilename()));
    }
}

Operation::FileType Operation::fileType() const
{
    return None;
}

void Operation::fromJsonObjectV1(const QJsonObject &object)
{
    setPath(object.value(Path).toString());
}

QJsonObject Operation::toJsonObjectV1()
{
    QJsonObject object;

    fillJsonObjectV1(object);

    return object;
}

void Operation::fillJsonObjectV1(QJsonObject &object)
{
    object.insert(QStringLiteral("type"), type());
    object.insert(Path, path());
}
