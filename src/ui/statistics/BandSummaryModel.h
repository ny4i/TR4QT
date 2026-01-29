#ifndef BANDSUMMARYMODEL_H
#define BANDSUMMARYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "RateCalculator.h"

namespace TR4QT {

/**
 * BandSummaryModel
 *
 * Table model for per-band/mode statistics display.
 *
 * Columns: Band, Mode, QSOs, Mults, Points, Score, Best Hr, This Hr
 */
class BandSummaryModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColBand = 0,
        ColMode,
        ColQsos,
        ColMults,
        ColPoints,
        ColScore,
        ColBestHr,
        ColThisHr,
        ColCount
    };

    explicit BandSummaryModel(QObject* parent = nullptr);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * Update statistics from RateCalculator
     */
    void updateStats(const QVector<BandModeStats>& stats);

    /**
     * Get current band/mode stats (for modification and re-update)
     */
    QVector<BandModeStats> getBandModeStats() const { return m_stats; }

    /**
     * Update multiplier and point data from contest scorer
     */
    void updateMultipliers(BandType band, ModeType mode, int multipliers);
    void updatePoints(BandType band, ModeType mode, int points, int score);

    /**
     * Clear all data
     */
    void clear();

    /**
     * Get totals row data
     */
    int getTotalQsos() const;
    int getTotalMults() const;
    int getTotalPoints() const;
    int getTotalScore() const;

private:
    QVector<BandModeStats> m_stats;
};

/**
 * OperatorStatsModel
 *
 * Table model for per-operator statistics (multi-op contests).
 */
class OperatorStatsModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColOperator = 0,
        ColQsos,
        ColLast60Min,
        ColPeakRate,
        ColOnAirTime,
        ColCount
    };

    explicit OperatorStatsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void updateStats(const QVector<OperatorStats>& stats);
    void clear();

    int getOperatorCount() const { return m_stats.size(); }

private:
    QVector<OperatorStats> m_stats;
};

/**
 * StationStatsModel
 *
 * Table model for per-station/radio statistics (SO2R).
 */
class StationStatsModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColStation = 0,
        ColQsos,
        ColLast60MinRate,
        ColDominantBand,
        ColCount
    };

    explicit StationStatsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void updateStats(const QVector<StationStats>& stats);
    void clear();

    int getStationCount() const { return m_stats.size(); }

private:
    QVector<StationStats> m_stats;
};

} // namespace TR4QT

#endif // BANDSUMMARYMODEL_H
