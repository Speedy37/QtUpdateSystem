#ifndef LZMA_H
#define LZMA_H

#include <QString>

class QIODevice;

class Lzma
{
public:
    Lzma();
    bool compress(QIODevice * input, QIODevice *output);
    bool decompress(QIODevice *input, QIODevice *output);
    int level() const;
    void setLevel(int level);

    QString errorString() const;

private:
    void loop(QIODevice *input, QIODevice *output);
    int m_level;
    QString m_errorString;
};

inline int Lzma::level() const
{
    return m_level;
}

inline void Lzma::setLevel(int level)
{
    Q_ASSERT(level >= 0 && level <= 9);
    m_level = level;
}

inline QString Lzma::errorString() const
{
    return m_errorString;
}

#endif // LZMA_H
