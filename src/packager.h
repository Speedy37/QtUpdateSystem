#ifndef PACKAGER_H
#define PACKAGER_H

#include "qtupdatesystem_global.h"
#include "common/packagemetadata.h"
#include "common/utils.h"
#include "packager/packagertask.h"
#include <vector>
#include <QString>
#include <QFileInfoList>
#include <QDir>

class QThreadPool;
class QTemporaryDir;

class QTUPDATESYSTEMSHARED_EXPORT Packager : public QObject
{
    Q_OBJECT
private:
    Q_DISABLE_COPY(Packager)

public:
    Packager(QObject * parent = 0);
    ~Packager();

    PackageMetadata generateForRepository(const QString &repositoryPath);
    PackageMetadata generate();

    QString newDirectoryPath() const;
    QString newRevisionName() const;
    void setNewSource(const QString &newDirectoryPath, const QString &newRevisionName);

    QString oldDirectoryPath() const;
    QString oldRevisionName() const;
    void setOldSource(const QString &oldDirectoryPath, const QString &oldRevisionName);

    QString deltaFilename() const;
    void setDeltaFilename(const QString &deltaFilename);

    QString deltaMetadataFilename() const;
    void setDeltaMetadataFilename(const QString &deltaMetadataFilename);

    QString tmpDirectoryPath();
    void setTmpDirectoryPath(const QString &tmpDirectoryPath);

    QString errorString() const;

private:
    // Configurable attributes
    QString m_oldDirectoryPath, m_oldRevisionName;
    QString m_newDirectoryPath, m_newRevisionName;
    QString m_deltaFilename, m_deltaMetadataFilename;
    QString m_tmpDirectoryPath;

    // Internal
    QTemporaryDir *m_temporaryDir;

private:
    QFileInfoList dirList(const QDir &dir);
    std::vector<PackagerTask> m_tasks;
    void compareDirectories(QString path, const QFileInfoList &newFiles, const QFileInfoList &oldFiles);
    void addTask(PackagerTask::Type operationType, QString path, QString newFilename = QString(), QString oldFilename = QString());
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
    m_newDirectoryPath = Utils::cleanPath(newDirectoryPath);
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
    m_oldDirectoryPath = Utils::cleanPath(oldDirectoryPath);
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

inline QString Packager::deltaMetadataFilename() const
{
    if(m_deltaMetadataFilename.isNull())
        return deltaFilename() + PackageMetadata::FileExtension;
    return m_deltaMetadataFilename;
}

inline void Packager::setDeltaMetadataFilename(const QString &deltaMetaDataFilename)
{
    m_deltaMetadataFilename = deltaMetaDataFilename;
}

inline QString Packager::tmpDirectoryPath()
{
    return m_tmpDirectoryPath;
}

inline void Packager::setTmpDirectoryPath(const QString &tmpDirectoryPath)
{
    m_tmpDirectoryPath = Utils::cleanPath(tmpDirectoryPath);
}

#endif // PACKAGER_H
