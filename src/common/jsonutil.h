#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>

namespace JsonUtil {
    QJsonObject asObject(const QJsonValue &value);
    QJsonArray asArray(const QJsonValue &value);
    QString asString(const QJsonValue &value);
    QJsonObject asObject(const QJsonObject &object, QString key);
    QJsonArray asArray(const QJsonObject &object, QString key);
    QString asString(const QJsonObject &object, QString key);
}

#endif // JSONUTIL_H
