#include "utils.h"
#include <QFile>
#include <QJsonDocument>

bool Utils::compareFile(const QString &file1, const QString &file2)
{
    QFile f1(file1);
    QFile f2(file2);

    if(!f1.exists() || !f2.exists())
        return false;

    if(f1.size() != f2.size())
        return false;

    if(!f1.open(QFile::ReadOnly))
        return false;
    if(!f2.open(QFile::ReadOnly))
        return false;

    char buffer1[8096];
    char buffer2[8096];
    qint64 read1, read2;
    read1 = f1.read(buffer1, sizeof(buffer1));
    read2 = f2.read(buffer2, sizeof(buffer2));
    while(read1 > 0 || read2 > 0)
    {
        if(read1 != read2)
            return false;
        if(memcmp(buffer1, buffer2, read1) != 0)
            return false;

        read1 = f1.read(buffer1, sizeof(buffer1));
        read2 = f2.read(buffer2, sizeof(buffer2));
    }

    return true;
}

bool Utils::compareJson(const QString &file1, const QString &file2, bool expectParseError)
{
    QFile f1(file1);
    QFile f2(file2);

    if(!f1.exists() || !f2.exists())
        return false;

    if(!f1.open(QFile::ReadOnly | QFile::Text))
        return false;
    if(!f2.open(QFile::ReadOnly | QFile::Text))
        return false;

    QJsonParseError jsonError1;
    QJsonDocument doc1 = QJsonDocument::fromJson(f1.readAll(), &jsonError1);
    QJsonParseError jsonError2;
    QJsonDocument doc2 = QJsonDocument::fromJson(f2.readAll(), &jsonError2);

    if(jsonError1.error == QJsonParseError::NoError && jsonError2.error == QJsonParseError::NoError)
        return !expectParseError && doc1 == doc2;

    return expectParseError &&
            jsonError1.error == jsonError2.error &&
            jsonError1.offset == jsonError2.offset;
}
