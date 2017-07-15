#ifndef LOCALREPOSITORY_H
#define LOCALREPOSITORY_H

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include "../common/utils.h"

class QSettings;

class QTUPDATESYSTEMSHARED_EXPORT LocalRepository
{
public:
    LocalRepository();
    LocalRepository(const QString &directory);
    bool load();
    void save();

    inline QString directory() const;
    void setDirectory(const QString &directory);

    inline QString revision() const;
    inline void setRevision(const QString &revision);

    bool isManaged(const QFileInfo &file) const;
    inline bool isConsistent() const;
    inline bool updateInProgress() const;
    inline void setUpdateInProgress(bool updateInProgress);

    inline QStringList fileList() const;
    inline void setFileList(const QStringList &files);

    inline QStringList dirList() const;
    inline void setDirList(const QStringList &dirList);

private:
	QJsonObject toJsonObject() const;
	void fromJsonObject(const QJsonObject & object);
    QStringList m_fileList, m_dirList;
    QString m_directory, m_localRevision;
    bool m_updateInProgress;
};

inline QString LocalRepository::directory() const
{
    return m_directory;
}

inline QString LocalRepository::revision() const
{
    return m_localRevision;
}

inline void LocalRepository::setRevision(const QString &revision)
{
    m_localRevision = revision;
}

inline bool LocalRepository::isConsistent() const
{
    return !m_updateInProgress;
}

inline bool LocalRepository::updateInProgress() const
{
    return m_updateInProgress;
}

inline void LocalRepository::setUpdateInProgress(bool updateInProgress)
{
    m_updateInProgress = updateInProgress;
}

inline QStringList LocalRepository::fileList() const
{
    return m_fileList;
}

inline void LocalRepository::setFileList(const QStringList &files)
{
    m_fileList = files;
}

inline QStringList LocalRepository::dirList() const
{
    return m_dirList;
}

inline void LocalRepository::setDirList(const QStringList &dirList)
{
    m_dirList = dirList;
}

#endif // LOCALREPOSITORY_H
