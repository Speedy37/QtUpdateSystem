#include <QString>
#include <QtTest>
#include <iostream>
#include <packager.h>
#include <qtlog.h>

const QString testDir = QString(SRCDIR);

class PackagerTest : public QObject
{
    Q_OBJECT

public:
    PackagerTest();

private Q_SLOTS:
    void packageTest1NewOld();
    void packageTest1New();
    void packageTest1Old();
};

PackagerTest::PackagerTest()
{
    QtLog::setLevel(QtLog::Error);
}

void PackagerTest::packageTest1NewOld()
{
    Packager packager;
    packager.setNewSource(testDir + "test1\\new", "REV1");
    QCOMPARE(packager.newDirectoryPath(), testDir + "test1/new/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    packager.setOldSource(testDir + "test1\\old", QStringLiteral("REV2"));
    QCOMPARE(packager.oldDirectoryPath(), testDir + "test1/old/");
    QCOMPARE(packager.oldRevisionName(), QStringLiteral("REV2"));

    packager.setTmpDirectoryPath(testDir + "test1\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(),testDir + "test1/tmp/");

    packager.setDeltaFilename(testDir + "test1/deltafile_new_old");
    QCOMPARE(packager.deltaFilename(), testDir + "test1/deltafile_new_old");
    QCOMPARE(packager.deltaMetaDataFilename(), testDir + "test1/deltafile_new_old.metadata");
    QFile::remove(packager.deltaFilename());
    QFile::remove(packager.deltaMetaDataFilename());
    packager.generate();
}

void PackagerTest::packageTest1New()
{
    Packager packager;
    packager.setNewSource(testDir + "test1\\new", "REV1");
    QCOMPARE(packager.newDirectoryPath(), testDir + "test1/new/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    QCOMPARE(packager.oldDirectoryPath(), QString());
    QCOMPARE(packager.oldRevisionName(), QString());

    packager.setTmpDirectoryPath(testDir + "test1\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(),testDir + "test1/tmp/");

    packager.setDeltaFilename(testDir + "test1/deltafile_new");
    QCOMPARE(packager.deltaFilename(), testDir + "test1/deltafile_new");
    QCOMPARE(packager.deltaMetaDataFilename(), testDir + "test1/deltafile_new.metadata");
    QFile::remove(packager.deltaFilename());
    QFile::remove(packager.deltaMetaDataFilename());
    packager.generate();
}

void PackagerTest::packageTest1Old()
{
    Packager packager;
    packager.setNewSource(testDir + "test1\\old", "REV1");
    QCOMPARE(packager.newDirectoryPath(), testDir + "test1/old/");
    QCOMPARE(packager.newRevisionName(), QStringLiteral("REV1"));

    QCOMPARE(packager.oldDirectoryPath(), QString());
    QCOMPARE(packager.oldRevisionName(), QString());

    packager.setTmpDirectoryPath(testDir + "test1\\tmp");
    QCOMPARE(packager.tmpDirectoryPath(),testDir + "test1/tmp/");

    packager.setDeltaFilename(testDir + "test1/deltafile_old");
    QCOMPARE(packager.deltaFilename(), testDir + "test1/deltafile_old");
    QCOMPARE(packager.deltaMetaDataFilename(), testDir + "test1/deltafile_old.metadata");
    QFile::remove(packager.deltaFilename());
    QFile::remove(packager.deltaMetaDataFilename());
    packager.generate();
}

QTEST_APPLESS_MAIN(PackagerTest)

#include "tst_packagertest.moc"
