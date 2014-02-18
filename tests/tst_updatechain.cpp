#include "tst_updatechain.h"
#include <repository.h>
#include <packager.h>
#include <updater.h>

const QString testDir = QString(SRCDIR);
const QString testNew = testDir + "updatechain_testNew";

void TestUpdateChain::initTestCase()
{
    cleanupTestCase();
    QDir dir(testNew);
    dir.mkpath("repo");
    dir.mkpath("tmp");
    dir.mkpath("local_repo");
    dir.mkpath("local_tmp");
}

void TestUpdateChain::cleanupTestCase()
{
    QDir(testNew + "/repo").removeRecursively();
    QDir(testNew + "/tmp").removeRecursively();
    QDir(testNew + "/local_repo").removeRecursively();
    QDir(testNew + "/local_tmp").removeRecursively();
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
        try {
            PackageMetadata metadata = p.generateForRepository(testNew + "/repo");
            pm.addPackage(metadata);
        }
        catch(QString & msg)
        {
            QFAIL(("Patch v1 to v2 : " +msg).toLatin1());
        }
    }
    {
        Packager p;
        p.setNewSource(testNew + "/rev2", "2");
        p.setTmpDirectoryPath(testNew + "/tmp");
        try {
            PackageMetadata metadata = p.generateForRepository(testNew + "/repo");
            pm.addPackage(metadata);
        }
        catch(QString & msg)
        {
            QFAIL(("Complete to v2 : " +msg).toLatin1());
        }
    }

    pm.setCurrentRevision("2");
    pm.save();

    {
        Updater u(testNew + "/local_repo");
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
}
