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

#include "CWTiming.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

// Initialize the Morse code timing units lookup table
// Each entry is the total units for that character INCLUDING internal spacing
// Formula: sum of (dit=1 or dah=3) + (n-1) spacing between elements
const QMap<QChar, int> CWTiming::s_morseUnits = CWTiming::initMorseUnits();

QMap<QChar, int> CWTiming::initMorseUnits() {
    QMap<QChar, int> units;

    // Letters (A-Z)
    // STANDARD: intra-character spacing = 1 element, inter-character = 3, inter-word = 7
    // Formula: sum(dit=1, dah=3) + (n-1) intra-char spaces where n = number of symbols
    units['A'] = 5;   // .-     2 symbols: 1+3+1 = 5
    units['B'] = 9;   // -...   4 symbols: 3+1+1+1+3 = 9
    units['C'] = 11;  // -.-.   4 symbols: 3+1+3+1+3 = 11
    units['D'] = 7;   // -..    3 symbols: 3+1+1+2 = 7
    units['E'] = 1;   // .      1 symbol: 1 = 1
    units['F'] = 9;   // ..-.   4 symbols: 1+1+3+1+3 = 9
    units['G'] = 9;   // --.    3 symbols: 3+3+1+2 = 9
    units['H'] = 7;   // ....   4 symbols: 1+1+1+1+3 = 7
    units['I'] = 3;   // ..     2 symbols: 1+1+1 = 3
    units['J'] = 13;  // .---   4 symbols: 1+3+3+3+3 = 13
    units['K'] = 9;   // -.-    3 symbols: 3+1+3+2 = 9
    units['L'] = 9;   // .-..   4 symbols: 1+3+1+1+3 = 9
    units['M'] = 7;   // --     2 symbols: 3+3+1 = 7
    units['N'] = 5;   // -.     2 symbols: 3+1+1 = 5
    units['O'] = 11;  // ---    3 symbols: 3+3+3+2 = 11
    units['P'] = 11;  // .--.   4 symbols: 1+3+3+1+3 = 11
    units['Q'] = 13;  // --.-   4 symbols: 3+3+1+3+3 = 13
    units['R'] = 7;   // .-.    3 symbols: 1+3+1+2 = 7
    units['S'] = 5;   // ...    3 symbols: 1+1+1+2 = 5
    units['T'] = 3;   // -      1 symbol: 3 = 3
    units['U'] = 7;   // ..-    3 symbols: 1+1+3+2 = 7
    units['V'] = 9;   // ...-   4 symbols: 1+1+1+3+3 = 9
    units['W'] = 9;   // .--    3 symbols: 1+3+3+2 = 9
    units['X'] = 11;  // -..-   4 symbols: 3+1+1+3+3 = 11
    units['Y'] = 13;  // -.--   4 symbols: 3+1+3+3+3 = 13
    units['Z'] = 11;  // --..   4 symbols: 3+3+1+1+3 = 11

    // Numbers (0-9)
    units['0'] = 19;  // -----  5 symbols: 3+3+3+3+3+4 = 19
    units['1'] = 17;  // .----  5 symbols: 1+3+3+3+3+4 = 17
    units['2'] = 15;  // ..---  5 symbols: 1+1+3+3+3+4 = 15
    units['3'] = 13;  // ...--  5 symbols: 1+1+1+3+3+4 = 13
    units['4'] = 11;  // ....-  5 symbols: 1+1+1+1+3+4 = 11
    units['5'] = 9;   // .....  5 symbols: 1+1+1+1+1+4 = 9
    units['6'] = 11;  // -....  5 symbols: 3+1+1+1+1+4 = 11
    units['7'] = 13;  // --...  5 symbols: 3+3+1+1+1+4 = 13
    units['8'] = 15;  // ---..  5 symbols: 3+3+3+1+1+4 = 15
    units['9'] = 17;  // ----.  5 symbols: 3+3+3+3+1+4 = 17

    // Common punctuation (6 symbols each have 5 intra-char spaces)
    units['.'] = 17;  // .-.-.-  6 symbols: 1+3+1+3+1+3+5 = 17
    units[','] = 19;  // --..--  6 symbols: 3+3+1+1+3+3+5 = 19
    units['?'] = 15;  // ..--..  6 symbols: 1+1+3+3+1+1+5 = 15
    units['/'] = 11;  // -..-.   5 symbols: 3+1+1+3+1+4 = 11
    units['='] = 13;  // -...-   5 symbols: 3+1+1+1+3+4 = 13
    units['+'] = 11;  // .-.-.   5 symbols: 1+3+1+3+1+4 = 11
    units['-'] = 15;  // -....-  6 symbols: 3+1+1+1+1+3+5 = 15
    units['@'] = 15;  // .--.-.  6 symbols: 1+3+3+1+3+1+5 = 15
    units['('] = 15;  // -.--.   5 symbols: 3+1+3+3+1+4 = 15
    units[')'] = 17;  // -.--.-  6 symbols: 3+1+3+3+1+3+5 = 17

    // Special (prosigns often sent as single character)
    // SK (end of contact): ...-.- = 13 units
    // AR (end of message): .-.-. = 13 units
    // BT (break): -...- = 13 units

    return units;
}

int CWTiming::getCharacterUnits(QChar ch) {
    // Convert to uppercase for lookup
    QChar upper = ch.toUpper();

    // Space is special - it's a word gap (7 units) but we handle it in calculateDuration
    if (upper == ' ') {
        return 0;  // Handled separately
    }

    // Look up character in table
    if (s_morseUnits.contains(upper)) {
        return s_morseUnits[upper];
    }

    // Unsupported character - log warning and skip
    LOG_WARN("CWTiming", QString("Unsupported CW character: '%1' (0x%2)")
             .arg(ch).arg(static_cast<int>(ch.unicode()), 0, 16));
    return 0;
}

int CWTiming::calculateDuration(const QString& text, int wpm) {
    if (text.isEmpty() || wpm <= 0) {
        return 0;
    }

    // Standard: 1 unit = 1200ms at 1 WPM
    // At N WPM: 1 unit = 1200/N ms
    const double msPerUnit = 1200.0 / wpm;

    int totalUnits = 0;
    bool lastWasSpace = false;
    bool firstChar = true;

    for (const QChar& ch : text) {
        if (ch == ' ') {
            // Word space = 7 units (includes the 3-unit char space)
            // But we already added 3-unit char space after last char, so add 4 more
            if (!lastWasSpace && !firstChar) {
                totalUnits += 4;  // 7 total - 3 already counted
            }
            lastWasSpace = true;
        } else {
            int charUnits = getCharacterUnits(ch);
            if (charUnits > 0) {
                // Add character units
                totalUnits += charUnits;

                // Add inter-character space (3 units) - except before first char
                if (!firstChar) {
                    totalUnits += 3;
                }

                firstChar = false;
                lastWasSpace = false;
            }
        }
    }

    // Convert units to milliseconds and round
    int durationMs = static_cast<int>(totalUnits * msPerUnit + 0.5);

    LOG_DEBUG("CWTiming", QString("CW '%1' at %2 WPM: %3 units = %4ms")
              .arg(text).arg(wpm).arg(totalUnits).arg(durationMs));

    return durationMs;
}

} // namespace TR4QT
