#include "K4Discovery_standalone.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <QLoggingCategory>

// Logging category for K4 discovery messages
// Enable with: QLoggingCategory::setFilterRules("K4Discovery.debug=true");
Q_LOGGING_CATEGORY(k4discovery, "K4Discovery")

const char* K4Discovery::DISCOVERY_MESSAGE = "findk4";
const char* K4Discovery::K4_RESPONSE_PREFIX = "k4";

K4Discovery::K4Discovery(QObject* parent)
    : QObject(parent)
    , m_timeoutTimer(new QTimer(this))
    , m_discoveryActive(false)
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(TIMEOUT_MS);

    connect(m_timeoutTimer, &QTimer::timeout, this, &K4Discovery::onDiscoveryTimeout);
}

K4Discovery::~K4Discovery() {
    // Clean up any active sockets
    for (QUdpSocket* socket : m_sockets) {
        socket->close();
        socket->deleteLater();
    }
    m_sockets.clear();
}

void K4Discovery::startDiscovery() {
    if (m_discoveryActive) {
        qCWarning(k4discovery) << "Discovery already in progress";
        return;
    }

    qCInfo(k4discovery) << "Starting K4 radio discovery...";
    m_discoveredRadios.clear();
    m_discoveryActive = true;

    // Clean up any existing sockets from previous discovery
    for (QUdpSocket* socket : m_sockets) {
        socket->close();
        socket->deleteLater();
    }
    m_sockets.clear();

    // Get all network interfaces
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    int interfaceCount = 0;
    for (const QNetworkInterface& interface : interfaces) {
        // Skip loopback and inactive interfaces
        if (interface.flags() & QNetworkInterface::IsLoopBack) {
            continue;
        }
        if (!(interface.flags() & QNetworkInterface::IsUp)) {
            continue;
        }
        if (!(interface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }

        sendDiscoveryMessage(interface);
        interfaceCount++;
    }

    if (interfaceCount == 0) {
        qCWarning(k4discovery) << "No active network interfaces found";
        emit error("No active network interfaces found");
        m_discoveryActive = false;
        emit discoveryFinished(0);
        return;
    }

    qCInfo(k4discovery) << "Sent discovery messages on" << interfaceCount << "network interface(s)";

    // Start timeout timer
    m_timeoutTimer->start();
}

void K4Discovery::sendDiscoveryMessage(const QNetworkInterface& netInterface) {
    // Get IPv4 addresses for this interface
    QList<QNetworkAddressEntry> entries = netInterface.addressEntries();

    for (const QNetworkAddressEntry& entry : entries) {
        QHostAddress address = entry.ip();

        // Only IPv4
        if (address.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }

        qCDebug(k4discovery) << "Sending discovery via interface" << netInterface.humanReadableName()
                             << "(" << address.toString() << ")";

        // CRITICAL: Create a socket bound to THIS SPECIFIC INTERFACE
        // The K4 responds to the source IP:port of the discovery packet.
        // By binding to the interface's IP, we ensure:
        // 1. The broadcast goes out on THIS interface
        // 2. The K4 sees the correct source IP
        // 3. The K4's response comes back to this socket
        QUdpSocket* socket = new QUdpSocket(this);

        // Bind to this interface's IP with port 0 (OS assigns ephemeral port)
        if (!socket->bind(address, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            qCWarning(k4discovery) << "Failed to bind to interface" << netInterface.humanReadableName()
                                   << "(" << address.toString() << "):" << socket->errorString();
            socket->deleteLater();
            continue;
        }

        quint16 localPort = socket->localPort();
        qCDebug(k4discovery) << "Bound socket to" << address.toString() << ":" << localPort
                             << "for interface" << netInterface.humanReadableName();

        // Connect this socket's readyRead signal to our handler
        connect(socket, &QUdpSocket::readyRead, this, &K4Discovery::onReadyRead);

        // Get the subnet broadcast address (e.g., 192.168.73.255)
        // This is more reliable than 255.255.255.255 for some networks
        QHostAddress broadcastAddr = entry.broadcast();
        if (broadcastAddr.isNull()) {
            // Fallback to global broadcast if subnet broadcast not available
            broadcastAddr = QHostAddress::Broadcast;
        }

        // Send broadcast message
        QByteArray message(DISCOVERY_MESSAGE);
        qint64 bytesSent = socket->writeDatagram(
            message,
            broadcastAddr,
            UDP_PORT
        );

        if (bytesSent == -1) {
            qCWarning(k4discovery) << "Failed to send on interface" << interface.humanReadableName()
                                   << ":" << socket->errorString();
            socket->close();
            socket->deleteLater();
        } else {
            qCDebug(k4discovery) << "Sent" << bytesSent << "bytes from"
                                 << address.toString() << ":" << localPort
                                 << "to" << broadcastAddr.toString() << ":" << UDP_PORT
                                 << "via" << interface.humanReadableName();

            // Keep this socket alive to receive responses
            m_sockets.append(socket);
        }
    }
}

void K4Discovery::onReadyRead() {
    // Figure out which socket triggered this
    QUdpSocket* socket = qobject_cast<QUdpSocket*>(sender());
    if (!socket) {
        qCWarning(k4discovery) << "onReadyRead() called but sender is not a QUdpSocket";
        return;
    }

    qCDebug(k4discovery) << "onReadyRead() triggered on socket"
                         << socket->localAddress().toString() << ":" << socket->localPort()
                         << "- pending datagrams:" << socket->hasPendingDatagrams();

    while (socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = socket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress sender = datagram.senderAddress();
        quint16 senderPort = datagram.senderPort();

        qCDebug(k4discovery) << "Received" << data.size() << "bytes from"
                             << sender.toString() << ":" << senderPort
                             << "on" << socket->localAddress().toString() << ":" << socket->localPort()
                             << "- data:" << QString::fromUtf8(data);

        K4RadioInfo radioInfo;
        if (parseK4Response(data, radioInfo)) {
            // Check if we already found this radio (by IP)
            bool isDuplicate = false;
            for (const K4RadioInfo& existing : m_discoveredRadios) {
                if (existing.ipAddress == radioInfo.ipAddress) {
                    isDuplicate = true;
                    break;
                }
            }

            if (!isDuplicate) {
                m_discoveredRadios.append(radioInfo);
                qCInfo(k4discovery) << "Found K4 serial" << radioInfo.serialNumber
                                    << "at" << radioInfo.ipAddress
                                    << "(" << radioInfo.hostname() << ")";
                emit radioFound(radioInfo);
            }
        } else {
            qCDebug(k4discovery) << "Received non-K4 response from" << sender.toString()
                                 << ":" << QString(data);
        }
    }
}

void K4Discovery::onDiscoveryTimeout() {
    qCInfo(k4discovery) << "Discovery timeout - found" << m_discoveredRadios.count() << "K4 radio(s)";

    // Close all sockets
    for (QUdpSocket* socket : m_sockets) {
        socket->close();
        socket->deleteLater();
    }
    m_sockets.clear();

    m_discoveryActive = false;

    emit discoveryFinished(m_discoveredRadios.count());
}

bool K4Discovery::parseK4Response(const QByteArray& data, K4RadioInfo& radioInfo) {
    // Check if response starts with "k4"
    if (!data.startsWith(K4_RESPONSE_PREFIX)) {
        return false;
    }

    // Parse format: "k4:index:ip:serial"
    QString response = QString::fromUtf8(data);
    QStringList parts = response.split(':');

    if (parts.count() != 4) {
        qCWarning(k4discovery) << "Invalid K4 response format:" << response;
        return false;
    }

    radioInfo.rigType = parts[0];        // "k4"
    radioInfo.rigIndex = parts[1].toInt(); // index (typically 0)
    radioInfo.ipAddress = parts[2];      // IP address
    radioInfo.serialNumber = parts[3];   // Serial number

    return true;
}
