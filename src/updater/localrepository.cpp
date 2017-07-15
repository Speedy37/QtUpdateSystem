#include "localrepository.h"
#include "../common/utils.h"
#include "../common/jsonutil.h"
#include "../exceptions.h"

#include <QDir>
#include <QSettings>

static const QString Revision = QStringLiteral("Revision");
static const QString UpdateInProgress = QStringLiteral("UpdateInProgress");
static const QString FileList = QStringLiteral("FileList");
static const QString DirList = QStringLiteral("DirList");
static const QString StatusFileName = QStringLiteral("status.json");

LocalRepository::LocalRepository()
{

}

LocalRepository::LocalRepository(const QString &directory)
{
    setDirectory(directory);
}

void LocalRepository::fromJsonObject(const QJsonObject & object)
{
	const QString version = JsonUtil::asString(object, QStringLiteral("version"));
	if (version != "1")
		THROW(UnsupportedVersion, version);
	m_localRevision = object.value(Revision).toString();
	m_updateInProgress = object.value(UpdateInProgress).toBool();
	m_fileList = object.value(FileList).toVariant().toStringList();
	m_dirList = object.value(DirList).toVariant().toStringList();
}

QJsonObject LocalRepository::toJsonObject() const
{
	QJsonObject object;

	object.insert(QStringLiteral("version"), QStringLiteral("1"));
	object.insert(Revision, m_localRevision);
	object.insert(UpdateInProgress, m_updateInProgress);
	object.insert(FileList, QJsonValue::fromVariant(m_fileList));
	object.insert(DirList, QJsonValue::fromVariant(m_dirList));

	return object;
}

bool LocalRepository::load()
{
	try
	{
		fromJsonObject(JsonUtil::fromJsonFile(m_directory + StatusFileName));
		return true;
	}
	catch (...) {
		return false;
	}
}

void LocalRepository::save()
{
	JsonUtil::toJsonFile(m_directory + StatusFileName, toJsonObject());
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
