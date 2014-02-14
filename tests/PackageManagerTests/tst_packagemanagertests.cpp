#include <QString>
#include <QtTest>
#include <packagemanager.h>

const QString testDir = QString(SRCDIR);

class PackageManagerTests : public QObject
{
    Q_OBJECT

public:
    PackageManagerTests();

private Q_SLOTS:
    void testCase1();
};

PackageManagerTests::PackageManagerTests()
{
}

void PackageManagerTests::testCase1()
{
    QString dir = testDir + "test1\\dir";
    PackageManager pm;

    QCOMPARE(pm.directory(), QString());

    pm.setDirectory(dir);
    QCOMPARE(pm.directory(), testDir + "test1/dir/");

    try
    {
        pm.load();
        pm.save();
    } catch(QString & msg) {
        QFAIL(msg.toLatin1());
    }
}

QTEST_APPLESS_MAIN(PackageManagerTests)

#include "tst_packagemanagertests.moc"
