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

#ifndef BANDCONSTANTS_H
#define BANDCONSTANTS_H

#include "Types.h"

namespace TR4QT {
namespace BandConstants {

/**
 * Band edge frequencies (in Hz)
 *
 * These constants define the default operating frequencies for each amateur radio band.
 * Typically set to the CW/digital portion of each band for contest use.
 *
 * Usage: freq_t freq = BandConstants::BAND_20M_EDGE;  // 14.000 MHz
 */

// HF Bands
constexpr freq_t BAND_160M_EDGE = 1800000;    // 1.800 MHz (CW)
constexpr freq_t BAND_80M_EDGE  = 3500000;    // 3.500 MHz (CW)
constexpr freq_t BAND_60M_EDGE  = 5330000;    // 5.330 MHz (CW/Digital - US)
constexpr freq_t BAND_40M_EDGE  = 7000000;    // 7.000 MHz (CW)
constexpr freq_t BAND_30M_EDGE  = 10100000;   // 10.100 MHz (CW/Digital only)
constexpr freq_t BAND_20M_EDGE  = 14000000;   // 14.000 MHz (CW)
constexpr freq_t BAND_17M_EDGE  = 18068000;   // 18.068 MHz (CW)
constexpr freq_t BAND_15M_EDGE  = 21000000;   // 21.000 MHz (CW)
constexpr freq_t BAND_12M_EDGE  = 24890000;   // 24.890 MHz (CW)
constexpr freq_t BAND_10M_EDGE  = 28000000;   // 28.000 MHz (CW)

// VHF/UHF Bands
constexpr freq_t BAND_6M_EDGE   = 50000000;   // 50.000 MHz (CW)
constexpr freq_t BAND_4M_EDGE   = 70000000;   // 70.000 MHz (CW - EU)
constexpr freq_t BAND_2M_EDGE   = 144000000;  // 144.000 MHz (CW)
constexpr freq_t BAND_70CM_EDGE = 420000000;  // 420.000 MHz (CW - US)

/**
 * Convert BandType enum to frequency (Hz)
 *
 * Returns the default operating frequency for the specified band,
 * typically at the lower edge (CW/digital portion).
 *
 * @param band The band to convert
 * @return Frequency in Hz, or 0 for invalid/unknown bands
 *
 * @note This is the single source of truth for band-to-frequency conversion.
 *       All other code should use this function instead of duplicating logic.
 */
inline freq_t bandToFrequency(BandType band) {
    switch (band) {
        case BandType::Band160M:
            return BAND_160M_EDGE;
        case BandType::Band80M:
            return BAND_80M_EDGE;
        case BandType::Band60M:
            return BAND_60M_EDGE;
        case BandType::Band40M:
            return BAND_40M_EDGE;
        case BandType::Band30M:
            return BAND_30M_EDGE;
        case BandType::Band20M:
            return BAND_20M_EDGE;
        case BandType::Band17M:
            return BAND_17M_EDGE;
        case BandType::Band15M:
            return BAND_15M_EDGE;
        case BandType::Band12M:
            return BAND_12M_EDGE;
        case BandType::Band10M:
            return BAND_10M_EDGE;
        case BandType::Band6M:
            return BAND_6M_EDGE;
        case BandType::Band4M:
            return BAND_4M_EDGE;
        case BandType::Band2M:
            return BAND_2M_EDGE;
        case BandType::Band70CM:
            return BAND_70CM_EDGE;
        default:
            return 0;  // Invalid band
    }
}

/**
 * Check if a frequency is valid (non-zero)
 *
 * Helper function to check if bandToFrequency() returned a valid frequency.
 *
 * @param freq Frequency to check
 * @return true if frequency is valid (> 0), false otherwise
 */
inline bool isValidFrequency(freq_t freq) {
    return freq > 0;
}

} // namespace BandConstants
} // namespace TR4QT

#endif // BANDCONSTANTS_H
