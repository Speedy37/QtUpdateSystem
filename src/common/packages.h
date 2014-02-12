#ifndef PACKAGES_H
#define PACKAGES_H

#include <QVector>
#include <QJsonObject>
#include "package.h"

class Packages
{
public:
    static const QString FileName;
    void addPackage(const Package & package);
    const QVector<Package> packages() const;

    QVector<Package> findBestPath(const QString &from, const QString &to);

    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;

    void fromJsonArrayV1(const QJsonArray &packages);
    QJsonArray toJsonArrayV1() const;

private:
    QVector<Package> m_packages;
};

inline const QVector<Package> Packages::packages() const
{
    return m_packages;
}

#endif // PACKAGES_H
