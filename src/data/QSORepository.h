#ifndef QSOREPOSITORY_H
#define QSOREPOSITORY_H

#include <QString>
#include <QList>
#include <QMap>
#include <QSqlQuery>
#include "../models/QSO.h"
#include "../core/Types.h"

namespace TR4QT {

/**
 * Repository for QSO database operations
 *
 * Provides CRUD operations for QSOs, dupe checking, and multiplier tracking.
 * Uses the Database singleton for all operations.
 *
 * Usage:
 *   QSORepository repo;
 *   QSO qso = createQSO();
 *   if (repo.saveQSO(qso, contestId)) {
 *       qDebug() << "QSO saved with ID:" << qso.id;
 *   }
 */
class QSORepository {
public:
    QSORepository();
    ~QSORepository() = default;

    // ===== QSO CRUD Operations =====

    /**
     * Save QSO to database (insert or update)
     * Sets qso.id on successful insert
     *
     * @param qso QSO to save (modified with ID if new)
     * @param contestId Contest ID this QSO belongs to
     * @return true if successful
     */
    bool saveQSO(QSO& qso, int contestId);

    /**
     * Update existing QSO
     *
     * @param qso QSO with modified fields (must have valid id)
     * @return true if successful
     */
    bool updateQSO(const QSO& qso);

    /**
     * Delete QSO (soft delete by default, sets deleted=1)
     *
     * @param qsoId QSO ID to delete
     * @param hardDelete If true, actually remove from database
     * @return true if successful
     */
    bool deleteQSO(int qsoId, bool hardDelete = false);

    /**
     * Delete all QSOs for a contest
     *
     * Hard deletes all QSOs and multipliers for the specified contest.
     * Used when clearing the log.
     *
     * @param contestId Contest ID
     * @return true if successful
     */
    bool deleteAllQSOs(int contestId);

    /**
     * Find QSO by ID
     *
     * @param qsoId QSO ID
     * @return QSO object (check qso.id >= 0 for success)
     */
    QSO findById(int qsoId) const;

    /**
     * Find all QSOs for a contest
     *
     * @param contestId Contest ID
     * @param includeDeleted Include soft-deleted QSOs
     * @return List of QSOs ordered by timestamp
     */
    QList<QSO> findByContest(int contestId, bool includeDeleted = false) const;

    /**
     * Get QSO count for contest
     *
     * @param contestId Contest ID
     * @param includeDeleted Include soft-deleted QSOs
     * @return Number of QSOs
     */
    int getQSOCount(int contestId, bool includeDeleted = false) const;

    /**
     * Get total QSO points for contest
     *
     * @param contestId Contest ID
     * @return Sum of qso_points
     */
    int getTotalPoints(int contestId) const;

    // ===== Dupe Checking =====

    /**
     * Duplicate check result with detailed information
     */
    struct DuplicateCheckResult {
        bool isDuplicate{false};
        QString dupeInfo;
        QDateTime timestamp;
    };

    /**
     * Check for duplicate with rule-based logic
     * Extracted from MainWindow::checkForDuplicate()
     *
     * Supports all duplicate checking rules:
     * - PerBandMode: Same call/band/mode
     * - AllBandMode: Same call/mode (any band)
     * - PerBand: Same call/band (any mode)
     * - AllBand: Same call (once per contest)
     *
     * @param callsign Callsign to check
     * @param band Band
     * @param mode Mode
     * @param rule Duplicate checking rule
     * @param contestId Contest ID
     * @return DuplicateCheckResult with isDuplicate flag, info string, and timestamp
     */
    DuplicateCheckResult checkDuplicate(
        const QString& callsign,
        BandType band,
        ModeType mode,
        DuplicateCheckingRule rule,
        int contestId
    ) const;

    /**
     * Check if QSO is a duplicate
     * Most contests: dupe = same call + band + mode
     *
     * @param callsign Callsign to check
     * @param band Band
     * @param mode Mode
     * @param contestId Contest ID
     * @return true if duplicate exists
     */
    bool isDuplicate(const QString& callsign, BandType band, ModeType mode, int contestId) const;

    /**
     * Find all QSOs with same callsign in contest
     *
     * @param callsign Callsign to search
     * @param contestId Contest ID
     * @return List of matching QSOs
     */
    QList<QSO> findByCallsign(const QString& callsign, int contestId) const;

    // ===== Multiplier Tracking =====

    /**
     * Check if multiplier is new (not worked before)
     *
     * @param multType Multiplier type (Country, CQZone, etc.)
     * @param multValue Multiplier value ("K", "5", "W1", etc.)
     * @param band Band (for per-band mults) or empty for all-band
     * @param contestId Contest ID
     * @return true if this is a new multiplier
     */
    bool isNewMultiplier(
        MultiplierType multType,
        const QString& multValue,
        const QString& band,
        int contestId) const;

    /**
     * Save a new multiplier
     *
     * @param multType Multiplier type
     * @param multValue Multiplier value
     * @param band Band (or empty for all-band)
     * @param contestId Contest ID
     * @param firstQsoId ID of first QSO that worked this mult
     * @return true if successful
     */
    bool saveMultiplier(
        MultiplierType multType,
        const QString& multValue,
        const QString& band,
        int contestId,
        int firstQsoId);

    /**
     * Get list of worked multiplier values
     *
     * @param multType Multiplier type
     * @param band Band filter (empty = all bands)
     * @param contestId Contest ID
     * @return List of multiplier values (e.g., ["K", "JA", "G"])
     */
    QStringList getWorkedMultipliers(
        MultiplierType multType,
        const QString& band,
        int contestId) const;

    /**
     * Get multiplier count by type
     *
     * @param multType Multiplier type
     * @param contestId Contest ID
     * @return Number of unique multipliers worked
     */
    int getMultiplierCount(MultiplierType multType, int contestId) const;

    /**
     * Get multiplier counts for all types
     *
     * @param contestId Contest ID
     * @return Map of MultiplierType -> count
     */
    QMap<MultiplierType, int> getAllMultiplierCounts(int contestId) const;

    // ===== Utility Methods =====

    /**
     * Get last error message
     */
    QString lastError() const { return m_lastError; }

private:
    /**
     * Convert database row to QSO object
     */
    QSO qsoFromQuery(const QSqlQuery& query) const;

    /**
     * Convert MultiplierType enum to string
     */
    QString multiplierTypeToString(MultiplierType type) const;

    /**
     * Convert string to MultiplierType enum
     */
    MultiplierType stringToMultiplierType(const QString& str) const;

    QString m_lastError;
};

} // namespace TR4QT

#endif // QSOREPOSITORY_H
