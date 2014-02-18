#include <QTest>
#include <QCoreApplication>
#include <qtlog.h>
#include <common/utils.h>

#include "tst_repository.h"
#include "tst_packager.h"
#include "tst_updater.h"
#include "tst_updatechain.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QtLog::setLevel(QtLog::Trace);
#ifdef Q_OS_WIN
    Utils::setLzmaProgram(QString(SRCDIR)+ "../lzma.exe");
    Utils::setXdeltaProgram(QString(SRCDIR)+ "../xdelta3.exe");
#else
    Utils::setLzmaProgram("lzmacon");
#endif

    {
        TestRepository t;
        QTest::qExec(&t, argc, argv);
    }

    {
        TestPackager t;
        QTest::qExec(&t, argc, argv);
    }

    {
        TestUpdater t;
        QTest::qExec(&t, argc, argv);
    }

    {
        TestUpdateChain t;
        QTest::qExec(&t, argc, argv);
    }

    return 0;
}
