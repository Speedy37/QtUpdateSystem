#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>

namespace JsonUtil {
    QJsonObject fromJsonFile(const QString &filename);
    QJsonObject fromJson(const QByteArray & json);
    QJsonObject asObject(const QJsonValue &value);
    QJsonArray asArray(const QJsonValue &value);
    QString asString(const QJsonValue &value);
    int asIntString(const QJsonValue &value);
    qint64 asInt64String(const QJsonValue &value);
    QJsonObject asObject(const QJsonObject &object, QString key);
    QJsonArray asArray(const QJsonObject &object, QString key);
    QString asString(const QJsonObject &object, QString key);
    int asIntString(const QJsonObject & object, QString key);
    qint64 asInt64String(const QJsonObject & object, QString key);
}

#endif // JSONUTIL_H
