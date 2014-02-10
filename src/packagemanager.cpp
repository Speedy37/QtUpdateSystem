#include "packagemanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

QJsonObject loadJsonInfo(const QString &filename)
{
    QFile file(filename);

    if(!file.open(QFile::ReadOnly | QFile::Text))
        throw QObject::tr("Unable to open file %1 for reading").arg(file.fileName());

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);

    if(error.error != QJsonParseError::NoError)
        throw QObject::tr("Json loading failed %1").arg(error.errorString());

    if(!document.isObject())
        throw QObject::tr("A json object was expected");

    return document.object();
}

void saveJsonInfo(const QString &filename, const QJsonObject &json)
{
    QFile file(filename);

    if(!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        throw QObject::tr("Unable to open file %1 for writing").arg(file.fileName());

    QByteArray data = QJsonDocument(json).toJson();
    if(file.write(data) != data.size())
        throw QObject::tr("Unable to write all json content : %1").arg(file.errorString());
}

PackageManager::PackageManager(const QString &directory) : QObject(0)
{
    m_directory = directory;
}
