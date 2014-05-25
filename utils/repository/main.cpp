#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <repository.h>
#include <packager.h>
#include <common/utils.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("repository"));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("command", QCoreApplication::tr("The command to execute."
                                                                 "\n - versions"
                                                                 "\n - mkpackage"
                                                                 "\n - addpackage"
                                                                 "\n - rmpackage"
                                                                 "\n - setversion"
                                                                 "\n - simplify"
                                                                 ), "<command>");

    QCommandLineOption verbose(QStringList() << "verbose"
         , QCoreApplication::tr("Run in verbose mode."));
    parser.addOption(verbose);

    parser.parse(QCoreApplication::arguments());

    QLoggingCategory::setFilterRules(QStringLiteral("updatesystem.*.debug=%1").arg(parser.isSet(verbose) ? "true" : "false"));

    const QStringList args = parser.positionalArguments();
    const QString command = args.isEmpty() ? QString() : args.first();
    if (command == "mkpackage")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("mkpackage", QCoreApplication::tr("Create a new package of the repository."));
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."), "<repository>");
        parser.addPositionalArgument("new_directory", QCoreApplication::tr("Path to the newest directory."), "<new_directory>");
        parser.addPositionalArgument("new_version", QCoreApplication::tr("Revision of the newest directory."), "<new_version>");
        parser.addPositionalArgument("old_directory", QCoreApplication::tr("Path to the oldest directory."), "[<old_directory>");
        parser.addPositionalArgument("old_version", QCoreApplication::tr("Revision of the oldest directory."), "<old_version>]");

        QCommandLineOption tmpDirectoryPath(QStringList() << "t" << "tmp"
             , QCoreApplication::tr("Path to use for temporary files.")
             , "tmp_directory");
        parser.addOption(tmpDirectoryPath);

        QCommandLineOption lzmaPath("lzma"
             , QCoreApplication::tr("Binary to use as lzma [lzma by default].")
             , "lzma_bin");
        parser.addOption(lzmaPath);

        QCommandLineOption xdeltaPath("xdelta3"
             , QCoreApplication::tr("Binary to use as xdelta3 [xdelta3 by default].")
             , "xdelta3_bin");
        parser.addOption(xdeltaPath);

        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.size() < 2)
            qWarning() << "Error : repository argument is missing";
        else if(args.size() < 3)
            qWarning() << "Error : new_directory argument is missing";
        else if(args.size() < 4)
            qWarning() << "Error : new_version argument is missing";
        else if(args.size() == 5)
            qWarning() << "Error : old_version argument is missing";
        else if(args.size() > 6)
            qWarning() << "Error : too much arguments";
        else
        {
            Packager packager;
            packager.setNewSource(args.at(2), args.at(3));

            if(args.size() == 6)
                packager.setOldSource(args.at(4), args.at(5));

            if(parser.isSet(tmpDirectoryPath))
                packager.setTmpDirectoryPath(parser.value(tmpDirectoryPath));

            if(parser.isSet(lzmaPath))
                Utils::setLzmaProgram(parser.value(lzmaPath));

            if(parser.isSet(xdeltaPath))
                Utils::setXdeltaProgram(parser.value(xdeltaPath));

            try
            {
                printf("Progression ...");
                fflush(stdout);
                QObject::connect(&packager, &Packager::progress, [](int pos, int total) {
                    printf("\rProgression %d/%d", pos, total);
                    fflush(stdout);
                });

                Repository repository;
                repository.setDirectory(args.at(1));
                repository.load();
                repository.addPackage(packager.generateForRepository(repository.directory()));
                repository.save();
                printf("\nPackage generated\n");
                return 0;
            }
            catch(std::exception &e)
            {
                qWarning() << e.what();
                return 2;
            }
        }
    }
    else if (command == "addpackage" || command == "rmpackage")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."), "<repository>");
        parser.addPositionalArgument("package_name", QCoreApplication::tr("Name of the package."), "<package_name>");
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.size() < 2)
            qWarning() << "Error : repository argument is missing";
        else if(args.size() < 3)
            qWarning() << "Error : package_name argument is missing";
        else if(args.size() > 3)
            qWarning() << "Error : too much arguments";
        else
        {
            Repository repository(args.at(1));
            repository.load();
            if(command == "addpackage")
                repository.addPackage(args.at(2));
            else
                repository.removePackage(args.at(2));
            return 0;
        }
    }
    else if (command == "setversion")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."), "<repository>");
        parser.addPositionalArgument("version", QCoreApplication::tr("Name of the version."));
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.size() < 2)
            qWarning() << "Error : repository argument is missing";
        else if(args.size() < 3)
            qWarning() << "Error : version argument is missing";
        else if(args.size() > 3)
            qWarning() << "Error : too much arguments";
        else
        {
            try
            {
                Repository repository(args.at(1));
                repository.load();
                if(repository.setCurrentRevision(args.at(2)))
                    printf("Repository current revision changed\n");
                else
                    printf("Unable to change the current revision of the repository\n");
                repository.save();
                return 0;
            }
            catch(std::exception &e)
            {
                qCritical() << e.what();
                return 2;
            }
        }
    }
    else if (command == "versions")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."), "<repository>");
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.size() < 2)
            qWarning() << "Error : repository argument is missing";
        else if(args.size() > 2)
            qWarning() << "Error : too much arguments";
        else
        {
            Repository repository(args.at(1));
            repository.load();
            foreach(const Version &version, repository.versions())
            {
                printf("%s\t:\t%s\n", qPrintable(version.revision), qPrintable(version.description));
            }
            return 0;
        }
    }
    else if (command == "simplify")
    {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("repository", QCoreApplication::tr("Path to the repository."), "<repository>");
        parser.process(app);

        const QStringList args = parser.positionalArguments();
        if(args.size() < 2)
            qWarning() << "Error : repository argument is missing";
        else if(args.size() > 2)
            qWarning() << "Error : too much arguments";
        else
        {
            Repository repository(args.at(1));
            repository.load();
            repository.simplify();
            repository.save();
            return 0;
        }
    }
    else if(!command.isEmpty())
    {
        qWarning() << "Error : command not supported " << command.toLatin1().data();
    }
    else
    {
        qWarning() << "Error : command not found";
    }

    parser.showHelp(1);

    return 1;
}
