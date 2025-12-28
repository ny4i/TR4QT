#ifndef TYPES_H
#define TYPES_H

#include <QString>

namespace TR4QT {

// Band types (from TR4W's TRadioBand)
enum class BandType {
    None,
    Band160M,
    Band80M,
    Band60M,
    Band40M,
    Band30M,
    Band20M,
    Band17M,
    Band15M,
    Band12M,
    Band10M,
    Band6M,
    Band4M,
    Band2M,
    Band70CM
};

// Mode types (from TR4W's TRadioMode)
enum class ModeType {
    None,
    CW,
    CWR,      // CW Reverse
    LSB,
    USB,
    FM,
    AM,
    RTTY,
    RTTYR,    // RTTY Reverse
    PSK,
    PSKR,     // PSK Reverse
    FT8,
    FT4,
    DATA,     // Generic data mode
    DATAR     // Data reverse
};

// Mode groups for contest scoring and statistics
enum class ModeGroup {
    Phone,    // SSB, FM, AM
    CW,       // CW, CWR
    Digital   // RTTY, PSK, FT8, FT4, DATA
};

// VFO selection
enum class VFO {
    VFO_A,
    VFO_B
};

// Radio types (subset of TR4W's InterfacedRadioType, focusing on priority radios)
enum class RadioType {
    None,
    K2,
    K3,
    K4,
    IC7610,
    IC7760,
    IC7300,
    FT991A,
    FTDX10,
    FTDX101,
    HamlibGeneric  // For any hamlib-supported radio
};

// Multiplier types
enum class MultiplierType {
    Country,    // DXCC countries
    CQZone,     // CQ zones 1-40
    ITUZone,    // ITU zones
    State,      // US states/Canadian provinces
    Section,    // ARRL/RAC sections
    Prefix,     // Callsign prefix (for WPX)
    Grid,       // Maidenhead grid squares
    Custom      // Contest-specific multiplier
};

// Multiplier scope - per band or all bands combined
enum class MultiplierScope {
    PerBand,        // Multiplier counts separately on each band
    AllBands        // Multiplier counts once across all bands
};

// Duplicate checking rules
enum class DuplicateCheckingRule {
    PerBandMode,    // Same call on same band/mode is a dupe (allows same call on different bands)
    AllBandMode,    // Same call on same mode is a dupe across all bands
    PerBand,        // Same call on same band is a dupe (any mode)
    AllBand         // Same call is a dupe across all bands/modes (once-per-contest)
};

// Continent codes
enum class Continent {
    None,
    AF,  // Africa
    AS,  // Asia
    EU,  // Europe
    NA,  // North America
    SA,  // South America
    OC   // Oceania
};

// Helper functions
QString bandToString(BandType band);
QString modeToString(ModeType mode);
QString modeGroupToString(ModeGroup group);
ModeGroup modeTypeToModeGroup(ModeType mode);
BandType stringToBand(const QString& str);
ModeType stringToMode(const QString& str);
QString continentToString(Continent cont);

/**
 * Get the base frequency (band edge) for a band in kHz
 * Used for relative frequency entry (e.g., "300" on 15m -> 21300 kHz)
 * Returns 0 if band is None or unknown
 */
unsigned long bandToBaseFrequency(BandType band);

} // namespace TR4QT

#endif // TYPES_H
