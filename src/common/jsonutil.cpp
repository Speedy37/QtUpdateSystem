#include "jsonutil.h"

#include <QJsonParseError>
#include <QObject>
#include <QFile>

namespace JsonUtil {

void toJsonFile(const QString &filename, const QJsonObject &object, QJsonDocument::JsonFormat format)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly | QFile::Text))
        throw(QObject::tr("Unable to write file %1 %2").arg(filename, file.errorString()));
    file.write(QJsonDocument(object).toJson(format));
}

QJsonObject fromJsonFile(const QString &filename)
{
    QFile file(filename);
    if(file.open(QFile::ReadOnly | QFile::Text))
    {
        return JsonUtil::fromJson(file.readAll());
    }
    throw(QObject::tr("Unable to read file %1").arg(filename));
}

QJsonObject fromJson(const QByteArray &json)
{
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &jsonError);

    if(jsonError.error != QJsonParseError::NoError)
        throw(QObject::tr("Unable to parse json").arg(jsonError.errorString()));

    if(!doc.isObject())
        throw(QObject::tr("Expecting an object"));

    return doc.object();
}

QString asString(const QJsonValue & value)
{
    if(!value.isString())
        throw(QObject::tr("A string was expected"));

    return value.toString();
}

QJsonArray asArray(const QJsonValue & value)
{
    if(!value.isArray())
        throw(QObject::tr("An array was expected"));

    return value.toArray();
}

QJsonObject asObject(const QJsonValue & value)
{
    if(!value.isObject())
        throw(QObject::tr("An object was expected"));

    return value.toObject();
}

int asIntString(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(QObject::tr("Unable to find '%1' in the object").arg(key));

    if(!value.isString())
        throw(QObject::tr("Unable to find '%1' as an Int String").arg(key));

    bool ok;
    int v = value.toString().toInt(&ok);
    if(!ok)
        throw(QObject::tr("Unable to find '%1' as an Int String").arg(key));

    return v;
}

qint64 asInt64String(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(QObject::tr("Unable to find '%1' in the object").arg(key));

    if(!value.isString())
        throw(QObject::tr("Unable to find '%1' as an Int64 String").arg(key));

    bool ok;
    qint64 v = value.toString().toLongLong(&ok);
    if(!ok)
        throw(QObject::tr("Unable to find '%1' as an Int64 String").arg(key));

    return v;
}

QString asString(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(QObject::tr("Unable to find '%1' in the object").arg(key));

    if(!value.isString())
        throw(QObject::tr("Unable to find '%1' as a string").arg(key));

    return value.toString();
}

QJsonArray asArray(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(QObject::tr("Unable to find '%1' in the object").arg(key));

    if(!value.isArray())
        throw(QObject::tr("Unable to find '%1' as an array").arg(key));

    return value.toArray();
}

QJsonObject asObject(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(QObject::tr("Unable to find '%1' in the object").arg(key));

    if(!value.isObject())
        throw(QObject::tr("Unable to find '%1' as an object").arg(key));

    return value.toObject();
}

int asIntString(const QJsonValue &value)
{
    if(!value.isString())
        throw(QObject::tr("Unable to find '%1' as an Int String"));

    bool ok;
    int v = value.toString().toInt(&ok);
    if(!ok)
        throw(QObject::tr("Unable to find '%1' as an Int String"));

    return v;
}

qint64 asInt64String(const QJsonValue &value)
{
    if(!value.isString())
        throw(QObject::tr("Unable to find '%1' as an Int64 String"));

    bool ok;
    qint64 v = value.toString().toLongLong(&ok);
    if(!ok)
        throw(QObject::tr("Unable to find '%1' as an Int64 String"));

    return v;
}

}
