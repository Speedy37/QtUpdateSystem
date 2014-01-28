#ifndef PACKAGERPLUGIN_H
#define PACKAGERPLUGIN_H

#include <QGenericPlugin>


class PackagerPlugin : public QGenericPlugin
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "Packager.json")
#endif // QT_VERSION >= 0x050000

public:
    PackagerPlugin(QObject *parent = 0);
};

#endif // PACKAGERPLUGIN_H
