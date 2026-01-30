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

#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <QWidget>
#include <QTableView>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QSplitter>
#include <QTimer>
#include "RateCalculator.h"
#include "RateDisplayWidget.h"
#include "BandSummaryModel.h"

namespace TR4QT {

/**
 * StatisticsWindow
 *
 * Comprehensive statistics window for contest operation.
 * Supports Solo, Multi-op, and SO2R views.
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────┐
 * │ View: [Solo ▼]                           [Settings]     │
 * ├─────────────────────────────────────────────────────────┤
 * │ 10-QSO: 210 │ 60-min: 175 │ This Hr: 162 │ Avg: 145    │
 * │ [████] [▁▂▃▅▆▇█▆▅▃▂▁]                                  │
 * ├─────────────────────────────────────────────────────────┤
 * │ Band │ Mode │ QSOs │ Mults │ Points │ Score │ Best │ Hr │
 * │ 160  │ CW   │   45 │    12 │    135 │  1620 │   28 │ 15 │
 * │ ...  │      │      │       │        │       │      │    │
 * │                                                         │
 * │ Total        │ 1500 │   234 │   4500 │ 45000 │      │    │
 * ├─────────────────────────────────────────────────────────┤
 * │ Operators          │ Stations                           │
 * │ (multi-op only)    │ (SO2R only)                        │
 * └─────────────────────────────────────────────────────────┘
 */
class StatisticsWindow : public QWidget {
    Q_OBJECT

public:
    enum ViewMode {
        ViewSolo = 0,
        ViewMultiOp,
        ViewSO2R
    };
    Q_ENUM(ViewMode)

    explicit StatisticsWindow(QWidget* parent = nullptr);
    ~StatisticsWindow() override;

    /**
     * Get the rate calculator for external connections
     */
    RateCalculator* rateCalculator() { return m_rateCalculator; }

    /**
     * Set contest start time
     */
    void setContestStartTime(const QDateTime& startTime);

    /**
     * Add a QSO to statistics
     */
    void addQso(const QDateTime& timestamp,
                BandType band,
                ModeType mode,
                const QString& operatorCall = QString(),
                const QString& stationId = QString());

    /**
     * Load historical QSO data
     */
    void loadHistory(const QVector<RateCalculator::QsoRecord>& records);

    /**
     * Clear all statistics (e.g., when switching contests)
     */
    void clearStats();

    /**
     * Update multiplier counts from contest scorer
     */
    void updateMultipliers(BandType band, ModeType mode, int multipliers);

    /**
     * Update point/score from contest scorer
     */
    void updatePoints(BandType band, ModeType mode, int points, int score);

    /**
     * Update scoring totals for display
     * Per-band-mode points come from QSO data via RateCalculator
     * @param totalMults Total multipliers
     * @param totalPoints Total QSO points
     * @param totalScore Final score
     */
    void updateScoringData(int totalMults, int totalPoints, int totalScore);

    /**
     * Set goal rate for display
     */
    void setGoalRate(int rate);

    /**
     * Get/set current view mode
     */
    ViewMode getViewMode() const { return m_viewMode; }
    void setViewMode(ViewMode mode);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onViewModeChanged(int index);
    void onTimeBucketChanged(int index);
    void onIncludeDupesChanged(int state);
    void onGoalRateChanged(int value);
    void onRatesUpdated(const RateSnapshot& snapshot);
    void onBandModeStatsUpdated(const QVector<BandModeStats>& stats);
    void onUpdateTimer();

private:
    void setupUI();
    void updateTotalsRow();
    void updateOperatorVisibility();
    void updateStationVisibility();
    void saveWindowSettings();
    void restoreWindowSettings();

    // View mode
    ViewMode m_viewMode{ViewSolo};
    QComboBox* m_viewModeCombo{nullptr};

    // Rate calculation options
    QComboBox* m_timeBucketCombo{nullptr};     // 20/30/60 min buckets
    QCheckBox* m_includeDupesCheck{nullptr};   // Include dupes in rate calcs
    QSpinBox* m_goalRateSpin{nullptr};         // Goal rate for graph overlay

    // Rate display (top section)
    RateDisplayWidget* m_rateDisplay{nullptr};

    // Band summary table (middle section)
    QTableView* m_bandTable{nullptr};
    BandSummaryModel* m_bandModel{nullptr};
    QLabel* m_totalsLabel{nullptr};

    // Operator table (bottom left, multi-op)
    QWidget* m_operatorSection{nullptr};
    QTableView* m_operatorTable{nullptr};
    OperatorStatsModel* m_operatorModel{nullptr};

    // Station table (bottom right, SO2R)
    QWidget* m_stationSection{nullptr};
    QTableView* m_stationTable{nullptr};
    StationStatsModel* m_stationModel{nullptr};

    // Bottom splitter for operator/station tables
    QSplitter* m_bottomSplitter{nullptr};

    // Rate calculator engine
    RateCalculator* m_rateCalculator{nullptr};

    // Scoring totals (from contest scorer)
    int m_totalMults{0};
    int m_totalPoints{0};
    int m_totalScore{0};

    // Update timer for history graph
    QTimer* m_historyUpdateTimer{nullptr};
    static const int HISTORY_UPDATE_INTERVAL_MS = 60000;  // Update history every minute
    static const int HISTORY_HOURS = 12;  // Show 12 hours of history
};

} // namespace TR4QT

#endif // STATISTICSWINDOW_H
