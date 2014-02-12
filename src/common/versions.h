#ifndef VERSIONS_H
#define VERSIONS_H

#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "version.h"

class Versions
{
public:
    void addVersion(const QString& revision, const QString &description = QString());
    QVector<Version> versions() const;

    void fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;

    void fromJsonArrayV1(const QJsonArray &versions);
    QJsonArray toJsonArrayV1() const;

private:
    QVector<Version> m_versions;
};

inline QVector<Version> Versions::versions() const
{
    return m_versions;
}

#endif // VERSIONS_H
