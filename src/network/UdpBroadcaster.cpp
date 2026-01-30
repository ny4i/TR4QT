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

#include "UdpBroadcaster.h"
#include "RadioInfo.h"
#include "ContactInfo.h"
#include "../logging/LogMacros.h"
#include <QNetworkInterface>

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
    LOG_TRACE("UdpBroadcaster", QString("sendRawData called, data size: %1 destinations: %2")
              .arg(data.size()).arg(m_destinations.size()));

    if (m_destinations.isEmpty()) {
        m_lastError = "No destinations configured";
        LOG_DEBUG("UdpBroadcaster", "No destinations configured");
        return false;
    }

    int successCount = 0;
    int enabledCount = 0;
    qint64 totalBytesSent = 0;

    // Send to all enabled destinations
    for (const auto& dest : m_destinations) {
        LOG_TRACE("UdpBroadcaster", QString("Checking destination: %1 enabled: %2")
                  .arg(dest.toString()).arg(dest.enabled));
        if (!dest.enabled) {
            continue;  // Skip disabled destinations
        }
        enabledCount++;

        if (sendToDestination(data, dest)) {
            successCount++;
            totalBytesSent += data.size();
            LOG_DEBUG("UdpBroadcaster", QString("Sent %1 bytes to %2")
                      .arg(data.size()).arg(dest.toString()));
        } else {
            LOG_WARN("UdpBroadcaster", QString("Failed to send to %1: %2")
                      .arg(dest.toString()).arg(m_lastError));
        }
    }

    LOG_TRACE("UdpBroadcaster", QString("Send summary: enabled destinations: %1 successful sends: %2")
              .arg(enabledCount).arg(successCount));

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
