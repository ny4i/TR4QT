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

#include "CWTemplateEngine.h"
#include <QDateTime>
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

QString CWTemplateEngine::substitute(const QString& templateStr, const Context& ctx) {
    // First expand special placeholder strings
    QString expanded = expandPlaceholders(templateStr, ctx);

    // Then substitute individual template variables
    QString result;
    result.reserve(expanded.length() * 2);  // Reserve space for expansion

    for (int i = 0; i < expanded.length(); i++) {
        QChar ch = expanded[i];

        // Check if this is a template variable
        if (ch == '\\' || ch == '@' || ch == '#' || ch == '!' || ch == '+' || ch == '^') {
            // Substitute the variable
            QString substitution = substituteVariable(ch, ctx);
            result.append(substitution);
        } else {
            // Regular character - keep as-is
            result.append(ch);
        }
    }

    LOG_DEBUG("CWTemplateEngine", QString("Template: %1 → Result: %2")
              .arg(templateStr).arg(result));

    return result;
}

QString CWTemplateEngine::substituteVariable(QChar var, const Context& ctx) {
    switch (var.unicode()) {
        case '\\':
            // My Call
            return ctx.myCall;

        case '@':
            // His Call
            return ctx.hisCall;

        case '#':
        case '!': {
            // QSO Number (serial) - use configurable width and cut numbers
            int width = AppSettings::instance().getSerialNumberWidth();
            QString serial = QString::number(ctx.qsoNumber).rightJustified(width, '0');

            // Apply cut numbers if enabled (replace digits with SHORT messages)
            if (AppSettings::instance().getCutNumbersEnabled()) {
                QString cutSerial;
                for (QChar ch : serial) {
                    if (ch.isDigit()) {
                        cutSerial += AppSettings::instance().getShortMessage(ch.digitValue());
                    } else {
                        cutSerial += ch;  // Keep non-digit characters as-is
                    }
                }
                return cutSerial;
            }

            return serial;
        }

        case '+':
            // GMT Time (HHMM format)
            return formatGMTTime();

        case '^':
            // Half space - Phase 1: treat as regular space
            // TODO Phase 4: Implement proper half-space timing
            return " ";

        default:
            // Unknown variable - log warning and return original character
            LOG_WARN("CWTemplateEngine", QString("Unknown template variable: %1").arg(var));
            return QString(var);
    }
}

QString CWTemplateEngine::formatGMTTime() {
    QDateTime now = QDateTime::currentDateTimeUtc();
    return now.toString("HHmm");  // HHMM format (e.g., "1430")
}

QString CWTemplateEngine::expandPlaceholders(const QString& templateStr, const Context& ctx) {
    QString result = templateStr;

    // Expand "Set_by_the_MY_CALL" to "\ \" (my call twice)
    // This placeholder is used in S&P mode F1 to send my callsign
    result.replace("Set_by_the_MY_CALL", "\\ \\");

    // Expand "Set_by_S&P_EXCHANGE" to the contest's sent exchange
    // This placeholder is used in S&P mode F2 to send my exchange
    if (result.contains("Set_by_S&P_EXCHANGE")) {
        if (!ctx.sentExchange.isEmpty()) {
            result.replace("Set_by_S&P_EXCHANGE", ctx.sentExchange);
        } else {
            // No exchange defined - use default RST + serial
            QString defaultExchange = QString("599 #");
            result.replace("Set_by_S&P_EXCHANGE", defaultExchange);
            LOG_INFO("CWTemplateEngine", "No sent exchange defined, using default: 599 #");
        }
    }

    return result;
}

} // namespace TR4QT
