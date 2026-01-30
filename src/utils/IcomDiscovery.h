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

#ifndef ICOMDISCOVERY_H
#define ICOMDISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QNetworkInterface>
#include <QHostAddress>

/**
 * @brief Information about a discovered Icom radio
 */
struct IcomRadioDiscoveryInfo {
    QString ipAddress;          // Radio IP address
    quint32 radioId;            // Radio ID from "I Am Here" response
    QString networkInterface;   // Network interface where radio was found (e.g., "en0", "Ethernet")
};

/**
 * @brief UDP broadcast discovery for Icom network radios
 *
 * Sends "Are You There" (type 0x03) broadcast packets on all network interfaces
 * and collects "I Am Here" (type 0x04) responses from Icom radios.
 *
 * Usage:
 *   IcomDiscovery* discovery = new IcomDiscovery(this);
 *   connect(discovery, &IcomDiscovery::radioFound, this, &MyClass::onRadioFound);
 *   connect(discovery, &IcomDiscovery::discoveryFinished, this, &MyClass::onDiscoveryFinished);
 *   discovery->startDiscovery();
 */
class IcomDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit IcomDiscovery(QObject *parent = nullptr);
    ~IcomDiscovery();

    /**
     * @brief Start discovery process
     *
     * Sends broadcast packets on all network interfaces and waits 3 seconds
     * for responses. Emits radioFound() for each discovered radio and
     * discoveryFinished() when complete.
     */
    void startDiscovery();

    /**
     * @brief Get list of all discovered radios
     * @return List of discovered Icom radios
     */
    QList<IcomRadioDiscoveryInfo> discoveredRadios() const { return m_discoveredRadios; }

    /**
     * @brief Check if discovery is currently running
     * @return true if discovery is active
     */
    bool isRunning() const { return m_running; }

signals:
    /**
     * @brief Emitted when a radio is discovered
     * @param radio Information about the discovered radio
     */
    void radioFound(const IcomRadioDiscoveryInfo& radio);

    /**
     * @brief Emitted when discovery process completes
     * @param count Number of radios discovered
     */
    void discoveryFinished(int count);

    /**
     * @brief Emitted on discovery errors
     * @param error Error message
     */
    void error(const QString& error);

private slots:
    void onReadyRead();
    void onDiscoveryTimeout();

private:
    void sendDiscoveryMessage(const QNetworkInterface& netInterface);
    void cleanup();
    quint32 calculateMyId(const QHostAddress& localIP, quint16 localPort);

    static constexpr int UDP_PORT = 50001;          // Icom control port
    static constexpr int DISCOVERY_TIMEOUT_MS = 3000; // 3 second timeout

    QList<QUdpSocket*> m_sockets;                   // One socket per interface
    QTimer* m_timeoutTimer;
    QList<IcomRadioDiscoveryInfo> m_discoveredRadios;
    bool m_running;
};

#endif // ICOMDISCOVERY_H
