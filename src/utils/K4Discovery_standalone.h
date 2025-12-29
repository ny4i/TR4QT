#ifndef K4DISCOVERY_STANDALONE_H
#define K4DISCOVERY_STANDALONE_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QTimer>
#include <QList>

/**
 * K4 Radio Information
 *
 * Information about a discovered K4 radio on the network
 */
struct K4RadioInfo {
    QString rigType;      // "k4"
    int rigIndex;         // Radio index (typically 0)
    QString ipAddress;    // IP address (e.g., "192.168.1.100")
    QString serialNumber; // Serial number (e.g., "278")

    QString hostname() const {
        // Generate K4-SN00278.local format hostname
        return QString("K4-SN%1.local").arg(serialNumber.rightJustified(5, '0'));
    }
};

/**
 * K4 Radio Network Discovery
 *
 * Broadcasts UDP discovery messages to find Elecraft K4 radios on the network.
 *
 * Discovery Protocol:
 * - Sends UDP broadcast message "findk4" to port 9100
 * - Creates one socket per network interface, bound to that interface's IP
 * - Listens for responses on the same ephemeral port used for sending
 * - Parses responses in format: "k4:index:ip:serial"
 *
 * Why per-interface sockets?
 * - The K4 responds to the SOURCE IP:PORT of the discovery packet
 * - Binding to a specific interface IP ensures the K4 sees the correct source
 * - Using 0.0.0.0 or generic binding causes responses to be lost
 *
 * Example Usage:
 *   K4Discovery* discovery = new K4Discovery(this);
 *   connect(discovery, &K4Discovery::radioFound, this, &MyClass::onK4Found);
 *   connect(discovery, &K4Discovery::discoveryFinished, this, &MyClass::onDiscoveryDone);
 *   discovery->startDiscovery();
 *
 * Standalone Version:
 * - This is a self-contained version that uses Qt's built-in logging
 * - No external dependencies beyond Qt Core and Qt Network
 * - Drop these two files into any Qt6 project and use immediately
 */
class K4Discovery : public QObject {
    Q_OBJECT

public:
    explicit K4Discovery(QObject* parent = nullptr);
    ~K4Discovery() override;

    /**
     * Start K4 radio discovery
     *
     * Broadcasts discovery messages to all network interfaces
     * and listens for responses for 3 seconds.
     */
    void startDiscovery();

    /**
     * Get list of discovered K4 radios
     */
    QList<K4RadioInfo> discoveredRadios() const { return m_discoveredRadios; }

signals:
    /**
     * Emitted when a K4 radio is discovered
     */
    void radioFound(const K4RadioInfo& radio);

    /**
     * Emitted when discovery process completes
     *
     * @param count Number of K4 radios found
     */
    void discoveryFinished(int count);

    /**
     * Emitted when an error occurs during discovery
     */
    void error(const QString& errorMessage);

private slots:
    void onReadyRead();
    void onDiscoveryTimeout();

private:
    void sendDiscoveryMessage(const QNetworkInterface& interface);
    bool parseK4Response(const QByteArray& data, K4RadioInfo& radioInfo);

    static constexpr int UDP_PORT = 9100;           // K4 listens on this port
    static constexpr int TIMEOUT_MS = 3000;         // 3 second timeout
    static const char* DISCOVERY_MESSAGE;           // "findk4"
    static const char* K4_RESPONSE_PREFIX;          // "k4"

    QList<QUdpSocket*> m_sockets;  // One socket per interface for sending/receiving
    QTimer* m_timeoutTimer;
    QList<K4RadioInfo> m_discoveredRadios;
    bool m_discoveryActive;
};

#endif // K4DISCOVERY_STANDALONE_H
