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

#ifndef RATECALCULATOR_H
#define RATECALCULATOR_H

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QTimer>
#include "../../core/Types.h"

namespace TR4QT {

/**
 * Snapshot of current rate calculations
 */
struct RateSnapshot {
    int qso10Rate{0};       // Rate based on last 10 QSOs (QSOs/hour)
    int qso100Rate{0};      // Rate based on last 100 QSOs (QSOs/hour)
    int last60MinRate{0};   // QSOs in last 60 minutes extrapolated to hourly
    int thisHourRate{0};    // QSOs since current clock hour started
    int thisHourCount{0};   // Actual count this clock hour
    int averageRate{0};     // Contest lifetime average rate
    int totalQsos{0};       // Total QSO count
    QDateTime timestamp;    // When this snapshot was taken
};

/**
 * Per-band/mode statistics
 */
struct BandModeStats {
    BandType band{BandType::None};
    ModeType mode{ModeType::None};
    int qsoCount{0};
    int multipliers{0};
    int points{0};
    int score{0};
    int bestHourRate{0};
    int thisHourRate{0};
    int thisHourCount{0};
};

/**
 * Per-operator statistics (for multi-op)
 */
struct OperatorStats {
    QString callsign;
    int totalQsos{0};
    int last60MinQsos{0};
    int peakRate{0};
    int onAirMinutes{0};
};

/**
 * Per-station/radio statistics (for SO2R)
 */
struct StationStats {
    QString stationId;      // "RADIO1", "RADIO2", "RUN", "MULT"
    int qsoCount{0};
    int last60MinRate{0};
    BandType dominantBand{BandType::None};
};

/**
 * Hourly rate data point for history graph
 */
struct HourlyRatePoint {
    QDateTime hourStart;
    int qsoCount{0};
    int rate{0};            // QSOs per hour
};

/**
 * RateCalculator
 *
 * Calculates various QSO rates for contest statistics display.
 * Maintains a cache of recent QSO timestamps for efficient sliding window calculations.
 *
 * Rate calculations:
 * - 10-QSO rate: Time between 10th oldest and most recent QSO, extrapolated to hourly
 * - 100-QSO rate: Time between 100th oldest and most recent QSO, extrapolated to hourly
 * - 60-min rate: QSOs in last 60 minutes (actual count, not extrapolated)
 * - Clock hour rate: QSOs since current hour started (e.g., since 14:00)
 * - Average rate: Total QSOs / total contest time
 */
class RateCalculator : public QObject {
    Q_OBJECT

public:
    explicit RateCalculator(QObject* parent = nullptr);
    ~RateCalculator() override = default;

    /**
     * Set the contest start time for average rate calculation
     */
    void setContestStartTime(const QDateTime& startTime);

    /**
     * Add a QSO timestamp to the cache
     * Call this when a new QSO is logged
     */
    void addQso(const QDateTime& timestamp,
                BandType band = BandType::None,
                ModeType mode = ModeType::None,
                const QString& operatorCall = QString(),
                const QString& stationId = QString());

    /**
     * Clear all cached data (e.g., when switching contests)
     */
    void clear();

    /**
     * Load historical QSO data from database
     * @param timestamps List of QSO timestamps to load
     */
    void loadHistory(const QVector<QDateTime>& timestamps);

    /**
     * Load detailed history with band/mode/operator info
     */
    struct QsoRecord {
        QDateTime timestamp;
        BandType band;
        ModeType mode;
        QString operatorCall;
        QString stationId;
        int qsoPoints{0};       // Points for this QSO (from contest scoring)
        bool isDupe{false};     // Is this a duplicate QSO?

        // Default station ID for solo operation (non-SO2R)
        static constexpr const char* DEFAULT_STATION_ID = "RADIO1";
    };
    void loadDetailedHistory(const QVector<QsoRecord>& records);

    // Rate calculations
    int calculate10QsoRate() const;
    int calculate100QsoRate() const;
    int calculateLastNMinutesRate(int minutes) const;
    int calculateThisClockHourRate() const;
    int calculateThisClockHourCount() const;
    int calculateAverageRate() const;
    int getTotalQsoCount() const { return m_timestamps.size(); }

    /**
     * Get current rate snapshot
     */
    RateSnapshot getCurrentSnapshot() const;

    /**
     * Get hourly rate history for graphing
     * @param hours Number of hours of history to return
     * @return Vector of hourly rate points, oldest first
     */
    QVector<HourlyRatePoint> getHourlyRateHistory(int hours) const;

    /**
     * Get per-band statistics
     */
    QVector<BandModeStats> getBandModeStats() const;

    /**
     * Get per-operator statistics (for multi-op)
     */
    QVector<OperatorStats> getOperatorStats() const;

    /**
     * Get per-station statistics (for SO2R)
     */
    QVector<StationStats> getStationStats() const;

    /**
     * Set time bucket size for history calculations
     * @param minutes Bucket size (20, 30, or 60)
     */
    void setTimeBucket(int minutes);
    int getTimeBucket() const { return m_timeBucketMinutes; }

    /**
     * Set whether to include duplicate QSOs in rate calculations
     * Triggers recalculation of all stats
     */
    void setIncludeDupes(bool include);
    bool getIncludeDupes() const { return m_includeDupes; }

    /**
     * Force recalculation of all statistics
     */
    void recalculateStats();

    /**
     * Start/stop automatic rate updates
     */
    void startAutoUpdate(int intervalMs = 5000);
    void stopAutoUpdate();

signals:
    /**
     * Emitted when rates are recalculated
     */
    void ratesUpdated(const RateSnapshot& snapshot);

    /**
     * Emitted when band/mode stats change
     */
    void bandModeStatsUpdated(const QVector<BandModeStats>& stats);

private slots:
    void onUpdateTimer();

private:
    int calculateSlidingWindowRate(int qsoCount) const;
    void updateBandModeStats();
    void updateOperatorStats();
    void updateStationStats();

    // QSO timestamp cache (for rate calculations)
    QVector<QDateTime> m_timestamps;

    // Detailed QSO records (for per-band/operator/station breakdowns)
    QVector<QsoRecord> m_qsoRecords;

    // Band/mode aggregations
    QMap<QPair<BandType, ModeType>, BandModeStats> m_bandModeStats;

    // Operator aggregations
    QMap<QString, OperatorStats> m_operatorStats;

    // Station aggregations
    QMap<QString, StationStats> m_stationStats;

    // Best hourly rates per band (for "Best Hr" column)
    QMap<BandType, int> m_bestHourlyRates;

    QDateTime m_contestStartTime;
    QTimer* m_updateTimer{nullptr};
    int m_timeBucketMinutes{60};
    bool m_includeDupes{false};

    static const int MAX_CACHED_TIMESTAMPS = 10000;  // Limit cache size
};

} // namespace TR4QT

#endif // RATECALCULATOR_H
