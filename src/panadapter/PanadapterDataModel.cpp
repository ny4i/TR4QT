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

#include "PanadapterDataModel.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

PanadapterDataModel::PanadapterDataModel(QObject* parent)
    : QObject(parent)
{
}

void PanadapterDataModel::pushPacket(const PanadapterPacket& packet)
{
    QMutexLocker locker(&m_mutex);

    char panId = packet.panId;

    // Update current state
    m_centerFreq[panId] = packet.centerFreqHz;
    m_sampleRate[panId] = packet.sampleRateHz;
    m_noiseFloor[panId] = packet.noiseFloor;

    // Add to queue
    m_queues[panId].enqueue(packet);

    // Drop oldest if queue too deep (prevents memory buildup if renderer is slow)
    while (m_queues[panId].size() > m_maxQueueDepth) {
        m_queues[panId].dequeue();
    }

    locker.unlock();

    emit samplesAvailable(panId);
}

bool PanadapterDataModel::popPacket(char panId, PanadapterPacket& packet)
{
    QMutexLocker locker(&m_mutex);

    if (!m_queues.contains(panId) || m_queues[panId].isEmpty()) {
        return false;
    }

    packet = m_queues[panId].dequeue();
    return true;
}

QVector<PanadapterPacket> PanadapterDataModel::popAllPackets(char panId)
{
    QMutexLocker locker(&m_mutex);

    QVector<PanadapterPacket> packets;

    if (m_queues.contains(panId)) {
        while (!m_queues[panId].isEmpty()) {
            packets.append(m_queues[panId].dequeue());
        }
    }

    return packets;
}

int PanadapterDataModel::availablePackets(char panId) const
{
    QMutexLocker locker(&m_mutex);

    if (!m_queues.contains(panId)) {
        return 0;
    }

    return m_queues[panId].size();
}

qint64 PanadapterDataModel::centerFrequency(char panId) const
{
    QMutexLocker locker(&m_mutex);
    return m_centerFreq.value(panId, 0);
}

int PanadapterDataModel::sampleRate(char panId) const
{
    QMutexLocker locker(&m_mutex);
    return m_sampleRate.value(panId, 48000);
}

float PanadapterDataModel::noiseFloor(char panId) const
{
    QMutexLocker locker(&m_mutex);
    return m_noiseFloor.value(panId, -130.0f);
}

void PanadapterDataModel::clear()
{
    QMutexLocker locker(&m_mutex);
    m_queues.clear();
    m_centerFreq.clear();
    m_sampleRate.clear();
    m_noiseFloor.clear();
}

void PanadapterDataModel::setMaxQueueDepth(int depth)
{
    QMutexLocker locker(&m_mutex);
    m_maxQueueDepth = qMax(1, depth);
}

} // namespace TR4QT
