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

#include "TelnetClient.h"
#include "../logging/LogMacros.h"
#include <QRegularExpression>
#include <QDateTime>
#include <hamlib/rig.h>

namespace TR4QT {

TelnetClient::TelnetClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_port(0)
    , m_isConnected(false)
    , m_loginSent(false)
    , m_loginTimer(new QTimer(this))
{
    connect(m_socket, &QTcpSocket::connected,
            this, &TelnetClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TelnetClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TelnetClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &TelnetClient::onError);

    // Setup login timer (4 seconds)
    m_loginTimer->setSingleShot(true);
    m_loginTimer->setInterval(4000);
    connect(m_loginTimer, &QTimer::timeout,
            this, &TelnetClient::onLoginTimeout);
}

TelnetClient::~TelnetClient() {
    if (m_isConnected) {
        disconnectFromServer();
    }
}

void TelnetClient::connectToServer(const QString& server, int port) {
    if (m_isConnected) {
        disconnectFromServer();
    }

    m_server = server;
    m_port = port;
    m_dataBuffer.clear();
    m_loginSent = false;  // Reset login flag for new connection

    m_socket->connectToHost(server, port);
}

void TelnetClient::disconnectFromServer() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_isConnected = false;
}

void TelnetClient::sendCommand(const QString& command) {
    if (!m_isConnected) {
        return;
    }

    QString cmd = command.trimmed() + "\r\n";
    m_socket->write(cmd.toUtf8());
    m_socket->flush();
}

void TelnetClient::setAutoLoginCallsign(const QString& callsign) {
    m_autoLoginCallsign = callsign.trimmed().toUpper();
    LOG_DEBUG("TelnetClient", QString("Auto-login callsign set to: %1").arg(m_autoLoginCallsign));
}

void TelnetClient::onConnected() {
    m_isConnected = true;
    emit connected();

    // Start login timer if auto-login is configured
    if (!m_autoLoginCallsign.isEmpty()) {
        LOG_DEBUG("TelnetClient", "Starting 4-second login timer");
        m_loginTimer->start();
    }
}

void TelnetClient::onDisconnected() {
    m_isConnected = false;
    m_loginTimer->stop();  // Stop login timer on disconnect
    emit disconnected();
}

void TelnetClient::onReadyRead() {
    // Read all available data
    QByteArray data = m_socket->readAll();
    QString text = QString::fromUtf8(data);

    // Add to buffer
    m_dataBuffer += text;

    // Process complete lines
    QStringList lines = m_dataBuffer.split('\n');

    // Keep last incomplete line in buffer
    m_dataBuffer = lines.takeLast();

    // Process each complete line
    for (const QString& line : lines) {
        QString cleanLine = line.trimmed();
        if (!cleanLine.isEmpty()) {
            // Emit raw data for display
            emit dataReceived(cleanLine);

            // Check for login prompt and auto-login if configured
            if (!m_loginSent && !m_autoLoginCallsign.isEmpty()) {
                // Common login prompts from various DX Cluster software versions
                // AR-Cluster v6: "Enter your callsign"
                // CC-Cluster: "Please enter your call"
                // DXSpider: "login:" or "Please login with"
                // TODO: Add support for other cluster server types (v5, v4, etc.)
                //       and their specific login prompts/sequences
                if (cleanLine.contains("Enter your callsign", Qt::CaseInsensitive) ||
                    cleanLine.contains("Please enter your call", Qt::CaseInsensitive) ||
                    cleanLine.contains("login:", Qt::CaseInsensitive) ||
                    cleanLine.contains("Please login", Qt::CaseInsensitive)) {

                    LOG_DEBUG("TelnetClient", QString("Login prompt detected, sending callsign: %1").arg(m_autoLoginCallsign));
                    m_loginTimer->stop();  // Stop timeout timer
                    sendCommand(m_autoLoginCallsign);
                    m_loginSent = true;
                }
            }

            // Try to parse as DX spot
            parseSpotLine(cleanLine);
        }
    }
}

void TelnetClient::onError(QAbstractSocket::SocketError error) {
    QString errorMsg;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorMsg = "Connection refused";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorMsg = "Remote host closed connection";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorMsg = "Host not found";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorMsg = "Connection timeout";
        break;
    case QAbstractSocket::NetworkError:
        errorMsg = "Network error";
        break;
    default:
        errorMsg = m_socket->errorString();
        break;
    }

    emit connectionError(errorMsg);
    m_isConnected = false;
}

void TelnetClient::onLoginTimeout() {
    // If no login prompt received after 4 seconds, send callsign anyway
    if (!m_loginSent && !m_autoLoginCallsign.isEmpty()) {
        LOG_DEBUG("TelnetClient", QString("No login prompt received after 4 seconds, sending callsign anyway: %1").arg(m_autoLoginCallsign));
        sendCommand(m_autoLoginCallsign);
        m_loginSent = true;
    }
}

bool TelnetClient::parseSpotLine(const QString& line) {
    // DX Cluster spot format:
    // AR-Cluster:  DX de W1AW:     14025.0  DL1ABC        CQ                      2345 Z
    // CC Cluster:  DX de NG7M-#:    14047.0  W1ND         CW 17 dB 28 WPM CQ           K 2019Z K
    // DXSpider:    DX de WX7V/5-#:   7032.0  G7MVH        CW 18 dB 15 WPM CQ             0433Z

    // Regex pattern for DX spots (handles variable spacing and optional location codes)
    // Allows /, -, # in callsigns
    static QRegularExpression spotRegex(
        R"(^DX\s+de\s+([A-Z0-9/\-#]+):\s+([0-9.]+)\s+([A-Z0-9/\-]+)\s+(.+?)\s+(?:[A-Z]+\s+)?(\d{4})Z?)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match = spotRegex.match(line);
    if (match.hasMatch()) {
        QString spotter = match.captured(1);
        double frequency = match.captured(2).toDouble();
        QString callsign = match.captured(3);
        QString comment = match.captured(4).trimmed();
        QString time = match.captured(5);

        // Convert frequency to Hz (input is in kHz)
        freq_t freqHz = static_cast<freq_t>(frequency * 1000);

        // Emit frequency in Hz (cast to double for signal compatibility)
        emit spotReceived(callsign, static_cast<double>(freqHz), spotter, comment, time);
        return true;
    }

    return false;
}

// TelnetThread implementation

TelnetThread::TelnetThread(QObject* parent)
    : QThread(parent)
    , m_client(nullptr)
{
}

TelnetThread::~TelnetThread() {
    if (isRunning()) {
        quit();
        wait();
    }
}

void TelnetThread::run() {
    // Create client in this thread
    m_client = new TelnetClient();

    // Run event loop
    exec();

    // Cleanup
    delete m_client;
    m_client = nullptr;
}

} // namespace TR4QT
