#ifndef CONTACTINFO_H
#define CONTACTINFO_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <hamlib/rig.h>

namespace TR4QT {

/**
 * ContactInfo - N1MM+ compatible QSO message
 *
 * Represents a logged QSO for UDP broadcast to external applications.
 * Uses N1MM+ ContactInfo XML format for compatibility.
 *
 * This message is sent immediately when a new QSO is logged.
 */
class ContactInfo {
public:
    ContactInfo();
    ~ContactInfo() = default;

    /**
     * Generate N1MM+ compatible XML message
     */
    QByteArray toXml() const;

    // Application identity
    QString app{"TR4QT"};
    QString contestName;            // e.g., "CQ-WW-CW", "CQ-WPX-SSB"
    QString stationName;            // Station name for multi-station setups

    // Timestamp
    QString timestamp;              // ISO 8601 format: "2025-12-24T12:34:56Z"

    // Station identification
    QString mycall;                 // Station callsign
    QString call;                   // Worked callsign

    // Frequency and mode
    int freq{0};                    // Frequency in tens of Hz (N1MM+ format)
    QString band;                   // "160M", "80M", "40M", "20M", "15M", "10M", etc.
    QString mode;                   // "CW", "SSB", "RTTY", "FT8", etc.

    // Exchange
    QString rstSent;                // RST sent (e.g., "599")
    QString rstRcvd;                // RST received (e.g., "599")
    QString exchangeSent;           // Exchange sent (e.g., "5" for CQ zone)
    QString exchangeRcvd;           // Exchange received (e.g., "14" for CQ zone)

    // DXCC/Geographic info
    QString dxccPrefix;             // DXCC prefix (e.g., "DL", "W", "JA")
    QString continent;              // "NA", "SA", "EU", "AF", "AS", "OC"
    int cqZone{0};                  // CQ Zone (1-40)
    int ituZone{0};                 // ITU Zone
    QString state;                  // US state or Canadian province

    // Contest scoring
    int points{0};                  // QSO points
    bool isDupe{false};             // Duplicate flag
    bool isMultiplier{false};       // New multiplier flag

    // Station info
    int radioNr{1};                 // Radio number (1 or 2 for SO2R)

    // Operator info
    QString operator_;              // Operator callsign (for multi-op contests)

    // Serial number (if contest uses them)
    int serialNumber{0};            // Our sent serial number
    int serialNumberRcvd{0};        // Received serial number (if applicable)
};

} // namespace TR4QT

#endif // CONTACTINFO_H
