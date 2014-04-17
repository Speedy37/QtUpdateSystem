#include "tst_updater.h"
#include "testutils.h"
#include <updater.h>

const QString dataCopy = dataDir + "/updater_copy";
const QString dataRev1Local = dataDir + "/rev1_local";
const QString testOutput = testDir + "/tst_updater_output";
const QString testOutputCopy = testOutput + "/copy";
const QString testOutputIsManaged = testOutput + "/isManaged";

void TestUpdater::initTestCase()
{
    FORCED_CLEANUP
    QVERIFY(QDir().mkpath(testOutput));
    QVERIFY(QDir().mkpath(testOutputCopy));
    QVERIFY(QDir().mkpath(testOutputIsManaged));
    QVERIFY(QDir().mkpath(testOutput + "/tmp"));
    QVERIFY(QFile::copy(dataCopy + "/init_repo/status.ini", testOutputCopy + "/status.ini"));
    QVERIFY(QFile::copy(dataRev1Local + "/status.ini", testOutputIsManaged + "/status.ini"));
    QVERIFY(QFile::copy(dataRev1Local + "/status.ini", testOutputIsManaged + "/unmanaged.ini"));
    QVERIFY(QFile::copy(dataRev1Local + "/path_diff.txt", testOutputIsManaged + "/path_diff.txt"));
    QVERIFY(QDir().mkpath(testOutputIsManaged + "/dirs/empty_dir2"));
    QVERIFY(QDir().mkpath(testOutputIsManaged + "/dirs/empty_dir1"));
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

void TestUpdater::updaterIsManaged()
{
    Updater u;
    u.setLocalRepository(testOutputIsManaged);
    QVERIFY(u.isManaged(testOutputIsManaged + "/status.ini"));
    QVERIFY(u.isManaged(testOutputIsManaged + "/path_diff.txt"));
    QVERIFY(u.isManaged(testOutputIsManaged + "/dirs/empty_dir2/"));
    QVERIFY(u.isManaged(testOutputIsManaged + "/dirs/"));
    QVERIFY(!u.isManaged(testOutputIsManaged + "/dirs/empty_dir1/"));
    QVERIFY(!u.isManaged(testOutputIsManaged + "/unmanaged.ini"));
}

void TestUpdater::updaterRemoveOtherFiles()
{
    Updater u;
    u.setLocalRepository(testOutputIsManaged);
    QVERIFY(QFile::exists(testOutputIsManaged + "/status.ini"));
    QVERIFY(QFile::exists(testOutputIsManaged + "/unmanaged.ini"));
    QVERIFY(QDir(testOutputIsManaged + "/dirs/empty_dir1").exists());
    u.removeOtherFiles();
    QVERIFY(QFile::exists(testOutputIsManaged + "/status.ini"));
    QVERIFY(!QFile::exists(testOutputIsManaged + "/unmanaged.ini"));
    QVERIFY(!QDir(testOutputIsManaged + "/dirs/empty_dir1").exists());
}
