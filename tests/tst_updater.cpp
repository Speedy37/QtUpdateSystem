#include "tst_updater.h"
#include "testutils.h"
#include <updater.h>

const QString dataCopy = dataDir + "/updater_copy";
const QString testOutput = testDir + "/tst_updater_output";
const QString testOutputCopy = testOutput + "/copy";

void TestUpdater::initTestCase()
{
    FORCED_CLEANUP
    QVERIFY(QDir().mkpath(testOutput));
    QVERIFY(QDir().mkpath(testOutputCopy));
    QVERIFY(QDir().mkpath(testOutput + "/tmp"));
    QVERIFY(QFile::copy(dataCopy + "/init_repo/status.ini", testOutputCopy + "/status.ini"));
    QVERIFY(QFile::copy(dataCopy + "/init_repo/add.txt", testOutputCopy + "/add.txt"));
}

void TestUpdater::cleanupTestCase()
{
    if(!TestUtils::cleanup)
        return;

    QDir(testOutput).removeRecursively();
}

void TestUpdater::updaterCopy()
{
    Updater u;
    u.setLocalRepository(dataCopy + "/local_repo");
    u.copy(testOutputCopy);
    QSignalSpy spy(&u, SIGNAL(copyFinished(bool)));
    QVERIFY(spy.wait());

    try {
        (TestUtils::assertFileEquals(dataCopy + "/local_repo/patch_same.txt", testOutputCopy + "/patch_same.txt"));
        (TestUtils::assertFileEquals(dataCopy + "/local_repo/path_diff.txt", testOutputCopy + "/path_diff.txt"));
        (TestUtils::assertFileEquals(dataCopy + "/local_repo/path_diff2.txt", testOutputCopy + "/path_diff2.txt"));
        (TestUtils::assertFileEquals(dataCopy + "/local_repo/rmfile.txt", testOutputCopy + "/rmfile.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testOutputCopy + "/add.txt"));
}
