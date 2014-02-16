#include "packagemetadata.h"

#include "jsonutil.h"
#include "../operations/addoperation.h"
#include "../operations/patchoperation.h"
#include "../operations/removedirectoryoperation.h"
#include "../operations/removeoperation.h"

#include <qtlog.h>

const QString PackageMetadata::FileExtension = ".metadata";

PackageMetadata::PackageMetadata()
{

}

PackageMetadata::~PackageMetadata()
{

}

void PackageMetadata::setup(const QString &updateDir, const QString &tmpUpdateDir)
{
    for(int i = 0; i < m_operations.size(); ++i)
    {
        m_operations[i]->setDataFilename(QString("%1Operation%2").arg(tmpUpdateDir).arg(i));
        m_operations[i]->setUpdateDirectory(updateDir);
    }
}

void PackageMetadata::fromJsonObject(const QJsonObject &object, bool loadOperations, bool loadPackage)
{
    QString version = JsonUtil::asString(object, QStringLiteral("version"));

    if(version == "1")
    {
        if(loadOperations)
            operationsFromJsonArrayV1(JsonUtil::asArray(object, QStringLiteral("operations")));
        if(loadPackage)
            m_package.fromJsonObjectV1(JsonUtil::asObject(object, QStringLiteral("package")));
    }
    else
    {
        throw(QObject::tr("Unsupported version %1").arg(version));
    }
}

QJsonObject PackageMetadata::toJsonObject() const
{
    QJsonObject object;

    object.insert(QStringLiteral("version"), QStringLiteral("1"));
    object.insert(QStringLiteral("operations"), operationsToJsonArrayV1());
    object.insert(QStringLiteral("package"), m_package.toJsonObjectV1());

    return object;
}

void PackageMetadata::operationsFromJsonArrayV1(const QJsonArray &operations)
{
    m_operations.clear();

    try
    {
        m_operations.resize(operations.size());
        for(int i = 0; i < operations.size(); ++i)
        {
            QJsonObject jsonOperation = JsonUtil::asObject(operations[i]);
            QString action = JsonUtil::asString(jsonOperation, QStringLiteral("action"));
            Operation * op;
            if(action == RemoveOperation::Action)
                op = new RemoveOperation();
            else if(action == RemoveDirectoryOperation::Action)
                op = new RemoveDirectoryOperation();
            else if(action == AddOperation::Action)
                op = new AddOperation();
            else if(action == PatchOperation::Action)
                op = new PatchOperation();
            else
                throw(QObject::tr("'action' \"%1\" is not supported").arg(action));

            m_operations[i] = QSharedPointer<Operation>(op);
            op->fromJsonObjectV1(jsonOperation);
        }
    }
    catch(...)
    {
        m_operations.clear();
        throw;
    }
}

QJsonArray PackageMetadata::operationsToJsonArrayV1() const
{
    QJsonArray array;

    for(int i = 0; i < m_operations.size(); ++i)
    {
        array.append(m_operations[i]->toJsonObjectV1());
    }

    return array;
}

