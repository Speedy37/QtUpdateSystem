#ifndef WARNING_H
#define WARNING_H

#include "../qtupdatesystem_global.h"
#include "../operations/operation.h"
#include <QString>
#include <QMetaType>
#include <QSharedPointer>
#include <QDebug>

#if defined(ERROR_CONTEXT)
#  define EMIT_WARNING(type, message, operation) emit warning(Warning(Warning::##type, message, operation, __FILE__, __LINE__, Q_FUNC_INFO))
#else
#  define EMIT_WARNING(type, message, operation) emit warning(Warning(Warning::##type, message, operation))
#endif

class QTUPDATESYSTEMSHARED_EXPORT Warning
{
public:
    struct OperationInfo
    {
        OperationInfo(const QString &path = QString()) :
            path(path), status(Operation::Unknown) {}
        OperationInfo(QSharedPointer<Operation> operation) :
            path(operation->path()), status(operation->status()) {}
        QString path;
        Operation::Status status;
        QString statusString() const
        {
            switch (status) {
            case Operation::DownloadRequired:
                return QStringLiteral("DownloadRequired");
            case Operation::ApplyRequired:
                return QStringLiteral("ApplyRequired");
            case Operation::Valid:
                return QStringLiteral("Valid");
            case Operation::LocalFileInvalid:
                return QStringLiteral("LocalFileInvalid");
            case Operation::ApplyFailed:
                return QStringLiteral("ApplyFailed");
            default:
                return QStringLiteral("Unknown");
            }
        }
    };

    enum Type {
        Unknown,
        OperationPreparation,
        OperationDownload,
        OperationApply,
        RemoveFiles,
        RemoveDirs,
        Copy,
        CopyRemove,
        CopyMkPath
    };

#if defined(ERROR_CONTEXT)
    Warning() :
        m_type(Unknown), m_file(nullptr), m_line(-1), m_function(nullptr) {}
    Warning(Type type, const QString &message, const OperationInfo &operation) :
        m_type(type), m_message(message), m_operation(operation), m_file(nullptr), m_line(-1), m_function(nullptr) {}
    Warning(Type type, const QString &message, const OperationInfo &operation, const char * file, int line, const char *function) :
        m_type(type), m_message(message), m_operation(operation), m_file(file), m_line(line), m_function(function) {}
#else
    Warning() :
        m_type(Unknown) {}
    Warning(Type type, const QString &message, const OperationInfo &operation) :
        m_type(type), m_message(message), m_operation(operation) {}
#endif

    Type type() const;
    QString message() const;
    QString typeString() const;
    OperationInfo operation() const;
    bool operator==(const Warning &w) const;

private:
    Type m_type;
    QString m_message;
    OperationInfo m_operation;
#if defined(ERROR_CONTEXT)
    const char * m_file;
    int m_line;
    const char *m_function;
#endif
};

inline QDebug operator<<(QDebug dbg, const Warning::OperationInfo &o)
{
    dbg << "(" << o.path << ", " << o.statusString() << ")";

    return dbg.space();
}

inline Warning::Type Warning::type() const
{
    return m_type;
}

inline QString Warning::message() const
{
    return m_message;
}

inline Warning::OperationInfo Warning::operation() const
{
    return m_operation;
}

inline bool Warning::operator==(const Warning &w) const
{
    return type() == w.type();
}

Q_DECLARE_METATYPE(Warning)

#endif // WARNING_H
