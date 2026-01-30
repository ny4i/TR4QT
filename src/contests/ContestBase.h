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

#ifndef CONTESTBASE_H
#define CONTESTBASE_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include "../core/Types.h"
#include "../models/StationInfo.h"
#include "../models/QSO.h"

namespace TR4QT {

// Forward declarations
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
 * Configuration field for contest creation dialog
 * Describes a user-configurable value needed to set up the contest
 * Each contest declares what fields it needs via getConfigFields()
 */
struct ContestConfigField {
    QString id;              // Unique ID matching placeholder (e.g., "NAME", "STATE", "CHECK")
    QString label;           // UI label (e.g., "Contest Name:", "Check (year licensed):")
    QString placeholder;     // Placeholder text for input field
    QString settingsKey;     // AppSettings key for default value (empty if none)
    int maxLength;           // Maximum input length (0 = unlimited)
    bool required;           // Whether field is required

    enum class Type {
        Text,           // Free text input
        DropDown        // Selection from options
    };
    Type type = Type::Text;

    QStringList options;     // For DropDown type: list of options

    // Convenience constructors
    ContestConfigField() : maxLength(0), required(true), type(Type::Text) {}

    // Text field constructor
    ContestConfigField(const QString& _id, const QString& _label, const QString& _placeholder,
                       const QString& _settingsKey, int _maxLength = 0, bool _required = true)
        : id(_id), label(_label), placeholder(_placeholder), settingsKey(_settingsKey),
          maxLength(_maxLength), required(_required), type(Type::Text) {}

    // DropDown field constructor
    static ContestConfigField dropdown(const QString& _id, const QString& _label,
                                        const QStringList& _options, bool _required = true) {
        ContestConfigField field;
        field.id = _id;
        field.label = _label;
        field.type = Type::DropDown;
        field.options = _options;
        field.required = _required;
        return field;
    }
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
 * The contest class stores operating station information (callsign, location, etc.)
 * passed at construction. This enables location-dependent contest configuration
 * (e.g., ARRL DX shows different multipliers for W/VE vs DX stations).
 */
class ContestBase {
protected:
    StationInfo m_myStation;  // Operating station information
    QString m_exchangeSent;    // Contest-specific sent exchange (e.g., "1H WCF" for WFD)

public:
    /**
     * Constructor - requires station information
     * @param myStation Operating station info (callsign, location, etc.)
     */
    explicit ContestBase(const StationInfo& myStation)
        : m_myStation(myStation) {}

    virtual ~ContestBase() = default;

    /**
     * Update station information if it changes mid-contest
     * (e.g., user updates their callsign or location in preferences)
     * @param station Updated station information
     */
    virtual void updateStationInfo(const StationInfo& station) {
        m_myStation = station;
    }

    /**
     * Set contest-specific sent exchange
     * Called after contest creation to configure the exchange from database
     * @param exchangeSent Sent exchange string (e.g., "1H WCF" for WFD)
     */
    virtual void setExchangeSent(const QString& exchangeSent) {
        m_exchangeSent = exchangeSent;
    }

    /**
     * Get contest-specific sent exchange
     * @return Sent exchange string (e.g., "1H WCF" for WFD)
     */
    virtual QString getExchangeSent() const {
        return m_exchangeSent;
    }

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

    /**
     * Get WA7BNM Contest Calendar ID for this contest
     * Returns the numeric contest ID from https://www.contestcalendar.com/
     * Used in N1MM+ contactinfo UDP broadcasts as <contestnr>
     *
     * @return WA7BNM contest calendar ID, or 0 if not assigned
     */
    virtual int getWA7BNMContestId() const { return 0; }  // Default: not assigned

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
     * Get configuration fields needed at contest creation time
     * Override this to declare what user input your contest needs
     * The ContestChooserDialog will dynamically create UI for each field
     *
     * Example for NAQP: returns Name and State fields
     * Example for ARRL SS: returns Check and Precedence fields
     * Example for WFD: returns Class and Section fields
     *
     * @return List of configuration fields, empty if none needed
     */
    virtual QList<ContestConfigField> getConfigFields() const {
        return QList<ContestConfigField>();  // Default: no config fields
    }

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
     * Does this contest include RST in the received exchange?
     * Auto-detects by checking if exchange fields contain "RST"
     * If true, RST will be prepended to exchangeReceived when logging QSO
     * @return true if exchange includes RST field
     */
    virtual bool includesRSTInReceivedExchange() const {
        QList<ExchangeField> fields = getReceivedExchangeFields();
        for (const ExchangeField& field : fields) {
            if (field.name.toUpper() == "RST") {
                return true;
            }
        }
        return false;  // No RST field found
    }

