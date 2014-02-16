#include "repository.h"
#include "common/jsonutil.h"
#include "common/utils.h"

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

void Repository::load(bool safetyChecks)
{
    try
    {
        m_versions.fromJsonObject(JsonUtil::fromJsonFile(m_directory + VersionsFile));
    }
    catch(const QString &) {}

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
    catch(const QString &)
    {
        m_currentVersion = m_versions.size()-1;
    }

    try
    {
        m_packages.fromJsonObject(JsonUtil::fromJsonFile(m_directory + PackagesFile));
    }
    catch(const QString &) {}

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

void Repository::addPackage(const QString &packageFullName)
{
    QFile metadataFile(m_directory + packageFullName + PackageMetadata::FileExtension);
    if(metadataFile.exists())
    {
        if(metadataFile.open(QFile::ReadOnly | QFile::Text))
        {
            PackageMetadata packageMetadata;
            packageMetadata.fromJsonObject(JsonUtil::fromJson(metadataFile.readAll()), false);
            addPackage(packageMetadata);
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

void Repository::addPackage(const Package &package)
{
    if(package.to.isEmpty())
        return;
    ensurePackageVersionExists(package);
    m_packages.append(package);
}
