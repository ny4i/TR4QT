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

#ifndef MORSETABLE_H
#define MORSETABLE_H

#include <QChar>
#include <QHash>
#include <QString>

namespace TR4QT {

/**
 * Static lookup table mapping characters to Morse code patterns.
 *
 * Pattern encoding: '.' = dit, '-' = dah
 * Example: 'A' -> ".-", 'B' -> "-..."
 *
 * Supports letters A-Z, digits 0-9, and common punctuation.
 * Unknown characters return an empty string.
 */
class MorseTable {
public:
    /**
     * Look up the Morse pattern for a character.
     * @param ch Character to look up (case-insensitive)
     * @return Pattern string (e.g., ".-" for 'A'), or empty if unknown
     */
    static QString pattern(QChar ch) {
        static const QHash<QChar, QString> table = buildTable();
        return table.value(ch.toUpper());
    }

    /**
     * Check if a character has a Morse representation.
     */
    static bool isKnown(QChar ch) {
        return !pattern(ch).isEmpty();
    }

    /**
     * Check if a character is a word separator (space).
     */
    static bool isWordSpace(QChar ch) {
        return ch == ' ';
    }

private:
    static QHash<QChar, QString> buildTable() {
        QHash<QChar, QString> t;

        // Letters
        t['A'] = ".-";
        t['B'] = "-...";
        t['C'] = "-.-.";
        t['D'] = "-..";
        t['E'] = ".";
        t['F'] = "..-.";
        t['G'] = "--.";
        t['H'] = "....";
        t['I'] = "..";
        t['J'] = ".---";
        t['K'] = "-.-";
        t['L'] = ".-..";
        t['M'] = "--";
        t['N'] = "-.";
        t['O'] = "---";
        t['P'] = ".--.";
        t['Q'] = "--.-";
        t['R'] = ".-.";
        t['S'] = "...";
        t['T'] = "-";
        t['U'] = "..-";
        t['V'] = "...-";
        t['W'] = ".--";
        t['X'] = "-..-";
        t['Y'] = "-.--";
        t['Z'] = "--..";

        // Digits
        t['0'] = "-----";
        t['1'] = ".----";
        t['2'] = "..---";
        t['3'] = "...--";
        t['4'] = "....-";
        t['5'] = ".....";
        t['6'] = "-....";
        t['7'] = "--...";
        t['8'] = "---..";
        t['9'] = "----.";

        // Punctuation
        t['.'] = ".-.-.-";    // Period
        t[','] = "--..--";    // Comma
        t['?'] = "..--..";    // Question mark
        t['/'] = "-..-.";     // Slash
        t['='] = "-...-";     // BT (double dash / break)
        t['+'] = ".-.-.";     // AR (end of message)
        t['-'] = "-....-";    // Hyphen
        t['@'] = ".--.-.";    // At sign

        // Prosigns (mapped to special chars)
        t['<'] = "-.-.-";     // KA (starting signal) - mapped to <
        t['>'] = "...-.-";    // SK (end of contact) - mapped to >

        return t;
    }
};

} // namespace TR4QT

#endif // MORSETABLE_H