    /**
     * Parse received exchange and populate QSO fields
     * Populates dedicated QSO fields (arrlSection, serialNumber, contestClass, state, rstReceived)
     * and qso.parsedExchange only for contest-specific fields without dedicated members
     *
     * IMPORTANT: Must also set qso.exchangeReceived with complete exchange for Cabrillo export
     * - If contest includes RST: prepend rstReceived (e.g., "599 005")
     * - If no RST: use raw exchange (e.g., "DAVE CT" for NAQP)
     *
     * @param exchange Raw exchange string (without RST)
     * @param qso QSO object to populate
     */
    virtual void parseReceivedExchange(const QString& exchange, QSO& qso) const = 0;

    /**
     * Helper: Format exchangeReceived in canonical field order
     * Call this at the end of parseReceivedExchange() to set qso.exchangeReceived
     * Uses getReceivedExchangeFields() to determine field order
     * @param exchange Raw exchange string (unused, kept for compatibility)
     * @param qso QSO object (must have all fields already parsed)
     */
    void formatExchangeReceived(const QString& exchange, QSO& qso) const {
        Q_UNUSED(exchange);  // We build from parsed QSO fields, not raw input
        qso.exchangeReceived = buildCanonicalExchange(qso);
    }

    /**
     * Build exchange string in canonical field order from parsed QSO fields
     * Order is determined by getReceivedExchangeFields()
     * @param qso QSO with parsed fields
     * @return Exchange string in canonical order (e.g., "JOHN MA" not "MA JOHN")
     */
    QString buildCanonicalExchange(const QSO& qso) const {
        QStringList parts;

        for (const ExchangeField& field : getReceivedExchangeFields()) {
            QString value = getQSOFieldValue(qso, field.name);
            if (!value.isEmpty()) {
                parts.append(value);
            }
        }

        return parts.join(" ");
    }

    /**
     * Extract a field value from QSO by exchange field name
     * Maps field names (from ExchangeField.name) to QSO struct members
     * @param qso The QSO to extract from
     * @param fieldName The exchange field name (e.g., "RST", "Name", "State")
     * @return The field value, or empty string if not found
     */
    QString getQSOFieldValue(const QSO& qso, const QString& fieldName) const {
        // Map exchange field names to QSO struct members
        if (fieldName == "RST") return qso.rstReceived;
        if (fieldName == "Name") return qso.operatorName;
        if (fieldName == "State") return qso.state;
        if (fieldName == "County") return qso.county;
        if (fieldName == "Zone" || fieldName == "CQ Zone") {
            return qso.cqZone > 0 ? QString::number(qso.cqZone) : QString();
        }
        if (fieldName == "ITU Zone") {
            return qso.ituZone > 0 ? QString::number(qso.ituZone) : QString();
        }
        if (fieldName == "Serial") {
            return qso.serialNumberReceived > 0 ? QString::number(qso.serialNumberReceived) : QString();
        }
        if (fieldName == "Section") return qso.arrlSection;
        if (fieldName == "Class") return qso.contestClass;
        if (fieldName == "Check") return qso.check;
        if (fieldName == "Precedence") return qso.precedence;
        if (fieldName == "Power") return qso.power;
        if (fieldName == "Grid" || fieldName == "Grid Square") return qso.gridSquare;
        // Composite field: State OR Power (ARRL DX)
        if (fieldName == "State/Power") {
            return qso.state.isEmpty() ? qso.power : qso.state;
        }

        // Unknown field name - return empty
        return QString();
    }

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

    /**
     * Get list of bands allowed for this contest
     * Some contests have mode-specific restrictions (e.g., RTTY contests exclude 160m)
     * Field Day contests may include VHF/UHF bands if enabled in preferences
     *
     * Note: WARC bands (60m, 30m, 17m, 12m) are NEVER used in contests
     *
     * @return List of allowed HF bands for this contest
     */
    virtual QList<BandType> getAllowedBands() const {
        // Default: Standard HF contest bands (160m-10m)
        return { BandType::Band160M, BandType::Band80M, BandType::Band40M,
                 BandType::Band20M, BandType::Band15M, BandType::Band10M };
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
     *
     * Can use m_myStation to return location-dependent multipliers.
     * Example: ARRL DX returns Countries for W/VE, States for DX
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
     * Does this contest require an exchange to be entered?
     * Most contests require an exchange (default: true)
     * General Logging allows empty exchange (returns false)
     *
     * @return true if exchange is required, false if optional
     */
    virtual bool requiresExchange() const {
        return true;  // Default: most contests require an exchange
    }

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
