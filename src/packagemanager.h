#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include "common/packages.h"
#include "common/package.h"
#include "common/packagemetadata.h"
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

    Packages packages();
    void addPackage(const QString &packageFullName);
    void addPackage(const PackageMetadata &packageMetadata);
    void addPackage(const Package &package);

    QString currentRevision() const;
    void setCurrentRevision(const QString &revision);

private:
    QString m_directory, m_currentRevision;
    Packages m_packages;
};

inline QString PackageManager::directory() const
{
    return m_directory;
}

inline Packages PackageManager::packages()
{
    return m_packages;
}

inline void PackageManager::addPackage(const PackageMetadata &packageMetadata)
{
    addPackage(packageMetadata.package());
}

inline void PackageManager::addPackage(const Package &package)
{
    m_packages.addPackage(package);
}

inline QString PackageManager::currentRevision() const
{
    return m_currentRevision;
}

inline void PackageManager::setCurrentRevision(const QString &revision)
{
    m_currentRevision = revision;
}

#endif // PACKAGEMANAGER_H
