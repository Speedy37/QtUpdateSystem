#ifndef UPDATER_PACKAGE_H
#define UPDATER_PACKAGE_H

#include <QString>

class Package
{
public:
    QString to;
    QString from;
    qint64 size;
};

#endif // UPDATER_PACKAGE_H
