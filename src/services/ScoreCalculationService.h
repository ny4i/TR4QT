/**
 * @file ScoreCalculationService.h
 * @brief Service for calculating contest scores and band statistics
 *
 * Phase 11 extraction from MainWindow.
 * Encapsulates score calculation, band statistics, and multiplier tracking.
 */

#ifndef SCORECALCULATIONSERVICE_H
#define SCORECALCULATIONSERVICE_H

#include <QString>
#include <QMap>
#include <QSet>
#include <QList>
#include "../core/Types.h"
#include "../models/QSO.h"

namespace TR4QT {

class ContestBase;

/**
 * @brief Per-band statistics for score display
 */
struct BandStatistics {
    int qsoCount = 0;
    int points = 0;
    int multipliers = 0;
    int zones = 0;
};

/**
 * @brief Mode group statistics for mixed-mode contests
 */
struct ModeGroupStatistics {
    QMap<BandType, int> qsosPerBand;
    int totalQSOs = 0;
};

/**
 * @brief Complete score calculation result
 */
struct ScoreResult {
    // Per-band statistics
    QMap<BandType, BandStatistics> bandStats;

    // Mode group statistics (for mixed-mode contests)
    QMap<ModeGroup, ModeGroupStatistics> modeGroupStats;
    bool usesModeGroupBreakdown = false;

    // Totals
    int totalQSOs = 0;
    int totalQSOPoints = 0;
    int totalMultipliers = 0;
    int totalZones = 0;
    int finalScore = 0;

    // Multiplier breakdown by type (for scoring formula)
    QMap<MultiplierType, int> multiplierCounts;
};

/**
 * @brief Service for contest score calculations
 *
 * Extracted from MainWindow Phase 11. Encapsulates:
 * - Band statistics calculation
 * - Mode group breakdown for mixed-mode contests
 * - Multiplier tracking and counting
 * - Final score calculation using contest formula
 */
class ScoreCalculationService {
public:
    ScoreCalculationService() = default;

    /**
     * @brief Calculate complete score from QSO list
     * @param qsos List of QSOs to calculate from
     * @param contest Active contest (for multiplier types and scoring formula)
     * @return ScoreResult with all statistics
     */
    ScoreResult calculateScore(
        const QList<QSO>& qsos,
        const ContestBase* contest) const;

    /**
     * @brief Get standard HF contest bands
     * @return List of band types in order
     */
    static QList<BandType> getStandardBands();
};

} // namespace TR4QT

#endif // SCORECALCULATIONSERVICE_H
