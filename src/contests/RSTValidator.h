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

#ifndef RSTVALIDATOR_H
#define RSTVALIDATOR_H

#include <QString>
#include <QRegularExpression>
#include "../core/Types.h"

namespace TR4QT {

/**
 * RSTValidator - Centralized RST (Readability-Strength-Tone) validation
 *
 * Provides consistent RST validation and default value generation across
 * all contest implementations. Eliminates duplicate validation logic.
 *
 * RST Format:
 * - First digit: 1-5 (Readability)
 * - Second digit: 1-9 (Signal Strength)
 * - Third digit: 1-9 (Tone quality) - required for CW, optional for phone
 *
 * Examples:
 * - CW modes: "599", "579", "339" (must be 3 digits)
 * - Phone modes: "59", "599", "33" (2 or 3 digits)
 */
class RSTValidator {
public:
    /**
     * Validate RST format based on mode
     *
     * @param rst The RST string to validate (e.g., "599", "59")
     * @param mode The operating mode (digital modes require 3 digits, phone allows 2 or 3)
     * @return true if RST is valid for the given mode, false otherwise
     *
     * Digital modes (CW, RTTY, PSK, FT8, etc.): Must be exactly 3 digits
     * Phone modes (SSB, FM, AM): Can be 2 or 3 digits
     *
     * Examples:
     *   isValid("599", ModeType::CW)    -> true
     *   isValid("59", ModeType::CW)     -> false (CW requires 3 digits)
     *   isValid("599", ModeType::RTTY)  -> true
     *   isValid("59", ModeType::RTTY)   -> false (RTTY requires 3 digits)
     *   isValid("59", ModeType::USB)    -> true
     *   isValid("599", ModeType::USB)   -> true (3 digits also valid for phone)
     */
    static bool isValid(const QString& rst, ModeType mode) {
        // Digital modes require exactly 3 digits [1-5][1-9][1-9]
        if (mode == ModeType::CW || mode == ModeType::CWR ||
            mode == ModeType::RTTY || mode == ModeType::RTTYR ||
            mode == ModeType::PSK || mode == ModeType::PSKR ||
            mode == ModeType::FT8 || mode == ModeType::FT4 ||
            mode == ModeType::DATA || mode == ModeType::DATAR) {
            QRegularExpression re("^[1-5][1-9][1-9]$");
            return re.match(rst).hasMatch();
        } else {
            // Phone modes: Can be 2 or 3 digits [1-5][1-9][1-9]?
            QRegularExpression re("^[1-5][1-9][1-9]?$");
            return re.match(rst).hasMatch();
        }
    }

    /**
     * Get default RST value for a given mode
     *
     * @param mode The operating mode
     * @return "599" for CW and digital modes, "59" for phone modes
     *
     * Digital modes (CW, RTTY, PSK, FT8, etc.) use 3-digit RST: "599"
     * Phone modes (SSB, FM, AM) use 2-digit RST: "59"
     *
     * Usage:
     *   QString rst = RSTValidator::getDefault(ModeType::CW);    // Returns "599"
     *   QString rst = RSTValidator::getDefault(ModeType::RTTY);  // Returns "599"
     *   QString rst = RSTValidator::getDefault(ModeType::USB);   // Returns "59"
     */
    static QString getDefault(ModeType mode) {
        // Digital modes use 3-digit RST (like CW)
        if (mode == ModeType::CW || mode == ModeType::CWR ||
            mode == ModeType::RTTY || mode == ModeType::RTTYR ||
            mode == ModeType::PSK || mode == ModeType::PSKR ||
            mode == ModeType::FT8 || mode == ModeType::FT4 ||
            mode == ModeType::DATA || mode == ModeType::DATAR) {
            return "599";
        }
        // Phone modes use 2-digit RST
        return "59";
    }
};

} // namespace TR4QT

#endif // RSTVALIDATOR_H
