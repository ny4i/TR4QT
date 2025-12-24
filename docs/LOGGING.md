# TR4QT Logging System

## Overview

TR4QT uses a centralized logging system that provides categorized, leveled logging with support for both file and console output. The logging system is designed to be lightweight, flexible, and easy to use.

## Architecture

The logging system consists of three main components:

1. **Logger** (`src/logging/Logger.h/cpp`) - Singleton logger that manages log output
2. **LogMacros** (`src/logging/LogMacros.h`) - Convenience macros for easy logging
3. **Appenders** - Output handlers (FileAppender, ConsoleAppender)

## Log Levels

The system supports six log levels (from lowest to highest severity):

- `Trace` - Very detailed diagnostic information
- `Debug` - Debugging information for development
- `Info` - General informational messages
- `Warn` - Warning messages for potentially problematic situations
- `Error` - Error messages for recoverable errors
- `Fatal` - Critical errors that may cause application termination

## Using the Logging System

### Basic Usage

1. Include the logging header in your source file:
```cpp
#include "../logging/LogMacros.h"  // Adjust path as needed
```

2. Use the logging macros with a category and message:
```cpp
LOG_INFO("MyClass", "Application started");
LOG_DEBUG("MyClass", "Processing item");
LOG_WARN("MyClass", "Resource usage high");
LOG_ERROR("MyClass", "Failed to open file");
```

### Available Macros

**Simple logging:**
- `LOG_TRACE(category, message)`
- `LOG_DEBUG(category, message)`
- `LOG_INFO(category, message)`
- `LOG_WARN(category, message)`
- `LOG_ERROR(category, message)`
- `LOG_FATAL(category, message)`

**Printf-style formatting:**
- `LOG_TRACE_F(category, format, ...)`
- `LOG_DEBUG_F(category, format, ...)`
- `LOG_INFO_F(category, format, ...)`
- `LOG_WARN_F(category, format, ...)`
- `LOG_ERROR_F(category, format, ...)`
- `LOG_FATAL_F(category, format, ...)`

### Category Naming

Use meaningful category names that identify the source of the log message:

- Use the class name for class-based code: `"MainWindow"`, `"RadioController"`
- Use a functional category for utility code: `"Database"`, `"Network"`, `"FileIO"`
- Use subsystem names for modules: `"DXCluster"`, `"BandMap"`, `"Backup"`

**Examples:**
```cpp
LOG_DEBUG("MainWindow", "Window initialized");
LOG_INFO("RadioController", "Connected to radio");
LOG_ERROR("Database", "Query failed");
```

### Formatting Log Messages

For simple string messages, pass the QString directly:
```cpp
LOG_INFO("MyClass", "Simple message");
```

For messages with variables, use QString's arg() method:
```cpp
QString filename = "logbook.db";
LOG_INFO("Database", QString("Opening database: %1").arg(filename));

int count = 42;
LOG_DEBUG("QSORepository", QString("Loaded %1 QSOs").arg(count));

// Multiple arguments
LOG_DEBUG("MainWindow", QString("Frequency: %1 Hz, Mode: %2")
    .arg(frequency)
    .arg(mode));
```

For boolean values, convert to string:
```cpp
bool connected = true;
LOG_DEBUG("Radio", QString("Connected: %1")
    .arg(connected ? "true" : "false"));
```

For numeric values with formatting:
```cpp
double freq = 14074000.0;
LOG_INFO("Radio", QString("Frequency: %1 Hz").arg(QString::number(freq)));
```

### Printf-Style Formatting (Alternative)

The `_F` macros provide printf-style formatting:
```cpp
LOG_DEBUG_F("MyClass", "Value: %d, String: %s", 42, "hello");
LOG_INFO_F("Radio", "Frequency: %.3f MHz", frequency / 1000000.0);
```

## Migrating from qDebug/qWarning

When converting existing code from Qt's qDebug/qWarning:

**Before:**
```cpp
qDebug() << "Processing" << count << "items";
qWarning() << "Failed to open file:" << filename;
qCritical() << "Database error:" << error;
```

