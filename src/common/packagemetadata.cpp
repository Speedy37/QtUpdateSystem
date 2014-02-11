#include "packagemetadata.h"

#include "jsonutil.h"
#include "../operations/addoperation.h"
#include "../operations/patchoperation.h"
#include "../operations/removedirectoryoperation.h"
#include "../operations/removeoperation.h"

#include <qtlog.h>

PackageMetadata::PackageMetadata()
{

}

PackageMetadata::~PackageMetadata()
{
    clearOperations();
}

void PackageMetadata::fromJsonObject(const QJsonObject &object)
{
    QString version = JsonUtil::asString(object, QStringLiteral("version"));
    if(version == "1")
        loadMetadata1(object);
    else
        throw(QObject::tr("Unsupported version %1").arg(version));
}

void PackageMetadata::setup(const QString &updateDir, const QString &tmpUpdateDir)
{
    for(int i = 0; i < m_operations.size(); ++i)
    {
        Operation *operation = m_operations[i];
        operation->setDataFilename(QString("%1Operation%2").arg(tmpUpdateDir, i));
        operation->setUpdateDirectory(updateDir);
    }
}

void PackageMetadata::loadMetadata1(const QJsonObject & object)
{
    const QJsonArray operations = JsonUtil::asArray(object, QStringLiteral("operations"));

    m_size = JsonUtil::asInt64String(object, QStringLiteral("size"));
    if(m_size != m_package.size)
        LOG_WARN(QObject::tr("Metadata size %1 != %2 Packages size").arg(m_size).arg(m_package.size));

    m_to = JsonUtil::asString(object, QStringLiteral("to"));
    if(m_to != m_package.to)
        LOG_WARN(QObject::tr("Metadata to %1 != %2 Packages to").arg(m_to).arg(m_package.to));

    m_from = object.value(QStringLiteral("from")).toString();
    if(m_from != m_package.from)
        LOG_WARN(QObject::tr("Metadata from %1 != %2 Packages from").arg(m_from).arg(m_package.from));

    clearOperations();

    try
    {
        for(int i = 0; i < operations.size(); ++i)
        {
            QJsonObject jsonOperation = JsonUtil::asObject(operations[i]);
            QString action = JsonUtil::asString(jsonOperation, QStringLiteral("action"));
            Operation * op;
            if(action == QLatin1String("RM"))
                op = new RemoveOperation();
            else if(action == QLatin1String("RMDIR"))
                op = new RemoveDirectoryOperation();
            else if(action == QLatin1String("ADD"))
                op = new AddOperation();
            else if(action == QLatin1String("PATCH"))
                op = new PatchOperation();
            else
                throw(tr("'action' \"%1\" is not supported").arg(action));

            m_operations[i] = op;
            op->load1(jsonOperation);
        }
    }
    catch(...)
    {
        clearOperations();
        throw;
    }
}

void PackageMetadata::clearOperations()
{
    for(int i = 0; i < m_operations.size(); ++i)
    {
        if(m_operation[i] != NULL)
        {
            delete m_operations[i];
            m_operations[i] = NULL;
        }
    }
    m_operations.clear();
}

