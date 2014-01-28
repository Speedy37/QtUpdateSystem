#include "operation.h"

const QString PATH = QStringLiteral("path");

void Operation::load1(const QJsonObject &object)
{
    path = object.value(PATH).toString();
}

QJsonObject Operation::save1()
{
    QJsonObject object;
    save1(object);
    return object;
}

void Operation::save1(QJsonObject &object)
{
    object.insert(PATH, path);
}
