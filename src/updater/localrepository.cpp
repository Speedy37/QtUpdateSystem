#include "localrepository.h"
#include "../common/utils.h"

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
    m_directory = Utils::cleanPath(directory);
    load();
}

void LocalRepository::load()
{
    QSettings settings(m_directory + StatusFileName, QSettings::IniFormat);
    m_localRevision = settings.value(Revision).toString();
    m_updateInProgress  = settings.value(UpdateInProgress).toBool();
    m_fileList = settings.value(FileList).toStringList();
    m_dirList = settings.value(DirList).toStringList();
}

void LocalRepository::save()
{
    QSettings settings(m_directory + StatusFileName, QSettings::IniFormat);
    settings.setValue(Revision, m_localRevision);
    settings.setValue(UpdateInProgress, m_updateInProgress);
    settings.setValue(FileList, m_fileList);
    settings.setValue(DirList, m_dirList);
}
