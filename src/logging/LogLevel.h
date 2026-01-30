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

#ifndef LOGLEVEL_H
#define LOGLEVEL_H

#include <QString>

namespace TR4QT {

/**
 * Log levels matching TR4W's Log4D framework
 * Order from most verbose to least verbose
 */
enum class LogLevel {
    Trace = 0,  // Most verbose - trace execution flow
    Debug = 1,  // Debug information
    Info  = 2,  // Informational messages (default)
    Warn  = 3,  // Warning conditions
    Error = 4,  // Error conditions
    Fatal = 5,  // Fatal errors (program termination)
    Off   = 6   // Logging disabled
};

// String conversion
QString logLevelToString(LogLevel level);
LogLevel stringToLogLevel(const QString& str, LogLevel defaultLevel = LogLevel::Info);

// Short names for TR4W compatibility (%p placeholder)
// Returns: "trace", "debug", "info", "warn", "error", "fatal"
QString logLevelToShortString(LogLevel level);

} // namespace TR4QT

#endif // LOGLEVEL_H
