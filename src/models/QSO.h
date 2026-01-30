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

#ifndef QSO_H
#define QSO_H

#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QUuid>
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
    QString guid;               // Globally Unique Identifier (UUID)

    // QSO timing
    QDateTime timestamp;        // When contact was made (UTC)

    // Station worked
    QString callsign;           // Callsign worked (normalized to uppercase)

    // Frequency/Mode/Band
    freq_t frequency{0};        // Frequency in Hz
    ModeType mode{ModeType::None};
    QString submode;            // ADIF SUBMODE (e.g., "FT4" when mode is MFSK, blank for FT8)
    BandType band{BandType::None};

    // Exchange (contest-specific)
    QString rstSent{"599"};
    QString rstReceived{"599"};
    QString exchangeSent;       // What we sent (e.g., "599 005" for WPX)
    QString exchangeReceived;   // What we received (e.g., "599 14" for zones)

    // DXCC/Geographic info (from cty.dat lookup)
    QString dxccEntity;         // Country name (e.g., "United States")
    QString dxccPrefix;         // Primary prefix (e.g., "K")
    int dxccEntityCode{0};      // ADIF DXCC Entity Code (e.g., 291 for USA)
    int cqZone{0};              // CQ Zone (1-40)
    int ituZone{0};             // ITU Zone
    QString continent;          // Continent code (NA, SA, EU, AF, AS, OC)

    // Core geographic fields (commonly used across many contests)
    // These prevent TR4W's anti-pattern of overloading a single QTH field
    QString state;              // US state or Canadian province (e.g., "MA", "ON")
    QString county;             // US county (for ARRL section mapping)
    QString arrlSection;        // ARRL/RAC section (WMA, NFL, SCV, AB, etc.)
    QString gridSquare;         // Maidenhead grid locator (e.g., "FN42", "DM79")
    QString iotaReference;      // IOTA reference (e.g., "NA-001", "EU-005")

    QString contestClass;       // Contest class (e.g., "2A" for Field Day, "M" for multi-op)

    // Contest exchange fields
    // All contests use a flat structure - space overhead is negligible for <10,000 QSOs
    int serialNumberReceived{0};    // Serial number received from other station
    QString precedence;             // Sweepstakes precedence (Q/A/B/M/U/S)
    QString check;                  // Sweepstakes check (last 2 digits of year)
    QString power;                  // Power level (e.g., "100" for ARRL DX)
    QString operatorName;           // Operator name (NAQP)
    QString ituZoneExchange;        // Raw ITU zone exchange (number or HQ/AC/R1/R2/R3 for IARU HF)

    // Scoring
    int qsoPoints{0};           // Points for this QSO (calculated by contest)
    bool isDupe{false};         // Duplicate contact?
    bool isMultiplier{false};   // Does this provide any new multipliers?
    QStringList multipliers;    // List of multiplier values this QSO provides

    // Metadata
    int serialNumber{0};        // Our sent serial number (if contest uses them)
    QString operatorCall;       // Operator who made this QSO (for multi-op contests)
    bool isRunQSO{false};       // Run vs S&P indicator (N1MM compatibility)
    bool deleted{false};        // Soft delete flag
    QString notes;              // Optional notes
    int radioNr{1};             // Which radio logged this QSO (1 or 2 for SO2R)

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
