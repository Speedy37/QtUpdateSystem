#include "tst_updatechain.h"
#include "testutils.h"
#include <repository.h>
#include <packager.h>
#include <updater.h>

const QString dataUpdateChain = dataDir + "/updatechain";
const QString testOutput = testDir + "/tst_updatechain_output";
const QString testOutputRepo = testOutput + "/repo";
const QString testOutputTmp = testOutput + "/tmp";
const QString testOutputLocalRepo = testOutput + "/local_repo";
const QString testOutputLocalTmp = testOutput + "/local_tmp";

void TestUpdateChain::initTestCase()
{
    FORCED_CLEANUP
    QVERIFY(QDir().mkpath(testOutput));
    QVERIFY(QDir().mkpath(testOutputRepo));
    QVERIFY(QDir().mkpath(testOutputTmp));
    QVERIFY(QDir().mkpath(testOutputLocalRepo));
    QVERIFY(QDir().mkpath(testOutputLocalTmp));
}

void TestUpdateChain::cleanupTestCase()
{
    if(!TestUtils::cleanup)
        return;

    QDir(testOutput).removeRecursively();
}

void TestUpdateChain::newRepository()
{
    Repository pm;
    pm.setDirectory(testOutputRepo);
    pm.load();

    Packager p;
    p.setNewSource(dataDir + "/rev1", "1");
    p.setTmpDirectoryPath(testOutputTmp);
    try {
        PackageMetadata metadata = p.generateForRepository(pm.directory());
        pm.addPackage(metadata);
    }
    catch(QString & msg)
    {
        QFAIL(("Complete to v1 : " +msg).toLatin1());
    }
    QVERIFY(pm.setCurrentRevision("1"));
    pm.save();
}

void TestUpdateChain::updateToV1()
{
    Updater u;
    u.setLocalRepository(testOutputLocalRepo);
    QCOMPARE(u.localRevision(), QString());
    u.setTmpDirectory(testOutputLocalTmp);
    u.setRemoteRepository("file:///" + testOutputRepo + "/");
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
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/dir2/patch_same.txt", dataDir + "/rev1/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff.txt", dataDir + "/rev1/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff2.txt", dataDir + "/rev1/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/rmfile.txt", dataDir + "/rev1/rmfile.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(QFileInfo(testOutputLocalRepo + "/empty_dir").isDir());
    QVERIFY(QFileInfo(testOutputLocalRepo + "/empty_dir2").isDir());
}

void TestUpdateChain::createPatchV1toV2()
{
    Repository pm;
    pm.setDirectory(testOutputRepo);
    pm.load();

    Packager p;
    p.setOldSource(dataDir + "/rev1", "1");
    p.setNewSource(dataDir + "/rev2", "2");
    p.setTmpDirectoryPath(testOutputTmp);
    try {
        PackageMetadata metadata = p.generateForRepository(pm.directory());
        pm.addPackage(metadata);
    }
    catch(QString & msg)
    {
        QFAIL(("Patch v1 to v2 : " +msg).toLatin1());
    }
    QVERIFY(pm.setCurrentRevision("2"));
    pm.save();
}

void TestUpdateChain::updateToV2()
{
    Updater u;
    u.setLocalRepository(testOutputLocalRepo);
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testOutputLocalTmp);
    u.setRemoteRepository("file:///" + testOutputRepo + "/");
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
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/dir2/patch_same.txt", dataDir + "/rev2/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff.txt", dataDir + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff2.txt", dataDir + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/add.txt", dataDir + "/rev2/add.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/empty_dir", dataDir + "/rev2/empty_dir"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testOutputLocalRepo + "/rmfile.txt"));
    QVERIFY(!QFileInfo(testOutputLocalRepo + "/empty_dir2").isDir());
}

