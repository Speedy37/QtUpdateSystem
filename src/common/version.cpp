#include "version.h"
#include "jsonutil.h"
#include "../exceptions.h"

#include <QObject>

Version::Version(const QString &revision, const QString &description)
{
    this->revision = revision;
    this->description = description;
}

void Version::fromJsonObject(const QJsonObject &object)
{
    QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        fromJsonObjectV1(JsonUtil::asObject(object, QStringLiteral("current")));
    else
        THROW(UnsupportedVersion, version);
}

QJsonObject Version::toJsonObject() const
{
    QJsonObject object;

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("current"), toJsonObjectV1());

    return object;
}

void Version::fromJsonObjectV1(const QJsonObject &object)
{
    revision = JsonUtil::asString(object, QStringLiteral("revision"));
    description = JsonUtil::asString(object, QStringLiteral("description"));
}

QJsonObject Version::toJsonObjectV1() const
{
    QJsonObject object;

    object.insert(QStringLiteral("revision"), revision);
    object.insert(QStringLiteral("description"), description);

    return object;
}

bool Version::operator==(const Version &other) const
{
    return revision == other.revision;
}
