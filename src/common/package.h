#ifndef UPDATER_PACKAGE_H
#define UPDATER_PACKAGE_H

#include <QJsonObject>
#include <QString>

class Operation;

class Package
{
public:
    QString to;
    QString from;
    qint64 size;

    QString url() const;
    QString metadataUrl() const;
    void fromJsonObject(const QJsonObject &packageObject);
    QJsonObject toJsonObject() const;
};

inline QString Package::metadataUrl() const
{
    return url() + QStringLiteral(".metadata");
}

#endif // UPDATER_PACKAGE_H
