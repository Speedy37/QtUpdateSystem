#include "warning.h"

int WarningMetatypeId = qMetaTypeId<Warning>();

QString Warning::typeString() const
{
    switch (type()) {
    case OperationPreparation:
        return QStringLiteral("OperationPreparation");
    case OperationDownload:
        return QStringLiteral("OperationDownload");
    case OperationApply:
        return QStringLiteral("OperationApply");
    case RemoveFiles:
        return QStringLiteral("RemoveFiles");
    default:
        return QStringLiteral("Unknown");
    }
}
