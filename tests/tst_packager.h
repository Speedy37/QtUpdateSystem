#ifndef TST_PACKAGER_H
#define TST_PACKAGER_H

#include <QtTest>

class TestPackager : public QObject
{
    Q_OBJECT

public:
    TestPackager();

private Q_SLOTS:
    void packageTest1NewOld();
    void packageTest1New();
    void packageTest1Old();
};

#endif // TST_PACKAGER_H
