#ifndef TST_PACKAGER_H
#define TST_PACKAGER_H

#include <QtTest>

class TestPackager : public QObject
{
    Q_OBJECT

public:
    TestPackager() {}

private Q_SLOTS:
    void initTestCase();
    void createPatch();
    void createComplete();
    void cleanupTestCase();
};

#endif // TST_PACKAGER_H
