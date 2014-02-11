#ifndef PACKAGEMETADATA_H
#define PACKAGEMETADATA_H

#include "package.h"
#include <QVector>

class PackageMetadata
{
public:
    PackageMetadata();
    ~PackageMetadata();

    QString dataUrl() const;
    QString metadataUrl() const;
    QString to() const;
    QString from() const;
    qint64 size() const;
    QVector<Operation *> operations() const;
    Operation *operation(int i) const;
    void setPackage(const Package & package);
    void fromJsonObject(const QJsonObject &object);
    void setup(const QString &updateDir, const QString &tmpUpdateDir);

private:
    void loadMetadata1(const QJsonObject &object);
    void clearOperations();

    qint64 m_size;
    QString m_to, m_from;
    Package m_package;
    QVector<Operation *> m_operations;

    Q_DISABLE_COPY(PackageMetadata)
};

inline QString PackageMetadata::dataUrl() const
{
    return m_package.url();
}

inline QString PackageMetadata::metadataUrl() const
{
    return m_package.metadataUrl();
}

inline QString PackageMetadata::to() const
{
    return m_to;
}

inline QString PackageMetadata::from() const
{
    return m_from;
}

inline qint64 PackageMetadata::size() const
{
    return m_size;
}

inline QVector<Operation *> PackageMetadata::operations() const
{
    return m_operations;
}

inline Operation* PackageMetadata::operation(int i) const
{
    return m_operations.value(i);
}

inline void PackageMetadata::setPackage(const Package &package)
{
    m_package = package;
}

#endif // PACKAGEMETADATA_H
