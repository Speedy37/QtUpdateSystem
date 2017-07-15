#include "xdelta3.h"
extern "C" {
#include "../deps/xdelta/xdelta3/xdelta3.h"
}

inline QString defaultedError(const QString &msg, const QString &def) {
    return msg.isEmpty() ? msg : def;
}

class XDelta3QIODevice : public QIODevice
{
public:
    XDelta3QIODevice(QIODevice *source, QIODevice *base, bool encode, QObject *parent = nullptr);
    virtual ~XDelta3QIODevice();
    bool atEnd() const;
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    bool getSrcBlk();
    static const usize_t BufferSize = 65536;
    QIODevice *_source;
    QIODevice *_base;
    xd3_stream _xd3stream;
    xd3_config _xd3config;
    xd3_source _xd3source;
    quint8 _buffer[BufferSize];
    quint8 _xd3source_buffer[BufferSize];
    int _ret;
    bool _encode;
};


QIODevice *XDelta3(QIODevice *source, QIODevice *base, bool encode, QObject *parent)
{
    return new XDelta3QIODevice(source, base, encode, parent);
}


XDelta3QIODevice::XDelta3QIODevice(QIODevice *source, QIODevice *base, bool encode, QObject *parent /*= nullptr*/) :
    QIODevice(parent), _source(source), _base(base), _encode(encode)
{
    setOpenMode(QIODevice::ReadOnly);
    _ret = XD3_INPUT;
    xd3_init_config(&_xd3config, XD3_ADLER32);
    _xd3config.winsize = BufferSize;

    memset(&_xd3source, 0, sizeof(_xd3source));
    _xd3source.blksize = sizeof(_xd3source_buffer);
    _xd3source.curblk = _xd3source_buffer;
    getSrcBlk();

    memset(&_xd3stream, 0, sizeof(_xd3stream));
    xd3_config_stream(&_xd3stream, &_xd3config);
    xd3_set_source(&_xd3stream, &_xd3source);
}

XDelta3QIODevice::~XDelta3QIODevice()
{
    xd3_close_stream(&_xd3stream);
    xd3_free_stream(&_xd3stream);
}

bool XDelta3QIODevice::atEnd() const
{
    return _ret == XD3_WINFINISH && _source->atEnd();
}

bool XDelta3QIODevice::getSrcBlk()
{
    if (!_base->seek(_xd3source.blksize * _xd3source.getblkno)) {
        setErrorString(defaultedError(_base->errorString(), QStringLiteral("cannot seek")));
        return false;
    }
    qint64 r = _base->read((char *)_xd3source_buffer, sizeof(_xd3source_buffer));
    if (r == -1) {
        setErrorString(defaultedError(_base->errorString(), QStringLiteral("cannot read base")));
        return false;
    }

    _xd3source.onblk = r;
    _xd3source.curblkno = _xd3source.getblkno;
    return true;
}

qint64 XDelta3QIODevice::readData(char *data, qint64 maxlen)
{
    if (_xd3stream.avail_out == 0) {
        do {
            if (_ret == XD3_WINFINISH && _source->atEnd()) // we are done
                break;
            if (_ret == XD3_INPUT) {
                qint64 r = _source->read((char*)_buffer, BufferSize);
                if (r <= 0) {
                    setErrorString(defaultedError(_source->errorString(), QStringLiteral("No more input")));
                    return -1;
                }
                if (_source->atEnd())
                    xd3_set_flags(&_xd3stream, XD3_FLUSH | _xd3stream.flags);
                xd3_avail_input(&_xd3stream, (const uint8_t*)_buffer, r);
            }
            if (_encode)
                _ret = xd3_encode_input(&_xd3stream);
            else
                _ret = xd3_decode_input(&_xd3stream);

            if (_ret == XD3_GETSRCBLK) {
               if (!getSrcBlk())
                   return -1;
            }
            else if (_ret == XD3_INVALID || _ret == XD3_INVALID_INPUT) {
                const char *msg = xd3_errstring(&_xd3stream);
                if (!msg) msg = "unknown error";
                setErrorString(msg);
                return -1;
            }
        } while (_ret != XD3_OUTPUT);
    }

    qint64 s = qMin(maxlen, (qint64)_xd3stream.avail_out);
    memcpy(data, _xd3stream.next_out, s);
    _xd3stream.avail_out -= s;
    return s;
}

qint64 XDelta3QIODevice::writeData(const char *, qint64)
{
    return -1; // todo: throw exception ?
}
