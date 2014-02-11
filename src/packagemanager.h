#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include "common/packages.h"
#include <QObject>
#include <QVector>

/**
 * @brief Manage a package repository
 * A package repository is a directory that contains the following files :
 *  - current : a json file that hold the current distributed revision (revision)
 *  - versions : a json file that hold a list of revisions
 *  - packages : a json file that hold a list of available packages (from/to/size)
 *  - PACKNAME.metadata : a json file that hold informations about a package (operations/size/...)
 *  - PACKNAME : a data file that contains all data necessary for a package
 */
class PackageManager : public QObject
{
    Q_OBJECT
public:
    PackageManager(const QString &directory = QString(), QObject *parent = 0);
    bool isValid() const;

    QString directory() const;
    void setDirectory(const QString &directory);

    const Packages &packages() const;
    Packages packages();
    void addPackage(const QString &packageFullName);

    void setCurrentRevision(const QString &revision);
private:
    QString m_directory;
    Packages m_packages;
};

inline QString PackageManager::directory() const
{
    return m_directory;
}

inline const Packages &PackageManager::packages() const
{
    return m_packages;
}

inline Packages PackageManager::packages()
{
    return m_packages;
}

#endif // PACKAGEMANAGER_H
