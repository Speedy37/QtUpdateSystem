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
    void newRepository();
    void updateToV1();
    void createPatchV1toV2();
    void updateToV2();
    void fallbackToV1();
    void updateToV2WithFailures();
    void cleanupTestCase();
    void integrityCheck();
};

#endif // TST_UPDATECHAIN_H
