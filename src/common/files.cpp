#include "files.h"
#include "jsonutil.h"

#include <QObject>

void Files::loadFiles(const QJsonObject &object)
{
    const QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        loadFiles1(object);
    else
        throw(QObject::tr("Unsupported version %1").arg(version));
}

void Files::loadFiles1(const QJsonObject object)
{
    m_files.clear();

    const QJsonObject files = JsonUtil::asObject(object, QStringLiteral("files"));
    QJsonObject::const_iterator it = files.constBegin();
    while (it != files.constEnd())
    {
        QVector<File> filesInPath;
        const QJsonArray filesInPathArray = JsonUtil::asArray(it.value());
        filesInPath.resize(filesInPathArray.size());
        for(int i = 0; i < filesInPathArray.size(); ++i)
        {
            filesInPath[i].fromJsonObject(JsonUtil::asObject(filesInPathArray[i]));
        }
        m_files.insert(it.key(), filesInPath);
    }
}

QJsonObject Files::toJsonObject() const
{
    QJsonObject object;
    QJsonObject files;

    QMap<QString, QVector<File>>::const_iterator i = m_files.constBegin();
    while (i != m_files.constEnd())
    {
        const QVector<File> & filesInPath = i.value();
        QJsonArray filesInPathArray;
        foreach(const File & file, filesInPath)
        {
            filesInPathArray.append(file.toJsonObject());
        }

        files.insert(i.key(), filesInPathArray);
        ++i;
    }

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("files"), files);

    return object;
}
