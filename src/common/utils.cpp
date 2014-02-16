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

    QString cleaned = QDir::fromNativeSeparators(pathName);

    if(!cleaned.endsWith('/'))
        cleaned += '/';

    return cleaned;
}

QString Utils::xdeltaProgram()
{
    return xdelta;
}

void Utils::setXdeltaProgram(const QString &program)
{
    xdelta = program;
}

QString Utils::lzmaProgram()
{
    return lzma;
}

void Utils::setLzmaProgram(const QString &program)
{
    lzma = program;
}

}
