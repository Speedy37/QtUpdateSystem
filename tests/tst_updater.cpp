#include "tst_updater.h"
#include "testutils.h"
#include <updater.h>
#include <QFileInfo>

const QString dataCopy = dataDir + "/updater_copy";
const QString dataRev1Local = dataDir + "/rev1_local";
const QString dataRepoToV1 = dataDir + "/repo_v1_rev1";
const QString dataRepoToV2 = dataDir + "/repo_v1_rev2";
const QString testOutput = testDir + "/tst_updater_output";
const QString testOutputCopy = testOutput + "/copy";
const QString testOutputIsManaged = testOutput + "/isManaged";
const QString testOutputUpdate = testOutput + "/update";
const QString testOutputUpdateTmp = testOutput + "/update_tmp";

void TestUpdater::initTestCase()
{
    FORCED_CLEANUP
    QVERIFY(QDir().mkpath(testOutput));
    QVERIFY(QDir().mkpath(testOutputCopy));
    QVERIFY(QDir().mkpath(testOutputIsManaged));
    QVERIFY(QDir().mkpath(testOutputUpdate));
    QVERIFY(QDir().mkpath(testOutputUpdateTmp));
    QVERIFY(QFile::copy(dataCopy + "/init_repo/status.json", testOutputCopy + "/status.json"));
    QVERIFY(QFile::copy(dataRev1Local + "/status.json", testOutputIsManaged + "/status.json"));
    QVERIFY(QFile::copy(dataRev1Local + "/status.json", testOutputIsManaged + "/unmanaged.json"));
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
    QVERIFY(u.isManaged(testOutputIsManaged + "/status.json"));
    QVERIFY(u.isManaged(testOutputIsManaged + "/path_diff.txt"));
    QVERIFY(u.isManaged(testOutputIsManaged + "/dirs/empty_dir2/"));
    QVERIFY(u.isManaged(testOutputIsManaged + "/dirs/"));
    QVERIFY(!u.isManaged(testOutputIsManaged + "/dirs/empty_dir1/"));
    QVERIFY(!u.isManaged(testOutputIsManaged + "/unmanaged.json"));
}

void TestUpdater::updaterRemoveOtherFiles()
{
    Updater u;
    u.setLocalRepository(testOutputIsManaged);
    QVERIFY(QFile::exists(testOutputIsManaged + "/status.json"));
    QVERIFY(QFile::exists(testOutputIsManaged + "/unmanaged.json"));
    QVERIFY(QDir(testOutputIsManaged + "/dirs/empty_dir1").exists());

    u.removeOtherFiles([](QFileInfo file) {
        return file.fileName() != "unmanaged.json";
    });
    QVERIFY(QFile::exists(testOutputIsManaged + "/status.json"));
    QVERIFY(QFile::exists(testOutputIsManaged + "/unmanaged.json"));
    QVERIFY(!QDir(testOutputIsManaged + "/dirs/empty_dir1").exists());

    u.removeOtherFiles();
    QVERIFY(QFile::exists(testOutputIsManaged + "/status.json"));
    QVERIFY(!QFile::exists(testOutputIsManaged + "/unmanaged.json"));
    QVERIFY(!QDir(testOutputIsManaged + "/dirs/empty_dir1").exists());
}

void TestUpdater::updateToV1()
{
    Updater u;
    u.setLocalRepository(testOutputUpdate);
    QCOMPARE(u.localRevision(), QString());
    u.setTmpDirectory(testOutputUpdateTmp);
    u.setRemoteRepository("file:///" + dataRepoToV1 + "/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished(bool)));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
    }
    {
        QSignalSpy spyWarnings(&u, SIGNAL(warning(Warning)));
        QSignalSpy spy(&u, SIGNAL(updateFinished(bool)));
        u.update();
        QVERIFY(spy.wait());
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy[0].size(), 1);
        QCOMPARE(spy[0][0].toBool(), true);
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("1"));
        QCOMPARE(spyWarnings.size(), 0);
    }
    try {
        (TestUtils::assertFileEquals(testOutputUpdate + "/dir2/patch_same.txt", dataDir + "/rev1/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/path_diff.txt", dataDir + "/rev1/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/path_diff2.txt", dataDir + "/rev1/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/rmfile.txt", dataDir + "/rev1/rmfile.txt"));
    } catch(std::exception &msg) {
        QFAIL(msg.what());
    }
    QVERIFY(QFileInfo(testOutputUpdate + "/empty_dir").isDir());
    QVERIFY(QFileInfo(testOutputUpdate + "/dirs/empty_dir2").isDir());
    QCOMPARE(QDir(testOutputUpdateTmp).entryList(QDir::NoDotAndDotDot).count(), 0);
}

void TestUpdater::updateToV2()
{
    Updater u;
    u.setLocalRepository(testOutputUpdate);
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testOutputUpdateTmp);
    u.setRemoteRepository("file:///" + dataRepoToV2 + "/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished(bool)));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
    }
    {
        QSignalSpy spyWarnings(&u, SIGNAL(warning(Warning)));
        QSignalSpy spy(&u, SIGNAL(updateFinished(bool)));
        u.update();
        QVERIFY(spy.wait());
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy[0].size(), 1);
        QCOMPARE(spy[0][0].toBool(), true);
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
        QCOMPARE(spyWarnings.size(), 0);
    }
    try{
        (TestUtils::assertFileEquals(testOutputUpdate + "/dir2/patch_same.txt", dataDir + "/rev2/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/path_diff.txt", dataDir + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/path_diff2.txt", dataDir + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/add.txt", dataDir + "/rev2/add.txt"));
        (TestUtils::assertFileEquals(testOutputUpdate + "/empty_dir", dataDir + "/rev2/empty_dir"));
    } catch(std::exception &msg) {
        QFAIL(msg.what());
    }
    QVERIFY(!QFile::exists(testOutputUpdate + "/rmfile.txt"));
    QVERIFY(!QFileInfo(testOutputUpdate + "/dirs/empty_dir2").isDir());
    QCOMPARE(QDir(testOutputUpdateTmp).entryList(QDir::NoDotAndDotDot).count(), 0);
}