**After:**
```cpp
LOG_DEBUG("MyClass", QString("Processing %1 items").arg(count));
LOG_WARN("MyClass", QString("Failed to open file: %1").arg(filename));
LOG_ERROR("MyClass", QString("Database error: %1").arg(error));
```

**Key differences:**
- `qDebug()` → `LOG_DEBUG(category, ...)`
- `qWarning()` → `LOG_WARN(category, ...)`
- `qInfo()` → `LOG_INFO(category, ...)`
- `qCritical()` → `LOG_ERROR(category, ...)`
- Streaming (`<<`) → QString formatting (`.arg()`)

## Configuration

Logging settings can be configured via `AppSettings`:

```cpp
AppSettings& settings = AppSettings::instance();

// Set log level
settings.setLogLevel(LogLevel::Debug);

// Enable/disable file logging
settings.setFileLoggingEnabled(true);
settings.setLogFilePath("/path/to/logfile.log");

// Enable/disable console logging
settings.setConsoleLoggingEnabled(true);

// Set file rotation
settings.setLogMaxFileSize(10 * 1024 * 1024);  // 10 MB
settings.setLogMaxBackupFiles(5);
```

## Log Output Format

Log messages are formatted as:
```
YYYY-MM-DD HH:MM:SS.mmm elapsed [thread_id] level category - message
```

**Example:**
```
2025-12-24 15:42:34.076 0 [8775016576] info TR4QTMain - TR4QT Version 2.40.2
2025-12-24 15:42:34.165 90 [8775016576] info TR4QTMain - Hamlib backends loaded
2025-12-24 15:42:42.342 8267 [6129414144] warn RadioController - rig_open failed
```

## Best Practices

### 1. Choose Appropriate Log Levels

- **Trace**: Detailed flow tracking, variable dumps (usually disabled in production)
- **Debug**: Development debugging, state changes, method entry/exit
- **Info**: Important application events (startup, shutdown, major state changes)
- **Warn**: Recoverable errors, deprecated features, unusual conditions
- **Error**: Error conditions that prevent normal operation
- **Fatal**: Critical errors that require application termination

### 2. Use Meaningful Messages

**Good:**
```cpp
LOG_ERROR("Database", QString("Failed to execute query: %1").arg(query));
LOG_INFO("Contest", QString("Loaded %1 QSOs from %2").arg(count).arg(filename));
```

**Bad:**
```cpp
LOG_ERROR("Database", "Error");  // Too vague
LOG_DEBUG("MainWindow", QString("x=%1").arg(x));  // Not descriptive
```

### 3. Don't Log Sensitive Information

Avoid logging passwords, API keys, or personal information:
```cpp
// WRONG
LOG_DEBUG("Auth", QString("Password: %1").arg(password));

// RIGHT
LOG_DEBUG("Auth", "Authentication attempted");
```

### 4. Use Consistent Categories

Stick to a consistent naming scheme within each file:
```cpp
// In MainWindow.cpp, always use "MainWindow"
LOG_DEBUG("MainWindow", "Window created");
LOG_INFO("MainWindow", "Contest loaded");

// Don't mix categories in the same file
// LOG_DEBUG("Main", ...)  // Inconsistent
// LOG_DEBUG("Window", ...)  // Inconsistent
```

### 5. Log Exceptions and Errors

Always log error conditions with context:
```cpp
if (!file.open(QIODevice::ReadOnly)) {
    LOG_ERROR("FileIO", QString("Failed to open file: %1 - %2")
        .arg(filename)
        .arg(file.errorString()));
    return false;
}
```

### 6. Avoid Logging in Tight Loops

High-frequency logging can impact performance:
```cpp
// WRONG - logs every iteration
for (int i = 0; i < 1000000; i++) {
    LOG_TRACE("Processing", QString("Item %1").arg(i));
    processItem(i);
}

// RIGHT - log summary
LOG_DEBUG("Processing", QString("Processing %1 items").arg(count));
for (int i = 0; i < count; i++) {
    processItem(i);
}
LOG_DEBUG("Processing", "Processing complete");
```

