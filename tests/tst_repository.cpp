#include "tst_repository.h"
#include "testutils.h"
#include <repository.h>
#include <QString>
#include <exceptions.h>

const QString testOutput = testDir + "/tst_repository_output";
const QString testOutputNew = testOutput + "/new";
const QString testOutputAddPackage= testOutput + "/add_package";
const QString dataAddDir = dataDir + "/repository_add";
const QString dataNewDir = dataDir + "/repository_new";

void TestRepository::initTestCase()
{
    FORCED_CLEANUP
    QVERIFY(QDir().mkpath(testOutput));
    QVERIFY(QDir().mkpath(testOutputNew));
    QVERIFY(QDir().mkpath(testOutputAddPackage));
    QVERIFY(QFile::copy(dataAddDir + "/init/versions", testOutputAddPackage + "/versions"));
    QVERIFY(QFile::copy(dataAddDir + "/init/packages", testOutputAddPackage + "/packages"));
    QVERIFY(QFile::copy(dataAddDir + "/init/current", testOutputAddPackage + "/current"));
    QVERIFY(QFile::copy(dataAddDir + "/init/test2", testOutputAddPackage + "/test2"));
    QVERIFY(QFile::copy(dataAddDir + "/init/test2.metadata", testOutputAddPackage + "/test2.metadata"));
}

void TestRepository::cleanupTestCase()
{
    if(!TestUtils::cleanup)
        return;

    QDir(testOutput).removeRecursively();
}

void TestRepository::newRepository()
{
    Repository pm;

    QCOMPARE(pm.directory(), QString());

    pm.setDirectory(testOutput + "\\new");
    QCOMPARE(pm.directory(), testOutput + "/new/");

    try {
        pm.load();
        QCOMPARE(pm.packages().size(), 0);
        QCOMPARE(pm.versions().size(), 0);
        QVERIFY(pm.currentRevision().isEmpty());
    } catch(std::exception & msg) {
        QFAIL(QStringLiteral("Load failed : %1").arg(msg.what()).toLatin1());
    }

    try {
        pm.save();
        QVERIFY(!QFile::exists(testOutputNew + "/current"));
        QVERIFY(TestUtils::compareJson(testOutputNew + "/packages", dataNewDir + "/packages"));
        QVERIFY(TestUtils::compareJson(testOutputNew + "/versions", dataNewDir + "/versions"));
    } catch(std::exception & msg) {
        QFAIL(QStringLiteral("Save failed : %1").arg(msg.what()).toLatin1());
    }

    try {
        Repository pm2;
        pm2.setDirectory(testOutputNew);
        pm2.load();
        QCOMPARE(pm2.packages().size(), 0);
        QCOMPARE(pm2.versions().size(), 0);
        QVERIFY(pm2.currentRevision().isEmpty());
    } catch(std::exception & msg) {
        QFAIL(QStringLiteral("2nd Load failed : %1").arg(msg.what()).toLatin1());
    }
}

void TestRepository::addPackage()
{
    Repository pm;
    pm.setDirectory(testOutputAddPackage);
    QCOMPARE(pm.directory(), testOutputAddPackage + "/");

    try {
        pm.load();
        QCOMPARE(pm.packages().size(), 1);
        QCOMPARE(pm.versions().size(), 1);
        QCOMPARE(pm.currentRevision(), QString("REV1"));
    } catch(std::exception & msg) {
        QFAIL(QStringLiteral("Load failed : %1").arg(msg.what()).toLatin1());
    }

    try {
        pm.addPackage("test2");
        QCOMPARE(pm.packages().size(), 2);
        QCOMPARE(pm.versions().size(), 2);
        QCOMPARE(pm.currentRevision(), QString("REV1"));
    } catch(std::exception & e) {
        QFAIL(QStringLiteral("Add package failed : %1").arg(e.what()).toLatin1());
    }

    QCOMPARE(pm.setCurrentRevision(QString("REV2")), true);
    QCOMPARE(pm.currentRevision(), QString("REV2"));

    QCOMPARE(pm.setCurrentRevision(QString("REV3")), false);
    QCOMPARE(pm.currentRevision(), QString("REV2"));

    try {
        pm.save();
        QVERIFY(TestUtils::compareJson(testOutputAddPackage + "/current", dataAddDir + "/expected/current"));
        QVERIFY(TestUtils::compareJson(testOutputAddPackage + "/packages", dataAddDir + "/expected/packages"));
        QVERIFY(TestUtils::compareJson(testOutputAddPackage + "/versions", dataAddDir + "/expected/versions"));
    } catch(std::exception & e) {
        QFAIL(QStringLiteral("Save failed : %1").arg(e.what()).toLatin1());
    }

    try {
        Repository pm2;
        pm2.setDirectory(testOutputAddPackage);
        pm2.load();
        QCOMPARE(pm2.packages().size(), 2);
        QCOMPARE(pm2.versions().size(), 2);
        QCOMPARE(pm2.currentRevision(), QString("REV2"));
    } catch(std::exception & e) {
        QFAIL(QStringLiteral("2nd load failed : %1").arg(e.what()).toLatin1());
    }
}

void TestRepository::fixRepository()
{
    QWARN("test not implemented");
}
