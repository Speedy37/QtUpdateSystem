#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include "qtupdatesystem_global.h"
#include "common/versions.h"
#include "common/packages.h"
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
class QTUPDATESYSTEMSHARED_EXPORT PackageManager : public QObject
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
    bool setCurrentRevision(const QString &revision);

    void load();
    void save();
private:
    static const QString CurrentVersionFile;
    static const QString VersionsFile;
    static const QString PackagesFile;
    QString m_directory;
    Packages m_packages;
    Versions m_versions;
    int m_currentVersion;
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
    return m_currentVersion != -1 ? m_versions.at(m_currentVersion).revision : QString();
}



#endif // PACKAGEMANAGER_H
