#ifndef LOCALREPOSITORY_H
#define LOCALREPOSITORY_H

#include <QString>
#include <QStringList>

class QSettings;

class LocalRepository
{
public:
    LocalRepository(const QString &directory);
    ~LocalRepository();
    void load();
    void save();

    QString directory() const;

    QString revision() const;
    void setRevision(const QString &revision);

    bool isConsistent() const;
    QString updatingTo() const;
    void setUpdatingTo(const QString &revision);

    QStringList fileList() const;
    void setFileList(const QStringList &files);
private:
    static const QString Revision , UpdatingTo, FileList, StatusFileName;
    QSettings *m_settings;
    QStringList m_fileList;
    QString m_directory, m_localRevision, m_updatingToRevision;

    Q_DISABLE_COPY(LocalRepository)
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
    return m_updatingToRevision.isEmpty();
}

inline QString LocalRepository::updatingTo() const
{
    return m_updatingToRevision;
}

inline void LocalRepository::setUpdatingTo(const QString &revision)
{
    m_updatingToRevision = revision;
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
