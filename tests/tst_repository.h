#ifndef TST_PACKAGEMANAGER_H
#define TST_PACKAGEMANAGER_H

#include <QtTest>

class TestRepository : public QObject
{
    Q_OBJECT

public:
    TestRepository() {}

private Q_SLOTS:
    void initTestCase();
    void newRepository();
    void addPackage();
    void fixRepository();
    void cleanupTestCase();
};

#endif // TST_PACKAGEMANAGER_H
