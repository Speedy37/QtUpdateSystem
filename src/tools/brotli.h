#ifndef QTBROTLI_H
#define QTBROTLI_H

#include <QIODevice>

QIODevice * BrotliCompressor(QIODevice *source, quint32 quality = 9, quint32 lgwin = 0, QObject *parent = nullptr);
QIODevice * BrotliDecompressor(QIODevice *source, QObject *parent = nullptr);

#endif // QTBROTLI_H
