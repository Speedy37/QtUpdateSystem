#ifndef VERSIONS_H
#define VERSIONS_H

#include <QVector>
#include <QJsonObject>
#include "version.h"

class Versions
{
public:
    void addVersion(const QString& revision, const QString &description = QString());
    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;
    const QVector<Version> versions() const;

private:
    QVector<Version> m_versions;
    void fromJsonObject1(const QJsonObject object);
};

inline const QVector<Version> Versions::versions() const
{
    return m_versions;
}

#endif // VERSIONS_H
