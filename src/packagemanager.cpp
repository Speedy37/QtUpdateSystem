#include "packagemanager.h"

#include <QFile>
#include <QFileInfo>
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
}
