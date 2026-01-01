#ifndef RADIOINFO_H
#define RADIOINFO_H

#include <QString>
#include <QByteArray>
#include <hamlib/rig.h>

namespace TR4QT {

/**
 * RadioInfo - N1MM+ compatible radio state message
 *
 * Represents current radio state for UDP broadcast to external applications.
 * Uses N1MM+ RadioInfo XML format for compatibility.
 *
 * Frequency representation:
 * - Internally hamlib uses Hz (e.g., 14025000 Hz)
 * - N1MM+ uses "tens of Hz" (e.g., 1402500 for 14.025 MHz)
 * - Conversion: tens_of_hz = hz / 10
 */
class RadioInfo {
public:
    RadioInfo();
    ~RadioInfo() = default;

    /**
     * Generate N1MM+ compatible XML message
     */
    QByteArray toXml() const;

    // Application identity
    QString app{"TR4QT"};
    QString stationName;

    // Radio identification
    int radioNr{1};                 // Radio number (1 or 2 for SO2R)
    QString radioName;              // e.g., "K4", "IC-7610"

    // Frequencies (in tens of Hz for N1MM+ compatibility)
    // Example: 14.025 MHz = 14,025,000 Hz = 1,402,500 tens of Hz
    int freq{0};                    // RX frequency in tens of Hz
    int txFreq{0};                  // TX frequency in tens of Hz

    // Operating parameters
    QString mode;                   // "CW", "SSB", "RTTY", "FT8", etc.
    QString mycall;                 // Station callsign
    QString opCall;                 // Operator callsign

    // Status flags
    bool isRunning{false};          // Contest running
    bool isTransmitting{false};     // Currently transmitting
    bool isSplit{false};            // Split mode enabled
    bool isStereo{false};           // Stereo mode (for SO2R)
    bool isConnected{true};         // Radio connected
    bool isRunMode{false};          // CQ/Run mode (true) vs S&P mode (false)

    // UI state (for N1MM+ compatibility, mostly unused in TR4QT)
    int focusEntry{0};
    int entryWindowHwnd{0};
    int focusRadioNr{1};
    int activeRadioNr{1};
    QString functionKeyCaption;

    // Antenna/Rotor
    int antenna{0};
    QString rotors;
    int auxAntSelected{-1};
    QString auxAntSelectedName;

    // Frequency conversion utilities
    static int hzToTensOfHz(freq_t hz);
    static freq_t tensOfHzToHz(int tensOfHz);

    // Alternative conversions (for compatibility)
    static int mhzToTensOfHz(double mhz);
    static double tensOfHzToMhz(int tensOfHz);
};

} // namespace TR4QT

#endif // RADIOINFO_H
