#ifndef UPDATER_PACKAGE_H
#define UPDATER_PACKAGE_H

#include <QJsonObject>
#include <QString>

class Package
{
public:
    QString to;
    QString from;
    qint64 size;

    void fromJsonObject(const QJsonObject &packageObject);
    QJsonObject toJsonObject() const;
};

#endif // UPDATER_PACKAGE_H
