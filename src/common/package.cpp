#include "package.h"
#include "jsonutil.h"

#include <QObject>

void Package::fromJsonObject(const QJsonObject &packageObject)
{
    from = JsonUtil::asString(packageObject, QStringLiteral("from"));
    to = JsonUtil::asString(packageObject, QStringLiteral("to"));
    bool ok;
    size = JsonUtil::asString(packageObject, QStringLiteral("size")).toLongLong(&ok);
    if(!ok)
        throw(QObject::tr("package 'size' is not a qint64 string"));
}

QJsonObject Package::toJsonObject() const
{
    QJsonObject packageObject;
    packageObject.insert(QStringLiteral("from"), from);
    packageObject.insert(QStringLiteral("to"), to);
    packageObject.insert(QStringLiteral("size"), QString::number(size));

    return packageObject;
}
