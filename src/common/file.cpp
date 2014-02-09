#include "file.h"
#include "jsonutil.h"

#include <QObject>

void File::fromJsonObject(const QJsonObject &packageObject)
{
    revision = JsonUtil::asString(packageObject, QStringLiteral("revision"));
    sha1 = JsonUtil::asString(packageObject, QStringLiteral("sha1"));
    bool ok;
    size = JsonUtil::asString(packageObject, QStringLiteral("size")).toLongLong(&ok);
    if(!ok)
        throw(QObject::tr("package 'size' is not a qint64 string"));
}

QJsonObject File::toJsonObject() const
{
    QJsonObject packageObject;
    packageObject.insert(QStringLiteral("revision"), revision);
    packageObject.insert(QStringLiteral("sha1"), sha1);
    packageObject.insert(QStringLiteral("size"), QString::number(size));

    return packageObject;
}

