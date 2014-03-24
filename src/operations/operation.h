#ifndef UPDATER_OPERATION_H
#define UPDATER_OPERATION_H

#include <QString>
#include <QProcess>
#include <QJsonObject>
#include <QFile>
#include <functional>

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
    enum FileType {
        None,
        File,
        Folder
    };

    enum Status {
        Unknown,
        DownloadRequired,
        ApplyRequired,
        Valid,
        LocalFileInvalid,
        ApplyFailed
    };

    Operation();
    virtual ~Operation() {}

    qint64 offset() const;
    void setOffset(qint64 offset);
    qint64 size() const;

    QString dataFilename() const;
    QString dataDownloadFilename() const;
    void setDataFilename(const QString &dataFilename);

    QString localFilename() const;
    void setUpdateDirectory(const QString &updateDirectory);

    QString path() const;
    QString sha1() const;
    QString sha1(QFile *dataFile) const;

    QString errorString() const;
    Status status() const;

    // Update methods
    void setup(const QString &updateDir, const QString &tmpUpdateDir, int uniqueid);
    void checkLocalData(); // FileManager thread
    void apply(); // FileManager thread
    virtual void cleanup();  // DownloadManager thread

    virtual FileType fileType() const;
    virtual void fromJsonObjectV1(const QJsonObject &object);
    QJsonObject toJsonObjectV1();

    void setWarningListener(std::function<void(const QString &message)> listener);

private:
    Status m_status;
    QString m_localFilename, m_dataFilename;
    Q_DISABLE_COPY(Operation)

protected:
    static const QString Path;
    virtual Status localDataStatus() = 0; // FileManager thread
    virtual void applyData() = 0; // FileManager thread
    virtual QString type() const = 0;
    virtual void fillJsonObjectV1(QJsonObject & object);
    void throwWarning(const QString &warning);

    std::function<void(const QString &message)> m_warningListener;
    qint64 m_offset, m_size;
    QString m_path, m_sha1, m_errorString;
};

inline qint64 Operation::offset() const
{
    return m_offset;
}

inline void Operation::setOffset(qint64 offset)
{
    m_offset = offset;
}

inline qint64 Operation::size() const
{
    return m_size;
}

inline QString Operation::dataFilename() const
{
    return m_dataFilename;
}

inline QString Operation::dataDownloadFilename() const
{
    return m_dataFilename + ".part";
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

inline void Operation::setWarningListener(std::function<void (const QString &)> listener)
{
    m_warningListener = listener;
}

inline void Operation::throwWarning(const QString &warning)
{
    if(m_warningListener)
        m_warningListener(warning);
}

#endif // UPDATER_OPERATION_H
