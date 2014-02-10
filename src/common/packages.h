#ifndef PACKAGES_H
#define PACKAGES_H

#include <QJsonDocument>
#include <QVector>

class Package;

class Packages
{
public:
    void addPackage(const Package & package);
    void loadPackages(const QJsonObject &object);
    QJsonObject toJsonObject() const;
    QVector<Package> findBestPath(const QString &from, const QString &to);
    const QVector<Package> packages() const;

private:
    QVector<Package> m_packages;
    void loadPackages1(const QJsonObject object);
};

inline const QVector<Package> Packages::packages() const
{
    return m_packages;
}

#endif // PACKAGES_H
