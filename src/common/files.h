#ifndef FILES_H
#define FILES_H

#include <QString>
#include <QMap>
#include <QList>

class File;

class Files
{
public:
    QMap<QString, QList<File*>> files;
};

#endif // FILES_H