void TestUpdateChain::fallbackToV1()
{
    Repository pm;
    pm.setDirectory(testOutputRepo);
    pm.load();
    QVERIFY(pm.setCurrentRevision("1"));
    pm.save();

    Updater u;
    u.setLocalRepository(testOutputLocalRepo);
    QCOMPARE(u.localRevision(), QString("2"));
    u.setTmpDirectory(testOutputLocalTmp);
    u.setRemoteRepository("file:///" + testOutputRepo + "/");
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
        QCOMPARE(spyWarnings.size(), 1);
        qDebug() << (spyWarnings[0][0].value<Warning>().message());
    }
    try{
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/dir2/patch_same.txt", dataDir + "/rev1/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff.txt", dataDir + "/rev1/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff2.txt", dataDir + "/rev1/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/rmfile.txt", dataDir + "/rev1/rmfile.txt"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testOutputLocalRepo + "/add.txt"));
    QVERIFY(QFileInfo(testOutputLocalRepo + "/empty_dir").isDir());
    QVERIFY(QFileInfo(testOutputLocalRepo + "/empty_dir2").isDir());
}

void TestUpdateChain::updateToV2WithFailures()
{
    Repository pm;
    pm.setDirectory(testOutputRepo);
    pm.load();
    QVERIFY(pm.setCurrentRevision("2"));
    pm.save();

    QFile::remove(testOutputLocalRepo + "/path_diff.txt");
    QFile::remove(testOutputLocalRepo + "/path_diff2.txt");
    Updater u;
    u.setLocalRepository(testOutputLocalRepo);
    QCOMPARE(u.localRevision(), QString("1"));
    u.setTmpDirectory(testOutputLocalTmp);
    u.setRemoteRepository("file:///" + testOutputRepo + "/");
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
        QCOMPARE(spyWarnings.size(), 2);
        QCOMPARE(spyWarnings[0].size(), 1);
        QCOMPARE(spyWarnings[0][0].typeName(), "Warning");
        QCOMPARE(spyWarnings[0][0].value<Warning>().type(), Warning::OperationPreparation);
        QCOMPARE(spyWarnings[1].size(), 1);
        QCOMPARE(spyWarnings[1][0].typeName(), "Warning");
        QCOMPARE(spyWarnings[1][0].value<Warning>().type(), Warning::OperationPreparation);
    }
    try {
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/dir2/patch_same.txt", dataDir + "/rev2/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff.txt", dataDir + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff2.txt", dataDir + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/add.txt", dataDir + "/rev2/add.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/empty_dir", dataDir + "/rev2/empty_dir"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testOutputLocalRepo + "/rmfile.txt"));
    QVERIFY(!QFileInfo(testOutputLocalRepo + "/empty_dir2").isDir());
}

void TestUpdateChain::integrityCheck()
{
    Updater u;
    u.setLocalRepository(testOutputLocalRepo);
    QCOMPARE(u.localRevision(), QString("2"));
    u.setTmpDirectory(testOutputLocalTmp);
    u.setRemoteRepository("file:///" + testOutputRepo + "/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished(bool)));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::AlreadyUptodate, u.errorString().toLatin1());
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished(bool)));
        u.update();
        QVERIFY(spy.wait());
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy[0].size(), 1);
        QCOMPARE(spy[0][0].toBool(), true);
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
    }
    try {
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/dir2/patch_same.txt", dataDir + "/rev2/dir2/patch_same.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff.txt", dataDir + "/rev2/path_diff.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/path_diff2.txt", dataDir + "/rev2/path_diff2.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/add.txt", dataDir + "/rev2/add.txt"));
        (TestUtils::assertFileEquals(testOutputLocalRepo + "/empty_dir", dataDir + "/rev2/empty_dir"));
    } catch(QString &msg) {
        QFAIL(msg.toLatin1());
    }
    QVERIFY(!QFile::exists(testOutputLocalRepo + "/rmfile.txt"));
    QVERIFY(!QFileInfo(testOutputLocalRepo + "/empty_dir2").isDir());
}
