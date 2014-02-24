#ifndef VERSION_H
#define VERSION_H

#include <QJsonObject>
#include <QString>

class Version
{
public:
    Version(const QString &revision = QString(), const QString &description = QString());
    QString revision;
    QString description;
    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;

    void fromJsonObjectV1(const QJsonObject &object);
    QJsonObject toJsonObjectV1() const;
    bool operator==(const Version &other) const;
};

#endif // VERSION_H
