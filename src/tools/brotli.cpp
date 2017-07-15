#include "brotli.h"
#include <QIODevice>
#include <brotli/decode.h>
#include <brotli/encode.h>

class BrotliCompressorQIODevice : public QIODevice
{
public:
    BrotliCompressorQIODevice(QIODevice *source, quint32 quality = 9, quint32 lgwin = 0, QObject *parent = nullptr);
    virtual ~BrotliCompressorQIODevice();
    bool atEnd() const;
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    static const int BufferSize = 65536;
    BrotliEncoderState *_state;
    QIODevice *_source;
    quint8 _buffer[BufferSize];
    const uint8_t *_next_in;
    size_t _available_in;
    BrotliEncoderOperation _op;
};

class BrotliDecompressorQIODevice : public QIODevice
{
public:
    BrotliDecompressorQIODevice(QIODevice *source, QObject *parent = nullptr);
    virtual ~BrotliDecompressorQIODevice();
    bool atEnd() const;
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    static const int BufferSize = 65536;
    BrotliDecoderState *_state;
    BrotliDecoderResult _result;
    QIODevice *_source;
    quint8 _buffer[BufferSize];
    const uint8_t *_next_in;
    size_t _available_in;

};

QIODevice * BrotliCompressor(QIODevice *source, quint32 quality, quint32 lgwin, QObject *parent)
{
    return new BrotliCompressorQIODevice(source, quality, lgwin, parent);
}

QIODevice * BrotliDecompressor(QIODevice *source, QObject *parent)
{
    return new BrotliDecompressorQIODevice(source, parent);
}

BrotliCompressorQIODevice::BrotliCompressorQIODevice(QIODevice *source, quint32 quality /*= 9*/, quint32 lgwin /*= 0*/, QObject *parent /*= nullptr*/) : QIODevice(parent)
{
	setOpenMode(QIODevice::ReadOnly);
	_state = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
	_source = source;
	_available_in = 0;
    _op = BROTLI_OPERATION_PROCESS;
	BrotliEncoderSetParameter(_state, BROTLI_PARAM_QUALITY, quality);
	BrotliEncoderSetParameter(_state, BROTLI_PARAM_LGWIN, lgwin);
}

BrotliCompressorQIODevice::~BrotliCompressorQIODevice()
{
	BrotliEncoderDestroyInstance(_state);
}

bool BrotliCompressorQIODevice::atEnd() const
{
	return BrotliEncoderIsFinished(_state);
}

qint64 BrotliCompressorQIODevice::readData(char *data, qint64 maxlen)
{
    qint64 len = 0;
    do {
        if (_available_in == 0 && _op != BROTLI_OPERATION_FINISH) {
            qint64 r = _source->read((char*)_buffer, BufferSize);
            if (r == -1) {
                this->setErrorString(_source->errorString());
                return -1;
            }
            _available_in = (size_t)r;
            _next_in = _buffer;
            if (_source->atEnd())
                _op = BROTLI_OPERATION_FINISH;
        }

        size_t available_out = maxlen;
        uint8_t* next_out = (uint8_t*)data;
        if (!BrotliEncoderCompressStream(this->_state, _op, &_available_in, &_next_in, &available_out, &next_out, nullptr)) {
            this->setErrorString(QStringLiteral("failed to compress data"));
            return -1;
        }
        len = maxlen - available_out;
    } while (len == 0 && !BrotliEncoderIsFinished(_state));
    return len;
}

qint64 BrotliCompressorQIODevice::writeData(const char *, qint64)
{
    return -1; // todo: throw exception ?
}

BrotliDecompressorQIODevice::BrotliDecompressorQIODevice(QIODevice *source, QObject *parent /*= nullptr*/) : QIODevice(parent)
{
	setOpenMode(QIODevice::ReadOnly);
	_state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
	_result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
	_source = source;
	_available_in = 0;
}

BrotliDecompressorQIODevice::~BrotliDecompressorQIODevice()
{
	BrotliDecoderDestroyInstance(_state);
}

bool BrotliDecompressorQIODevice::atEnd() const
{
	return BrotliDecoderIsFinished(_state);
}

qint64 BrotliDecompressorQIODevice::readData(char *data, qint64 maxlen)
{
	size_t available_out = maxlen;
	uint8_t* next_out = (uint8_t*)data;
	do {
		if (_result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
			if (this->_source->atEnd()) {
				this->setErrorString(QStringLiteral("corrupt input"));
                return -1;
			}
			_available_in = _source->read((char*)_buffer, BufferSize);
			_next_in = _buffer;
		}
		_result = BrotliDecoderDecompressStream(_state, &_available_in, &_next_in, &available_out, &next_out, nullptr);
	} while (_result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT);
	return maxlen - available_out;
}

qint64 BrotliDecompressorQIODevice::writeData(const char *, qint64)
{
    return -1; // todo: throw exception ?
}
