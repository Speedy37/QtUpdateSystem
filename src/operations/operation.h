#ifndef UPDATER_OPERATION_H
#define UPDATER_OPERATION_H

#include <QString>
#include <QProcess>
#include <QJsonObject>
#include <QFile>

class QFile;
class DownloadManager;

inline bool waitForFinished(QProcess & process)
{
    return (process.state() == QProcess::NotRunning || process.waitForFinished()) &&
            process.exitCode() == 0;
}

class Operation
{
public:
    enum Status {
        Unknown,
        DownloadRequired,
        ApplyRequired,
        Valid,
        LocalFileInvalid,
        ApplyFailed
    };

    Operation();
    qint64 offset() const;
    qint64 size() const;

    QString dataFilename() const;
    void setDataFilename(const QString &dataFilename);

    QString localFilename() const;
    void setUpdateDirectory(const QString &updateDirectory);

    QString path() const;
    QString sha1() const;
    QString sha1(QFile *dataFile) const;


    QString errorString() const;
    Status status() const;
    void checkLocalData(); // FileManager thread
    void apply(); // FileManager thread

    virtual void create(const QString &path, const QString &oldFilename, const QString &newFilename, const QString &tmpDirectory) = 0;
    virtual void fromJsonObjectV1(const QJsonObject &object);
    QJsonObject toJsonObjectV1();

private:
    QString m_localFilename, m_dataFilename;

protected:
    virtual Status localDataStatus() = 0; // FileManager thread
    virtual void applyData() = 0; // FileManager thread
    virtual QString action() = 0;
    virtual void toJsonObjectV1(QJsonObject & object);

    qint64 m_offset, m_size;
    QString m_path, m_sha1, m_errorString;
    Status m_status;
};

inline qint64 Operation::offset() const
{
    return m_offset;
}

inline qint64 Operation::size() const
{
    return m_size;
}

inline QString Operation::dataFilename() const
{
    return m_dataFilename;
}

inline void Operation::setDataFilename(const QString &dataFilename)
{
    m_dataFilename = dataFilename;
}

inline QString Operation::localFilename() const
{
    return m_localFilename;
}

inline void Operation::setUpdateDirectory(const QString &updateDirectory)
{
    m_localFilename = updateDirectory + m_path;
}

inline QString Operation::path() const
{
    return m_path;
}

inline QString Operation::sha1() const
{
    return m_sha1;
}

inline QString Operation::errorString() const
{
    return m_errorString;
}

inline Operation::Status Operation::status() const
{
    return m_status;
}

#endif // UPDATER_OPERATION_H
