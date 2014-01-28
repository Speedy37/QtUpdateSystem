#ifndef UPDATER_ADDOPERATION_H
#define UPDATER_ADDOPERATION_H

#include "operation.h"
#include <QCryptographicHash>

class DownloadManager;
class QFile;

class AddOperation : public Operation
{
    Q_OBJECT
public:
    enum State {
        DownloadFailed = -1,
        Waiting = 0,
        Downloaded = 1,
        Applied = 2
    };

    AddOperation(DownloadManager * _update) : Operation(_update), m_dataChecked(false)
    {
        m_state = Waiting;
    }
    virtual void load1(const QJsonObject &object);
    virtual QString actionString() Q_DECL_OVERRIDE  { return QStringLiteral("ADD"); }
    virtual void run() Q_DECL_OVERRIDE;
    virtual void applyLocally(const QString &localFolder) Q_DECL_OVERRIDE;
    virtual qint64 offset() const Q_DECL_OVERRIDE { return dataOffset; }
    virtual qint64 size() const Q_DECL_OVERRIDE { return dataLength; }
    virtual bool isDataValid() Q_DECL_OVERRIDE;

    State state() const { return m_state; }
    void setState(State state) { m_state = state; }
protected:
    virtual void save1(QJsonObject & object);
    enum Compression
    {
        Uncompressed,
        Lzma,
        Xdelta,
        XdeltaLzma
    };
    Compression dataCompression;
    bool m_dataChecked;
    qint64 dataOffset, dataLength, finalSize;
    QCryptographicHash::Algorithm dataHashType, finalHashType;
    QString dataHash, finalHash;
    QString fileHash(QFile * file, qint64 length, QCryptographicHash::Algorithm algorithm);
    void loadInt64(const QJsonValue &value, qint64 *dest);
    QString getHashType(QCryptographicHash::Algorithm algo);
    void loadHashType(const QJsonValue &value, QCryptographicHash::Algorithm *dest);
    QString getCompression(AddOperation::Compression algo);
    void loadCompression(const QJsonValue &value, Compression *dest);
private:
    State m_state;
};

#endif // UPDATER_ADDOPERATION_H
