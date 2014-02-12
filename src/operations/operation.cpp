#include "operation.h"
#include <qtlog.h>
#include <QFile>
#include <QCryptographicHash>

const QString PATH = QStringLiteral("path");
const QString ACTION = QStringLiteral("action");

Operation::Operation()
{
    m_offset = -1;
    m_size = -1;
    m_status = Unknown;
}

QString Operation::sha1(QFile * file) const
{
    if(!file->open(QIODevice::ReadOnly))
    {
        LOG_ERROR(QObject::tr("Unable to open %1").arg(file->fileName()));
        return QString();
    }

    QCryptographicHash sha1Hash(QCryptographicHash::Sha1);
    sha1Hash.addData(file);

    file->close();

    return QString(sha1Hash.result().toHex());
}

void Operation::checkLocalData()
{
    m_status = localDataStatus();
}

void Operation::apply()
{
    try
    {
        m_errorString = QString();
        Q_ASSERT(localDataStatus() == ApplyRequired);
        applyData();
        Q_ASSERT(localDataStatus() == Valid);
        m_status = Valid;
    }
    catch(const QString &msg)
    {
        m_errorString = msg;
        m_status = ApplyFailed;
    }
}

void Operation::fromJsonObjectV1(const QJsonObject &object)
{
    m_path = object.value(PATH).toString();
}

QJsonObject Operation::toJsonObjectV1()
{
    QJsonObject object;

    toJsonObjectV1(object);

    return object;
}

void Operation::toJsonObjectV1(QJsonObject &object)
{
    object.insert(ACTION, action());
    object.insert(PATH, m_path);
}
