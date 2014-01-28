#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QObject>

class PackageManager : public QObject
{
    Q_OBJECT
public:
    explicit PackageManager(QObject *parent = 0);

    QString directory() const;
    void setDirectory(const QString &directory);
    void setCurrentRevision(const QString &revision);
    void addRevision(const QString &revision);
    void addFile(const QString &path, const QString &revision, const QString &sha1, qint64 size);
    void addPackage(const QString &packageFullName);
    void addPackage(const QString &to, const QString &from, qint64 size);
private:
    QString m_directory;
};

inline QString PackageManager::directory() const
{
    return m_directory;
}

inline void PackageManager::setDirectory(const QString &directory)
{
    m_directory = directory;
}


#endif // PACKAGEMANAGER_H
