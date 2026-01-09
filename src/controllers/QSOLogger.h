#ifndef QSOLOGGER_H
#define QSOLOGGER_H

#include <QString>
#include <QList>
#include "../models/QSO.h"
#include "../models/StationInfo.h"
#include "../core/Types.h"
#include "../contests/ContestBase.h"
#include "../utils/CountryFile.h"
#include "../radio/RadioInterface.h"

using namespace TR4QT;

/**
 * QSOLogger - Handles QSO logging workflow
 *
 * Extracted from MainWindow::onLogQSO() to isolate core logging logic
 * from UI and database concerns. Responsible for:
 * - Validating callsign, exchange, band/mode
 * - Creating QSO objects with radio state snapshot
 * - Looking up country/zone via CountryFile
 * - Checking for duplicates
 * - Calculating QSO points via contest
 * - Detecting new multipliers
 *
 * Does NOT handle:
 * - UI updates (caller's responsibility)
 * - Database persistence (caller's responsibility)
 * - Special commands like OPON/UDP (MainWindow-specific)
 * - Error dialogs (returns error messages for caller to display)
 */
class QSOLogger {
public:
    // Configuration passed to constructor
    struct Config {
        ContestBase* contest = nullptr;     // Active contest (can be nullptr for general logging)
        CountryFile* countryFile = nullptr; // For DXCC lookups
        StationInfo myStation;              // My station info for points calculation
    };

    // Input parameters for logging a QSO
    struct Input {
        QString callsign;               // Station callsign (required)
        QString exchange;               // Received exchange (contest-dependent)
        RadioState radioState;          // Current radio state (frequency, band, mode)
        QString operatorCallsign;       // Operator callsign (can be empty)
        int serialNumber;               // Current serial number (for contests using serials)
        OperatingMode operatingMode;    // CQ vs S&P mode
    };

    // Result returned from logQSO()
    struct Result {
        bool success = false;           // true if QSO created successfully
        QString errorMessage;           // Error message if success = false
        QSO qso;                        // Created QSO object (if success = true)
        bool isDuplicate = false;       // true if this is a duplicate QSO
        QString dupeInfo;               // Human-readable duplicate info
        bool isNewMultiplier = false;   // true if QSO provides new multiplier(s)
        QStringList multiplierValues;   // List of new multiplier values (e.g., "Country:W", "CQZone:5")
        int updatedSerialNumber = 0;    // Updated serial number (incremented if contest uses serials)
    };

    /**
     * Constructor
     * @param config Configuration with contest, country file, station info
     */
    explicit QSOLogger(const Config& config);

    /**
     * Logs a QSO with validation, duplicate checking, and scoring
     *
     * @param input Input parameters (callsign, exchange, radio state, etc.)
     * @param existingQSOs List of existing QSOs for duplicate/multiplier checking
     * @return Result with success status, QSO object, dupe/mult info, or error message
     */
    Result logQSO(const Input& input, const QList<QSO>& existingQSOs);

private:
    Config m_config;

    // Validation helpers
    bool validateCallsign(const QString& callsign, QString& errorMsg);
    bool validateExchange(const QString& exchange, QString& errorMsg);
    bool validateBandMode(const RadioState& radioState, QString& errorMsg);

    // QSO creation helpers
    void populateQSOFromInput(QSO& qso, const Input& input);
    void populateDXCCFields(QSO& qso);
    void parseExchangeIntoQSO(QSO& qso, const QString& exchange);
    QString formatSentExchange(int serialNumber, const QString& rstSent);
    QString substituteSentExchangeTemplate(const QString& exchangeTemplate,
                                           int serialNumber,
                                           const QString& rstSent);

    // Duplicate checking
    bool checkForDuplicate(const QString& callsign,
                           BandType band,
                           ModeType mode,
                           const QList<QSO>& existingQSOs,
                           QString& dupeInfo);

    // Scoring helpers
    int calculateQSOPoints(const QSO& qso);

    // Multiplier checking
    void checkForMultipliers(QSO& qso,
                             const QList<QSO>& existingQSOs,
                             bool& isNewMultiplier,
                             QStringList& multiplierValues);
};

#endif // QSOLOGGER_H
