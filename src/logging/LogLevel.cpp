#include "LogLevel.h"

namespace TR4QT {

QString logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "Trace";
        case LogLevel::Debug: return "Debug";
        case LogLevel::Info:  return "Info";
        case LogLevel::Warn:  return "Warn";
        case LogLevel::Error: return "Error";
        case LogLevel::Fatal: return "Fatal";
        case LogLevel::Off:   return "Off";
        default: return "Unknown";
    }
}

LogLevel stringToLogLevel(const QString& str, LogLevel defaultLevel) {
    QString lower = str.toLower();

    if (lower == "trace") return LogLevel::Trace;
    if (lower == "debug") return LogLevel::Debug;
    if (lower == "info")  return LogLevel::Info;
    if (lower == "warn" || lower == "warning") return LogLevel::Warn;
    if (lower == "error") return LogLevel::Error;
    if (lower == "fatal") return LogLevel::Fatal;
    if (lower == "off")   return LogLevel::Off;

    return defaultLevel;
}

QString logLevelToShortString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "trace";
        case LogLevel::Debug: return "debug";
        case LogLevel::Info:  return "info";
        case LogLevel::Warn:  return "warn";
        case LogLevel::Error: return "error";
        case LogLevel::Fatal: return "fatal";
        case LogLevel::Off:   return "off";
        default: return "unknown";
    }
}

} // namespace TR4QT
