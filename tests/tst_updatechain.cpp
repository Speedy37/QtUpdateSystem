#include "tst_updatechain.h"
#include <repository.h>
#include <packager.h>
#include <updater.h>

const QString testDir = QString(SRCDIR);
const QString testNew = testDir + "updatechain_testNew";

void TestUpdateChain::initTestCase()
{
    QFile::remove(testNew + "/repo/versions");
    QFile::remove(testNew + "/repo/packages");
    QFile::remove(testNew + "/repo/current");
    QFile::remove(testNew + "/repo/patch1_2");
    QFile::remove(testNew + "/repo/patch1_2.metadata");
    QFile::remove(testNew + "/repo/complete_2");
    QFile::remove(testNew + "/repo/complete_2.metadata");
}

void TestUpdateChain::cleanupTestCase()
{
}

void TestUpdateChain::testNewRepository()
{
    Repository pm;
    pm.setDirectory(testNew + "/repo");
    pm.load();

    {
        Packager p;
        p.setOldSource(testNew + "/rev1", "1");
        p.setNewSource(testNew + "/rev2", "2");
        p.setTmpDirectoryPath(testNew + "/tmp");
        PackageMetadata metadata = p.generateForRepository(testNew + "/repo");
        pm.addPackage(metadata);
    }
    {
        Packager p;
        p.setNewSource(testNew + "/rev2", "2");
        p.setTmpDirectoryPath(testNew + "/tmp");
        PackageMetadata metadata = p.generateForRepository(testNew + "/repo");
        pm.addPackage(metadata);
    }

    pm.setCurrentRevision("2");
    pm.save();

    Updater u;
    u.setLocalRepository(testNew + "/local_repo");
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
    }
}
