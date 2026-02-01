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

#ifndef PANADAPTERDATAMODEL_H
#define PANADAPTERDATAMODEL_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QMap>
#include "PanadapterTypes.h"

namespace TR4QT {

/**
 * @brief Thread-safe data model for panadapter samples
 *
 * Acts as a buffer between the TCP reader thread and the rendering thread.
 * The reader pushes packets, the renderer pulls them at its own frame rate.
 */
class PanadapterDataModel : public QObject {
    Q_OBJECT

public:
    explicit PanadapterDataModel(QObject* parent = nullptr);
    ~PanadapterDataModel() override = default;

    /**
     * @brief Push a new packet (called from reader thread)
     * @param packet The parsed panadapter packet
     */
    void pushPacket(const PanadapterPacket& packet);

    /**
     * @brief Pop the next packet for a pan ID (called from render thread)
     * @param panId The pan ID ('A', 'B', 'Y', 'Z')
     * @param packet Output packet
     * @return true if a packet was available
     */
    bool popPacket(char panId, PanadapterPacket& packet);

    /**
     * @brief Pop all available packets for a pan ID (for batch processing)
     * @param panId The pan ID
     * @return List of packets (may be empty)
     */
    QVector<PanadapterPacket> popAllPackets(char panId);

    /**
     * @brief Get number of packets waiting for a pan ID
     */
    int availablePackets(char panId) const;

    /**
     * @brief Get the most recent state for a pan ID (without removing from queue)
     */
    qint64 centerFrequency(char panId) const;
    int sampleRate(char panId) const;
    float noiseFloor(char panId) const;

    /**
     * @brief Clear all buffered packets
     */
    void clear();

    /**
     * @brief Set maximum queue depth per pan (oldest dropped when exceeded)
     */
    void setMaxQueueDepth(int depth);

signals:
    /**
     * @brief Emitted when new samples are available
     * @param panId The pan ID that has new data
     */
    void samplesAvailable(char panId);

private:
    static const int DEFAULT_MAX_QUEUE_DEPTH = 10;

    mutable QMutex m_mutex;
    QMap<char, QQueue<PanadapterPacket>> m_queues;
    QMap<char, qint64> m_centerFreq;
    QMap<char, int> m_sampleRate;
    QMap<char, float> m_noiseFloor;
    int m_maxQueueDepth{DEFAULT_MAX_QUEUE_DEPTH};
};

} // namespace TR4QT

#endif // PANADAPTERDATAMODEL_H
