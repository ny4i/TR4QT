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

#ifndef CALLSIGNVALIDATOR_H
#define CALLSIGNVALIDATOR_H

#include <QString>

namespace TR4QT {

/**
 * Callsign validation utility
 *
 * Validates amateur radio callsigns using a "broad international" regex pattern.
 *
 * IMPORTANT: No regex can match ALL valid callsigns globally due to:
 * - Special event callsigns (e.g., GB75RD)
 * - Temporary 1x1 callsigns (e.g., K1A, W0W)
 * - National variations from ITU Article 19
 * - Portable/mobile modifiers (/P, /M, /MM, /KH6, etc.)
 *
 * This validator uses a reasonable pattern that catches obvious typos (like TEST123)
 * while allowing legitimate edge cases to proceed with a warning.
 *
 * Pattern: ([A-Z]{1,2}|[0-9][A-Z]|[A-Z][0-9])[0-9][A-Z]{1,4}
 * - Prefix: 1-2 letters, OR digit+letter, OR letter+digit
 * - Separator: Single digit
 * - Suffix: 1-4 letters
 * - Optional: Portable/mobile modifier (e.g., /P, /M, /MM, /KH6)
 *
 * Valid examples: W1AW, G3ABC, VK9CZ, 3A2MW, K1A/P, KH6/W1AW
 * Invalid examples: TEST123, 123TEST, AB, ABCD1234XYZ
 */
class CallsignValidator {
public:
    /**
     * Validate a callsign against international patterns
     *
     * @param callsign Callsign to validate (case-insensitive)
     * @param errorMessage Set to human-readable message if invalid
     * @return true if callsign matches expected pattern, false otherwise
     */
    static bool validate(const QString& callsign, QString* errorMessage = nullptr);

    /**
     * Check if a callsign looks valid (same as validate but ignores errorMessage)
     *
     * @param callsign Callsign to check
     * @return true if valid, false if suspicious
     */
    static bool isValid(const QString& callsign);

private:
    CallsignValidator() = delete;  // Static utility class, no instances

    /**
     * Strip portable/mobile modifiers from callsign
     * Examples: W1AW/P → W1AW, KH6/W1AW → W1AW, W1AW/MM → W1AW
     *
     * @param callsign Full callsign with possible modifiers
     * @return Base callsign without modifiers
     */
    static QString stripModifiers(const QString& callsign);
};

} // namespace TR4QT

#endif // CALLSIGNVALIDATOR_H