## Advanced Usage

### Custom Appenders

You can create custom appenders by inheriting from `LogAppender`:

```cpp
class MyCustomAppender : public LogAppender {
public:
    void append(LogLevel level, const QString& category,
                const QString& message, const QString& file,
                int line) override {
        // Custom handling
    }
};

// Register with logger
Logger::instance().addAppender(new MyCustomAppender());
```

### Runtime Log Level Changes

Change the log level at runtime:
```cpp
// Show only warnings and errors
Logger::instance().setLogLevel(LogLevel::Warn);

// Show everything (verbose debugging)
Logger::instance().setLogLevel(LogLevel::Trace);
```

### Conditional Logging

For expensive log message formatting, check level first:
```cpp
if (Logger::instance().isEnabled(LogLevel::Debug)) {
    QString expensiveDebugInfo = generateDetailedDump();
    LOG_DEBUG("MyClass", expensiveDebugInfo);
}
```

## Troubleshooting

### Logs Not Appearing

1. Check if the log level is appropriate:
   ```cpp
   AppSettings::instance().setLogLevel(LogLevel::Debug);
   ```

2. Verify appenders are enabled:
   ```cpp
   AppSettings::instance().setFileLoggingEnabled(true);
   AppSettings::instance().setConsoleLoggingEnabled(true);
   ```

3. Check file permissions for log file directory

### Log File Growing Too Large

Configure file rotation:
```cpp
settings.setLogMaxFileSize(10 * 1024 * 1024);  // 10 MB
settings.setLogMaxBackupFiles(5);  // Keep 5 old files
```

### Performance Issues

1. Increase log level to reduce output
2. Disable console logging in production
3. Avoid logging in performance-critical code paths
4. Use conditional logging for expensive operations

## Migration Checklist

When converting a file to use the logging system:

- [ ] Add `#include "../logging/LogMacros.h"` (adjust path as needed)
- [ ] Choose a consistent category name for the file
- [ ] Replace all `qDebug()` with `LOG_DEBUG(category, ...)`
- [ ] Replace all `qWarning()` with `LOG_WARN(category, ...)`
- [ ] Replace all `qInfo()` with `LOG_INFO(category, ...)`
- [ ] Replace all `qCritical()` with `LOG_ERROR(category, ...)`
- [ ] Convert streaming syntax (`<<`) to QString `.arg()` formatting
- [ ] Test that all log messages still appear correctly
- [ ] Verify log level filtering works as expected

## Examples by Use Case

### Application Lifecycle
```cpp
LOG_INFO("TR4QTMain", "Application starting");
LOG_INFO("TR4QTMain", QString("Version %1").arg(APP_VERSION));
LOG_INFO("TR4QTMain", "Application shutting down");
```

### Radio Operations
```cpp
LOG_INFO("RadioController", "Connecting to radio");
LOG_DEBUG("RadioController", QString("Radio model: %1").arg(model));
LOG_WARN("RadioController", "Radio not responding");
LOG_ERROR("RadioController", QString("Connection failed: %1").arg(error));
```

### Database Operations
```cpp
LOG_DEBUG("Database", QString("Opening database: %1").arg(dbPath));
LOG_INFO("Database", QString("Loaded %1 QSOs").arg(count));
LOG_ERROR("Database", QString("Query failed: %1").arg(db.lastError()));
```

### Network Operations
```cpp
LOG_INFO("DXCluster", QString("Connecting to %1:%2").arg(host).arg(port));
LOG_DEBUG("DXCluster", QString("Received spot: %1").arg(callsign));
LOG_WARN("DXCluster", "Connection timeout");
```

## See Also

- `src/logging/Logger.h` - Logger class implementation
- `src/logging/LogMacros.h` - Macro definitions
- `src/logging/LogLevel.h` - Log level enumeration
- `src/utils/AppSettings.h` - Logging configuration settings
