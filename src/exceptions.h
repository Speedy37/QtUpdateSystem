#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "qtupdatesystem_global.h"
#include <exception>
#include <QDebug>
#include <QObject>

#if defined(ERROR_CONTEXT)
#  define THROW(type, ...) throw type(__VA_ARGS__, __FILE__, __LINE__, Q_FUNC_INFO)
#  define THROW_CONTEXT_PARAMS , const char * file, int line, const char *function
#  define THROW_CONTEXT_INIT , m_file(file), m_line(line), m_function(function)
#  define THROW_CONTEXT_PARENT1 file, line, function
#  define THROW_CONTEXT_PARENT2 , file, line, function
#else
#  define THROW(type, ...) throw type(__VA_ARGS__)
#  define THROW_CONTEXT_PARAMS
#  define THROW_CONTEXT_INIT
#  define THROW_CONTEXT_PARENT1
#  define THROW_CONTEXT_PARENT2
#endif

class QTUPDATESYSTEMSHARED_EXPORT Exception : public std::exception
{
public:
    Exception(const QString &message THROW_CONTEXT_PARAMS) :
        m_message(message) THROW_CONTEXT_INIT {}

    QString message() const;
    const char* what() const noexcept {
        if(m_8bitmessage.isNull())
            const_cast<QByteArray&>(m_8bitmessage) = m_message.toLocal8Bit();
        return m_8bitmessage.constData();
    }
#if defined(ERROR_CONTEXT)
    const char * file() { return m_file; }
    int line() { return m_line; }
    const char * function() { return m_function; }
#endif
private:
    QByteArray m_8bitmessage;
    QString m_message;
#if defined(ERROR_CONTEXT)
    const char * m_file;
    int m_line;
    const char *m_function;
#endif
};

inline QString Exception::message() const
{
    return m_message;
}

inline QDebug operator<<(QDebug dbg, const Exception &e)
{
    dbg << e.message();

    return dbg.space();
}

class JsonError : public Exception
{
public:
    JsonError(const QString &_reason THROW_CONTEXT_PARAMS)
        : Exception(_reason THROW_CONTEXT_PARENT2)
    {}
};

class InvalidPackageName : public Exception
{
public:
    InvalidPackageName(const QString &_currentName, const QString &_expectedName THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Invalid package name %1, %2 expected").arg(_currentName, _expectedName) THROW_CONTEXT_PARENT2)
    {}
};

class UnsupportedVersion : public Exception
{
public:
    UnsupportedVersion(const QString &_version THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Unsupported version %1").arg(_version) THROW_CONTEXT_PARENT2)
    {}
};

class UnableToOpenFile : public Exception
{
public:
    UnableToOpenFile(const QString &_filename THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Unable to open %1").arg(_filename) THROW_CONTEXT_PARENT2)
    {}
    UnableToOpenFile(const QString &_filename, const QString &_reason THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Unable to open %1, %2").arg(_filename, _reason) THROW_CONTEXT_PARENT2)
    {}
};

class FileMissing : public Exception
{
public:
    FileMissing(const QString &_filename THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Unable to find %1").arg(_filename) THROW_CONTEXT_PARENT2)
    {}
};

class InvalidPackage : public Exception
{
public:
    InvalidPackage(const QString &reason THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Invalid package, %1").arg(reason) THROW_CONTEXT_PARENT2)
    {}
};

class RequestFailed : public Exception
{
public:
    RequestFailed(const QString &reason THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Request failed, %1").arg(reason) THROW_CONTEXT_PARENT2)
    {}
};

class InitializationError : public Exception
{
public:
    InitializationError(const QString &reason THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Initialization failed, %1").arg(reason) THROW_CONTEXT_PARENT2)
    {}
};

class PackagingFailed : public Exception
{
public:
    PackagingFailed(const QString &reason THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Packaging failed, %1").arg(reason) THROW_CONTEXT_PARENT2)
    {}
};

class WriteFailure : public Exception
{
public:
    WriteFailure(const QString &reason THROW_CONTEXT_PARAMS)
        : Exception(QObject::tr("Write failed, %1").arg(reason) THROW_CONTEXT_PARENT2)
    {}
};

#undef THROW_CONTEXT_PARAMS
#undef THROW_CONTEXT_INIT
#undef THROW_CONTEXT_PARENT1
#undef THROW_CONTEXT_PARENT2

#endif // EXCEPTIONS_H
