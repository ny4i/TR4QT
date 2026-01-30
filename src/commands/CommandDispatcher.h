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
 * CommandDispatcher - Parse special commands in callsign field
 *
 * Extracted from MainWindow::onLogQSO() as part of Phase 1 god class refactoring.
 *
 * Design: Stateless utility class - pure command parsing, no execution.
 * Execution remains in MainWindow (requires UI access for dialogs).
 *
 * Commands supported:
 * - OPON: Change operator
 * - UDP: Rebroadcast log
 * - (Future: BAND, MODE, FREQ, etc.)
 */

#ifndef COMMANDDISPATCHER_H
#define COMMANDDISPATCHER_H

#include <QString>

namespace TR4QT {

/**
 * Stateless command parser for special callsign field commands
 *
 * Usage:
 *   CommandDispatcher::CommandResult result = CommandDispatcher::parseCommand(input);
 *   if (result.wasCommand) {
 *       // Handle command based on result.type
 *   } else {
 *       // Normal QSO logging
 *   }
 */
class CommandDispatcher {
public:
    /**
     * Types of recognized commands
     */
    enum CommandType {
        NotACommand,        // Input is not a command
        ChangeOperator,     // OPON - Open operator dialog
        RebroadcastLog,     // UDP - Rebroadcast entire log via UDP
        FindQSO,            // /find - Open QSO search dialog
        // Future commands:
        // ChangeBand,      // BAND <band> - Change band
        // ChangeMode,      // MODE <mode> - Change mode
        // SetFrequency,    // FREQ <khz> - Set frequency
    };

    /**
     * Result of command parsing
     */
    struct CommandResult {
        CommandType type;      // Type of command (or NotACommand)
        bool wasCommand;       // True if input was recognized as command
        QString payload;       // Command argument (empty for commands without args)

        // Constructor for convenience
        CommandResult()
            : type(NotACommand), wasCommand(false) {}

        CommandResult(CommandType t, bool was, const QString& p = QString())
            : type(t), wasCommand(was), payload(p) {}
    };

    /**
     * Parse input to check if it's a command
     *
     * @param input Raw input from callsign field (will be trimmed and uppercased)
     * @return CommandResult with type and payload
     *
     * Examples:
     *   parseCommand("OPON")     → {ChangeOperator, true, ""}
     *   parseCommand("  opon  ") → {ChangeOperator, true, ""} (case insensitive, trimmed)
     *   parseCommand("UDP")      → {RebroadcastLog, true, ""}
     *   parseCommand("K1ABC")    → {NotACommand, false, ""}
     */
    static CommandResult parseCommand(const QString& input);

private:
    // Private constructor - stateless utility class, no instantiation needed
    CommandDispatcher() = delete;
};

} // namespace TR4QT

#endif // COMMANDDISPATCHER_H
