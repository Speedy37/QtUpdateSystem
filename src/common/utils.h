#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include "../qtupdatesystem_global.h"

namespace Utils {
    QTUPDATESYSTEMSHARED_EXPORT QString cleanPath(const QString &pathName, bool separatorAtEnd = true);
    QTUPDATESYSTEMSHARED_EXPORT QString formatMs(qint64 millisec);
}
#endif // UTILS_H
