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
};

#endif // VERSION_H
