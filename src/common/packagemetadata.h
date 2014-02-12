#ifndef PACKAGEMETADATA_H
#define PACKAGEMETADATA_H

#include "package.h"
#include <QVector>

class Operation;

class PackageMetadata
{
public:
    PackageMetadata();
    ~PackageMetadata();

    static const QString FileExtension;
    QString dataUrl() const;
    QString metadataUrl() const;
    QString to() const;
    QString from() const;
    qint64 size() const;
    QVector<Operation *> operations() const;
    Operation *operation(int i) const;
    Package package() const;
    void setPackage(const Package & package);
    void setup(const QString &updateDir, const QString &tmpUpdateDir);

    void fromJsonObject(const QJsonObject &object, bool loadOperations = true, bool loadPackage = true);
    QJsonObject toJsonObject() const;

    void operationsFromJsonArrayV1(const QJsonArray &operations);
    QJsonArray operationsToJsonArrayV1() const;

private:
    void clearOperations();
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
    return m_package.to;
}

inline QString PackageMetadata::from() const
{
    return m_package.from;
}

inline qint64 PackageMetadata::size() const
{
    return m_package.size;
}

inline QVector<Operation *> PackageMetadata::operations() const
{
    return m_operations;
}

inline Operation* PackageMetadata::operation(int i) const
{
    return m_operations.value(i);
}

inline Package PackageMetadata::package() const
{
    return m_package;
}

inline void PackageMetadata::setPackage(const Package &package)
{
    m_package = package;
}

#endif // PACKAGEMETADATA_H
