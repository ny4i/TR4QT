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

#ifndef CWTEMPLATE_ENGINE_H
#define CWTEMPLATE_ENGINE_H

#include <QString>
#include <QChar>
#include "core/Types.h"

namespace TR4QT {

/**
 * @brief CW Template Engine for TR4W-style message substitution
 *
 * Parses TR4W template syntax and substitutes variables with actual values.
 * Supports TR4W special characters:
 *   \ = My Call
 *   @ = His Call
 *   # = QSO Number (serial)
 *   ! = Serial Number (same as #)
 *   + = GMT time (HHMM format)
 *   ^ = Half space (Phase 1: treated as regular space)
 *   * = Salutation/Name from callsign (Phase 4)
 *   % = Name from names database (Phase 4)
 *   | = Name from exchange window (Phase 4)
 *   > = Reset RIT (Phase 4)
 *   [ = Wait for RST placeholder (Phase 4)
 */
class CWTemplateEngine {
public:
    /**
     * @brief Context for template variable substitution
     *
     * Contains all values that can be substituted into CW message templates
     */
    struct Context {
        QString myCall;               // User's callsign
        QString hisCall;              // Other station's callsign
        int qsoNumber;                // Current QSO serial number
        QString sentExchange;         // Full sent exchange string
        QString receivedExchange;     // Full received exchange string
        QString contestName;          // Active contest name
        ModeType mode;                // Current radio mode
        BandType band;                // Current radio band
        QString operatorName;         // Operator's name

        Context()
            : qsoNumber(0)
            , mode(ModeType::None)
            , band(BandType::None)
        {}
    };

    /**
     * @brief Substitute template variables with actual values
     *
     * @param templateStr Template string with TR4W special characters
     * @param ctx Context containing values for substitution
     * @return Substituted string ready to send as CW
     *
     * Example:
     *   Template: "CQ WFD \ \ TEST"
     *   Context: myCall = "NY4I"
     *   Result: "CQ WFD NY4I NY4I TEST"
     */
    static QString substitute(const QString& templateStr, const Context& ctx);

private:
    /**
     * @brief Substitute a single template variable
     *
     * @param var Template variable character (e.g., '\', '@', '#')
     * @param ctx Context for substitution
     * @return Substituted value, or original character if not recognized
     */
    static QString substituteVariable(QChar var, const Context& ctx);

    /**
     * @brief Format current GMT time as HHMM
     *
     * @return Time string in HHMM format (e.g., "1430")
     */
    static QString formatGMTTime();

    /**
     * @brief Expand special placeholder strings
     *
     * Handles:
     *   - "Set_by_the_MY_CALL" → "\ \" (my call twice)
     *   - "Set_by_S&P_EXCHANGE" → sent exchange template
     *
     * @param templateStr Input template that may contain special placeholders
     * @param ctx Context for substitution
     * @return Template with placeholders expanded
     */
    static QString expandPlaceholders(const QString& templateStr, const Context& ctx);
};

} // namespace TR4QT

#endif // CWTEMPLATE_ENGINE_H
