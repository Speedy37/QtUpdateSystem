#ifndef UPDATER_PACKAGE_H
#define UPDATER_PACKAGE_H

#include <QJsonObject>
#include <QString>

class Operation;

class Package
{
public:
    Package();
    QString to;
    QString from;
    qint64 size;

    QString url() const;
    QString metadataUrl() const;
    void fromJsonObjectV1(const QJsonObject &packageObject);
    QJsonObject toJsonObjectV1() const;
};

inline QString Package::metadataUrl() const
{
    return url() + QStringLiteral(".metadata");
}

#endif // UPDATER_PACKAGE_H
