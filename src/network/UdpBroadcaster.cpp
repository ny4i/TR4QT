#include "UdpBroadcaster.h"
#include "RadioInfo.h"
#include "ContactInfo.h"
#include <QNetworkInterface>
#include <QDebug>

namespace TR4QT {

UdpBroadcaster::UdpBroadcaster(QObject* parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_multicastEnabled(false)
{
}

UdpBroadcaster::~UdpBroadcaster()
{
}

void UdpBroadcaster::setDestinations(const QList<UdpDestination>& destinations)
{
    m_destinations = destinations;
}

void UdpBroadcaster::addDestination(const UdpDestination& dest)
{
    // Avoid duplicates
    for (const auto& existing : m_destinations) {
        if (existing == dest) {
            return;
        }
    }
    m_destinations.append(dest);
}

void UdpBroadcaster::removeDestination(const QString& host, quint16 port)
{
    for (int i = 0; i < m_destinations.size(); ++i) {
        if (m_destinations[i].host == host && m_destinations[i].port == port) {
            m_destinations.removeAt(i);
            return;
        }
    }
}

void UdpBroadcaster::clearDestinations()
{
    m_destinations.clear();
}

void UdpBroadcaster::setMulticastEnabled(bool enabled)
{
    m_multicastEnabled = enabled;
}

void UdpBroadcaster::setMulticastInterface(const QNetworkInterface& iface)
{
    m_socket->setMulticastInterface(iface);
}

bool UdpBroadcaster::sendRadioInfo(const RadioInfo& info)
{
    QByteArray data = info.toXml();
    return sendRawData(data);
}

bool UdpBroadcaster::sendContactInfo(const ContactInfo& info)
{
    QByteArray data = info.toXml();
    return sendRawData(data);
}

bool UdpBroadcaster::sendRawData(const QByteArray& data)
{
    qDebug() << "UdpBroadcaster::sendRawData called, data size:" << data.size()
             << "destinations:" << m_destinations.size();

    if (m_destinations.isEmpty()) {
        m_lastError = "No destinations configured";
        qDebug() << "ERROR: No destinations configured";
        return false;
    }

    int successCount = 0;
    int enabledCount = 0;
    qint64 totalBytesSent = 0;

    // Send to all enabled destinations
    for (const auto& dest : m_destinations) {
        qDebug() << "Checking destination:" << dest.toString() << "enabled:" << dest.enabled;
        if (!dest.enabled) {
            continue;  // Skip disabled destinations
        }
        enabledCount++;

        if (sendToDestination(data, dest)) {
            successCount++;
            totalBytesSent += data.size();
            qDebug() << "Successfully sent" << data.size() << "bytes to" << dest.toString();
        } else {
            qDebug() << "Failed to send to" << dest.toString() << "error:" << m_lastError;
        }
    }

    qDebug() << "Send summary: enabled destinations:" << enabledCount
             << "successful sends:" << successCount;

    // Emit signal if at least one send succeeded
    if (successCount > 0) {
        emit messageSent(data, successCount, totalBytesSent);
        return true;
    }

    m_lastError = "Failed to send to any destination";
    return false;
}

bool UdpBroadcaster::sendToDestination(const QByteArray& data, const UdpDestination& dest)
{
    QHostAddress address(dest.host);

    // Validate multicast address if multicast is enabled
    if (m_multicastEnabled && !isValidMulticastAddress(address)) {
        QString error = QString("Multicast enabled but %1 is not a valid multicast address").arg(dest.host);
        emit sendError(dest.toString(), error);
        m_lastError = error;
        return false;
    }

    // Send datagram
    qint64 bytesSent = m_socket->writeDatagram(data, address, dest.port);

    if (bytesSent == -1) {
        QString error = m_socket->errorString();
        emit sendError(dest.toString(), error);
        m_lastError = error;
        return false;
    }

    if (bytesSent != data.size()) {
        QString error = QString("Partial send: %1 of %2 bytes").arg(bytesSent).arg(data.size());
        emit sendError(dest.toString(), error);
        m_lastError = error;
        return false;
    }

    return true;
}

bool UdpBroadcaster::isValidMulticastAddress(const QHostAddress& address) const
{
    // Multicast IPv4 range: 224.0.0.0 to 239.255.255.255
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        quint32 ip = address.toIPv4Address();
        quint32 multicastStart = QHostAddress("224.0.0.0").toIPv4Address();
        quint32 multicastEnd = QHostAddress("239.255.255.255").toIPv4Address();
        return ip >= multicastStart && ip <= multicastEnd;
    }

    // IPv6 multicast: ff00::/8
    if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        return address.isMulticast();
    }

    return false;
}

} // namespace TR4QT
