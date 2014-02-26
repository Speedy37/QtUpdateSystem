#include "versions.h"
#include "jsonutil.h"

#include <QLoggingCategory>

void Versions::fromJsonObject(const QJsonObject & object)
{
    const QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        fromJsonArrayV1(JsonUtil::asArray(object, QStringLiteral("versions")));
    else
        throw(QObject::tr("Unsupported version %1").arg(version));
}

QJsonObject Versions::toJsonObject() const
{
    QJsonObject object;

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("versions"), toJsonArrayV1());

    return object;
}

void Versions::fromJsonArrayV1(const QJsonArray &versions)
{
    resize(versions.size());

    for(int i = 0; i < versions.size(); ++i)
    {
        Version & version = (*this)[i];
        version.fromJsonObjectV1(JsonUtil::asObject(versions[i]));
    }
}

QJsonArray Versions::toJsonArrayV1() const
{
    QJsonArray versions;

    for(int i = 0; i < size(); ++i)
    {
        versions.append(at(i).toJsonObjectV1());
    }

    return versions;
}
