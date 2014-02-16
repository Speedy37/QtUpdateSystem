#ifndef TST_UPDATER_H
#define TST_UPDATER_H

#include <QtTest>

class TestUpdater : public QObject
{
    Q_OBJECT
public:
    TestUpdater() {}

private Q_SLOTS:
    void checkForUpdates();

};

#endif // TST_UPDATER_H
