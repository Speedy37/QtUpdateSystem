#include "packagemanager.h"
#include "common/jsonutil.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

PackageManager::PackageManager(const QString &directory, QObject *parent) : QObject(parent)
{
    setDirectory(directory);
}

bool PackageManager::isValid() const
{
    return directory().isEmpty();
}

void PackageManager::setDirectory(const QString &directory)
{
    m_directory = directory;
    if(!m_directory.endsWith(QDir::separator()))
        m_directory += QDir::separator();

    try
    {
        m_packages.fromJsonObject(JsonUtil::fromJsonFile(m_directory + Packages::FileName));
    }
    catch(const QString &) {}

    try
    {
        m_packages.fromJsonObject(JsonUtil::fromJsonFile(m_directory + Packages::FileName));
    }
    catch(const QString &) {}
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


