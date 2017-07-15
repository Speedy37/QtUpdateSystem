#include "utils.h"
#include <QDir>

namespace Utils {

#ifdef Q_OS_UNIX
    QString xdelta = QStringLiteral("xdelta3");
    QString lzma = QStringLiteral("lzma");
#elif defined(Q_OS_WIN)
    QString xdelta = QStringLiteral("xdelta3.exe");
    QString lzma = QStringLiteral("lzma.exe");
#endif

QString cleanPath(const QString &pathName, bool separatorAtEnd)
{
    if(pathName.isEmpty())
        return pathName;

    QString cleaned = pathName;
    cleaned.replace('\\', '/');

    if(separatorAtEnd)
    {
        if(!cleaned.endsWith('/'))
            cleaned += '/';
    }
    else
    {
        if(cleaned.endsWith('/'))
            cleaned.chop(1);
    }

    return cleaned;
}

QString formatMs(qint64 millisec)
{
    if(millisec < 1000)
        return QStringLiteral("%1 ms").arg(millisec);

    double time = millisec/1000.0;
    if(time < 60.0)
        return QStringLiteral("%1 seconds").arg(time, 0, 'g', 2);

    time /= 60.0;
    if(time < 60.0)
        return QStringLiteral("%1 minutes").arg(time, 0, 'g', 2);

    time /= 60.0;
    if(time < 24.0)
        return QStringLiteral("%1 hours").arg(time, 0, 'g', 2);

    time /= 24.0;
    return QStringLiteral("%1 days").arg(time, 0, 'g', 2);
}

}
