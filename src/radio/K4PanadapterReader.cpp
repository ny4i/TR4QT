/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "K4PanadapterReader.h"
#include "../logging/LogMacros.h"
#include <QDateTime>

namespace TR4QT {

// CRC-16 lookup table (same as K4LanExample)
static const quint16 crc16_table[16] = {
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

K4PanadapterReader::K4PanadapterReader(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_keepaliveTimer(new QTimer(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &K4PanadapterReader::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &K4PanadapterReader::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &K4PanadapterReader::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &K4PanadapterReader::onError);
    connect(m_keepaliveTimer, &QTimer::timeout, this, &K4PanadapterReader::onKeepaliveTimer);

    m_keepaliveTimer->setInterval(KEEPALIVE_INTERVAL_MS);
}

K4PanadapterReader::~K4PanadapterReader()
{
    disconnectFromRadio();
}

bool K4PanadapterReader::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void K4PanadapterReader::connectToRadio(const QString& host, int port)
{
    if (isConnected()) {
        disconnectFromRadio();
    }

    m_host = host;
    m_port = port;
    m_buffer.clear();

    LOG_INFO("K4PanadapterReader", QString("Connecting to %1:%2").arg(host).arg(port));
    m_socket->connectToHost(host, port);
}

void K4PanadapterReader::disconnectFromRadio()
{
    m_keepaliveTimer->stop();

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }

    m_buffer.clear();
}

void K4PanadapterReader::onConnected()
{
    LOG_INFO("K4PanadapterReader", QString("Connected to %1:%2").arg(m_host).arg(m_port));
    m_lastPacketTime = QDateTime::currentMSecsSinceEpoch();
    m_keepaliveTimer->start();
    emit connected();
}

void K4PanadapterReader::onDisconnected()
{
    LOG_INFO("K4PanadapterReader", "Disconnected");
    m_keepaliveTimer->stop();
    m_buffer.clear();
    emit disconnected();
}

void K4PanadapterReader::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

void K4PanadapterReader::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString errorMsg = m_socket->errorString();
    LOG_ERROR("K4PanadapterReader", QString("Socket error: %1").arg(errorMsg));
    emit error(errorMsg);
}

void K4PanadapterReader::onKeepaliveTimer()
{
    // Check if we've received data recently
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastPacketTime > KEEPALIVE_INTERVAL_MS) {
        // Send ping to check connection (radio ignores incoming data)
        if (m_socket->state() == QAbstractSocket::ConnectedState) {
            m_socket->write("PING");
            m_socket->flush();

            // Check socket state after write
            if (m_socket->state() != QAbstractSocket::ConnectedState) {
                LOG_WARN("K4PanadapterReader", "Connection lost during keepalive");
                emit error("Connection lost");
                disconnectFromRadio();
            }
        }
    }
}

void K4PanadapterReader::processBuffer()
{
    const int PACKET_SIZE = PanadapterPacket::PACKET_SIZE;

    while (m_buffer.size() >= PACKET_SIZE) {
        // Check for sync marker at start
        if (static_cast<quint8>(m_buffer[0]) != SYNC_BYTE_0 ||
            static_cast<quint8>(m_buffer[1]) != SYNC_BYTE_1 ||
            static_cast<quint8>(m_buffer[2]) != SYNC_BYTE_2 ||
            static_cast<quint8>(m_buffer[3]) != SYNC_BYTE_3) {

            // Not in sync - find next sync marker
            int syncPos = findSyncMarker(m_buffer, 1);
            if (syncPos > 0) {
                LOG_DEBUG("K4PanadapterReader", QString("Resync: discarding %1 bytes").arg(syncPos));
                m_buffer.remove(0, syncPos);
            } else {
                // No sync marker found - keep last 3 bytes in case marker spans buffers
                if (m_buffer.size() > 3) {
                    m_buffer.remove(0, m_buffer.size() - 3);
                }
                return;
            }
            continue;
        }

        // Extract packet
        QByteArray packetData = m_buffer.left(PACKET_SIZE);

        // Validate packet
        if (validatePacket(packetData)) {
            PanadapterPacket packet = parsePacket(packetData);
            if (packet.isValid) {
                m_lastPacketTime = QDateTime::currentMSecsSinceEpoch();
                emit packetReceived(packet);
            }
        } else {
            LOG_DEBUG("K4PanadapterReader", "Invalid packet (checksum failed)");
        }

        // Remove processed packet from buffer
        m_buffer.remove(0, PACKET_SIZE);
    }
}

