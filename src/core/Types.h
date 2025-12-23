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

// Contest types
enum class ContestType {
    None,
    CQWW_CW,
    CQWW_SSB,
    CQWPX_CW,
    CQWPX_SSB,
    WinterFieldDay,
    // More contests can be added later
    ARRL_DX_CW,
    ARRL_DX_SSB,
    IARU_HF
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
BandType stringToBand(const QString& str);
ModeType stringToMode(const QString& str);
QString continentToString(Continent cont);

} // namespace TR4QT

#endif // TYPES_H
