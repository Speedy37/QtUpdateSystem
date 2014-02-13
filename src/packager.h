#ifndef PACKAGER_H
#define PACKAGER_H

#include "qtupdatesystem_global.h"
#include "common/packagemetadata.h"
#include "packager/packagertask.h"
#include <vector>
#include <QString>
#include <QFileInfoList>

class QThreadPool;
class QTemporaryDir;

class QTUPDATESYSTEMSHARED_EXPORT Packager : public QObject
{
    Q_OBJECT
private:
    Q_DISABLE_COPY(Packager)

public:
    Packager(QObject * parent = 0);
    void generate();

    QString newDirectoryPath() const;
    QString newRevisionName() const;
    void setNewSource(const QString &newDirectoryPath, const QString &newRevisionName);

    QString oldDirectoryPath() const;
    QString oldRevisionName() const;
    void setOldSource(const QString &oldDirectoryPath, const QString &oldRevisionName);

    QString deltaFilename() const;
    void setDeltaFilename(const QString &deltaFilename);

    QString deltaMetaDataFilename() const;
    void setDeltaMetaDataFilename(const QString &deltaMetaDataFilename);

    QString tmpDirectoryPath();
    void setTmpDirectoryPath(const QString &tmpDirectoryPath);

    QString errorString() const;

private:
    // Configurable attributes
    QString m_oldDirectoryPath, m_oldRevisionName;
    QString m_newDirectoryPath, m_newRevisionName;
    QString m_deltaFilename, m_deltaMetaDataFilename;
    QString m_tmpDirectoryPath;

private:
    QFileInfoList dirList(const QDir &dir);
    std::vector<PackagerTask> m_tasks;
    void compareDirectories(QString path, const QFileInfoList &newFiles, const QFileInfoList &oldFiles);
    void addRemoveDirTask(QString path, QFileInfo &pathInfo);
    static QString generate_hash(const QString &srcFilename);
};

inline QString Packager::newDirectoryPath() const
{
    return m_newDirectoryPath;
}

inline QString Packager::newRevisionName() const
{
    return m_newRevisionName;
}

inline void Packager::setNewSource(const QString &newDirectoryPath, const QString &newRevisionName)
{
    m_newDirectoryPath = newDirectoryPath;
    m_newRevisionName = newRevisionName;
}

inline QString Packager::oldDirectoryPath() const
{
    return m_oldDirectoryPath;
}

inline QString Packager::oldRevisionName() const
{
    return m_oldRevisionName;
}

inline void Packager::setOldSource(const QString &oldDirectoryPath, const QString &oldRevisionName)
{
    m_oldDirectoryPath = oldDirectoryPath;
    m_oldRevisionName = oldRevisionName;
}

inline QString Packager::deltaFilename() const
{
    return m_deltaFilename;
}

inline void Packager::setDeltaFilename(const QString &deltaFilename)
{
    m_deltaFilename = deltaFilename;
}

inline QString Packager::deltaMetaDataFilename() const
{
    if(m_deltaMetaDataFilename.isNull())
        return deltaFilename() + PackageMetadata::FileExtension;
    return m_deltaMetaDataFilename;
}

inline void Packager::setDeltaMetaDataFilename(const QString &deltaMetaDataFilename)
{
    m_deltaMetaDataFilename = deltaMetaDataFilename;
}

inline QString Packager::tmpDirectoryPath()
{
    return m_tmpDirectoryPath;
}

inline void Packager::setTmpDirectoryPath(const QString &tmpDirectoryPath)
{
    m_tmpDirectoryPath = tmpDirectoryPath;
}

#endif // PACKAGER_H
