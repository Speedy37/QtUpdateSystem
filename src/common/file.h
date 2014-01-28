#ifndef FILE_H
#define FILE_H

#include <QString>

class File
{
public:
    QString revision;
    QString sha1;
    qint64 size;
};

#endif // FILE_H
