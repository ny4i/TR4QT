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
