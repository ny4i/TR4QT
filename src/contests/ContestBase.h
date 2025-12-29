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
 * Table column definition for QSO display
 * Contests can optionally provide column metadata to control
 * how exchange fields are displayed in the QSO table
 */
struct TableColumn {
    QString fieldName;      // Field name from ExchangeField (e.g., "Serial", "Precedence")
    QString headerText;     // Short column header (e.g., "#", "Prec", "Chk", "QTH")
    int preferredWidth;     // Suggested column width in pixels (0 = auto)

    enum class Alignment {
        Left,
        Center,
        Right
    };
    Alignment alignment = Alignment::Left;

    TableColumn() : preferredWidth(0), alignment(Alignment::Left) {}
    TableColumn(const QString& field, const QString& header, int width = 0, Alignment align = Alignment::Left)
        : fieldName(field), headerText(header), preferredWidth(width), alignment(align) {}
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

    /**
     * Get official ADIF Contest-ID for this contest
     * Returns the official ADIF Contest-ID per ADIF specification
     * (e.g., "CQ-WW-CW", "CQ-WPX-SSB", "ARRL-SS-CW")
     * This is NOT user-editable and must match ADIF spec exactly
     *
     * @return Official ADIF Contest-ID, empty string if none
     */
    virtual QString getADIFContestId() const = 0;

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
     * Get table column definitions for displaying exchange fields
     * Override this to provide custom column layout for contests with many fields
     * Default implementation returns empty list (uses legacy 2-column display)
     *
     * @return List of table column definitions, or empty for default behavior
     */
    virtual QList<TableColumn> getTableColumns() const {
        return QList<TableColumn>();  // Default: use legacy 2-column display
    }

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

    /**
     * Validate if a mode is allowed for this contest
     * Contests define supported modes in their metadata
     *
     * Default implementation checks against supportedModes list.
     * Override for contests with complex mode rules.
     *
     * @param mode The mode to validate
     * @param errorMsg Output parameter for error message if invalid
     * @return true if mode is allowed, false with errorMsg set if not allowed
     */
    virtual bool isValidMode(ModeType mode, QString& errorMsg) const {
        Q_UNUSED(mode);
        Q_UNUSED(errorMsg);
        // Default: all modes allowed (contests should override this)
        return true;
    }

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
     * Check if this contest uses multipliers in scoring calculation
     * Most contests multiply QSO points by multiplier count.
     * Some contests (like Winter Field Day) only track multipliers but don't
     * use them in score calculation.
     *
     * @return true if multipliers affect scoring (default), false if only tracked
     */
    virtual bool usesMultipliers() const {
        return true;  // Default: most contests use multipliers
    }

    /**
     * Get list of mode groups supported by this contest
     * For single-mode contests, return only one group.
     * For mixed-mode contests like Field Day, return all three.
     *
     * @return List of supported mode groups
     */
    virtual QList<ModeGroup> getSupportedModeGroups() const {
        // Default: all mode groups supported
        return {ModeGroup::Phone, ModeGroup::CW, ModeGroup::Digital};
    }

    /**
     * Check if this contest should show mode group breakdown in statistics
     * Field Day shows separate rows for Phone/CW/Digital QSOs.
     * Single-mode contests show just "QSOs" row.
     *
     * @return true if mode groups should be shown separately
     */
    virtual bool usesModeGroupBreakdown() const {
        return false;  // Default: single QSO row
    }

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
     * Get duplicate checking rule for this contest
     * Determines when a QSO is considered a duplicate
     *
     * @return DuplicateCheckingRule for this contest
     */
    virtual DuplicateCheckingRule getDuplicateCheckingRule() const = 0;

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
