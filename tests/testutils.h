#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <QString>

class TestUtils
{
public:
    static bool compareFile(const QString &file1, const QString &file2);
    static bool compareJson(const QString &file1, const QString &file2, bool expectParseError = false);
};

#endif // TESTUTILS_H
