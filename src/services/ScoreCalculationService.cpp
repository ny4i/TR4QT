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

/**
 * @file ScoreCalculationService.cpp
 * @brief Implementation of ScoreCalculationService
 *
 * Phase 11 extraction from MainWindow.
 */

#include "ScoreCalculationService.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

ScoreResult ScoreCalculationService::calculateScore(
    const QList<QSO>& qsos,
    const ContestBase* contest) const
{
    ScoreResult result;

    // Check if contest uses mode group breakdown
    result.usesModeGroupBreakdown = contest && contest->usesModeGroupBreakdown();

    // Get multiplier definitions from active contest
    QList<MultiplierDefinition> multDefs;
    if (contest) {
        multDefs = contest->getMultiplierTypes();
    }

    // Track unique multiplier values for scoring calculation
    QMap<MultiplierType, QSet<QString>> uniqueMultValues;

    // Track zones per band
    QMap<BandType, QSet<int>> zonesPerBand;

    // Process all QSOs
    for (const QSO& qso : qsos) {
        if (qso.band == BandType::None) {
            continue;  // Skip QSOs with no band
        }

        // Initialize band stats if not present
        if (!result.bandStats.contains(qso.band)) {
            result.bandStats[qso.band] = BandStatistics();
        }

        // Count QSOs per band
        result.bandStats[qso.band].qsoCount++;
        result.totalQSOs++;

        // Track mode group statistics if contest uses breakdown
        if (result.usesModeGroupBreakdown) {
            ModeGroup group = modeTypeToModeGroup(qso.mode);
            result.modeGroupStats[group].qsosPerBand[qso.band]++;
            result.modeGroupStats[group].totalQSOs++;
        }

        // Sum points per band
        result.bandStats[qso.band].points += qso.qsoPoints;
        result.totalQSOPoints += qso.qsoPoints;

        // Count multiplier QSOs per band
        if (qso.isMultiplier) {
            result.bandStats[qso.band].multipliers++;
            result.totalMultipliers++;
        }

        // Track unique multiplier values for scoring
        if (contest) {
            for (const MultiplierDefinition& multDef : multDefs) {
                QString multValue = contest->getMultiplierValue(
                    qso, multDef.type, QStringList());
                if (!multValue.isEmpty()) {
                    uniqueMultValues[multDef.type].insert(multValue);
                }
            }
        }

        // Track unique zones per band
        if (qso.cqZone > 0) {
            zonesPerBand[qso.band].insert(qso.cqZone);
        }
    }

    // Calculate zone counts per band and total
    for (auto it = zonesPerBand.begin(); it != zonesPerBand.end(); ++it) {
        result.bandStats[it.key()].zones = it.value().size();
        result.totalZones += it.value().size();
    }

    // Calculate multiplier counts per type for scoring
    for (auto it = uniqueMultValues.begin(); it != uniqueMultValues.end(); ++it) {
        result.multiplierCounts[it.key()] = it.value().size();
    }

    // Calculate final contest score using contest's formula
    result.finalScore = result.totalQSOPoints;  // Default if no contest active
    if (contest) {
        result.finalScore = contest->calculateTotalScore(
            result.totalQSOPoints, result.multiplierCounts);
    }

    return result;
}

QList<BandType> ScoreCalculationService::getStandardBands()
{
    return {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };
}

} // namespace TR4QT
