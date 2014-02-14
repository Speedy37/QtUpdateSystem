#include "packagemanager.h"
#include "common/jsonutil.h"
#include "common/utils.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>


const QString PackageManager::CurrentVersionFile = "current";
const QString PackageManager::VersionsFile = "versions";
const QString PackageManager::PackagesFile = "packages";

PackageManager::PackageManager(const QString &directory, QObject *parent) : QObject(parent)
{
    m_currentVersion = -1;
    setDirectory(directory);
}

bool PackageManager::isValid() const
{
    return directory().isEmpty();
}

void PackageManager::setDirectory(const QString &directory)
{
    m_directory = Utils::cleanPath(directory);
}

void PackageManager::load()
{
    try
    {
        m_packages.fromJsonObject(JsonUtil::fromJsonFile(m_directory + PackagesFile));
    }
    catch(const QString &) {}

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
}

void PackageManager::addPackage(const QString &packageFullName)
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

void PackageManager::save()
{
    JsonUtil::toJsonFile(m_directory + PackagesFile, m_packages.toJsonObject());
    JsonUtil::toJsonFile(m_directory + VersionsFile, m_versions.toJsonObject());
    if(m_currentVersion != -1)
        JsonUtil::toJsonFile(m_directory + CurrentVersionFile, m_versions.at(m_currentVersion).toJsonObject());
}

bool PackageManager::setCurrentRevision(const QString &revision)
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
