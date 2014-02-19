#include "tst_updatechain.h"
#include "testutils.h"
#include <repository.h>
#include <packager.h>
#include <updater.h>

const QString testDir = QString(SRCDIR);
const QString testNew = testDir + "updatechain_testNew";
const QString testUpdate = testDir + "updatechain_testUpdate";
bool cleanup = true;

void TestUpdateChain::initTestCase()
{
    cleanupTestCase();
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
    cleanup = false;
}

void TestUpdateChain::cleanupTestCase()
{
    if(!cleanup)
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
    Updater u(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString());
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished()));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished()));
        u.update();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("1"));
    }
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/patch_same.txt", testNew + "/rev1/patch_same.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff.txt", testNew + "/rev1/path_diff.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff2.txt", testNew + "/rev1/path_diff2.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/rmfile.txt", testNew + "/rev1/rmfile.txt"));
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
    Updater u(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished()));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished()));
        u.update();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
    }
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/patch_same.txt", testNew + "/rev2/patch_same.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff.txt", testNew + "/rev2/path_diff.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff2.txt", testNew + "/rev2/path_diff2.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/add.txt", testNew + "/rev2/add.txt"));
    QVERIFY(!QFile::exists(testNew + "/local_repo/rmfile.txt"));
}

void TestUpdateChain::fallbackToV1()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();
    pm.setCurrentRevision("1");
    pm.save();

    Updater u(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("2"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished()));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished()));
        u.update();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("1"));
    }
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/patch_same.txt", testNew + "/rev1/patch_same.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff.txt", testNew + "/rev1/path_diff.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff2.txt", testNew + "/rev1/path_diff2.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/rmfile.txt", testNew + "/rev1/rmfile.txt"));
}

void TestUpdateChain::updateToV2WithFailures()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();
    pm.setCurrentRevision("2");
    pm.save();

    QFile::remove(testNew + "/local_repo/path_diff.txt");
    Updater u(testNew + "/local_repo");
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testNew + "/local_tmp");
    u.setRemoteRepository("file:///" + testNew + "/repo/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished()));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished()));
        u.update();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
    }
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/patch_same.txt", testNew + "/rev2/patch_same.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff.txt", testNew + "/rev2/path_diff.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/path_diff2.txt", testNew + "/rev2/path_diff2.txt"));
    QVERIFY(TestUtils::compareFile(testNew + "/local_repo/add.txt", testNew + "/rev2/add.txt"));
    QVERIFY(!QFile::exists(testNew + "/local_repo/rmfile.txt"));
}
