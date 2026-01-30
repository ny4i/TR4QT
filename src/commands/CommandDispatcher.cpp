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
 * CommandDispatcher - Implementation
 */

#include "CommandDispatcher.h"

namespace TR4QT {

CommandDispatcher::CommandResult CommandDispatcher::parseCommand(const QString& input) {
    // Normalize input: trim whitespace and convert to uppercase
    QString normalized = input.trimmed().toUpper();

    // Check for empty input
    if (normalized.isEmpty()) {
        return CommandResult(NotACommand, false);
    }

    // Check for OPON command (change operator)
    if (normalized == "OPON") {
        return CommandResult(ChangeOperator, true);
    }

    // Check for UDP command (rebroadcast log)
    if (normalized == "UDP") {
        return CommandResult(RebroadcastLog, true);
    }

    // Check for /find command (QSO search)
    // Slash-prefixed commands are unambiguous vs callsigns (K1ABC/VE3 won't match)
    if (normalized == "/FIND") {
        return CommandResult(FindQSO, true);
    }

    // Future commands can be added here:
    // if (normalized.startsWith("BAND ")) {
    //     QString band = normalized.mid(5).trimmed();
    //     return CommandResult(ChangeBand, true, band);
    // }

    // Not a recognized command
    return CommandResult(NotACommand, false);
}

} // namespace TR4QT
