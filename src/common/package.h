#ifndef UPDATER_PACKAGE_H
#define UPDATER_PACKAGE_H

#include <QJsonObject>
#include <QString>

class Operation;

class Package
{
public:
    Package();
    Package(const QString &to, const QString &from, qint64 size);
    QString to;
    QString from;
    qint64 size;

    static QString repositoryPackageName(const QString &from, const QString &to);
    QString url() const;
    QString metadataUrl() const;
    void fromJsonObjectV1(const QJsonObject &packageObject);
    QJsonObject toJsonObjectV1() const;
    bool operator==(const Package &other);
};

inline QString Package::metadataUrl() const
{
    return url() + QStringLiteral(".metadata");
}

#endif // UPDATER_PACKAGE_H
