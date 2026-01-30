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

/**
 * @file FrequencyInputService.cpp
 * @brief Implementation of FrequencyInputService
 *
 * Extracted from MainWindow::onCallsignEnterPressed() - 55+ lines.
 */

#include "FrequencyInputService.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

FrequencyInputResult FrequencyInputService::parseFrequencyInput(const QString& input, BandType currentBand) const {
    FrequencyInputResult result;

    if (input.isEmpty()) {
        return result;
    }

    bool isNumeric = false;
    unsigned long targetFreqKHz = 0;

    // Check if input contains a decimal point (e.g., "14.200" for 14.200 MHz)
    if (input.contains('.')) {
        bool isDouble = false;
        double freqMHz = input.toDouble(&isDouble);
        if (isDouble && freqMHz > 0) {
            // Decimal entry - treat as MHz and convert to kHz
            targetFreqKHz = static_cast<unsigned long>(freqMHz * 1000.0);
            isNumeric = true;
            LOG_DEBUG("FrequencyInputService", QString("Decimal frequency entry: %1 MHz -> %2 kHz")
                .arg(freqMHz).arg(targetFreqKHz));
        }
    } else {
        // No decimal - try to parse as integer (kHz)
        unsigned long freqValue = input.toULong(&isNumeric);

        if (isNumeric && freqValue > 0) {
            // Determine if this is an offset or absolute frequency
            if (freqValue < 1000) {
                // Small number - treat as offset from band edge
                unsigned long bandEdge = getBandBaseFrequencyKHz(currentBand);
                if (bandEdge > 0) {
                    targetFreqKHz = bandEdge + freqValue;
                    LOG_DEBUG("FrequencyInputService", QString("Frequency offset entry: %1 + %2 = %3 kHz")
                        .arg(bandEdge).arg(freqValue).arg(targetFreqKHz));
                } else {
                    result.errorMessage = "Cannot determine band edge for current band";
                    return result;
                }
            } else {
                // Large number - treat as absolute frequency in kHz
                targetFreqKHz = freqValue;
                LOG_DEBUG("FrequencyInputService", QString("Absolute frequency entry: %1 kHz").arg(targetFreqKHz));
            }
        }
    }

    if (isNumeric && targetFreqKHz > 0) {
        result.isFrequency = true;
        result.frequencyHz = static_cast<freq_t>(targetFreqKHz) * 1000;
        result.statusMessage = QString("Frequency set to %1 kHz").arg(targetFreqKHz);
    }

    return result;
}

unsigned long FrequencyInputService::getBandBaseFrequencyKHz(BandType band) const {
    // Return lower edge of each band in kHz
    switch (band) {
        case BandType::Band160M: return 1800;
        case BandType::Band80M:  return 3500;
        case BandType::Band60M:  return 5330;   // 60m channel frequencies vary
        case BandType::Band40M:  return 7000;
        case BandType::Band30M:  return 10100;
        case BandType::Band20M:  return 14000;
        case BandType::Band17M:  return 18068;
        case BandType::Band15M:  return 21000;
        case BandType::Band12M:  return 24890;
        case BandType::Band10M:  return 28000;
        case BandType::Band6M:   return 50000;
        case BandType::Band2M:   return 144000;
        case BandType::Band70CM: return 420000;
        default: return 0;
    }
}

} // namespace TR4QT
