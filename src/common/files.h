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
    void loadFiles(const QJsonDocument &json);
    QJsonObject toJsonObject() const;
private:
    QMap<QString, QVector<File>> m_files;
};

#endif // FILES_H
