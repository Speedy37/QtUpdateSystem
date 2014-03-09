#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <updater.h>
#include <common/utils.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("updater"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption checkonly(QStringList() << "c" << "checkonly"
         , QCoreApplication::tr("Only check for updates."));

    QCommandLineOption tmpDirectoryPath(QStringList() << "t" << "tmp"
         , QCoreApplication::tr("Path to use for temporary files.")
         , "tmp_directory");

    QCommandLineOption lzmaPath("lzma"
         , QCoreApplication::tr("Binary to use as lzma [%1 by default].").arg(Utils::lzmaProgram())
         , "lzma_bin");

    QCommandLineOption xdeltaPath("xdelta3"
         , QCoreApplication::tr("Binary to use as xdelta3 [%1 by default].").arg(Utils::xdeltaProgram())
         , "xdelta3_bin");

    parser.addOption(checkonly);
    parser.addOption(tmpDirectoryPath);
    parser.addOption(lzmaPath);
    parser.addOption(xdeltaPath);
    parser.addPositionalArgument("local_repository", QCoreApplication::tr("Path to the local repository."), "<local_repository>");
    parser.addPositionalArgument("remote_repository", QCoreApplication::tr("Path to the remote repository."), "<remote_repository>");
    parser.addPositionalArgument("username", QCoreApplication::tr("Username for the remote repository."), "<username>");
    parser.addPositionalArgument("password", QCoreApplication::tr("Password for the remote repository."), "<password>");
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if(args.size() < 1)
        qWarning() << "Error : local_repository argument is missing";
    else if(args.size() < 2)
        qWarning() << "Error : remote_repository argument is missing";
    else if(args.size() > 2 && args.size() < 4)
        qWarning() << "Error : password argument is missing";
    else if(args.size() > 4)
        qWarning() << "Error : too much arguments";
    else
    {
        Updater updater(args[0], args[1]);

        if(parser.isSet(tmpDirectoryPath))
            updater.setTmpDirectory(parser.value(tmpDirectoryPath));

        if(parser.isSet(lzmaPath))
            Utils::setLzmaProgram(parser.value(lzmaPath));

        if(parser.isSet(xdeltaPath))
            Utils::setXdeltaProgram(parser.value(xdeltaPath));

        if(args.size() == 4)
            updater.setCredentials(args[2], args[3]);

        if(parser.isSet(checkonly))
        {
            updater.checkForUpdates();
            QEventLoop loop;
            QObject::connect(&updater, &Updater::checkForUpdatesFinished, &loop, &QEventLoop::quit);
            loop.exec();
            const char * state = Updater::staticMetaObject.enumerator(Updater::staticMetaObject.indexOfEnumerator("State")).valueToKey(updater.state());
            printf("%s", state);
        }
        else
        {
            updater.checkForUpdates();
            QEventLoop loop;
            QObject::connect(&updater, &Updater::checkForUpdatesFinished, &loop, &QEventLoop::quit);
            loop.exec();
            if(updater.isUpdateAvailable())
            {
                updater.update();
                QObject::connect(&updater, &Updater::updateFinished, &loop, &QEventLoop::quit);
                loop.exec();
            }
        }
    }

    return 1;
}
