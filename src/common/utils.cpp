#include "utils.h"
#include <QDir>

namespace Utils {

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
