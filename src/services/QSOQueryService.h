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
};

}  // namespace TR4QT

#endif  // QSOQUERYSERVICE_H
