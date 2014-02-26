#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <repository.h>
#include <packager.h>

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
    if (command == "mkpackage")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."));
        parser.addPositionalArgument("new_directory", QCoreApplication::tr("Path to the newest directory."));
        parser.addPositionalArgument("new_version", QCoreApplication::tr("Revision of the newest directory."));
        parser.addPositionalArgument("old_directory", QCoreApplication::tr("Path to the oldest directory."));
        parser.addPositionalArgument("old_version", QCoreApplication::tr("Revision of the oldest directory."));
        QCommandLineOption deltaMetadataFilename(QStringList() << "m" << "metadata"
             , QCoreApplication::tr("Path to generate the patch metadata.")
             , "metadata_file");
        parser.addOption(deltaMetadataFilename);

        QCommandLineOption tmpDirectoryPath(QStringList() << "t" << "tmp"
             , QCoreApplication::tr("Path to use for temporary files.")
             , "tmp_directory");
        parser.addOption(tmpDirectoryPath);
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.length() != 5)
        {
            qWarning() << QCoreApplication::tr("%1 arguments were expected, %2 were given").arg(5).arg(args.length());
            parser.showHelp(1);
        }

        Packager packager;
        packager.setNewSource(args.at(1), args.at(2));
        packager.setOldSource(args.at(3), args.at(4));

        if(parser.isSet(deltaMetadataFilename))
            packager.setDeltaMetadataFilename(parser.value(deltaMetadataFilename));

        if(parser.isSet(tmpDirectoryPath))
            packager.setTmpDirectoryPath(parser.value(tmpDirectoryPath));

        try
        {
            packager.generateForRepository(args.at(0));
        }
        catch(QString &str)
        {
            qWarning() << str.toLatin1();
            return 2;
        }

        return 0;
    }
    else if (command == "addpackage" || command == "rmpackage")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."));
        parser.addPositionalArgument("package_name", QCoreApplication::tr("Name of the package."));
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.length() != 2)
        {
            qWarning() << QCoreApplication::tr("%1 arguments were expected, %2 were given").arg(2).arg(args.length());
            parser.showHelp(1);
        }

        Repository repository(args.at(0));
        if(command == "addpackage")
            repository.addPackage(args.at(1));
        else
            repository.removePackage(args.at(1));

        return 0;
    }
    else if (command == "setversion")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."));
        parser.addPositionalArgument("version", QCoreApplication::tr("Name of the version."));
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.length() != 2)
        {
            qWarning() << QCoreApplication::tr("%1 arguments were expected, %2 were given").arg(2).arg(args.length());
            parser.showHelp(1);
        }

        Repository repository(args.at(0));
        repository.setCurrentRevision(args.at(1));

        return 0;
    }
    else if (command == "simplify")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."));
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.length() != 1)
        {
            qWarning() << QCoreApplication::tr("%1 arguments were expected, %2 were given").arg(1).arg(args.length());
            parser.showHelp(1);
        }

        Repository repository(args.at(0));
        repository.simplify();

        return 0;
    }
    else
    {
        printf("Command %s not supported", command.toLatin1().data());
        parser.showHelp(1);
    }

    return 1;
}
