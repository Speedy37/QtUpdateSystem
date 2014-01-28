#ifndef UPDATER_OPERATION_H
#define UPDATER_OPERATION_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>

class DownloadManager;

inline bool waitForFinished(QProcess & process)
{
    return (process.state() == QProcess::NotRunning || process.waitForFinished()) &&
            process.exitCode() == 0;
}

class Operation : public QObject
{
    Q_OBJECT
public:
    Operation(DownloadManager *_update)
    {
        updateDir = _update->updateDirectory();
    }
    virtual ~Operation() {}
    virtual void run() = 0;
    virtual void applyLocally(const QString &localFolder) = 0;
    virtual QString actionString() = 0;
    virtual void load1(const QJsonObject &object);
    QJsonObject save1();
    virtual qint64 offset() const { return -1; }
    virtual qint64 size() const { return -1; }
    virtual bool isDataValid() { return true; }


    int position() const { return m_position; }
    void setPosition(int position) { m_position = position; }

    QString name() const { return path; }
    QString filename() const { return m_filename; }
    void setFilename(const QString & _filename) { m_filename = _filename; }
    void setHash(const QString & _hash) { hash = _hash; }

    QString error() const { return m_error; }
    void setError(const QString & error) { m_error = error; }

protected:
    virtual void save1(QJsonObject & object);
    int m_position;
    QString path, updateDir, m_error;
    QString localpath(){ return updateDir+path; }
    QString m_filename, hash;
};


#endif // UPDATER_OPERATION_H
