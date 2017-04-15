#ifndef QTLZMA_H
#define QTLZMA_H

#include <QIODevice>

QIODevice * LZMACompressor(QIODevice *source, quint32 preset = 9, QObject *parent = nullptr);
QIODevice * LZMADecompressor(QIODevice *source, QObject *parent = nullptr);

#endif // QTLZMA_H
