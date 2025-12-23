#ifndef CONTESTBASE_H
#define CONTESTBASE_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include "../core/Types.h"
#include "../models/StationInfo.h"

namespace TR4QT {

// Forward declarations
struct QSO;
class CountryFile;

/**
 * Exchange field definition
 */
struct ExchangeField {
    QString name;           // Field name (e.g., "RST", "Zone", "Serial")
    QString hint;           // Placeholder text for UI
    bool autoFill;          // Auto-fill from station info or increment
    int maxLength;          // Maximum characters (0 = unlimited)
};

/**
 * Multiplier definition for a contest
 */
struct MultiplierDefinition {
    MultiplierType type;
    MultiplierScope scope;
    QString displayName;    // For UI display
};

/**
 * Abstract base class for all contests
 *
 * To add a new contest:
 * 1. Derive from ContestBase
 * 2. Implement all pure virtual methods
 * 3. Register in ContestFactory
 *
 * The contest class is stateless - it receives all necessary
 * information via method parameters and returns results.
 */
class ContestBase {
public:
    virtual ~ContestBase() = default;

    // ===== Contest Identity =====

    /**
     * Unique contest identifier (e.g., "CQWW_CW", "CQWPX_SSB")
     */
    virtual QString getContestId() const = 0;

    /**
     * Display name for UI (e.g., "CQ WW DX Contest - CW")
     */
    virtual QString getContestName() const = 0;

    /**
     * Contest mode restriction (CW, SSB, or Mixed)
     */
    virtual ModeType getContestMode() const = 0;

    // ===== Exchange Configuration =====

    /**
     * Get list of exchange fields to receive from other station
     * First field is typically RST and may be auto-populated
     */
    virtual QList<ExchangeField> getReceivedExchangeFields() const = 0;

    /**
     * Get list of exchange fields to send to other station
     * Used to configure the "sent exchange" in settings
     */
    virtual QList<ExchangeField> getSentExchangeFields() const = 0;

    /**
     * Format the exchange to send (may include auto-increment serial)
     * @param serialNumber Current serial number (if applicable)
     * @param rst RST to send
     * @return Formatted exchange string
     */
    virtual QString formatSentExchange(int serialNumber, const QString& rst = "599") const = 0;

    /**
     * Validate received exchange format
     * @param exchange The received exchange string
     * @param errorMsg Output parameter for error message
     * @return true if valid, false with errorMsg set if invalid
     */
    virtual bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const = 0;

    /**
     * Parse received exchange into components
     * @param exchange Raw exchange string
     * @return Map of field name -> value
     */
    virtual QMap<QString, QString> parseReceivedExchange(const QString& exchange) const = 0;

    // ===== Scoring =====

    /**
     * Calculate QSO points for this contact
     * Contest rules typically vary points by:
     * - Band (e.g., 160m/10m double points in WPX)
     * - Mode (CW often worth more than SSB)
     * - Distance/continent (same continent vs DX)
     * - Special rules (own country, same zone, etc.)
     *
     * @param qso The QSO to score
     * @param myStation My station information (country, continent, zone, etc.)
     * @return Points for this QSO
     */
    virtual int calculateQSOPoints(
        const QSO& qso,
        const StationInfo& myStation) const = 0;

    /**
     * Calculate total score from QSO points and multiplier counts
     * Most contests: score = QSO points × total multipliers
     * Some contests have different formulas
     *
     * @param totalQSOPoints Sum of all QSO points
     * @param multiplierCounts Map of MultiplierType -> count
     * @return Final contest score
     */
    virtual int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const = 0;

    // ===== Multipliers =====

    /**
     * Get list of multiplier types for this contest
     * Each type includes its scope (per-band or all-band)
     */
    virtual QList<MultiplierDefinition> getMultiplierTypes() const = 0;

    /**
     * Determine if this QSO provides a new multiplier
     *
     * @param qso The QSO to check
     * @param multType The multiplier type to check for
     * @param alreadyWorkedValues List of multiplier values already worked
     *        (contest system maintains this list)
     * @return The multiplier value if new, empty string if not a multiplier
     *
     * Example: For DXCC multiplier, might return "K" for United States
     *          For Zone multiplier, might return "5"
     *          For Prefix multiplier, might return "W1"
     */
    virtual QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const = 0;

    // ===== Special Rules =====

    /**
     * Check if QSO is valid for this contest
     * Some contests have special rules:
     * - Can't work own country (some DX contests)
     * - Mode restrictions
     * - Band restrictions
     * - Time restrictions
     *
     * @param qso The QSO to validate
     * @param myStation My station information
     * @param errorMsg Output parameter for error message
     * @return true if valid for contest, false with errorMsg if not
     */
    virtual bool isValidQSO(
        const QSO& qso,
        const StationInfo& myStation,
        QString& errorMsg) const {
        // Default: all QSOs valid
        Q_UNUSED(qso);
        Q_UNUSED(myStation);
        Q_UNUSED(errorMsg);
        return true;
    }

    /**
     * Does this contest use auto-incrementing serial numbers?
     */
    virtual bool usesSerialNumbers() const = 0;

    /**
     * Generate Cabrillo header fields specific to this contest
     * Base implementation provides common fields
     */
    virtual QMap<QString, QString> getCabrilloHeaders() const {
        QMap<QString, QString> headers;
        headers["CONTEST"] = getContestId();
        return headers;
    }
};

} // namespace TR4QT

#endif // CONTESTBASE_H
