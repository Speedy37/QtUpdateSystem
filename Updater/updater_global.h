#ifndef UPDATER_GLOBAL_H
#define UPDATER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(UPDATER_LIBRARY)
#  define UPDATERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define UPDATERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // UPDATER_GLOBAL_H
