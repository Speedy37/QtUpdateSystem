#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <packager.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("packager"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("new_directory", QCoreApplication::tr("Path to the newest directory."));
    parser.addPositionalArgument("new_version", QCoreApplication::tr("Revision of the newest directory."));
    parser.addPositionalArgument("old_directory", QCoreApplication::tr("Path to the oldest directory."));
    parser.addPositionalArgument("old_version", QCoreApplication::tr("Revision of the oldest directory."));
    parser.addPositionalArgument("patch_name", QCoreApplication::tr("Path to generate the patch."));
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
    packager.setNewSource(args.at(0), args.at(1));
    packager.setOldSource(args.at(2), args.at(3));
    packager.setDeltaFilename(args.at(4));

    if(parser.isSet(deltaMetadataFilename))
        packager.setDeltaMetadataFilename(parser.value(deltaMetadataFilename));

    if(parser.isSet(tmpDirectoryPath))
        packager.setTmpDirectoryPath(parser.value(tmpDirectoryPath));

    try
    {
        packager.generate();
        return 0;
    }
    catch(QString &str)
    {
        qWarning() << str.toLatin1();
        return 2;
    }
}
