#include "IcomDiscovery.h"
#include "../radio/icompackets.h"
#include "../logging/LogMacros.h"
#include <QNetworkDatagram>
#include <cstring>

IcomDiscovery::IcomDiscovery(QObject *parent)
    : QObject(parent)
    , m_timeoutTimer(nullptr)
    , m_running(false)
{
}

IcomDiscovery::~IcomDiscovery()
{
    cleanup();
}

void IcomDiscovery::startDiscovery()
{
    if (m_running) {
        LOG_WARN("IcomDiscovery", "Discovery already running");
        return;
    }

    m_running = true;
    m_discoveredRadios.clear();
    cleanup();  // Clean up any previous sockets

    LOG_INFO("IcomDiscovery", "Starting Icom radio discovery on all network interfaces");

    // Get all network interfaces
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    bool socketCreated = false;

    for (const QNetworkInterface& netInterface : interfaces) {
        // Skip loopback and inactive interfaces
        if (!(netInterface.flags() & QNetworkInterface::IsUp) ||
            !(netInterface.flags() & QNetworkInterface::IsRunning) ||
            (netInterface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        sendDiscoveryMessage(netInterface);
        socketCreated = true;
    }

    if (!socketCreated) {
        LOG_ERROR("IcomDiscovery", "No active network interfaces found");
        emit error("No active network interfaces found");
        m_running = false;
        return;
    }

    // Start timeout timer (3 seconds)
    m_timeoutTimer = new QTimer(this);
    connect(m_timeoutTimer, &QTimer::timeout, this, &IcomDiscovery::onDiscoveryTimeout);
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->start(DISCOVERY_TIMEOUT_MS);

    LOG_INFO("IcomDiscovery", QString("Broadcast sent on %1 interface(s), waiting %2ms for responses...")
             .arg(m_sockets.size()).arg(DISCOVERY_TIMEOUT_MS));
}

void IcomDiscovery::sendDiscoveryMessage(const QNetworkInterface& netInterface)
{
    // Get IPv4 addresses for this interface
    QList<QNetworkAddressEntry> addressEntries = netInterface.addressEntries();

    for (const QNetworkAddressEntry& entry : addressEntries) {
        if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) {
            continue;  // Skip non-IPv4 addresses
        }

        // Create socket for this interface
        QUdpSocket* socket = new QUdpSocket(this);

        // Bind to a random port (we need to bind to send from this interface)
        if (!socket->bind(entry.ip(), 0)) {
            LOG_WARN("IcomDiscovery", QString("Failed to bind socket on %1: %2")
                     .arg(entry.ip().toString()).arg(socket->errorString()));
            delete socket;
            continue;
        }

        quint16 localPort = socket->localPort();
        quint32 myId = calculateMyId(entry.ip(), localPort);

        LOG_DEBUG("IcomDiscovery", QString("Bound to %1:%2 (interface: %3) myId: 0x%4")
                  .arg(entry.ip().toString())
                  .arg(localPort)
                  .arg(netInterface.humanReadableName())
                  .arg(myId, 8, 16, QChar('0')));

        // Connect readyRead signal
        connect(socket, &QUdpSocket::readyRead, this, &IcomDiscovery::onReadyRead);

        // Build "Are You There" control packet (type 0x03)
        control_packet packet;
        memset(&packet, 0, sizeof(packet));
        packet.len = sizeof(packet);  // 0x10 (16 bytes)
        packet.type = 0x03;            // "Are You There"
        packet.seq = 0;
        packet.sentid = myId;
        packet.rcvdid = 0;             // 0 = broadcast (no specific radio)

        QByteArray data = QByteArray::fromRawData((const char*)&packet, sizeof(packet));

        // Determine broadcast address
        // Use subnet broadcast if available, otherwise global broadcast
        QHostAddress broadcastAddr;
        if (entry.broadcast().isNull() || entry.broadcast() == QHostAddress::Null) {
            broadcastAddr = QHostAddress::Broadcast;  // 255.255.255.255
        } else {
            broadcastAddr = entry.broadcast();  // x.x.x.255
        }

        // Send broadcast packet
        qint64 sent = socket->writeDatagram(data, broadcastAddr, UDP_PORT);
        if (sent == -1) {
            LOG_WARN("IcomDiscovery", QString("Failed to send broadcast on %1: %2")
                     .arg(entry.ip().toString()).arg(socket->errorString()));
            delete socket;
            continue;
        }

        LOG_DEBUG("IcomDiscovery", QString("Sent 'Are You There' broadcast to %1:%2 from %3 (interface: %4)")
                  .arg(broadcastAddr.toString())
                  .arg(UDP_PORT)
                  .arg(entry.ip().toString())
                  .arg(netInterface.humanReadableName()));

        // Add socket to list
        m_sockets.append(socket);
    }
}

void IcomDiscovery::onReadyRead()
{
    QUdpSocket* socket = qobject_cast<QUdpSocket*>(sender());
    if (!socket) return;

    while (socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = socket->receiveDatagram();
        QByteArray data = datagram.data();

        // Check if it's a control packet (minimum size 16 bytes)
        if (data.length() < CONTROL_SIZE) {
            continue;
        }

        const control_packet* ctrl = reinterpret_cast<const control_packet*>(data.constData());

        // Check if it's an "I Am Here" response (type 0x04)
        if (ctrl->type == 0x04) {
            QString radioIP = datagram.senderAddress().toString();
            quint32 radioId = ctrl->sentid;

            LOG_INFO("IcomDiscovery", QString("Found Icom radio at %1 (radioId: 0x%2)")
                     .arg(radioIP)
                     .arg(radioId, 8, 16, QChar('0')));

            // Check if we already discovered this radio (same IP)
            bool alreadyFound = false;
            for (const IcomRadioDiscoveryInfo& existing : m_discoveredRadios) {
                if (existing.ipAddress == radioIP) {
                    alreadyFound = true;
                    break;
                }
            }

            if (!alreadyFound) {
                // Get interface name from socket
                QString interfaceName = socket->localAddress().toString();
                QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
                for (const QNetworkInterface& iface : interfaces) {
                    for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                        if (entry.ip() == socket->localAddress()) {
                            interfaceName = iface.humanReadableName();
                            break;
                        }
                    }
                }

                IcomRadioDiscoveryInfo radio;
                radio.ipAddress = radioIP;
                radio.radioId = radioId;
                radio.networkInterface = interfaceName;

                m_discoveredRadios.append(radio);
                emit radioFound(radio);
            }
        }
    }
}

