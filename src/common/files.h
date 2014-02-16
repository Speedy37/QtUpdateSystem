#ifndef FILES_H
#define FILES_H

#include "file.h"
#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QJsonDocument>

class File;

class Files
{
public:
    void loadFiles(const QJsonObject &object);
    QJsonObject toJsonObject() const;
private:
    QMap< QString, QVector<File> > m_files;
    void loadFiles1(const QJsonObject object);
};

#endif // FILES_H
