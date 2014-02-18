#include "tst_updatechain.h"
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

void TestUpdateChain::testNewRepository()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();

    qDebug("Create package to v1");
    {
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

    qDebug("Update to v1");
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
    }

    qDebug("Create patch from v1 to v2");
    {
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

    qDebug("Update from v1 to v2");
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
    }

    qDebug("Fallback to v1 (test the download optimisation system");
    {
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
    }
}

void TestUpdateChain::testUpdateRepository()
{
    Updater u(testUpdate + "/local_repo");
    u.setTmpDirectory(testUpdate + "/local_tmp");
    u.setRemoteRepository("file:///" + testUpdate + "/repo/");
    {
        QSignalSpy spy(&u, SIGNAL(checkForUpdatesFinished()));
        u.checkForUpdates();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::UpdateRequired, u.errorString().toLatin1());
        //QCOMPARE(u.localRevision(), QString("1"));
        //QCOMPARE(u.updateRevision(), QString("2"));
    }
    {
        QSignalSpy spy(&u, SIGNAL(updateFinished()));
        u.update();
        QVERIFY(spy.wait());
        QVERIFY2(u.state() == Updater::Uptodate, u.errorString().toLatin1());
        QCOMPARE(u.localRevision(), QString("2"));
    }
}
