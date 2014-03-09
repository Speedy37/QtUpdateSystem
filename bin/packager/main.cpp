#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <packager.h>
#include <common/utils.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("packager"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("patch_name", QCoreApplication::tr("Path to generate the patch."), "<patch_name>");
    parser.addPositionalArgument("new_directory", QCoreApplication::tr("Path to the newest directory."), "<new_directory>");
    parser.addPositionalArgument("new_version", QCoreApplication::tr("Revision of the newest directory."), "<new_version>");
    parser.addPositionalArgument("old_directory", QCoreApplication::tr("Path to the oldest directory."), "[<old_directory>");
    parser.addPositionalArgument("old_version", QCoreApplication::tr("Revision of the oldest directory."), "<old_version>]");

    QCommandLineOption deltaMetadataFilename(QStringList() << "m" << "metadata"
         , QCoreApplication::tr("Path to generate the patch metadata.")
         , "metadata_file");
    parser.addOption(deltaMetadataFilename);

    QCommandLineOption tmpDirectoryPath(QStringList() << "t" << "tmp"
         , QCoreApplication::tr("Path to use for temporary files.")
         , "tmp_directory");
    parser.addOption(tmpDirectoryPath);
    parser.process(app);

    QCommandLineOption lzmaPath("lzma"
         , QCoreApplication::tr("Binary to use as lzma [%1 by default].").arg(Utils::lzmaProgram())
         , "lzma_bin");
    parser.addOption(lzmaPath);

    QCommandLineOption xdeltaPath("xdelta3"
         , QCoreApplication::tr("Binary to use as xdelta3 [%1 by default].").arg(Utils::xdeltaProgram())
         , "xdelta3_bin");
    parser.addOption(xdeltaPath);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if(args.size() < 1)
        qWarning() << "Error : patch_name argument is missing";
    else if(args.size() < 2)
        qWarning() << "Error : new_directory argument is missing";
    else if(args.size() < 3)
        qWarning() << "Error : new_version argument is missing";
    else if(args.size() == 4)
        qWarning() << "Error : old_version argument is missing";
    else if(args.size() > 5)
        qWarning() << "Error : too much arguments";
    else
    {
        Packager packager;
        packager.setNewSource(args.at(1), args.at(2));

        if(args.size() == 5)
            packager.setOldSource(args.at(3), args.at(4));

        if(parser.isSet(tmpDirectoryPath))
            packager.setTmpDirectoryPath(parser.value(tmpDirectoryPath));

        if(parser.isSet(deltaMetadataFilename))
            packager.setDeltaMetadataFilename(parser.value(deltaMetadataFilename));

        if(parser.isSet(lzmaPath))
            Utils::setLzmaProgram(parser.value(lzmaPath));

        if(parser.isSet(xdeltaPath))
            Utils::setXdeltaProgram(parser.value(xdeltaPath));

        try
        {
            packager.generate();
            return 0;
        }
        catch(QString &str)
        {
            qCritical() << str.toLatin1();
            return 2;
        }
    }
    parser.showHelp(1);
}
