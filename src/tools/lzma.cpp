#include "lzma.h"
#include <QIODevice>
#include "../deps/xz/src/liblzma/api/lzma.h"

class LZMACompressorQIODevice : public QIODevice
{
public:
    LZMACompressorQIODevice(QIODevice *source, quint32 preset = 9, QObject *parent = nullptr);
    virtual ~LZMACompressorQIODevice();
    bool atEnd() const;
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    static const int BufferSize = 65536;
    lzma_stream _strm;
    lzma_ret _ret;
    lzma_action _action;
    quint8 _buffer[BufferSize];
    QIODevice *_source;
};

class LZMADecompressorQIODevice : public QIODevice
{
public:
    LZMADecompressorQIODevice(QIODevice *source, QObject *parent = nullptr);
    virtual ~LZMADecompressorQIODevice();
    bool atEnd() const;
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    static const int BufferSize = 65536;
    lzma_stream _strm;
    lzma_ret _ret;
    lzma_action _action;
    quint8 _buffer[BufferSize];
    QIODevice *_source;
};

QIODevice * LZMACompressor(QIODevice *source, quint32 preset, QObject *parent)
{
    return new LZMACompressorQIODevice(source, preset, parent);
}

QIODevice * LZMADecompressor(QIODevice *source, QObject *parent)
{
    return new LZMADecompressorQIODevice(source, parent);
}

static QString message_strm(lzma_ret code)
{
    switch (code) {
    case LZMA_NO_CHECK:
        return QStringLiteral("No integrity check; not verifying file integrity");

    case LZMA_UNSUPPORTED_CHECK:
        return QStringLiteral("Unsupported type of integrity check; "
            "not verifying file integrity");

    case LZMA_MEM_ERROR:
        return QStringLiteral("Memory error");

    case LZMA_MEMLIMIT_ERROR:
        return QStringLiteral("Memory usage limit reached");

    case LZMA_FORMAT_ERROR:
        return QStringLiteral("File format not recognized");

    case LZMA_OPTIONS_ERROR:
        return QStringLiteral("Unsupported options");

    case LZMA_DATA_ERROR:
        return QStringLiteral("Compressed data is corrupt");

    case LZMA_BUF_ERROR:
        return QStringLiteral("Unexpected end of input");

    case LZMA_OK:
    case LZMA_STREAM_END:
    case LZMA_GET_CHECK:
    case LZMA_PROG_ERROR:
        // Without "default", compiler will warn if new constants
        // are added to lzma_ret, it is not too easy to forget to
        // add the new constants to this function.
        break;
    }

    return QStringLiteral("Internal error (bug)");
}

LZMACompressorQIODevice::LZMACompressorQIODevice(QIODevice *source, quint32 preset /*= 9*/, QObject *parent /*= nullptr*/) : QIODevice(parent)
{
    setOpenMode(QIODevice::ReadOnly);
    _strm = LZMA_STREAM_INIT;
    _ret = lzma_easy_encoder(&_strm, preset, LZMA_CHECK_CRC64);
    _source = source;
    _action = LZMA_RUN;
    if (_ret != LZMA_OK)
        setErrorString(message_strm(_ret));
}

LZMACompressorQIODevice::~LZMACompressorQIODevice()
{
    lzma_end(&_strm);
}

bool LZMACompressorQIODevice::atEnd() const
{
    return _ret == LZMA_STREAM_END;
}

qint64 LZMACompressorQIODevice::readData(char *data, qint64 maxlen)
{
    if (_strm.avail_in == 0 && _action != LZMA_FINISH) {
        qint64 r = _source->read((char*)_buffer, BufferSize);
        if (r == -1) {
            this->setErrorString(_source->errorString());
            return -1;
        }
        _strm.avail_in = (size_t)r;
        _strm.next_in = _buffer;
        if (_source->atEnd())
            _action = LZMA_FINISH;
    }

    _strm.avail_out = maxlen;
    _strm.next_out = (uint8_t*)data;
    _ret = lzma_code(&_strm, _action);
    if (_ret != LZMA_OK && _ret != LZMA_STREAM_END) {
        this->setErrorString(message_strm(_ret));
        return -1;
    }
    return maxlen - _strm.avail_out;
}

qint64 LZMACompressorQIODevice::writeData(const char *, qint64)
{
    return -1; // todo: throw exception ?
}

LZMADecompressorQIODevice::LZMADecompressorQIODevice(QIODevice *source, QObject *parent /*= nullptr*/) : QIODevice(parent)
{
    setOpenMode(QIODevice::ReadOnly);
    _strm = LZMA_STREAM_INIT;
    _ret = lzma_alone_decoder(&_strm, UINT64_MAX);
    _source = source;
    _action = LZMA_RUN;
    if (_ret != LZMA_OK)
        setErrorString(message_strm(_ret));
}

LZMADecompressorQIODevice::~LZMADecompressorQIODevice()
{
    lzma_end(&_strm);
}

bool LZMADecompressorQIODevice::atEnd() const
{
    return _ret == LZMA_STREAM_END;
}

qint64 LZMADecompressorQIODevice::readData(char *data, qint64 maxlen)
{
    _strm.avail_out = maxlen;
    _strm.next_out = (uint8_t*)data;
    while (_ret == LZMA_OK && _strm.avail_out > 0) {
        if (_strm.avail_in == 0) {
            if (_source->atEnd()) {
                setErrorString(QStringLiteral("corrupt input"));
                return -1;
            }
            _strm.avail_in = _source->read((char*)_buffer, BufferSize);
            _strm.next_in = _buffer;
            if (_source->atEnd())
                _action = LZMA_FINISH;
        }
        _ret = lzma_code(&_strm, _action);
    }
    if (_ret != LZMA_OK && _ret != LZMA_STREAM_END) {
        this->setErrorString(message_strm(_ret));
        return -1;
    }
    return maxlen - _strm.avail_out;
}

qint64 LZMADecompressorQIODevice::writeData(const char *, qint64)
{
    return -1; // todo: throw exception ?
}

