#include "localrepository.h"
#include "../common/utils.h"

#include <QSettings>


const QString LocalRepository::Revision = QStringLiteral("Revision");
const QString LocalRepository::UpdatingTo = QStringLiteral("UpdatingTo");
const QString LocalRepository::FileList = QStringLiteral("FileList");
const QString LocalRepository::StatusFileName = QStringLiteral("status.ini");

LocalRepository::LocalRepository(const QString &directory)
{
    m_directory = Utils::cleanPath(directory);
    m_settings = new QSettings(m_directory + StatusFileName, QSettings::IniFormat);
    load();
}

LocalRepository::~LocalRepository()
{
    delete m_settings;
}

void LocalRepository::load()
{
    m_localRevision = m_settings->value(Revision).toString();
    m_updatingToRevision  = m_settings->value(UpdatingTo).toString();
    m_fileList = m_settings->value(FileList).toStringList();
}

void LocalRepository::save()
{
    m_settings->setValue(Revision, m_localRevision);
    if(isConsistent())
        m_settings->remove(UpdatingTo);
    else
        m_settings->setValue(UpdatingTo, m_updatingToRevision);
    m_settings->setValue(FileList, m_fileList);
    m_settings->sync();
}
