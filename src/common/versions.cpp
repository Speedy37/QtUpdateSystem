#include "versions.h"
#include "jsonutil.h"

#include <qtlog.h>

void Versions::addVersion(const QString &revision, const QString &description)
{
    Version version;
    version.revision = revision;
    version.description = description;
    m_versions.append(version);
}

void Versions::fromJsonObject(const QJsonObject & object)
{
    const QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        loadPackages1(object);
    else
        throw(QObject::tr("Unsupported version %1").arg(version));
}

QJsonObject Versions::toJsonObject() const
{
    QJsonObject object;
    QJsonArray versions;

    for(int i = 0; i < m_versions.size(); ++i)
    {
        versions.append(m_versions[i].toJsonObject());
    }

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("versions"), versions);

    return object;
}

void Versions::fromJsonObject1(const QJsonObject object)
{
    const QJsonArray versions = JsonUtil::asArray(object, QStringLiteral("versions"));

    m_versions.resize(versions.size());

    for(int i = 0; i < versions.size(); ++i)
    {
        Version & version = m_versions[i];
        version.fromJsonObject(JsonUtil::asObject(versions[i]));
    }
}
