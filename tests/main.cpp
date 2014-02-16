#include <QTest>
#include "tst_packagemanager.h"
#include "tst_packager.h"
#include "tst_updater.h"
#include <qtlog.h>

int main(int argc, char *argv[])
{
    QtLog::setLevel(QtLog::Error);

    {
        TestPackageManager t;
        QTest::qExec(&t, argc, argv);
    }

    {
        TestPackager t;
        QTest::qExec(&t, argc, argv);
    }

    {
        TestUpdater t;
        QTest::qExec(&t, argc, argv);
    }

    return 0;
}
