#ifndef UTILS_H
#define UTILS_H

#include <QString>

namespace Utils {
    QString cleanPath(const QString &pathName);

    QString xdeltaProgram();
    void setXdeltaProgram(const QString &program);

    QString lzmaProgram();
    void setLzmaProgram(const QString &program);
}
#endif // UTILS_H
