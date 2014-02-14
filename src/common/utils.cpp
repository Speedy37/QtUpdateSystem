#include "utils.h"
#include <QDir>

namespace Utils {

#ifdef Q_OS_LINUX
    QString xdeltaProgram = QStringLiteral("xdelta3");
    QString lzmaProgram = QStringLiteral("lzma");
#elif defined(Q_OS_WIN)
    QString xdeltaProgram = QStringLiteral("xdelta3.exe");
    QString lzmaProgram = QStringLiteral("lzma.exe");
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

}
