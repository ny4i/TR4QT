#ifndef DATA_INTEGRITY_MANAGER_H
#define DATA_INTEGRITY_MANAGER_H

#include <QString>
#include <QList>
#include "../models/QSO.h"
#include "../models/StationInfo.h"
#include "../contests/ContestBase.h"
#include "../utils/CountryFile.h"

namespace TR4QT {

// Forward declarations
class QSOTableModel;
class MultiplierWidget;

/**
 * Statistics from contest rescore operation
 */
struct RescoreStats {
    int qsosUpdated = 0;
    int multsMarked = 0;
    int dupesFound = 0;
};

/**
 * Manages data integrity checking and contest rescoring operations
 *
 * Provides three tiers of integrity checking:
 * - Tier 1 (Quick): Count-based check (memory vs database)
 * - Tier 2 (Periodic): Triggered every 50 QSOs
 * - Tier 3 (Full): Detailed check with comprehensive report
 *
 * Also handles contest rescoring (recalculate points, multipliers, dupes)
 */
class DataIntegrityManager {
public:
    /**
     * Configuration for DataIntegrityManager
     */
    struct Config {
        CountryFile* countryFile = nullptr;
        int currentContestDbId = -1;
    };

    /**
     * Construct with configuration
     */
    explicit DataIntegrityManager(const Config& config);

    /**
     * Result from quick integrity check
     */
    struct QuickCheckResult {
        bool passed;
        int memoryCount;
        int dbCount;
    };

    /**
     * Quick integrity check (Tier 1)
     * Compares QSO count between memory and database
     *
     * @param memoryCount Number of QSOs in memory (from QSOTableModel)
     * @return QuickCheckResult with passed status and counts
     */
    QuickCheckResult quickIntegrityCheck(int memoryCount);

    /**
     * Full integrity check (Tier 3)
     * Performs comprehensive validation:
     * - Count mismatch
     * - Missing QSOs in database
     * - Orphaned QSOs in database
     * - Field value mismatches
     * - Unknown/None bands (CRITICAL)
     *
     * @param memoryQSOs List of QSOs from memory
     * @param criticalOnly Only report critical issues
     * @return Human-readable integrity report
     */
    QString fullIntegrityCheck(const QList<QSO>& memoryQSOs, bool criticalOnly = false);

    /**
     * Rescore entire contest (silent, no dialogs)
     * Recalculates:
     * - QSO points (contest scoring rules)
     * - Duplicate status (per contest duplicate rules)
     * - Multiplier flags (per contest multiplier rules)
     *
     * Used by ADIF import and manual rescore operations
     *
     * @param qsos List of QSOs to rescore (modified in place)
     * @param contest Contest for scoring rules
     * @param myStation My station info for scoring
     * @return RescoreStats with counts of updates/mults/dupes
     */
    RescoreStats rescoreContestSilent(
        QList<QSO>& qsos,
        ContestBase* contest,
        const StationInfo& myStation);

private:
    Config m_config;
};

} // namespace TR4QT

#endif // DATA_INTEGRITY_MANAGER_H
