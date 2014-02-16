#ifndef TST_UPDATER_H
#define TST_UPDATER_H

#include <QObject>

class TestUpdater : public QObject
{
    Q_OBJECT
public:
    TestUpdater();

public slots:
    void checkForUpdates();

};

#endif // TST_UPDATER_H
