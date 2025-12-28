#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QMutex>
#include <QtMessageHandler>
#include "LogLevel.h"
#include "FileAppender.h"

namespace TR4QT {

/**
 * TR4W-compatible logging system for TR4QT
 *
 * Features:
 * - TR4W log format: "%d %r [%t] %p %c %x - %m%n"
 * - Runtime configurable log level
 * - Rolling file appender
 * - Thread-safe operation
 * - Integration with Qt's message system
 */
class Logger {
public:
    // Singleton access
    static Logger& instance();

    // Configuration
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    void setFileLoggingEnabled(bool enabled);
    bool isFileLoggingEnabled() const;

    void setConsoleLoggingEnabled(bool enabled);
    bool isConsoleLoggingEnabled() const;

    void setLogFilePath(const QString& path);
    QString getLogFilePath() const;

    void setMaxFileSize(qint64 bytes);
    qint64 getMaxFileSize() const;

    void setMaxBackupFiles(int count);
    int getMaxBackupFiles() const;

    // Initialize/shutdown
    void initialize();
    void shutdown();

    // Message handler (called by Qt)
    static void messageHandler(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& msg);

    // Direct logging methods (for macro expansion)
    void log(LogLevel level, const char* category,
             const QString& message, const char* file = nullptr,
             int line = 0);

    // Elapsed time tracking
    qint64 elapsedMilliseconds() const;

    // Check if a log level is enabled
    bool isLevelEnabled(LogLevel level) const;

    // Get last N lines from log file (for error reporting)
    QString getLastLogLines(int lineCount = 50) const;

private:
    Logger();
    ~Logger();

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Internal message routing
    void handleMessage(LogLevel level, const QString& category,
                      const QString& message);

    // Convert Qt message type to LogLevel
    static LogLevel qtMsgTypeToLogLevel(QtMsgType type);

    // Extract category from file path
    static QString extractCategoryFromFile(const char* file);

    // Format and output message
    void outputMessage(LogLevel level, const QString& category,
                      const QString& message);

    mutable QMutex m_mutex;
    LogLevel m_logLevel;
    bool m_fileLoggingEnabled;
    bool m_consoleLoggingEnabled;
    QString m_logFilePath;
    FileAppender* m_fileAppender;
    QElapsedTimer m_startTime;  // For %r (elapsed milliseconds)
    bool m_initialized;
};

} // namespace TR4QT

#endif // LOGGER_H
