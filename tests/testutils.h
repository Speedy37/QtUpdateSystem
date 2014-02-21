#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <QString>

#define FORCED_CLEANUP {\
    bool oldValue = TestUtils::cleanup;\
    TestUtils::cleanup = true;\
    cleanupTestCase();\
    TestUtils::cleanup = oldValue;\
}\

class TestUtils
{
public:
    static bool cleanup;
    static void assertFileEquals(const QString &file1, const QString &file2);
    static bool compareJson(const QString &file1, const QString &file2, bool expectParseError = false);
};

#endif // TESTUTILS_H
