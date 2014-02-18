#include "tst_packager.h"
#include <QString>
#include <QtTest>
#include <iostream>
#include <packager.h>

const QString testDir = QString(SRCDIR);
const QString testNew = "packager_test1";

void TestPackager::packageTest1NewOld()
{
    Packager packager;
    packager.setNewSource(testDir + testNew + "\\new", "REV1");
    QCOMPARE(packager.newDirectoryPath(), testDir + testNew + "/new/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    packager.setOldSource(testDir + testNew + "\\old", QStringLiteral("REV2"));
    QCOMPARE(packager.oldDirectoryPath(), testDir + testNew + "/old/");
    QCOMPARE(packager.oldRevisionName(), QStringLiteral("REV2"));

    packager.setTmpDirectoryPath(testDir + testNew + "\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(),testDir + testNew + "/tmp/");

    packager.setDeltaFilename(testDir + testNew + "/deltafile_new_old");
    QCOMPARE(packager.deltaFilename(), testDir + testNew + "/deltafile_new_old");
    QCOMPARE(packager.deltaMetadataFilename(), testDir + testNew + "/deltafile_new_old.metadata");
    QFile::remove(packager.deltaFilename());
    QFile::remove(packager.deltaMetadataFilename());
    try {
        packager.generate();
    } catch(QString & msg) {
        QFAIL(msg.toLatin1());
    }
}

void TestPackager::packageTest1New()
{
    Packager packager;
    packager.setNewSource(testDir + testNew + "\\new", "REV1");
    QCOMPARE(packager.newDirectoryPath(), testDir + testNew + "/new/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    QCOMPARE(packager.oldDirectoryPath(), QString());
    QCOMPARE(packager.oldRevisionName(), QString());

    packager.setTmpDirectoryPath(testDir + testNew + "\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(),testDir + testNew + "/tmp/");

    packager.setDeltaFilename(testDir + testNew + "/deltafile_new");
    QCOMPARE(packager.deltaFilename(), testDir + testNew + "/deltafile_new");
    QCOMPARE(packager.deltaMetadataFilename(), testDir + testNew + "/deltafile_new.metadata");
    QFile::remove(packager.deltaFilename());
    QFile::remove(packager.deltaMetadataFilename());
    try {
        packager.generate();
    } catch(QString & msg) {
        QFAIL(msg.toLatin1());
    }
}

void TestPackager::packageTest1Old()
{
    Packager packager;
    packager.setNewSource(testDir + testNew + "\\old", "REV1");
    QCOMPARE(packager.newDirectoryPath(), testDir + testNew + "/old/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    QCOMPARE(packager.oldDirectoryPath(), QString());
    QCOMPARE(packager.oldRevisionName(), QString());

    packager.setTmpDirectoryPath(testDir + testNew + "\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(),testDir + testNew + "/tmp/");

    packager.setDeltaFilename(testDir + testNew + "/deltafile_old");
    QCOMPARE(packager.deltaFilename(), testDir + testNew + "/deltafile_old");
    QCOMPARE(packager.deltaMetadataFilename(), testDir + testNew + "/deltafile_old.metadata");
    QFile::remove(packager.deltaFilename());
    QFile::remove(packager.deltaMetadataFilename());
    try {
        packager.generate();
    } catch(QString & msg) {
        QFAIL(msg.toLatin1());
    }
}
