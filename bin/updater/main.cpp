#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <updater.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("updater"));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("command", QCoreApplication::tr("The command to execute."));
    parser.parse(QCoreApplication::arguments());

    const QStringList args = parser.positionalArguments();
    const QString command = args.isEmpty() ? QString() : args.first();
    if (command == "checkforupdates")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("directory", QCoreApplication::tr("Path to the local repository to check for updates."));
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.length() != 1)
        {
            qWarning() << QCoreApplication::tr("%1 arguments were expected, %2 were given").arg(1).arg(args.length());
            parser.showHelp(1);
        }
        Updater updater(args[0]);
        updater.checkForUpdates();
        QEventLoop loop;
        QObject::connect(&updater, &Updater::checkForUpdatesFinished, &loop, &QEventLoop::quit);
        loop.exec();
        const char * state = Updater::staticMetaObject.enumerator(Updater::staticMetaObject.indexOfEnumerator("State")).valueToKey(updater.state());
        printf("%s", state);

        return 0;
    }
    else if (command == "update")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("directory", QCoreApplication::tr("Path to the local repository to check for updates."));
        parser.process(app);

        QCommandLineOption tmpDirectoryPath(QStringList() << "t" << "tmp"
             , QCoreApplication::tr("Path to use for temporary files.")
             , "tmp_directory");
        parser.addOption(tmpDirectoryPath);

        const QStringList args = parser.positionalArguments();
        if(args.length() != 1)
        {
            qWarning() << QCoreApplication::tr("%1 arguments were expected, %2 were given").arg(1).arg(args.length());
            parser.showHelp(1);
        }

        Updater updater(args[0]);

        if(parser.isSet(tmpDirectoryPath))
            updater.setTmpDirectory(parser.value(tmpDirectoryPath));

        updater.update();
        QEventLoop loop;
        QObject::connect(&updater, &Updater::updateFinished, &loop, &QEventLoop::quit);
        loop.exec();
        const char * state = Updater::staticMetaObject.enumerator(Updater::staticMetaObject.indexOfEnumerator("State")).valueToKey(updater.state());
        printf("%s", state);

        return 0;
    }
    else
    {
        printf("Command %s not supported", command.toLatin1().data());
        parser.showHelp(1);
    }

    return 1;
}
