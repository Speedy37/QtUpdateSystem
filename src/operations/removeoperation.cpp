#include "removeoperation.h"
#include "../common/jsonutil.h"

#include <QLoggingCategory>
#include <QFileInfo>
#include <QDir>

Q_LOGGING_CATEGORY(LOG_RMFILEOP, "updatesystem.common.rmfile")

const QString RemoveOperation::Action = QStringLiteral("rm");

RemoveOperation::RemoveOperation() : Operation()
{
    m_localSize = 0;
}

void RemoveOperation::create(const QString &path, const QString &oldFilename)
{
    QFile file(oldFilename);
    if(!file.exists(oldFilename))
        throw QObject::tr("File %1 doesn't exists").arg(oldFilename);

    m_localSha1 = sha1(&file);
    m_localSize = file.size();
    m_path = path;
}

Operation::Status RemoveOperation::localDataStatus()
{
    QFileInfo fileInfo(localFilename());
    if(!fileInfo.exists())
    {
        qCDebug(LOG_RMFILEOP) << "File was already removed" << path();
        return Valid;
    }

    return ApplyRequired;
}

void RemoveOperation::applyData()
{
    QFileInfo dirInfo(localFilename());
    if(dirInfo.isDir())
    {
        qCWarning(LOG_RMFILEOP) << "The update was supposed to remove a file, but a directory was found" << path();
        if(!QDir().rmdir(localFilename()))
            throw QObject::tr("Failed to remove directory %1").arg(path());
    }
    else
    {
        if(!QFile::remove(localFilename()))
            throw QObject::tr("The update failed to remove the file %1").arg(path());
        qCDebug(LOG_RMFILEOP) << "File removed" << path();
    }
}

void RemoveOperation::fromJsonObjectV1(const QJsonObject &object)
{
    Operation::fromJsonObjectV1(object);

    m_localSize = JsonUtil::asInt64String(object, QStringLiteral("localSize"));
    m_localSha1 = JsonUtil::asString(object, QStringLiteral("localSha1"));
}

void RemoveOperation::fillJsonObjectV1(QJsonObject &object)
{
    Operation::fillJsonObjectV1(object);

    object.insert("localSize", QString::number(m_localSize));
    object.insert("localSha1", m_localSha1);
}
QString RemoveOperation::type() const
{
    return Action;
}
