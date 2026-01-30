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

#include "CallsignValidator.h"

namespace TR4QT {

/**
 * Callsign validation algorithm adapted from TR4W's LooksLikeACallSign function
 * (LOGSTUFF.PAS lines 2302-2380)
 *
 * This approach counts transitions between letters and numbers instead of using regex.
 * It's more flexible and handles edge cases naturally.
 *
 * Key rules:
 * - Must have at least 2 transitions (letter→number or number→letter)
 * - Must end with a letter (callsigns ending with numbers are invalid)
 * - Can't start with two numbers (e.g., "12ABC" is invalid)
 * - Slash '/' can appear for portable/mobile modifiers but not at start/end
 *
 * Valid examples: W1AW, G3ABC, VK9CZ, 3A2MW, K1A, W1AW/P, KH6/W1AW
 * Invalid examples: TEST123 (ends with number), 123TEST (starts with numbers), AB (no transitions)
 */

enum class CharType {
    None,
    Letter,
    Number
};

QString CallsignValidator::stripModifiers(const QString& callsign) {
    // Not used in new algorithm - kept for API compatibility
    return callsign.trimmed().toUpper();
}

bool CallsignValidator::validate(const QString& callsign, QString* errorMessage) {
    if (callsign.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Callsign is empty";
        }
        return false;
    }

    QString call = callsign.trimmed().toUpper();

    // Check for slash at start or end (invalid)
    if (call.startsWith('/') || call.endsWith('/')) {
        if (errorMessage) {
            *errorMessage = QString("Callsign '%1' cannot start or end with '/'").arg(callsign);
        }
        return false;
    }

    CharType currentType = CharType::None;
    int transitionCount = 0;
    bool hasSlash = false;

    for (int i = 0; i < call.length(); i++) {
        QChar ch = call[i];

        if (ch >= 'A' && ch <= 'Z') {
            // Letter
            if (currentType == CharType::Number) {
                transitionCount++;
            }
            currentType = CharType::Letter;
        }
        else if (ch >= '0' && ch <= '9') {
            // Number

            // Callsigns can't end with numbers (unless there's a slash after)
            if (i == call.length() - 1) {
                if (errorMessage) {
                    *errorMessage = QString(
                        "Callsign '%1' ends with a number.\n"
                        "Valid callsigns end with letters (e.g., W1AW, not TEST123)."
                    ).arg(callsign);
                }
                return false;
            }

            // Callsigns can't start with two numbers (e.g., "12ABC" is invalid)
            if (currentType == CharType::Number && i == 1) {
                if (errorMessage) {
                    *errorMessage = QString(
                        "Callsign '%1' starts with two numbers.\n"
                        "Valid callsigns don't start with multiple digits."
                    ).arg(callsign);
                }
                return false;
            }

            if (currentType == CharType::Letter) {
                transitionCount++;
            }
            currentType = CharType::Number;
        }
        else if (ch == '/') {
            // Slash found - indicates portable/mobile modifier
            hasSlash = true;

            // If slash appears after position 3 and not at the end, it's valid
            // (e.g., W1AW/P, KH6/W1AW)
            if (i > 3 && i != call.length() - 1) {
                return true;  // Valid portable/mobile callsign
            }

            // If slash appears between positions 1 and length-1, could still be valid
            if (i > 1 && i < call.length() - 1) {
                return true;  // Valid portable/mobile callsign
            }
        }
        else {
            // Invalid character
            if (errorMessage) {
                *errorMessage = QString(
                    "Callsign '%1' contains invalid character '%2'.\n"
                    "Valid callsigns contain only letters, numbers, and '/' for portable/mobile."
                ).arg(callsign).arg(ch);
            }
            return false;
        }
    }

    // Final validation: must have at least 2 transitions and end with a letter
    bool valid = (transitionCount >= 2) && (currentType == CharType::Letter);

    if (!valid && errorMessage) {
        if (transitionCount < 2) {
            *errorMessage = QString(
                "Callsign '%1' doesn't look valid.\n"
                "Expected pattern: letters and numbers alternating (e.g., W1AW, G3ABC)."
            ).arg(callsign);
        } else if (currentType != CharType::Letter) {
            *errorMessage = QString(
                "Callsign '%1' doesn't end with a letter.\n"
                "Valid callsigns end with letters (e.g., W1AW, not TEST123)."
            ).arg(callsign);
        }
    }

    return valid;
}

bool CallsignValidator::isValid(const QString& callsign) {
    return validate(callsign, nullptr);
}

} // namespace TR4QT
