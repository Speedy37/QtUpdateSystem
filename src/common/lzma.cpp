#include "lzma.h"
#include <lzma.h>

#include <QIODevice>

void checkLzmaRet(lzma_ret ret);

Lzma::Lzma()
{
    setLevel(7);
}

void checkLzmaRet(lzma_ret ret)
{
    switch (ret)
    {
    case LZMA_OK:
    case LZMA_STREAM_END:
        break;
    case LZMA_MEM_ERROR:
        throw QObject::tr("Memory allocation failed");
        break;
    case LZMA_FORMAT_ERROR:
        throw QObject::tr("The input is not in the .xz format");
        break;
    case LZMA_DATA_ERROR:
        throw QObject::tr("File size limits exceeded or compressed file is corrupt");
        break;
    case LZMA_BUF_ERROR:
        throw QObject::tr("Compressed file is truncated or otherwise corrupt");
        break;
    case LZMA_OPTIONS_ERROR:
        throw QObject::tr("Specified preset is not supported");
        break;
    case LZMA_UNSUPPORTED_CHECK:
        throw QObject::tr("Specified integrity check is not supported");
        break;
    case LZMA_DATA_ERROR:
        throw QObject::tr("Memory allocation failed");
        break;
    default:
        throw QObject::tr("Unknown error, possibly a bug");
        break;
    }
}

void Lzma::loop(QIODevice *input, QIODevice *output)
{
    lzma_action action = LZMA_RUN;

    uint8_t inbuf[BUFSIZ];
    uint8_t outbuf[BUFSIZ];
    qint64 read;

    strm->next_in = NULL;
    strm->avail_in = 0;
    strm->next_out = outbuf;
    strm->avail_out = sizeof(outbuf);

    // Loop until the file has been successfully compressed or until an error occurs.
    while (true)
    {
        // Fill the input buffer if it is empty.
        if (strm->avail_in == 0 && !input->atEnd())
        {
            read = input->read(inbuf, sizeof(inbuf));
            if(read == -1)
                throw QObject::tr("Read error: %1").arg(input->errorString());

            strm->next_in = inbuf;
            strm->avail_in = read;

            if (input->atEnd())
                action = LZMA_FINISH;
        }

        lzma_ret ret = lzma_code(strm, action);

        if (strm->avail_out == 0 || ret == LZMA_STREAM_END)
        {
            size_t write_size = sizeof(outbuf) - strm->avail_out;

            if (output->write(outbuf, write_size) != write_size)
                throw QObject::tr("Write error: %1").arg(input->errorString());

            // Reset next_out and avail_out.
            strm->next_out = outbuf;
            strm->avail_out = sizeof(outbuf);
        }

        checkLzmaRet(ret);
    }
}

bool Lzma::compress(QIODevice *input, QIODevice *output)
{
    m_errorString = QString();

    try
    {
        uint32_t preset = m_level;
        lzma_stream strm = LZMA_STREAM_INIT;

        checkLzmaRet(lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64));

        loop(input, output);

        lzma_end(&strm);

        return true;
    }
    catch(const QString &msg)
    {
        m_errorString = msg;
        lzma_end(&strm);
        return false;
    }
}

void Lzma::decompress(QIODevice *input, QIODevice *output)
{
    m_errorString = QString();

    try
    {
        lzma_stream strm = LZMA_STREAM_INIT;

        checkLzmaRet(lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED));

        loop(input, output);

        lzma_end(&strm);

        return true;
    }
    catch(const QString &msg)
    {
        m_errorString = msg;
        lzma_end(&strm);
        return false;
    }
}




