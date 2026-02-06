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

#ifndef K4PANADAPTERREADER_H
#define K4PANADAPTERREADER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include "../panadapter/PanadapterTypes.h"

namespace TR4QT {

/**
 * @brief TCP client for reading K4 panadapter data stream
 *
 * Connects to the K4 on port 9201 (CAT port + 1) and reads
 * continuous panadapter data packets.
 *
 * This class should be moved to a worker thread for non-blocking operation.
 */
class K4PanadapterReader : public QObject {
    Q_OBJECT

public:
    explicit K4PanadapterReader(QObject* parent = nullptr);
    ~K4PanadapterReader() override;

    /**
     * @brief Check if connected to panadapter port
     */
    bool isConnected() const;

public slots:
    /**
     * @brief Connect to the radio's panadapter port
     * @param host IP address or hostname
     * @param port Port number (default 9201)
     */
    void connectToRadio(const QString& host, int port = 9201);

    /**
     * @brief Disconnect from the radio
     */
    void disconnectFromRadio();

signals:
    /**
     * @brief Emitted when a valid packet is received
     */
    void packetReceived(const TR4QT::PanadapterPacket& packet);

    /**
     * @brief Emitted when connection is established
     */
    void connected();

    /**
     * @brief Emitted when disconnected
     */
    void disconnected();

    /**
     * @brief Emitted on error
     */
    void error(const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onKeepaliveTimer();

private:
    void processBuffer();
    bool validatePacket(const QByteArray& packet);
    PanadapterPacket parsePacket(const QByteArray& packet);
    int findSyncMarker(const QByteArray& data, int startPos = 0);
    quint16 computeCRC16(const QByteArray& data, int length);

    // Sync marker bytes
    static const quint8 SYNC_BYTE_0 = 0xFF;
    static const quint8 SYNC_BYTE_1 = 0xFE;
    static const quint8 SYNC_BYTE_2 = 0x01;
    static const quint8 SYNC_BYTE_3 = 0x00;

    // Keepalive interval (ms)
    static const int KEEPALIVE_INTERVAL_MS = 5000;

    QTcpSocket* m_socket{nullptr};
    QTimer* m_keepaliveTimer{nullptr};
    QByteArray m_buffer;
    QString m_host;
    int m_port{9201};
    qint64 m_lastPacketTime{0};
};

} // namespace TR4QT

#endif // K4PANADAPTERREADER_H
