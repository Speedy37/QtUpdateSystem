#ifndef FILE_H
#define FILE_H

#include <QJsonObject>
#include <QString>

class File
{
public:
    QString revision;
    QString sha1;
    qint64 size;
    void fromJsonObject(const QJsonObject &packageObject);
    QJsonObject toJsonObject() const;
};

#endif // FILE_H
