#ifndef LOCALREPOSITORY_H
#define LOCALREPOSITORY_H

#include <QString>
#include <QStringList>

class QSettings;

class LocalRepository
{
public:
    LocalRepository(const QString &directory);
    void load();
    void save();

    QString directory() const;

    QString revision() const;
    void setRevision(const QString &revision);

    bool isConsistent() const;
    bool updateInProgress() const;
    void setUpdateInProgress(bool updateInProgress);

    QStringList fileList() const;
    void setFileList(const QStringList &files);

private:
    static const QString Revision, UpdateInProgress, FileList, StatusFileName;
    QStringList m_fileList;
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

#endif // LOCALREPOSITORY_H
