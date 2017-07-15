#include <QTest>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <common/utils.h>

#include "testutils.h"
#include "tst_repository.h"
#include "tst_packager.h"
#include "tst_updater.h"
#include "tst_updatechain.h"

#ifdef QT_DEBUG
    const char * mode = "DEBUG";
#else
    const char * mode = "RELEASE";
#endif

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("tests"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption verbose(QStringList() << "verbose"
         , QCoreApplication::tr("Run in verbose mode."));
    parser.addOption(verbose);

    QCommandLineOption keep(QStringList() << "k" << "keep"
         , QCoreApplication::tr("Keep generated files."));
    parser.addOption(keep);

    parser.process(app);

    TestUtils::cleanup = !parser.isSet(keep);
    QLoggingCategory::setFilterRules(QStringLiteral("updatesystem.*.debug=%1").arg(parser.isSet(verbose) ? "true" : "false"));

    int ret = 0;

    QDir().mkpath(dataDir + "/rev1/empty_dir");
    QDir().mkpath(dataDir + "/rev1/dirs/empty_dir2");

    {
        TestPackager t;
        ret += QTest::qExec(&t, QStringList());
    }

    {
        TestRepository t;
        ret += QTest::qExec(&t, QStringList());
    }

    {
        TestUpdater t;
        ret += QTest::qExec(&t, QStringList());
    }

    {
        TestUpdateChain t;
        ret += QTest::qExec(&t, QStringList());
    }

    printf("Tests %s in %s mode\n", ret == 0 ? "PASSED" : "FAILED", mode);

    return ret;
}
