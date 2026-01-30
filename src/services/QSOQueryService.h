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
 * QSOQueryService - Query operations on QSO collections
 *
 * Part of Phase 13 extraction from MainWindow.
 * Provides thread-safe query operations on QSO data.
 *
 * Design principle: Methods receive QSO list as parameter rather than
 * holding reference to model. This ensures thread safety and testability.
 */

#ifndef QSOQUERYSERVICE_H
#define QSOQUERYSERVICE_H

#include <QDateTime>
#include <QList>
#include <QSet>
#include <QString>
#include "../core/Types.h"
#include "../models/QSO.h"

namespace TR4QT {

class ContestBase;
class StationInfoService;

/**
 * @brief Result struct for needs display calculation
 *
 * Contains the bands where a callsign has been worked and
 * the bands where its multiplier has been worked.
 */
struct NeedsDisplayData {
    QList<BandType> workedBands;      ///< Bands where callsign was worked
    QList<BandType> workedMultBands;  ///< Bands where multiplier was worked
    QString multiplierValue;           ///< Multiplier value for this callsign
};

/**
 * @brief Service for querying QSO collections
 *
 * All methods are stateless and operate on provided QSO lists.
 * This ensures thread safety - caller provides snapshot of data.
 */
class QSOQueryService {
public:
    QSOQueryService() = default;

    /**
     * @brief Get all unique callsigns from QSO list
     * @param qsos List of QSOs to search
     * @return Set of unique callsigns (uppercase)
     */
    QSet<QString> getWorkedCallsigns(const QList<QSO>& qsos) const;

    /**
     * @brief Get bands on which a callsign has been worked
     * @param qsos List of QSOs to search
     * @param callsign Callsign to look up (case-insensitive)
     * @return List of bands where callsign was worked
     */
    QList<BandType> getWorkedBandsForCallsign(const QList<QSO>& qsos,
                                               const QString& callsign) const;

    /**
     * @brief Get bands on which a multiplier has been worked
     * @param qsos List of QSOs to search
     * @param multValue Multiplier value to look up
     * @param type Type of multiplier
     * @param contest Contest instance (for multiplier value calculation)
     * @return List of bands where multiplier was worked
     */
    QList<BandType> getWorkedBandsForMultiplier(const QList<QSO>& qsos,
                                                 const QString& multValue,
                                                 MultiplierType type,
                                                 ContestBase* contest) const;

    /**
     * @brief Count QSOs within a time window
     * @param qsos List of QSOs to search
     * @param startTime Start of time window (inclusive)
     * @param endTime End of time window (inclusive)
     * @return Number of QSOs within the window
     */
    int countQSOsInTimeWindow(const QList<QSO>& qsos,
                               const QDateTime& startTime,
                               const QDateTime& endTime) const;

    /**
     * @brief Calculate QSO rate based on recent QSOs
     * @param qsos List of QSOs (must be in chronological order)
     * @param lookbackCount Number of recent QSOs to consider
     * @return Rate in QSOs per hour
     */
    int calculateRate(const QList<QSO>& qsos, int lookbackCount = 10) const;

    /**
     * @brief Get needs display data for a callsign
     *
     * Calculates worked bands and multiplier bands with proper handling
     * for AllBands multiplier scope (e.g., CQ WPX prefixes).
     *
     * @param qsos List of QSOs to search
     * @param callsign Callsign to look up
     * @param contest Contest instance for multiplier calculation
     * @param stationInfoService Service for getting multiplier value
     * @param vhfBandsEnabled Whether VHF bands are enabled in settings
     * @return NeedsDisplayData with worked bands and multiplier info
     */
    NeedsDisplayData getNeedsDisplayData(const QList<QSO>& qsos,
                                          const QString& callsign,
                                          ContestBase* contest,
                                          StationInfoService* stationInfoService,
                                          bool vhfBandsEnabled) const;
};

}  // namespace TR4QT

#endif  // QSOQUERYSERVICE_H
