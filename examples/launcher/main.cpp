#include <QApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <QQmlContext>
#include "mainwindow.h"
#include "updatedialog.h"

int main(int argc, char *argv[])
{
#if defined(QT_DEBUG)
    QLoggingCategory::setFilterRules(QStringLiteral("updatesystem.*.debug=true"));
#endif
    //qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);
    if(argc >= 4 )
    {
        if(QString(argv[1]) == "update")
        {
            QString source = QApplication::applicationDirPath();
            QString processId(argv[2]);
            QString dest(argv[3]);
            if(argc > 4)
                source = QString(argv[4]);
            if(!QFile::exists(dest))
                return -1;
            UpdateDialog d(processId, dest, source);
            d.show();
            d.update();
            return a.exec();
        }
    }

    MainWindow mainwindow;
    mainwindow.show();
    return a.exec();
}
