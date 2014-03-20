#include "tst_updatechain.h"
#include "testutils.h"
#include <repository.h>
#include <packager.h>
#include <updater.h>

const QString testDir = QString(SRCDIR);
const QString testNew = testDir + "updatechain_testNew";
const QString testUpdate = testDir + "updatechain_testUpdate";

void TestUpdateChain::initTestCase()
{
    FORCED_CLEANUP
    {
        QDir dir(testNew);
        dir.mkpath("repo");
        dir.mkpath("tmp");
        dir.mkpath("local_repo");
        dir.mkpath("local_tmp");
    }
    {
        QDir dir(testUpdate);
        dir.mkpath("local_repo");
        dir.mkpath("local_tmp");
    }
}

void TestUpdateChain::cleanupTestCase()
{
    if(!TestUtils::cleanup)
        return;

    QDir(testNew + "/repo").removeRecursively();
    QDir(testNew + "/tmp").removeRecursively();
    QDir(testNew + "/local_repo").removeRecursively();
    QDir(testNew + "/local_tmp").removeRecursively();

    QDir(testUpdate + "/local_repo").removeRecursively();
    QDir(testUpdate + "/local_tmp").removeRecursively();
}

void TestUpdateChain::newRepository()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();

    Packager p;
    p.setNewSource(testNew + "/rev1", "1");
    p.setTmpDirectoryPath(testNew + "/tmp");
    try {
        PackageMetadata metadata = p.generateForRepository(testNew + "/repo");
        pm.addPackage(metadata);
    }
    catch(QString & msg)
    {
        QFAIL(("Complete to v1 : " +msg).toLatin1());
    }
    pm.setCurrentRevision("1");
    pm.save();
}

void TestUpdateChain::updateToV1()
{
    Updater u;
    u.setLocalRepository(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString());
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
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
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("1"));
        QCOMPARE(spyWarnings.size(), 0);
    }
    try {
        (TestUtils::assertFileEquals(testNew + "/local_repo/patch_same.txt", testNew + "/rev1/patch_same.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff.txt", testNew + "/rev1/path_diff.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff2.txt", testNew + "/rev1/path_diff2.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/rmfile.txt", testNew + "/rev1/rmfile.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }

    QVERIFY(!QFile::exists(testNew + "/local_repo/add.txt"));
}

void TestUpdateChain::createPatchV1toV2()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();

    Packager p;
    p.setOldSource(testNew + "/rev1", "1");
    p.setNewSource(testNew + "/rev2", "2");
    p.setTmpDirectoryPath(testNew + "/tmp");
    try {
        PackageMetadata metadata = p.generateForRepository(testNew + "/repo");
        pm.addPackage(metadata);
    }
    catch(QString & msg)
    {
        QFAIL(("Patch v1 to v2 : " +msg).toLatin1());
    }
    pm.setCurrentRevision("2");
    pm.save();
}

void TestUpdateChain::updateToV2()
{
    Updater u;
    u.setLocalRepository(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
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
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
        QCOMPARE(spyWarnings.size(), 0);
    }
    try{
        (TestUtils::assertFileEquals(testNew + "/local_repo/patch_same.txt", testNew + "/rev2/patch_same.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff.txt", testNew + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff2.txt", testNew + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/add.txt", testNew + "/rev2/add.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testNew + "/local_repo/rmfile.txt"));
}

void TestUpdateChain::fallbackToV1()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();
    pm.setCurrentRevision("1");
    pm.save();

    Updater u;
    u.setLocalRepository(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("2"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
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
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("1"));
        QCOMPARE(spyWarnings.size(), 4);
    }
    try{
        (TestUtils::assertFileEquals(testNew + "/local_repo/patch_same.txt", testNew + "/rev1/patch_same.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff.txt", testNew + "/rev1/path_diff.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff2.txt", testNew + "/rev1/path_diff2.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/rmfile.txt", testNew + "/rev1/rmfile.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testNew + "/local_repo/add.txt"));
}

void TestUpdateChain::updateToV2WithFailures()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();
    pm.setCurrentRevision("2");
    pm.save();

    QFile::remove(testNew + "/local_repo/path_diff.txt");
    Updater u;
    u.setLocalRepository(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
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
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
        QCOMPARE(spyWarnings.size(), 1);
        QCOMPARE(spyWarnings[0].size(), 1);
        QCOMPARE(spyWarnings[0][0].typeName(), "Warning");
        QCOMPARE(spyWarnings[0][0].value<Warning>().type(), Warning::OperationPreparation);
    }
    try {
        (TestUtils::assertFileEquals(testNew + "/local_repo/patch_same.txt", testNew + "/rev2/patch_same.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff.txt", testNew + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff2.txt", testNew + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/add.txt", testNew + "/rev2/add.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testNew + "/local_repo/rmfile.txt"));
}

void TestUpdateChain::integrityCheck()
{
    Updater u;
    u.setLocalRepository(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("2"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished(bool)));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::AlreadyUptodate, u.errorString().toLatin1());
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished(bool)));
        QSignalSpy spyDownloadProgress(&u, SIGNAL(updateDownloadProgress(qint64,qint64)));
        QSignalSpy spyApplyProgress(&u, SIGNAL(updateApplyProgress(qint64,qint64)));
        QSignalSpy spyCheckProgress(&u, SIGNAL(updateCheckProgress(qint64,qint64)));
        u.update();
        QVERIFY(spy.wait());
        QCOMPARE(spyDownloadProgress.count(), 0);
        QCOMPARE(spyApplyProgress.count(), 0);
        QCOMPARE(spyCheckProgress.count(), 4);
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
    }
    try {
        (TestUtils::assertFileEquals(testNew + "/local_repo/patch_same.txt", testNew + "/rev2/patch_same.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff.txt", testNew + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/path_diff2.txt", testNew + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testNew + "/local_repo/add.txt", testNew + "/rev2/add.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
}
