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
 * @file FrequencyInputService.h
 * @brief Service for parsing frequency input from callsign field
 *
 * Extracted from MainWindow::onCallsignEnterPressed() - 55+ lines.
 * Handles various frequency input formats:
 * - Decimal MHz (14.200)
 * - Absolute kHz (14210)
 * - Offset from band edge (300)
 */

#ifndef FREQUENCYINPUTSERVICE_H
#define FREQUENCYINPUTSERVICE_H

#include <QString>
#include <hamlib/rig.h>  // For freq_t type
#include "../core/Types.h"

namespace TR4QT {

/**
 * @brief Result of frequency input parsing
 */
struct FrequencyInputResult {
    bool isFrequency = false;       ///< True if input was parsed as frequency
    freq_t frequencyHz = 0;         ///< Target frequency in Hz
    QString statusMessage;          ///< Status message for UI
    QString errorMessage;           ///< Error message if parsing failed
};

/**
 * @brief Service for parsing frequency input
 *
 * Parses user input that might be a frequency change command:
 * - "14.200" -> 14200 kHz (decimal MHz)
 * - "14210" -> 14210 kHz (absolute kHz)
 * - "300" -> band edge + 300 kHz (offset)
 */
class FrequencyInputService {
public:
    FrequencyInputService() = default;

    /**
     * @brief Parse input string as potential frequency
     *
     * @param input User input from callsign field
     * @param currentBand Current operating band (for offset calculation)
     * @return FrequencyInputResult with parsed frequency or indication it's not a frequency
     */
    FrequencyInputResult parseFrequencyInput(const QString& input, BandType currentBand) const;

private:
    /**
     * @brief Get base frequency (lower edge) for a band in kHz
     */
    unsigned long getBandBaseFrequencyKHz(BandType band) const;
};

} // namespace TR4QT

#endif // FREQUENCYINPUTSERVICE_H
