#include "localrepository.h"
#include "../common/utils.h"

#include <QDir>
#include <QSettings>


const QString LocalRepository::Revision = QStringLiteral("Revision");
const QString LocalRepository::UpdateInProgress = QStringLiteral("UpdateInProgress");
const QString LocalRepository::FileList = QStringLiteral("FileList");
const QString LocalRepository::DirList = QStringLiteral("DirList");
const QString LocalRepository::StatusFileName = QStringLiteral("status.ini");

LocalRepository::LocalRepository()
{

}

LocalRepository::LocalRepository(const QString &directory)
{
    setDirectory(directory);
}

bool LocalRepository::load()
{
    QSettings settings(m_directory + StatusFileName, QSettings::IniFormat);
    m_localRevision = settings.value(Revision).toString();
    m_updateInProgress  = settings.value(UpdateInProgress).toBool();
    m_fileList = settings.value(FileList).toStringList();
    m_dirList = settings.value(DirList).toStringList();

    return settings.status() == QSettings::NoError;
}

bool LocalRepository::save()
{
    QSettings settings(m_directory + StatusFileName, QSettings::IniFormat);
    settings.setValue(Revision, m_localRevision);
    settings.setValue(UpdateInProgress, m_updateInProgress);
    settings.setValue(FileList, m_fileList);
    settings.setValue(DirList, m_dirList);
    settings.sync();

    return settings.status() == QSettings::NoError;
}

bool LocalRepository::isManaged(const QFileInfo &file) const
{
    QString filename = Utils::cleanPath(file.absoluteFilePath(), false);
    if(filename.startsWith(directory()))
    {
        filename = filename.remove(0, directory().size());
        if(file.isDir())
            return dirList().contains(filename);
        else
            return StatusFileName == filename || fileList().contains(filename);
    }

    return false;
}

void LocalRepository::setDirectory(const QString &directory)
{
    m_directory = Utils::cleanPath(QDir(directory).absolutePath());
    load();
}
