#ifndef CONTESTMANAGER_H
#define CONTESTMANAGER_H

#include <QString>
#include <QList>
#include "../core/Types.h"
#include "../models/QSO.h"
#include "../contests/ContestBase.h"
#include "../ui/dialogs/ContestChooserDialog.h"

namespace TR4QT {

// Forward declarations
class CountryFile;

/**
 * Result structure for contest activation
 * Contains all information needed by MainWindow to update UI
 */
struct ActivateContestResult {
    bool success = false;
    QString errorMessage;

    // Contest instance (ownership transferred to caller)
    ContestBase* contest = nullptr;

    // Contest database info
    int contestDbId = -1;
    int nextSerialNumber = 1;
    QString exchangeSent;  // Sent exchange (e.g., "1H WCF" for WFD)

    // Contest configuration (for Cabrillo export)
    QString category;        // SINGLE-OP, MULTI-OP, CHECKLOG, etc.
    QString powerClass;      // HIGH, LOW, QRP
    QString assisted;        // ASSISTED, NON-ASSISTED
    QString operatorName;    // Name used for this contest (for {NAME} substitution)

    // Loaded QSOs (for existing contests)
    QList<QSO> loadedQSOs;

    // Station info (built from settings and cty.dat)
    StationInfo myStation;

    // Exchange field definitions (for UI configuration)
    QList<ExchangeField> receivedFields;
    QList<ExchangeField> sentFields;
    QList<TableColumn> tableColumns;

    // Contest capabilities (for UI configuration)
    bool usesMultipliers = false;
    bool usesModeGroupBreakdown = false;
    bool usesZones = false;
    QList<BandType> allowedBands;
    QList<MultiplierDefinition> multiplierTypes;
};

/**
 * ContestManager
 *
 * Handles contest activation, configuration, and state management.
 * Separates contest business logic from MainWindow UI updates.
 *
 * Responsibilities:
 * - Open contest database
 * - Load or create contest record
 * - Create contest instance via registry
 * - Load existing QSOs from database
 * - Build station info from settings and cty.dat
 * - Calculate next serial number
 * - Extract exchange field definitions
 *
 * UI updates are NOT handled here - MainWindow uses the returned
 * ActivateContestResult to update all widgets and windows.
 */
class ContestManager {
public:
    /**
     * Configuration for ContestManager
     */
    struct Config {
        CountryFile* countryFile = nullptr;
    };

    /**
     * Construct a ContestManager
     * @param config Configuration with dependencies
     */
    explicit ContestManager(const Config& config);

    /**
     * Destructor
     */
    ~ContestManager();

    /**
     * Activate a contest
     *
     * Opens the contest database, loads or creates contest record,
     * creates contest instance, loads existing QSOs, builds station info,
     * and extracts all contest configuration needed by MainWindow.
     *
     * Returns ActivateContestResult with success/error and all contest data.
     * On success, caller takes ownership of the contest instance.
     * On failure, contest instance is nullptr and errorMessage is set.
     *
     * @param contestInfo Contest to activate
     * @return Result with contest instance and configuration
     */
    ActivateContestResult activateContest(const ContestInfo& contestInfo);

private:
    /**
     * Find or create contest record in database
     * @param contestInfo Contest info
     * @param result Output: populated with contest data (contestDbId, nextSerialNumber,
     *               exchangeSent, loadedQSOs, category, powerClass, assisted, operatorName)
     * @return true on success, false on error
     */
    bool findOrCreateContestRecord(
        const ContestInfo& contestInfo,
        ActivateContestResult& result);

    /**
     * Calculate next serial number from loaded QSOs
     * Ensures we don't reuse serial numbers even if DB is out of sync
     * @param qsos Loaded QSOs
     * @return Next serial number (max + 1)
     */
    int calculateNextSerialNumber(const QList<QSO>& qsos) const;

    /**
     * Build station info from settings and cty.dat lookup
     * @return Station info
     */
    StationInfo buildStationInfo() const;

    /**
     * Determine mode type from contest info
     * @param contestInfo Contest info with mode string
     * @return Mode type (CW, USB, or None for mixed)
     */
    ModeType determineModeType(const ContestInfo& contestInfo) const;

    /**
     * Check if contest uses zone multipliers
     * @param contest Contest instance
     * @return true if uses CQ or ITU zone multipliers
     */
    bool contestUsesZones(ContestBase* contest) const;

private:
    CountryFile* m_countryFile;
};

} // namespace TR4QT

#endif // CONTESTMANAGER_H
