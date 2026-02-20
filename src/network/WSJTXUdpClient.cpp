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

#include "WSJTXUdpClient.h"
#include "../logging/LogMacros.h"

static const char* LOG_TAG = "WSJTXUdpClient";

namespace TR4QT {

WSJTXUdpClient::WSJTXUdpClient(QObject* parent)
    : QObject(parent)
{
    m_heartbeatTimer.setSingleShot(true);
    connect(&m_heartbeatTimer, &QTimer::timeout,
            this, &WSJTXUdpClient::onHeartbeatTimeout);
}

WSJTXUdpClient::~WSJTXUdpClient()
{
    stop();
}

bool WSJTXUdpClient::bind(quint16 port, const QString& multicastGroup)
{
    stop();

    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead,
            this, &WSJTXUdpClient::onReadyRead);

    // ShareAddress + ReuseAddressHint allows multiple listeners on the same port
    if (!m_socket->bind(QHostAddress::AnyIPv4, port,
                         QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        LOG_ERROR(LOG_TAG, QString("Failed to bind UDP port %1: %2")
                  .arg(port).arg(m_socket->errorString()));
        delete m_socket;
        m_socket = nullptr;
        return false;
    }

    // Join multicast group if specified
    if (!multicastGroup.isEmpty()) {
        m_multicastGroup = QHostAddress(multicastGroup);
        if (m_multicastGroup.isMulticast()) {
            if (m_socket->joinMulticastGroup(m_multicastGroup)) {
                m_multicastJoined = true;
                LOG_INFO(LOG_TAG, QString("Joined multicast group %1 on port %2")
                         .arg(multicastGroup).arg(port));
            } else {
                LOG_WARN(LOG_TAG, QString("Failed to join multicast group %1: %2")
                         .arg(multicastGroup).arg(m_socket->errorString()));
            }
        } else {
            LOG_WARN(LOG_TAG, QString("Invalid multicast address: %1").arg(multicastGroup));
        }
    }

    LOG_INFO(LOG_TAG, QString("Listening on UDP port %1").arg(port));
    return true;
}

void WSJTXUdpClient::stop()
{
    m_heartbeatTimer.stop();

    if (m_socket) {
        if (m_multicastJoined) {
            m_socket->leaveMulticastGroup(m_multicastGroup);
            m_multicastJoined = false;
        }
        m_socket->close();
        delete m_socket;
        m_socket = nullptr;
    }

    if (m_connected) {
        m_connected = false;
        emit connectionLost();
    }

    m_wsjtxId.clear();
    m_peerAddress.clear();
    m_peerPort = 0;
}

void WSJTXUdpClient::onReadyRead()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        m_socket->readDatagram(datagram.data(), datagram.size(),
                                &senderAddr, &senderPort);

        // Capture peer address for reply path
        m_peerAddress = senderAddr;
        m_peerPort = senderPort;

        // Parse header
        QDataStream stream(datagram);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader header;
        if (!WSJTXMessageCodec::parseHeader(stream, header))
            continue;

        switch (header.type) {
        case WSJTXMessageType::Heartbeat: {
            WSJTXHeartbeat msg;
            msg.id = header.id;
            if (WSJTXMessageCodec::parseHeartbeat(stream, msg)) {
                m_wsjtxId = msg.id;

                if (!m_connected) {
                    m_connected = true;
                    LOG_INFO(LOG_TAG, QString("WSJT-X connected: id=%1 version=%2")
                             .arg(msg.id, msg.version));
                    emit connectionEstablished(msg.id, msg.version);
                }

                // Reset heartbeat timeout
                m_heartbeatTimer.start(HEARTBEAT_TIMEOUT_MS);
                emit heartbeatReceived(msg);
            }
            break;
        }

        case WSJTXMessageType::Status: {
            WSJTXStatus msg;
            msg.id = header.id;
            if (WSJTXMessageCodec::parseStatus(stream, msg))
                emit statusReceived(msg);
            break;
        }

        case WSJTXMessageType::Decode: {
            WSJTXDecode msg;
            msg.id = header.id;
            if (WSJTXMessageCodec::parseDecode(stream, msg))
                emit decodeReceived(msg);
            break;
        }

        case WSJTXMessageType::QSOLogged: {
            WSJTXQSOLogged msg;
            msg.id = header.id;
            if (WSJTXMessageCodec::parseQSOLogged(stream, msg)) {
                LOG_INFO(LOG_TAG, QString("QSO logged from WSJT-X: %1 on %2 Hz %3")
                         .arg(msg.dxCall)
                         .arg(msg.txFrequency)
                         .arg(msg.mode));
                emit qsoLoggedReceived(msg);
            }
            break;
        }

        case WSJTXMessageType::LoggedADIF: {
            WSJTXLoggedADIF msg;
            msg.id = header.id;
            if (WSJTXMessageCodec::parseLoggedADIF(stream, msg))
                emit loggedAdifReceived(msg);
            break;
        }

        case WSJTXMessageType::Close: {
            LOG_INFO(LOG_TAG, "WSJT-X sent Close message");
            m_heartbeatTimer.stop();
            if (m_connected) {
                m_connected = false;
                emit connectionLost();
            }
            emit closeReceived();
            break;
        }

        default:
            // Unknown message types silently ignored (forward compatibility)
            break;
        }
    }
}

void WSJTXUdpClient::onHeartbeatTimeout()
{
    if (m_connected) {
        LOG_WARN(LOG_TAG, "WSJT-X heartbeat timeout — connection lost");
        m_connected = false;
        emit connectionLost();
    }
}

bool WSJTXUdpClient::sendHighlightCallsign(const QString& callsign,
                                              const QColor& bgColor,
                                              const QColor& fgColor,
                                              bool highlightLast)
{
    if (m_wsjtxId.isEmpty()) return false;

    QByteArray data = WSJTXMessageCodec::buildHighlightCallsign(
        m_wsjtxId, callsign, bgColor, fgColor, highlightLast);
    return sendDatagram(data);
}

bool WSJTXUdpClient::sendClearHighlights()
{
    if (m_wsjtxId.isEmpty()) return false;

    QByteArray data = WSJTXMessageCodec::buildClearHighlights(m_wsjtxId);
    return sendDatagram(data);
}

bool WSJTXUdpClient::sendDatagram(const QByteArray& data)
{
    if (!m_socket || m_peerAddress.isNull() || m_peerPort == 0)
        return false;

    qint64 sent = m_socket->writeDatagram(data, m_peerAddress, m_peerPort);
    return sent == data.size();
}

} // namespace TR4QT
