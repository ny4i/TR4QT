#ifndef CWTIMING_H
#define CWTIMING_H

#include <QString>
#include <QMap>

namespace TR4QT {

/**
 * @brief Accurate CW (Morse Code) timing calculator
 *
 * Uses standard Morse code timing units:
 * - Dit: 1 unit
 * - Dah: 3 units
 * - Space between elements (within character): 1 unit
 * - Space between characters: 3 units
 * - Space between words: 7 units
 *
 * Standard timing reference: "PARIS" = 50 units
 * At 1 WPM: 1 unit = 1200ms (60,000ms/min ÷ 50 units)
 * At N WPM: 1 unit = 1200/N ms
 *
 * Example: "CQ" at 20 WPM
 * C (-.-.): 3+1+1+1+3+1+1+1+3 = 15 units
 * Space: 3 units
 * Q (--.-): 3+1+3+1+1+1+3 = 13 units
 * Total: 15+3+13 = 31 units
 * Duration: 31 * (1200/20) = 31 * 60 = 1860ms
 */
class CWTiming {
public:
    /**
     * @brief Calculate accurate CW transmission duration
     * @param text Text to send (supports A-Z, 0-9, space, and common punctuation)
     * @param wpm Speed in words per minute
     * @return Duration in milliseconds
     */
    static int calculateDuration(const QString& text, int wpm);

    /**
     * @brief Get timing units for a single character
     * @param ch Character (case-insensitive)
     * @return Number of timing units (0 if unsupported character)
     */
    static int getCharacterUnits(QChar ch);

private:
    // Morse code patterns (number of units including internal spacing)
    static const QMap<QChar, int> s_morseUnits;

    // Initialize the lookup table
    static QMap<QChar, int> initMorseUnits();
};

} // namespace TR4QT

#endif // CWTIMING_H
