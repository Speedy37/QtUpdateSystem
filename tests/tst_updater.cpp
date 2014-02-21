#include "tst_updater.h"
#include "testutils.h"
#include <updater.h>

const QString testDir = QString(SRCDIR);
const QString testCopy = testDir + "updater_copy";

void TestUpdater::initTestCase()
{
    FORCED_CLEANUP
    {
        QDir dir(testCopy);
        dir.mkpath("local_copy");
        QFile::copy(testCopy + "/init_repo/status.ini", testCopy + "/local_copy/status.ini");
        QFile::copy(testCopy + "/init_repo/add.txt", testCopy + "/local_copy/add.txt");
    }
}

void TestUpdater::cleanupTestCase()
{
    if(!TestUtils::cleanup)
        return;

    QDir(testCopy + "/local_copy").removeRecursively();
}

void TestUpdater::updaterCopy()
{
    QVERIFY(QFile::exists(testCopy + "/local_copy/add.txt"));
    Updater u(testCopy + "/local_repo");
    u.copy(testCopy + "/local_copy");
    QSignalSpy spy(&u, SIGNAL(copyFinished()));
    QVERIFY(spy.wait());

    try {
        (TestUtils::assertFileEquals(testCopy + "/local_repo/patch_same.txt", testCopy + "/local_copy/patch_same.txt"));
        (TestUtils::assertFileEquals(testCopy + "/local_repo/path_diff.txt", testCopy + "/local_copy/path_diff.txt"));
        (TestUtils::assertFileEquals(testCopy + "/local_repo/path_diff2.txt", testCopy + "/local_copy/path_diff2.txt"));
        (TestUtils::assertFileEquals(testCopy + "/local_repo/rmfile.txt", testCopy + "/local_copy/rmfile.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testCopy + "/local_copy/add.txt"));
}
