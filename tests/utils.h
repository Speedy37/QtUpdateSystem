#ifndef UTILS_H
#define UTILS_H

#include <QString>

class Utils
{
public:
    static bool compareFile(const QString &file1, const QString &file2);
    static bool compareJson(const QString &file1, const QString &file2, bool expectParseError = false);
};

#endif // UTILS_H
