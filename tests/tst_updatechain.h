#ifndef TST_UPDATECHAIN_H
#define TST_UPDATECHAIN_H

#include <QtTest>

class TestUpdateChain : public QObject
{
    Q_OBJECT
public:
    TestUpdateChain() {}

private Q_SLOTS:
    void initTestCase();
    void testNewRepository();
    void cleanupTestCase();
};

#endif // TST_UPDATECHAIN_H
