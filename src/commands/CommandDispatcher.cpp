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

    // Future commands can be added here:
    // if (normalized.startsWith("BAND ")) {
    //     QString band = normalized.mid(5).trimmed();
    //     return CommandResult(ChangeBand, true, band);
    // }

    // Not a recognized command
    return CommandResult(NotACommand, false);
}

} // namespace TR4QT
