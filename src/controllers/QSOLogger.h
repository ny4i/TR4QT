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
 * Enum indicating which field caused a validation error.
 * Used by QSOLoggingService to set focus to the correct input field.
 */
enum class ValidationErrorField {
    None,       // No error or unknown
    Callsign,   // Callsign validation failed
    Exchange,   // Exchange validation failed
    BandMode    // Band or mode validation failed
};

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
        QString operatorName;               // Operator name for this contest (for {NAME} substitution)
    };

    // Input parameters for logging a QSO
    struct Input {
        QString callsign;               // Station callsign (required)
        QString exchange;               // Received exchange (contest-dependent)
        RadioState radioState;          // Current radio state (frequency, band, mode)
        QString operatorCallsign;       // Operator callsign (can be empty)
        int serialNumber;               // Current serial number (for contests using serials)
        OperatingMode operatingMode;    // CQ vs S&P mode
        int radioNumber = 1;            // Which radio logged this QSO (1 or 2 for SO2R)
        QDateTime timestamp;            // QSO timestamp (invalid = use current UTC)
    };

    // Result returned from logQSO()
    struct Result {
        bool success = false;           // true if QSO created successfully
        QString errorMessage;           // Error message if success = false
        ValidationErrorField errorField = ValidationErrorField::None;  // Which field failed
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
