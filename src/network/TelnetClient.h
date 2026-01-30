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

#ifndef TELNETCLIENT_H
#define TELNETCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QThread>
#include <QTimer>

namespace TR4QT {

/**
 * Telnet client for DX Cluster connections
 *
 * Runs in separate thread to ensure UI is never blocked.
 * Handles connection, data reception, and command sending.
 */
class TelnetClient : public QObject {
    Q_OBJECT

public:
    explicit TelnetClient(QObject* parent = nullptr);
    ~TelnetClient() override;

    bool isConnected() const { return m_isConnected; }
    QString currentServer() const { return m_server; }
    int currentPort() const { return m_port; }

public slots:
    /**
     * Connect to telnet server
     */
    void connectToServer(const QString& server, int port);

    /**
     * Disconnect from server
     */
    void disconnectFromServer();

    /**
     * Send command to server
     */
    void sendCommand(const QString& command);

    /**
     * Set callsign for auto-login
     */
    void setAutoLoginCallsign(const QString& callsign);

signals:
    /**
     * Emitted when connection state changes
     */
    void connected();
    void disconnected();
    void connectionError(const QString& error);

    /**
     * Emitted when data is received from server
     */
    void dataReceived(const QString& data);

    /**
     * Emitted when a DX spot is detected in the data
     */
    void spotReceived(const QString& callsign,
                     double frequency,
                     const QString& spotter,
                     const QString& comment,
                     const QString& timestamp);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onLoginTimeout();

private:
    void parseData(const QString& data);
    bool parseSpotLine(const QString& line);

    QTcpSocket* m_socket;
    QString m_server;
    int m_port;
    bool m_isConnected;
    QString m_dataBuffer;  // Buffer for incomplete lines
    QString m_autoLoginCallsign;  // Callsign for auto-login
    bool m_loginSent;  // Track if we've already sent login
    QTimer* m_loginTimer;  // Timer for auto-login if no prompt received
};

/**
 * Worker thread for telnet client
 *
 * Ensures all network operations run in background thread
 */
class TelnetThread : public QThread {
    Q_OBJECT

public:
    explicit TelnetThread(QObject* parent = nullptr);
    ~TelnetThread() override;

    TelnetClient* client() const { return m_client; }

protected:
    void run() override;

private:
    TelnetClient* m_client;
};

} // namespace TR4QT

#endif // TELNETCLIENT_H