int K4PanadapterReader::findSyncMarker(const QByteArray& data, int startPos)
{
    for (int i = startPos; i < data.size() - 3; ++i) {
        if (static_cast<quint8>(data[i]) == SYNC_BYTE_0 &&
            static_cast<quint8>(data[i + 1]) == SYNC_BYTE_1 &&
            static_cast<quint8>(data[i + 2]) == SYNC_BYTE_2 &&
            static_cast<quint8>(data[i + 3]) == SYNC_BYTE_3) {
            return i;
        }
    }
    return -1;
}

bool K4PanadapterReader::validatePacket(const QByteArray& packet)
{
    // Validate header checksum (sum of first 64 bytes should be 0)
    quint8 headerSum = 0;
    for (int i = 0; i < PanadapterPacket::HEADER_SIZE; ++i) {
        headerSum += static_cast<quint8>(packet[i]);
    }
    if (headerSum != 0) {
        LOG_TRACE("K4PanadapterReader", "Header checksum failed");
        return false;
    }

    // Validate CRC-16 (last 2 bytes)
    quint16 computed = computeCRC16(packet, packet.size() - 2);
    quint8 crcHigh = static_cast<quint8>(packet[packet.size() - 2]);
    quint8 crcLow = static_cast<quint8>(packet[packet.size() - 1]);
    quint16 received = (crcHigh << 8) | crcLow;

    if (computed != received) {
        LOG_TRACE("K4PanadapterReader", QString("CRC failed: computed 0x%1, received 0x%2")
                  .arg(computed, 4, 16, QChar('0')).arg(received, 4, 16, QChar('0')));
        return false;
    }

    return true;
}

quint16 K4PanadapterReader::computeCRC16(const QByteArray& data, int length)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < length; ++i) {
        quint8 c = static_cast<quint8>(data[i]);
        crc = ((crc >> 4) & 0x0FFF) ^ crc16_table[(crc ^ c) & 0x0F];
        c >>= 4;
        crc = ((crc >> 4) & 0x0FFF) ^ crc16_table[(crc ^ c) & 0x0F];
    }

    return ~crc & 0xFFFF;
}

PanadapterPacket K4PanadapterReader::parsePacket(const QByteArray& packet)
{
    PanadapterPacket result;

    // Version (byte 4)
    int version = static_cast<quint8>(packet[4]);
    if (version != 2) {
        LOG_WARN("K4PanadapterReader", QString("Unexpected version: %1").arg(version));
    }

    // Sequence number (byte 5)
    result.sequenceNumber = static_cast<quint8>(packet[5]);

    // Pan ID (byte 6)
    result.panId = packet[6];

    // Sample rate (bytes 7-12, ASCII)
    QString sampleRateStr = QString::fromLatin1(packet.mid(7, 6)).trimmed();
    bool ok;
    result.sampleRateHz = sampleRateStr.toInt(&ok);
    if (!ok) {
        LOG_WARN("K4PanadapterReader", QString("Failed to parse sample rate: '%1'").arg(sampleRateStr));
        result.sampleRateHz = 48000;
    }

    // Center frequency (bytes 13-23, ASCII)
    QString centerFreqStr = QString::fromLatin1(packet.mid(13, 11)).trimmed();
    result.centerFreqHz = centerFreqStr.toLongLong(&ok);
    if (!ok) {
        LOG_WARN("K4PanadapterReader", QString("Failed to parse center freq: '%1'").arg(centerFreqStr));
        result.centerFreqHz = 0;
    }

    // Noise floor / AutoRef (bytes 24-28, ASCII, value * 10)
    QString noiseFloorStr = QString::fromLatin1(packet.mid(24, 5)).trimmed();
    int noiseFloorRaw = noiseFloorStr.toInt(&ok);
    if (ok) {
        result.noiseFloor = noiseFloorRaw / 10.0f;
    } else {
        LOG_WARN("K4PanadapterReader", QString("Failed to parse noise floor: '%1'").arg(noiseFloorStr));
        result.noiseFloor = -130.0f;
    }

    // Parse samples (bytes 64-4159, 2048 x 16-bit big-endian)
    result.samples.resize(PanadapterPacket::SAMPLE_COUNT);
    const int payloadStart = PanadapterPacket::HEADER_SIZE;

    for (int i = 0; i < PanadapterPacket::SAMPLE_COUNT; ++i) {
        int offset = payloadStart + (i * 2);
        qint16 rawValue = (static_cast<quint8>(packet[offset]) << 8) |
                          static_cast<quint8>(packet[offset + 1]);
        result.samples[i] = rawValue / 10.0f;
    }

    result.isValid = true;
    return result;
}

} // namespace TR4QT
