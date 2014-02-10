#include "version.h"
#include "jsonutil.h"

void Version::fromJsonObject(const QJsonObject &object)
{
    revision = JsonUtil::asString(object, QStringLiteral("revision"));
    description = JsonUtil::asString(object, QStringLiteral("description"));
}

QJsonObject Version::toJsonObject() const
{
    QJsonObject object;

    object.insert(QStringLiteral("revision"), revision);
    object.insert(QStringLiteral("description"), description);

    return object;
}
