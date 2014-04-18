#include "repository.h"
#include "common/jsonutil.h"
#include "common/utils.h"
#include "exceptions.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>


const QString Repository::CurrentVersionFile = "current";
const QString Repository::VersionsFile = "versions";
const QString Repository::PackagesFile = "packages";

Repository::Repository(const QString &directory, QObject *parent) : QObject(parent)
{
    m_currentVersion = -1;
    setDirectory(directory);
}

bool Repository::isValid() const
{
    return directory().isEmpty();
}

void Repository::setDirectory(const QString &directory)
{
    m_directory = Utils::cleanPath(directory);
}

Packages Repository::packages(const QString &version) const
{
    Packages packages;
    foreach(const Package & package, m_packages)
    {
        if(package.to == version)
            packages.append(package);
    }
    return packages;
}

void Repository::load(bool safetyChecks)
{
    try
    {
        m_versions.fromJsonObject(JsonUtil::fromJsonFile(m_directory + VersionsFile));
    }
    catch(...) {}

    try
    {
        Version version;
        version.fromJsonObject(JsonUtil::fromJsonFile(m_directory + CurrentVersionFile));

        for(m_currentVersion = 0; m_currentVersion < m_versions.size(); ++m_currentVersion)
        {
            if(version == m_versions.at(m_currentVersion))
                break;
        }
        if(m_currentVersion == m_versions.size())
        {
            m_versions.append(version);
        }
    }
    catch(...)
    {
        m_currentVersion = m_versions.size()-1;
    }

    try
    {
        m_packages.fromJsonObject(JsonUtil::fromJsonFile(m_directory + PackagesFile));
    }
    catch(...) {}

    if(safetyChecks)
    {
        int i, findPos;

        // Remove version duplicates
        for(i = 0; i < m_versions.size(); ++i)
        {
            const Version & version = m_versions.at(i);
            while((findPos = m_versions.indexOf(version, i + 1)) > i)
            {
                m_versions.remove(findPos);
            }
        }

        // Remove package duplicates
        for(i = 0; i < m_packages.size(); ++i)
        {
            const Package & package = m_packages.at(i);
            while((findPos = m_packages.indexOf(package, i + 1)) > i)
            {
                m_packages.remove(findPos);
            }
            if(package.to.isEmpty())
            {
                m_packages.remove(i);
                --i;
                continue;
            }
            ensurePackageVersionExists(package);
        }
    }
}

/*!
   \brief Add a package to this repository
   \param packageFullName Name of the package to add.
       The package must be in the repository directory and have a valid conventionnal name.
   \throw FileMissing
   \throw UnableToOpenFile
   \throw InvalidPackageName
   \throw InvalidPackage
   \sa importPackage()
 */
void Repository::addPackage(const QString &packageFullName)
{
    QFile metadataFile(m_directory + packageFullName + PackageMetadata::FileExtension);
    if(metadataFile.exists())
    {
        if(metadataFile.open(QFile::ReadOnly | QFile::Text))
        {
            PackageMetadata packageMetadata;
            packageMetadata.fromJsonObject(JsonUtil::fromJson(metadataFile.readAll()), false);

            if(packageMetadata.dataUrl() != packageFullName)
                THROW(InvalidPackageName, packageFullName, packageMetadata.dataUrl());

            addPackage(packageMetadata);
            return;
        }
        THROW(UnableToOpenFile, metadataFile.fileName());
    }
    THROW(FileMissing, metadataFile.fileName());
}

/*!
   \brief Add a package to this repository
   \param packageFullName Name of the package to add.
       The package must be in the repository directory and have a valid conventionnal name.
   \throw InvalidPackage
   \sa importPackage()
 */
void Repository::addPackage(const Package &package)
{
    if(package.to.isEmpty())
        THROW(InvalidPackage, tr("the 'to' property is empty"));
    ensurePackageVersionExists(package);
    m_packages.append(package);
}

void Repository::removePackage(const QString &packageFullName)
{
    QFile metadataFile(m_directory + packageFullName + PackageMetadata::FileExtension);
    if(metadataFile.exists())
    {
        if(metadataFile.open(QFile::ReadOnly | QFile::Text))
        {
            PackageMetadata packageMetadata;
            packageMetadata.fromJsonObject(JsonUtil::fromJson(metadataFile.readAll()), false);
            removePackage(packageMetadata);
            return;
        }
        THROW(UnableToOpenFile, metadataFile.fileName());
    }
    THROW(FileMissing, metadataFile.fileName());
}

void Repository::removePackage(const Package &package)
{
    int pos = m_packages.indexOf(package);
    if(pos != -1)
    {
        m_packages.remove(pos);
        return;
    }
}

void Repository::simplify()
{
    QVector<bool> useful(m_packages.size(), false);
    const QString & currentVersion = m_versions.at(m_currentVersion).revision;

    markPackagesAsUseful(useful, m_packages.findBestPath(QString(), currentVersion));
    for(int i = 0; i < m_versions.size(); ++i)
    {
        markPackagesAsUseful(useful, m_packages.findBestPath(m_versions.at(i).revision, currentVersion));
    }

    for(int i = m_packages.size() - 1; i >= 0; --i)
    {
        if(!useful.at(i))
            m_packages.remove(i);
    }
}

void Repository::markPackagesAsUseful(QVector<bool> &useful, const QVector<Package> packages)
{
    foreach(const Package &usefulPackage, packages)
    {
        for(int i = 0; i < m_packages.size(); ++i)
        {
            if(m_packages.at(i) == usefulPackage)
            {
                useful[i] = true;
                break;
            }
        }
    }
}

void Repository::save()
{
    JsonUtil::toJsonFile(m_directory + PackagesFile, m_packages.toJsonObject());
    JsonUtil::toJsonFile(m_directory + VersionsFile, m_versions.toJsonObject());
    if(m_currentVersion != -1)
        JsonUtil::toJsonFile(m_directory + CurrentVersionFile, m_versions.at(m_currentVersion).toJsonObject());
}

bool Repository::setCurrentRevision(const QString &revision)
{
    for(int i = 0; i < m_versions.size(); ++i)
    {
        if(m_versions.at(i).revision == revision)
        {
            m_currentVersion = i;
            return true;
        }
    }

    return false;
}

/*!
   \brief Makes sure both the 'from' and 'to' versions of package exists
   \param package
 */
void Repository::ensurePackageVersionExists(const Package &package)
{
    int toIndex = 0, len = m_versions.size();

    if(!package.from.isEmpty())
    {
        int fromIndex = 0;
        for(; fromIndex < len && m_versions.at(fromIndex).revision != package.from; ++fromIndex)
            ;

        if(fromIndex == len)
        {
            m_versions.insert(toIndex, package.from);
        }
    }

    for(; toIndex < len && m_versions.at(toIndex).revision != package.to; ++toIndex)
        ;

    if(toIndex == len)
    {
        m_versions.append(package.to);
    }
}
