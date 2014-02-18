#include "utils.h"
#include <QDir>

namespace Utils {

#ifdef Q_OS_LINUX
    QString xdelta = QStringLiteral("xdelta3");
    QString lzma = QStringLiteral("lzma");
#elif defined(Q_OS_WIN)
    QString xdelta = QStringLiteral("xdelta3.exe");
    QString lzma = QStringLiteral("lzma.exe");
#endif

QString cleanPath(const QString &pathName)
{
    if(pathName.isEmpty())
        return pathName;

    QString cleaned = pathName;
    cleaned.replace('\\', '/');

    if(!cleaned.endsWith('/'))
        cleaned += '/';

    return cleaned;
}

QString xdeltaProgram()
{
    return xdelta;
}

void setXdeltaProgram(const QString &program)
{
    xdelta = program;
}

QString lzmaProgram()
{
    return lzma;
}

void setLzmaProgram(const QString &program)
{
    lzma = program;
}

}
