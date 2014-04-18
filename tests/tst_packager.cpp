#include "tst_packager.h"
#include "testutils.h"
#include <QString>
#include <QtTest>
#include <iostream>
#include <packager.h>

const QString testOutput = testDir + "/tst_packager_output";

void TestPackager::initTestCase()
{
    FORCED_CLEANUP
    QVERIFY(QDir().mkpath(testOutput));
    QVERIFY(QDir().mkpath(testOutput + "/tmp"));
}

void TestPackager::cleanupTestCase()
{
    if(!TestUtils::cleanup)
        return;

    QDir(testOutput).removeRecursively();
}

void TestPackager::createPatch()
{
    Packager packager;
    packager.setNewSource(dataDir + "\\rev1", "REV1");
    QCOMPARE(packager.newDirectoryPath(), dataDir + "/rev1/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    packager.setOldSource(dataDir + "/rev2", QStringLiteral("REV2"));
    QCOMPARE(packager.oldDirectoryPath(), dataDir + "/rev2/");
    QCOMPARE(packager.oldRevisionName(), QStringLiteral("REV2"));

    packager.setTmpDirectoryPath(testOutput + "\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(), testOutput + "/tmp/");

    packager.setDeltaFilename(testOutput + "/deltafile_new_old");
    QCOMPARE(packager.deltaFilename(), testOutput + "/deltafile_new_old");
    QCOMPARE(packager.deltaMetadataFilename(), testOutput + "/deltafile_new_old.metadata");
    try {
        packager.generate();
    } catch(std::exception & msg) {
        QFAIL(msg.what());
    }
}

void TestPackager::createComplete()
{
    Packager packager;
    packager.setNewSource(dataDir + "\\rev1", "REV1");
    QCOMPARE(packager.newDirectoryPath(), dataDir + "/rev1/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));
    QCOMPARE(packager.oldDirectoryPath(), QString());
    QCOMPARE(packager.oldRevisionName(), QString());

    packager.setTmpDirectoryPath(testOutput + "\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(), testOutput + "/tmp/");

    packager.setDeltaFilename(testOutput + "/deltafile_new");
    QCOMPARE(packager.deltaFilename(), testOutput + "/deltafile_new");
    packager.setDeltaMetadataFilename(testOutput + "/deltafile_new_metadata");
    QCOMPARE(packager.deltaMetadataFilename(), testOutput + "/deltafile_new_metadata");
    try {
        packager.generate();
    } catch(std::exception & msg) {
        QFAIL(msg.what());
    }
}
