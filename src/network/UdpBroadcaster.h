#ifndef UDPBROADCASTER_H
#define UDPBROADCASTER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QList>

namespace TR4QT {

class RadioInfo;
class ContactInfo;

/**
 * UDP destination configuration
 */
struct UdpDestination {
    QString host;                   // IP address or hostname
    quint16 port;                   // UDP port
    bool enabled{true};             // Allow per-destination enable/disable

    QString toString() const {
        return QString("%1:%2").arg(host).arg(port);
    }

    bool operator==(const UdpDestination& other) const {
        return host == other.host && port == other.port;
    }
};

/**
 * UdpBroadcaster - UDP multicast/broadcast sender
 *
 * Sends UDP messages to multiple destinations simultaneously.
 * Supports both unicast and multicast addresses.
 *
 * Features:
 * - Multiple simultaneous destinations
 * - Multicast support (224.0.0.0 - 239.255.255.255)
 * - Per-destination enable/disable
 * - Error handling and reporting
 */
class UdpBroadcaster : public QObject {
    Q_OBJECT

public:
    explicit UdpBroadcaster(QObject* parent = nullptr);
    ~UdpBroadcaster() override;

    // Destination management
    void setDestinations(const QList<UdpDestination>& destinations);
    void addDestination(const UdpDestination& dest);
    void removeDestination(const QString& host, quint16 port);
    void clearDestinations();
    QList<UdpDestination> destinations() const { return m_destinations; }

    // Multicast configuration
    void setMulticastEnabled(bool enabled);
    void setMulticastInterface(const QNetworkInterface& iface);
    bool isMulticastEnabled() const { return m_multicastEnabled; }

    // Message sending
    bool sendRadioInfo(const RadioInfo& info);
    bool sendContactInfo(const ContactInfo& info);
    bool sendRawData(const QByteArray& data);

    // Error handling
    QString lastError() const { return m_lastError; }

signals:
    /**
     * Emitted when message successfully sent to all destinations
     * @param data Message data sent
     * @param destinationCount Number of destinations sent to
     * @param bytesSent Total bytes sent (sum across all destinations)
     */
    void messageSent(const QByteArray& data, int destinationCount, qint64 bytesSent);

    /**
     * Emitted when send fails to a specific destination
     * @param destination Host:port string
     * @param error Error message
     */
    void sendError(const QString& destination, const QString& error);

private:
    QUdpSocket* m_socket;
    QList<UdpDestination> m_destinations;
    bool m_multicastEnabled{false};
    QString m_lastError;

    /**
     * Send data to a single destination
     * @return true if successful
     */
    bool sendToDestination(const QByteArray& data, const UdpDestination& dest);

    /**
     * Validate multicast address
     */
    bool isValidMulticastAddress(const QHostAddress& address) const;
};

} // namespace TR4QT

#endif // UDPBROADCASTER_H
