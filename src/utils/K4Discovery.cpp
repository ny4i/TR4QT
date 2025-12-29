#include "K4Discovery.h"
#include "../logging/LogMacros.h"
#include <QNetworkDatagram>

namespace TR4QT {

const char* K4Discovery::DISCOVERY_MESSAGE = "findk4";
const char* K4Discovery::K4_RESPONSE_PREFIX = "k4";

K4Discovery::K4Discovery(QObject* parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_timeoutTimer(new QTimer(this))
    , m_discoveryActive(false)
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(TIMEOUT_MS);

    connect(m_socket, &QUdpSocket::readyRead, this, &K4Discovery::onReadyRead);
    connect(m_timeoutTimer, &QTimer::timeout, this, &K4Discovery::onDiscoveryTimeout);
}

K4Discovery::~K4Discovery() {
    if (m_socket) {
        m_socket->close();
    }
}

void K4Discovery::startDiscovery() {
    if (m_discoveryActive) {
        LOG_WARN("K4Discovery", "Discovery already in progress");
        return;
    }

    LOG_INFO("K4Discovery", "Starting K4 radio discovery...");
    m_discoveredRadios.clear();
    m_discoveryActive = true;

    // Bind socket to receive responses
    // Using port 0 lets system choose an available port
    if (!m_socket->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress)) {
        QString errorMsg = QString("Failed to bind UDP socket: %1").arg(m_socket->errorString());
        LOG_ERROR("K4Discovery", errorMsg);
        emit error(errorMsg);
        m_discoveryActive = false;
        return;
    }

    LOG_DEBUG("K4Discovery", QString("Bound to port %1 for receiving").arg(m_socket->localPort()));

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
        LOG_WARN("K4Discovery", "No active network interfaces found");
        emit error("No active network interfaces found");
        m_socket->close();
        m_discoveryActive = false;
        emit discoveryFinished(0);
        return;
    }

    LOG_INFO("K4Discovery", QString("Sent discovery messages on %1 network interface(s)").arg(interfaceCount));

    // Start timeout timer
    m_timeoutTimer->start();
}

void K4Discovery::sendDiscoveryMessage(const QNetworkInterface& interface) {
    // Get IPv4 addresses for this interface
    QList<QNetworkAddressEntry> entries = interface.addressEntries();

    for (const QNetworkAddressEntry& entry : entries) {
        QHostAddress address = entry.ip();

        // Only IPv4
        if (address.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }

        LOG_DEBUG("K4Discovery", QString("Sending discovery via interface %1 (%2)")
            .arg(interface.humanReadableName())
            .arg(address.toString()));

        // CRITICAL FIX: Use the SAME socket (m_socket) for sending that we use for receiving
        // The K4 radio responds back to the source port, so if we create a temporary socket
        // here, it gets destroyed before the K4 can respond, and the response goes to a dead port.
        // By using m_socket (which is already bound and listening), the K4's response comes
        // back to the port we're actually listening on.

        // Send broadcast message using the main socket
        QByteArray message(DISCOVERY_MESSAGE);
        qint64 bytesSent = m_socket->writeDatagram(
            message,
            QHostAddress::Broadcast,
            UDP_PORT
        );

        if (bytesSent == -1) {
            LOG_WARN("K4Discovery", QString("Failed to send on interface %1: %2")
                .arg(interface.humanReadableName())
                .arg(m_socket->errorString()));
        } else {
            LOG_DEBUG("K4Discovery", QString("Sent %1 bytes on interface %2")
                .arg(bytesSent)
                .arg(interface.humanReadableName()));
        }
    }
}

void K4Discovery::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress sender = datagram.senderAddress();

        LOG_DEBUG("K4Discovery", QString("Received %1 bytes from %2")
            .arg(data.size())
            .arg(sender.toString()));

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
                LOG_INFO("K4Discovery", QString("Found K4 serial %1 at %2 (%3)")
                    .arg(radioInfo.serialNumber)
                    .arg(radioInfo.ipAddress)
                    .arg(radioInfo.hostname()));
                emit radioFound(radioInfo);
            }
        } else {
            LOG_DEBUG("K4Discovery", QString("Received non-K4 response from %1: %2")
                .arg(sender.toString())
                .arg(QString(data)));
        }
    }
}

void K4Discovery::onDiscoveryTimeout() {
    LOG_INFO("K4Discovery", QString("Discovery timeout - found %1 K4 radio(s)")
        .arg(m_discoveredRadios.count()));

    m_socket->close();
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
        LOG_WARN("K4Discovery", QString("Invalid K4 response format: %1").arg(response));
        return false;
    }

    radioInfo.rigType = parts[0];        // "k4"
    radioInfo.rigIndex = parts[1].toInt(); // index (typically 0)
    radioInfo.ipAddress = parts[2];      // IP address
    radioInfo.serialNumber = parts[3];   // Serial number

    return true;
}

} // namespace TR4QT
