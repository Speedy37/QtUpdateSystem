#include "jsonutil.h"

namespace JsonUtil {


QString asString(const QJsonValue & value)
{
    if(!value.isString())
        throw(tr("A string was expected").arg(key));

    return value.toString();
}

QJsonArray asArray(const QJsonValue & value)
{
    if(!value.isArray())
        throw(tr("An array was expected").arg(key));

    return value.toArray();
}

QJsonObject asObject(const QJsonValue & value)
{
    if(!value.isObject())
        throw(tr("An object was expected").arg(key));

    return value.toObject();
}

QString asString(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(tr("Unable to find '%1' in the object").arg(key));

    if(!value.isString())
        throw(tr("Unable to find '%1' as a string").arg(key));

    return value.toString();
}

QJsonArray asArray(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(tr("Unable to find '%1' in the object").arg(key));

    if(!value.isArray())
        throw(tr("Unable to find '%1' as an array").arg(key));

    return value.toArray();
}

QJsonObject asObject(const QJsonObject & object, QString key)
{
    const QJsonValue value = object.value(key);

    if(value.isUndefined())
        throw(tr("Unable to find '%1' in the object").arg(key));

    if(!value.isObject())
        throw(tr("Unable to find '%1' as an object").arg(key));

    return value.toObject();
}

}
