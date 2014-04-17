#ifndef TST_UPDATER_H
#define TST_UPDATER_H

#include <QtTest>

class TestUpdater : public QObject
{
    Q_OBJECT
public:
    TestUpdater() {}

private Q_SLOTS:
    void initTestCase();
    void updaterCopy();
    void updaterIsManaged();
    void updaterRemoveOtherFiles();
    void cleanupTestCase();
};

#endif // TST_UPDATER_H
