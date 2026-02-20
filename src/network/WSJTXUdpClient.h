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

#ifndef WSJTXUDPCLIENT_H
#define WSJTXUDPCLIENT_H

#include <QObject>
#include <QHostAddress>
#include <QTimer>
#include <QUdpSocket>
#include "WSJTXMessage.h"

namespace TR4QT {

/**
 * WSJT-X UDP client — listens for and sends WSJT-X protocol messages.
 *
 * Lives on the main thread. UDP readDatagram is microseconds, not blocking I/O,
 * so no worker thread is needed for the socket itself.
 *
 * Captures the sender address/port from the first Heartbeat and uses it as
 * the reply path for HighlightCallsign and other outbound messages.
 */
class WSJTXUdpClient : public QObject {
    Q_OBJECT

public:
    explicit WSJTXUdpClient(QObject* parent = nullptr);
    ~WSJTXUdpClient() override;

    /**
     * Bind to a UDP port, optionally joining a multicast group.
     * @param port UDP port (default 2237)
     * @param multicastGroup Multicast group address (empty = unicast)
     * @return true if bind succeeded
     */
    bool bind(quint16 port, const QString& multicastGroup = QString());

    /**
     * Stop listening and release the socket.
     */
    void stop();

    /**
     * Send a highlight callsign message to WSJT-X.
     * Only works after we've received at least one datagram (know peer address).
     */
    bool sendHighlightCallsign(const QString& callsign,
                                const QColor& bgColor,
                                const QColor& fgColor,
                                bool highlightLast = true);

    /**
     * Send clear-all-highlights to WSJT-X.
     */
    bool sendClearHighlights();

    /**
     * Whether we have a valid connection (received heartbeat within timeout).
     */
    bool isConnected() const { return m_connected; }

    /**
     * The WSJT-X instance ID from the last heartbeat.
     */
    QString wsjtxId() const { return m_wsjtxId; }

signals:
    void heartbeatReceived(const WSJTXHeartbeat& msg);
    void statusReceived(const WSJTXStatus& msg);
    void decodeReceived(const WSJTXDecode& msg);
    void qsoLoggedReceived(const WSJTXQSOLogged& msg);
    void loggedAdifReceived(const WSJTXLoggedADIF& msg);
    void closeReceived();
    void connectionEstablished(const QString& id, const QString& version);
    void connectionLost();

private slots:
    void onReadyRead();
    void onHeartbeatTimeout();

private:
    bool sendDatagram(const QByteArray& data);

    QUdpSocket* m_socket{nullptr};
    QTimer m_heartbeatTimer;

    // Peer address captured from incoming datagrams
    QHostAddress m_peerAddress;
    quint16 m_peerPort{0};

    // WSJT-X instance tracking
    QString m_wsjtxId;
    bool m_connected{false};

    // Multicast state
    QHostAddress m_multicastGroup;
    bool m_multicastJoined{false};

    // Heartbeat timeout: 30s = 2x the 15s heartbeat interval
    static constexpr int HEARTBEAT_TIMEOUT_MS = 30000;
};

} // namespace TR4QT

#endif // WSJTXUDPCLIENT_H
