#ifndef VERSION_H
#define VERSION_H

#include <QJsonObject>
#include <QString>

class Version
{
public:
    QString revision;
    QString description;
    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;

    void fromJsonObjectV1(const QJsonObject &object);
    QJsonObject toJsonObjectV1() const;
};

#endif // VERSION_H
