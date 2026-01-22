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
    QString contestName;            // ADIF Contest-ID (e.g., "CQ-WW-CW", "CQ-WPX-SSB")
    int contestNr{0};               // WA7BNM Contest Calendar ID
    QString stationName;            // Station name for multi-station setups

    // Timestamp
    QString timestamp;              // N1MM+ format: "YYYY-MM-DD HH:MM:SS"

    // Station identification
    QString mycall;                 // Station callsign
    QString call;                   // Worked callsign

    // Frequency and mode
    int freq{0};                    // Frequency in tens of Hz (N1MM+ format) - legacy/compat
    int rxfreq{0};                  // RX frequency in tens of Hz (N1MM+ format)
    int txfreq{0};                  // TX frequency in tens of Hz (N1MM+ format)
    QString band;                   // "1.8", "3.5", "7", "14", "21", "28", etc. (MHz format)
    QString mode;                   // "CW", "SSB", "RTTY", "FT8", etc.

    // Exchange - N1MM+ field names
    QString rstSent;                // RST sent (snt in XML)
    QString rstRcvd;                // RST received (rcv in XML)
    QString exchangeSent;           // Exchange sent
    QString exchangeRcvd;           // Exchange received (exchange1 in XML)

    // DXCC/Geographic info
    QString dxccPrefix;             // DXCC prefix (countryprefix in XML)
    QString wpxPrefix;              // WPX prefix (e.g., "W1", "DL7")
    QString stationPrefix;          // Station prefix
    QString continent;              // "NA", "SA", "EU", "AF", "AS", "OC"
    int cqZone{0};                  // CQ Zone (zone in XML)
    int ituZone{0};                 // ITU Zone (ituzone in XML, TR4QT extension)
    QString state;                  // US state or Canadian province
    QString gridsquare;             // Maidenhead grid locator
    QString section;                // ARRL section

    // Worked station info (per N1MM+ spec)
    QString qth;                    // Worked station's QTH
    QString name;                   // Worked station's operator name
    QString power;                  // Worked station's power

    // Contest scoring
    int points{0};                  // QSO points
    bool isDupe{false};             // Duplicate flag
    bool isMultiplier{false};       // Primary multiplier (ismultiplier1)
    bool isMultiplier2{false};      // Secondary multiplier
    bool isMultiplier3{false};      // Tertiary multiplier

    // Station info
    int radioNr{1};                 // Radio number (1 or 2 for SO2R)

    // Operator info
    QString operator_;              // Operator callsign (for multi-op contests)

    // Serial number (if contest uses them)
    int serialNumber{0};            // Our sent serial number (sntnr in XML)
    int serialNumberRcvd{0};        // Received serial number (rcvnr in XML)

    // Sweepstakes-specific fields
    QString prec;                   // Precedence (A, B, M, Q, S, U)
    QString ck;                     // Check (2-digit year first licensed)

    // Miscellaneous
    QString comment;                // Comment field
    QString misctext;               // Miscellaneous text

    // Run/S&P and rover
    QString run1run2;               // "1" for Run, "2" for S&P
    QString roverLocation;          // Rover grid location

    // Network and computer info
    QString radioInterfaced;        // Radio interfaced flag
    int networkedCompNr{0};         // Networked computer number
    QString netBiosName;            // Computer NetBIOS name

    // QSO flags
    bool isOriginal{true};          // Original QSO (not edited)
    bool isRunQSO{true};            // Run mode QSO (vs S&P)
    bool isClaimedQso{true};        // Claimed QSO for scoring

    // Edit tracking
    QString oldtimestamp;           // Original timestamp (for edits)
    QString oldcall;                // Original call (for edits)

    // Unique identifier
    QString id;                     // GUID for this QSO (32 hex chars, no hyphens)
};

} // namespace TR4QT

#endif // CONTACTINFO_H
