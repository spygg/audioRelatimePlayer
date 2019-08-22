#include "pcmstream.h"
#include <QDebug>

PcmStream::PcmStream()
{
    m_pos = 0;
}

void PcmStream::start()
{
    open(QIODevice::ReadOnly);
}

void PcmStream::stop()
{
    m_pos = 0;
    close();
}

qint64 PcmStream::readData(char *data, qint64 len)
{
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (len - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            total += chunk;
        }

    }


    return total;
}

qint64 PcmStream::setData(const char *data, qint64 len)
{
    qDebug() << "setData" << len;
    m_buffer.append(data, len);
    return len;
}

qint64 PcmStream::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return  0;
}

qint64 PcmStream::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}
