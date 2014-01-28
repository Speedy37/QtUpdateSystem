#include "packagerplugin.h"


PackagerPlugin::PackagerPlugin(QObject *parent) :
    QGenericPlugin(parent)
{
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(Packager, PackagerPlugin)
#endif // QT_VERSION < 0x050000