void IcomDiscovery::onDiscoveryTimeout()
{
    LOG_INFO("IcomDiscovery", QString("Discovery completed: found %1 radio(s)").arg(m_discoveredRadios.size()));

    cleanup();
    m_running = false;
    emit discoveryFinished(m_discoveredRadios.size());
}

void IcomDiscovery::cleanup()
{
    // Stop and delete timeout timer
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }

    // Close and delete all sockets
    for (QUdpSocket* socket : m_sockets) {
        disconnect(socket, nullptr, this, nullptr);  // Disconnect signals
        socket->close();
        socket->deleteLater();
    }
    m_sockets.clear();
}

quint32 IcomDiscovery::calculateMyId(const QHostAddress& localIP, quint16 localPort)
{
    // Same formula as IcomNetwork::calculateMyId()
    // ID format: (IP[2] << 24) | (IP[3] << 16) | (port & 0xffff)
    // Example: 192.168.1.100 port 50001 -> 0x64010001 (wrong)
    // Correct: (168 << 24) | (100 << 16) | 50001 = 0xA8006450 (wrong)
    // Looking at IcomNetwork.cpp line 214:
    // (addr >> 8 & 0xff) << 24 | (addr & 0xff) << 16 | (port & 0xffff)
    // For 192.168.1.100 = 0xC0A80164:
    //   (0xC0A80164 >> 8) & 0xff = 0xA8 (168)
    //   0xC0A80164 & 0xff = 0x64 (100)
    //   So: (168 << 24) | (100 << 16) | port = 0xA8640000 | port

    quint32 addr = localIP.toIPv4Address();
    return (addr >> 8 & 0xff) << 24 | (addr & 0xff) << 16 | (localPort & 0xffff);
}
