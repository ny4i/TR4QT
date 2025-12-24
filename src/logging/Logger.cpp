#include "Logger.h"
#include "LogFormatter.h"
#include <QDateTime>
#include <QThread>
#include <QFileInfo>
#include <iostream>

namespace TR4QT {

Logger::Logger()
    : m_logLevel(LogLevel::Info)
    , m_fileLoggingEnabled(true)
    , m_consoleLoggingEnabled(true)
    , m_fileAppender(nullptr)
    , m_initialized(false)
{
}

Logger::~Logger() {
    shutdown();
}

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::initialize() {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return;  // Already initialized
    }

    // Start elapsed timer
    m_startTime.start();

    // Create file appender if file logging enabled
    if (m_fileLoggingEnabled && !m_logFilePath.isEmpty()) {
        m_fileAppender = new FileAppender(m_logFilePath);
        if (!m_fileAppender->open()) {
            delete m_fileAppender;
            m_fileAppender = nullptr;
            std::cerr << "Warning: Failed to open log file: "
                     << m_logFilePath.toStdString() << std::endl;
        }
    }

    m_initialized = true;
}

void Logger::shutdown() {
    QMutexLocker locker(&m_mutex);

    if (m_fileAppender) {
        m_fileAppender->close();
        delete m_fileAppender;
        m_fileAppender = nullptr;
    }

    m_initialized = false;
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

LogLevel Logger::getLogLevel() const {
    QMutexLocker locker(&m_mutex);
    return m_logLevel;
}

void Logger::setFileLoggingEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);

    if (m_fileLoggingEnabled == enabled) {
        return;  // No change
    }

    m_fileLoggingEnabled = enabled;

    if (m_initialized) {
        if (enabled && !m_fileAppender && !m_logFilePath.isEmpty()) {
            // Enable file logging
            m_fileAppender = new FileAppender(m_logFilePath);
            m_fileAppender->open();
        } else if (!enabled && m_fileAppender) {
            // Disable file logging
            m_fileAppender->close();
            delete m_fileAppender;
            m_fileAppender = nullptr;
        }
    }
}

bool Logger::isFileLoggingEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_fileLoggingEnabled;
}

void Logger::setConsoleLoggingEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_consoleLoggingEnabled = enabled;
}

bool Logger::isConsoleLoggingEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_consoleLoggingEnabled;
}

void Logger::setLogFilePath(const QString& path) {
    QMutexLocker locker(&m_mutex);

    if (m_logFilePath == path) {
        return;  // No change
    }

    m_logFilePath = path;

    if (m_initialized && m_fileLoggingEnabled) {
        // Close old file
        if (m_fileAppender) {
            m_fileAppender->close();
            delete m_fileAppender;
            m_fileAppender = nullptr;
        }

        // Open new file
        if (!m_logFilePath.isEmpty()) {
            m_fileAppender = new FileAppender(m_logFilePath);
            m_fileAppender->open();
        }
    }
}

QString Logger::getLogFilePath() const {
    QMutexLocker locker(&m_mutex);
    return m_logFilePath;
}

void Logger::setMaxFileSize(qint64 bytes) {
    QMutexLocker locker(&m_mutex);
    if (m_fileAppender) {
        m_fileAppender->setMaxFileSize(bytes);
    }
}

qint64 Logger::getMaxFileSize() const {
    QMutexLocker locker(&m_mutex);
    if (m_fileAppender) {
        return m_fileAppender->maxFileSize();
    }
    return 10 * 1024 * 1024;  // Default 10MB
}

void Logger::setMaxBackupFiles(int count) {
    QMutexLocker locker(&m_mutex);
    if (m_fileAppender) {
        m_fileAppender->setMaxBackupFiles(count);
    }
}

int Logger::getMaxBackupFiles() const {
    QMutexLocker locker(&m_mutex);
    if (m_fileAppender) {
        return m_fileAppender->maxBackupFiles();
    }
    return 5;  // Default 5 backups
}

qint64 Logger::elapsedMilliseconds() const {
    return m_startTime.elapsed();
}

bool Logger::isLevelEnabled(LogLevel level) const {
    QMutexLocker locker(&m_mutex);
    return level >= m_logLevel;
}

void Logger::messageHandler(QtMsgType type,
                            const QMessageLogContext& context,
                            const QString& msg) {
    Logger& logger = Logger::instance();
    LogLevel level = qtMsgTypeToLogLevel(type);

    // Extract category from context or file
    QString category;
    if (context.category && QString(context.category) != "default") {
        category = context.category;
    } else if (context.file) {
        category = extractCategoryFromFile(context.file);
    } else {
        category = "Unknown";
    }

    logger.handleMessage(level, category, msg);
}

void Logger::log(LogLevel level, const char* category,
                const QString& message, const char* file, int line) {
    Q_UNUSED(file);
    Q_UNUSED(line);

    QString cat = category ? QString(category) : "Unknown";
    handleMessage(level, cat, message);
}

void Logger::handleMessage(LogLevel level, const QString& category,
                          const QString& message) {
    // Early exit if level is filtered
    if (!isLevelEnabled(level)) {
        return;
    }

    outputMessage(level, category, message);
}

void Logger::outputMessage(LogLevel level, const QString& category,
                          const QString& message) {
    QMutexLocker locker(&m_mutex);

    // Format message using TR4W pattern
    QDateTime timestamp = QDateTime::currentDateTime();
    qint64 elapsed = m_startTime.elapsed();
    Qt::HANDLE threadId = QThread::currentThreadId();

    QString formattedMessage = LogFormatter::format(
        timestamp, elapsed, threadId, level, category, message
    );

    // Output to console (stderr)
    if (m_consoleLoggingEnabled) {
        std::cerr << formattedMessage.toStdString();
    }

    // Output to file
    if (m_fileLoggingEnabled && m_fileAppender) {
        m_fileAppender->append(formattedMessage);
    }
}

LogLevel Logger::qtMsgTypeToLogLevel(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:    return LogLevel::Debug;
        case QtInfoMsg:     return LogLevel::Info;
        case QtWarningMsg:  return LogLevel::Warn;
        case QtCriticalMsg: return LogLevel::Error;
        case QtFatalMsg:    return LogLevel::Fatal;
        default:            return LogLevel::Info;
    }
}

QString Logger::extractCategoryFromFile(const char* file) {
    if (!file) {
        return "Unknown";
    }

    // Get filename without path: "/path/to/MainWindow.cpp" → "MainWindow.cpp"
    QString filePath(file);
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    // Remove extension: "MainWindow.cpp" → "MainWindow"
    int dotIndex = fileName.lastIndexOf('.');
    if (dotIndex > 0) {
        return fileName.left(dotIndex);
    }

    return fileName;
}

} // namespace TR4QT
