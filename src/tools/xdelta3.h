#ifndef QTXDELTA3_H
#define QTXDELTA3_H

#include <QIODevice>

QIODevice *XDelta3(QIODevice *source, QIODevice *base, bool encode, QObject *parent = nullptr);

#endif // QTXDELTA3_H
