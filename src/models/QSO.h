#ifndef QSO_H
#define QSO_H

#include <QString>
#include <QDateTime>
#include <QMetaType>
#include "../core/Types.h"
#include <hamlib/rig.h>

namespace TR4QT {

/**
 * Represents a single QSO (contact) in the log
 *
 * This is the core data structure used throughout the application.
 * Contest classes receive QSO objects and determine scoring/multipliers.
 */
struct QSO {
    // Database ID
    int id{-1};                 // -1 = not yet saved to database

    // QSO timing
    QDateTime timestamp;        // When contact was made (UTC)

    // Station worked
    QString callsign;           // Callsign worked (normalized to uppercase)

    // Frequency/Mode/Band
    freq_t frequency{0};        // Frequency in Hz
    ModeType mode{ModeType::None};
    BandType band{BandType::None};

    // Exchange (contest-specific)
    QString rstSent{"599"};
    QString rstReceived{"599"};
    QString exchangeSent;       // What we sent (e.g., "599 005" for WPX)
    QString exchangeReceived;   // What we received (e.g., "599 14" for zones)

    // DXCC/Geographic info (from cty.dat lookup)
    QString dxccEntity;         // Country name (e.g., "United States")
    QString dxccPrefix;         // Primary prefix (e.g., "K")
    int cqZone{0};              // CQ Zone (1-40)
    int ituZone{0};             // ITU Zone
    QString continent;          // Continent code (NA, SA, EU, AF, AS, OC)
    QString state;              // US state or Canadian province (if applicable)

    // Contest-specific parsed fields (from exchangeReceived)
    // Populated by contest's parseReceivedExchange() method
    QMap<QString, QString> parsedExchange;

    // Scoring
    int qsoPoints{0};           // Points for this QSO (calculated by contest)
    bool isDupe{false};         // Duplicate contact?
    bool isMultiplier{false};   // Does this provide any new multipliers?
    QStringList multipliers;    // List of multiplier values this QSO provides

    // Metadata
    int serialNumber{0};        // Our sent serial number (if contest uses them)
    QString operatorCall;       // Operator who made this QSO (for multi-op contests)
    bool deleted{false};        // Soft delete flag
    QString notes;              // Optional notes

    /**
     * Normalize callsign (uppercase, trim whitespace)
     */
    void normalizeCallsign() {
        callsign = callsign.trimmed().toUpper();
    }

    /**
     * Check if QSO has been saved to database
     */
    bool isPersisted() const {
        return id >= 0;
    }

    /**
     * Check if QSO is valid (has minimum required fields)
     */
    bool isValid() const {
        return !callsign.isEmpty() &&
               timestamp.isValid() &&
               band != BandType::None &&
               mode != ModeType::None;
    }

    /**
     * Get a unique key for dupe checking
     * Most contests: callsign + band + mode
     * Some contests may override this logic
     */
    QString getDupeKey() const {
        return QString("%1_%2_%3")
            .arg(callsign)
            .arg(static_cast<int>(band))
            .arg(static_cast<int>(mode));
    }
};

} // namespace TR4QT

// Register QSO as Qt metatype for signals/slots
Q_DECLARE_METATYPE(TR4QT::QSO)

#endif // QSO_H
