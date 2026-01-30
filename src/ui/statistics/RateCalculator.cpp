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

#include "RateCalculator.h"
#include <algorithm>

namespace TR4QT {

RateCalculator::RateCalculator(QObject* parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
{
    connect(m_updateTimer, &QTimer::timeout, this, &RateCalculator::onUpdateTimer);
}

void RateCalculator::setContestStartTime(const QDateTime& startTime) {
    m_contestStartTime = startTime;
}

void RateCalculator::addQso(const QDateTime& timestamp,
                            BandType band,
                            ModeType mode,
                            const QString& operatorCall,
                            const QString& stationId) {
    // Add to timestamp cache
    m_timestamps.append(timestamp);

    // Keep cache bounded
    if (m_timestamps.size() > MAX_CACHED_TIMESTAMPS) {
        m_timestamps.removeFirst();
    }

    // Add to detailed records
    QsoRecord record;
    record.timestamp = timestamp;
    record.band = band;
    record.mode = mode;
    record.operatorCall = operatorCall;
    record.stationId = stationId.isEmpty() ? "RADIO1" : stationId;
    m_qsoRecords.append(record);

    // Keep detailed records bounded
    if (m_qsoRecords.size() > MAX_CACHED_TIMESTAMPS) {
        m_qsoRecords.removeFirst();
    }

    // Update aggregations
    updateBandModeStats();
    updateOperatorStats();
    updateStationStats();

    // Emit updated rates
    emit ratesUpdated(getCurrentSnapshot());
}

void RateCalculator::clear() {
    m_timestamps.clear();
    m_qsoRecords.clear();
    m_bandModeStats.clear();
    m_operatorStats.clear();
    m_stationStats.clear();
    m_bestHourlyRates.clear();
    m_contestStartTime = QDateTime();
}

void RateCalculator::loadHistory(const QVector<QDateTime>& timestamps) {
    m_timestamps = timestamps;

    // Sort by time
    std::sort(m_timestamps.begin(), m_timestamps.end());

    // Trim to max size (keep most recent)
    while (m_timestamps.size() > MAX_CACHED_TIMESTAMPS) {
        m_timestamps.removeFirst();
    }

    emit ratesUpdated(getCurrentSnapshot());
}

void RateCalculator::loadDetailedHistory(const QVector<QsoRecord>& records) {
    m_qsoRecords = records;
    m_timestamps.clear();

    // Sort by time
    std::sort(m_qsoRecords.begin(), m_qsoRecords.end(),
              [](const QsoRecord& a, const QsoRecord& b) {
                  return a.timestamp < b.timestamp;
              });

    // Extract timestamps and trim
    for (const auto& record : m_qsoRecords) {
        m_timestamps.append(record.timestamp);
    }

    while (m_timestamps.size() > MAX_CACHED_TIMESTAMPS) {
        m_timestamps.removeFirst();
        m_qsoRecords.removeFirst();
    }

    // Update all aggregations
    updateBandModeStats();
    updateOperatorStats();
    updateStationStats();

    emit ratesUpdated(getCurrentSnapshot());
    emit bandModeStatsUpdated(getBandModeStats());
}

int RateCalculator::calculate10QsoRate() const {
    return calculateSlidingWindowRate(10);
}

int RateCalculator::calculate100QsoRate() const {
    return calculateSlidingWindowRate(100);
}

int RateCalculator::calculateSlidingWindowRate(int qsoCount) const {
    if (m_timestamps.size() < qsoCount) {
        // Not enough QSOs yet - use all available
        if (m_timestamps.size() < 2) {
            return 0;
        }
        qsoCount = m_timestamps.size();
    }

    // Get timestamps for the window
    const QDateTime& oldest = m_timestamps[m_timestamps.size() - qsoCount];
    const QDateTime& newest = m_timestamps.last();

    // Calculate time span in seconds
    qint64 spanSeconds = oldest.secsTo(newest);
    if (spanSeconds <= 0) {
        return 0;
    }

    // Extrapolate to hourly rate
    // Rate = (qsoCount - 1) / spanSeconds * 3600
    // Using (qsoCount - 1) because the span is between first and last QSO
    double rate = static_cast<double>(qsoCount - 1) / spanSeconds * 3600.0;
    return static_cast<int>(std::round(rate));
}

int RateCalculator::calculateLastNMinutesRate(int minutes) const {
    if (m_timestamps.isEmpty()) {
        return 0;
    }

    QDateTime cutoff = QDateTime::currentDateTimeUtc().addSecs(-minutes * 60);
    int count = 0;

    // Count QSOs after cutoff (iterate backwards for efficiency)
    for (int i = m_timestamps.size() - 1; i >= 0; --i) {
        if (m_timestamps[i] >= cutoff) {
            ++count;
        } else {
            break;  // Sorted, so we can stop
        }
    }

    // Return actual count (not extrapolated)
    // If caller wants hourly extrapolation: count * (60 / minutes)
    return count;
}

int RateCalculator::calculateThisClockHourRate() const {
    return calculateThisClockHourCount();
}

int RateCalculator::calculateThisClockHourCount() const {
    if (m_timestamps.isEmpty()) {
        return 0;
    }

    // Get start of current clock hour
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime hourStart(now.date(), QTime(now.time().hour(), 0, 0), Qt::UTC);

    int count = 0;
    for (int i = m_timestamps.size() - 1; i >= 0; --i) {
        if (m_timestamps[i] >= hourStart) {
            ++count;
        } else {
            break;
        }
    }

    return count;
}

int RateCalculator::calculateAverageRate() const {
    if (m_timestamps.isEmpty() || !m_contestStartTime.isValid()) {
        return 0;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    qint64 totalSeconds = m_contestStartTime.secsTo(now);

    if (totalSeconds <= 0) {
        return 0;
    }

    // Rate = totalQsos / totalHours
    double hours = static_cast<double>(totalSeconds) / 3600.0;
    return static_cast<int>(std::round(m_timestamps.size() / hours));
}

RateSnapshot RateCalculator::getCurrentSnapshot() const {
    RateSnapshot snapshot;
    snapshot.qso10Rate = calculate10QsoRate();
    snapshot.qso100Rate = calculate100QsoRate();
    snapshot.last60MinRate = calculateLastNMinutesRate(60);
    snapshot.thisHourCount = calculateThisClockHourCount();
    snapshot.thisHourRate = snapshot.thisHourCount;  // Same for now
    snapshot.averageRate = calculateAverageRate();
    snapshot.totalQsos = m_timestamps.size();
    snapshot.timestamp = QDateTime::currentDateTimeUtc();
    return snapshot;
}

QVector<HourlyRatePoint> RateCalculator::getHourlyRateHistory(int hours) const {
    QVector<HourlyRatePoint> history;

    if (m_timestamps.isEmpty()) {
        return history;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime currentHourStart(now.date(), QTime(now.time().hour(), 0, 0), Qt::UTC);

    // Go back 'hours' hours
    for (int h = hours - 1; h >= 0; --h) {
        QDateTime hourStart = currentHourStart.addSecs(-h * 3600);
        QDateTime hourEnd = hourStart.addSecs(3600);

        HourlyRatePoint point;
        point.hourStart = hourStart;
        point.qsoCount = 0;

        // Count QSOs in this hour
        for (const auto& ts : m_timestamps) {
            if (ts >= hourStart && ts < hourEnd) {
                ++point.qsoCount;
            }
        }

        point.rate = point.qsoCount;  // Hourly rate = count for 1-hour bucket
        history.append(point);
    }

    return history;
}

QVector<BandModeStats> RateCalculator::getBandModeStats() const {
    return m_bandModeStats.values().toVector();
}

QVector<OperatorStats> RateCalculator::getOperatorStats() const {
    return m_operatorStats.values().toVector();
}

QVector<StationStats> RateCalculator::getStationStats() const {
    return m_stationStats.values().toVector();
}

void RateCalculator::setTimeBucket(int minutes) {
    if (minutes == 20 || minutes == 30 || minutes == 60) {
        m_timeBucketMinutes = minutes;
    }
}

void RateCalculator::setIncludeDupes(bool include) {
    if (m_includeDupes != include) {
        m_includeDupes = include;
        recalculateStats();
    }
}

void RateCalculator::recalculateStats() {
    updateBandModeStats();
    updateOperatorStats();
    updateStationStats();

    emit ratesUpdated(getCurrentSnapshot());
    emit bandModeStatsUpdated(getBandModeStats());
}

void RateCalculator::startAutoUpdate(int intervalMs) {
    m_updateTimer->start(intervalMs);
}

void RateCalculator::stopAutoUpdate() {
    m_updateTimer->stop();
}

void RateCalculator::onUpdateTimer() {
    emit ratesUpdated(getCurrentSnapshot());
}

void RateCalculator::updateBandModeStats() {
    m_bandModeStats.clear();

    if (m_qsoRecords.isEmpty()) {
        return;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime currentHourStart(now.date(), QTime(now.time().hour(), 0, 0), Qt::UTC);

    // First pass: Count totals, sum points, and count this-hour
    for (const auto& record : m_qsoRecords) {
        // Skip dupes if not including them
        if (record.isDupe && !m_includeDupes) {
            continue;
        }

        auto key = qMakePair(record.band, record.mode);

        if (!m_bandModeStats.contains(key)) {
            BandModeStats stats;
            stats.band = record.band;
            stats.mode = record.mode;
            m_bandModeStats[key] = stats;
        }

        m_bandModeStats[key].qsoCount++;
        m_bandModeStats[key].points += record.qsoPoints;  // Sum points from QSO records

        // Count this hour (current UTC hour)
        if (record.timestamp >= currentHourStart) {
            m_bandModeStats[key].thisHourCount++;
        }
    }

    // Second pass: Calculate historical best hour per band+mode
    // Group QSOs by (band, mode, hour_bucket) and find max
    QMap<QPair<BandType, ModeType>, QMap<QDateTime, int>> hourBuckets;

    for (const auto& record : m_qsoRecords) {
        // Skip dupes if not including them
        if (record.isDupe && !m_includeDupes) {
            continue;
        }

        auto key = qMakePair(record.band, record.mode);
        // Bucket by hour start time
        QDateTime hourBucket(record.timestamp.date(),
                             QTime(record.timestamp.time().hour(), 0, 0),
                             Qt::UTC);
        hourBuckets[key][hourBucket]++;
    }

    // Find best hour for each band+mode
    for (auto it = hourBuckets.constBegin(); it != hourBuckets.constEnd(); ++it) {
        const auto& key = it.key();
        const QMap<QDateTime, int>& buckets = it.value();

        int bestHour = 0;
        for (int count : buckets) {
            if (count > bestHour) {
                bestHour = count;
            }
        }

        if (m_bandModeStats.contains(key)) {
            m_bandModeStats[key].bestHourRate = bestHour;
        }
    }

    // Update this hour rates
    for (auto& stats : m_bandModeStats) {
        stats.thisHourRate = stats.thisHourCount;
    }
}

void RateCalculator::updateOperatorStats() {
    m_operatorStats.clear();

    if (m_qsoRecords.isEmpty()) {
        return;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime cutoff60 = now.addSecs(-3600);

    // First pass: count totals and recent QSOs
    for (const auto& record : m_qsoRecords) {
        if (record.operatorCall.isEmpty()) {
            continue;
        }

        if (!m_operatorStats.contains(record.operatorCall)) {
            OperatorStats stats;
            stats.callsign = record.operatorCall;
            m_operatorStats[record.operatorCall] = stats;
        }

        m_operatorStats[record.operatorCall].totalQsos++;

        if (record.timestamp >= cutoff60) {
            m_operatorStats[record.operatorCall].last60MinQsos++;
        }
    }

    // Second pass: Calculate historical peak rates using sliding window
    // Group QSOs by operator and sort by time
    QMap<QString, QVector<QDateTime>> operatorQsoTimes;
    for (const auto& record : m_qsoRecords) {
        if (!record.operatorCall.isEmpty()) {
            operatorQsoTimes[record.operatorCall].append(record.timestamp);
        }
    }

    // For each operator, find the peak 60-minute rate
    for (auto it = operatorQsoTimes.begin(); it != operatorQsoTimes.end(); ++it) {
        const QString& op = it.key();
        QVector<QDateTime>& times = it.value();

        if (times.size() < 2) {
            m_operatorStats[op].peakRate = times.size();
            continue;
        }

        // Sort times
        std::sort(times.begin(), times.end());

        // Sliding window: for each QSO, count how many QSOs in the 60 minutes ending at that QSO
        int peakRate = 0;
        for (int i = 0; i < times.size(); ++i) {
            QDateTime windowStart = times[i].addSecs(-3600);
            int count = 0;

            // Count QSOs in the 60-minute window ending at times[i]
            for (int j = i; j >= 0 && times[j] >= windowStart; --j) {
                count++;
            }

            if (count > peakRate) {
                peakRate = count;
            }
        }

        m_operatorStats[op].peakRate = peakRate;
    }

    // Update current 60-min rate to peak if higher (for live updates)
    for (auto& stats : m_operatorStats) {
        if (stats.last60MinQsos > stats.peakRate) {
            stats.peakRate = stats.last60MinQsos;
        }
    }
}

void RateCalculator::updateStationStats() {
    m_stationStats.clear();

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime cutoff60 = now.addSecs(-3600);
    QDateTime hourStart(now.date(), QTime(now.time().hour(), 0, 0), Qt::UTC);

    // Track band counts per station for dominant band
    QMap<QString, QMap<BandType, int>> stationBandCounts;

    for (const auto& record : m_qsoRecords) {
        QString stationId = record.stationId.isEmpty() ? "RADIO1" : record.stationId;

        if (!m_stationStats.contains(stationId)) {
            StationStats stats;
            stats.stationId = stationId;
            m_stationStats[stationId] = stats;
        }

        m_stationStats[stationId].qsoCount++;

        if (record.timestamp >= cutoff60) {
            m_stationStats[stationId].last60MinRate++;
        }

        // Track band for dominant band calculation (this hour only)
        if (record.timestamp >= hourStart) {
            stationBandCounts[stationId][record.band]++;
        }
    }

    // Determine dominant band per station
    for (auto it = stationBandCounts.begin(); it != stationBandCounts.end(); ++it) {
        const QString& stationId = it.key();
        const QMap<BandType, int>& bandCounts = it.value();

        BandType dominant = BandType::None;
        int maxCount = 0;

        for (auto bit = bandCounts.begin(); bit != bandCounts.end(); ++bit) {
            if (bit.value() > maxCount) {
                maxCount = bit.value();
                dominant = bit.key();
            }
        }

        if (m_stationStats.contains(stationId)) {
            m_stationStats[stationId].dominantBand = dominant;
        }
    }
}

} // namespace TR4QT
