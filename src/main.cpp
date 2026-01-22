#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCommandLineParser>
#include <QTimer>
#include <QLockFile>
#include <QStandardPaths>
#include <QtGlobal>      // For Q_OS_* macros
#include <cstdio>
#include <cstring>
#include <csignal>
#if defined(Q_OS_UNIX) || defined(Q_OS_MAC)
#include <unistd.h>      // For STDERR_FILENO on Unix/Mac
#elif defined(Q_OS_WIN)
#include <io.h>          // For _write on Windows
#define STDERR_FILENO 2
#define write _write
#endif
#include <hamlib/rig.h>
#include "core/Constants.h"
#include "utils/CountryFile.h"
#include "utils/CountryFileDownloader.h"
#include "utils/AppSettings.h"
#include "utils/PathManager.h"
#include "utils/DialogHelper.h"
#include "data/GlobalDatabase.h"
#include "radio/RadioInterface.h"
#include "logging/Logger.h"
#include "logging/LogMacros.h"
#include "ui/MainWindow.h"

// Global pointer to application for signal handler
static QApplication* g_app = nullptr;
static TR4QT::MainWindow* g_mainWindow = nullptr;

/**
 * Signal handler for graceful shutdown (SIGTERM, SIGINT)
 * Ensures settings are saved before the application exits
 */
static void signalHandler(int signal) {
    const char* signalName = (signal == SIGTERM) ? "SIGTERM" :
                             (signal == SIGINT) ? "SIGINT" : "UNKNOWN";

    // Use write() instead of LOG_* since we're in a signal handler
    // (printf/QString are not async-signal-safe)
    const char* msg = "Signal received, performing graceful shutdown...\n";
    [[maybe_unused]] auto ignored = write(STDERR_FILENO, msg, strlen(msg));

    // Save window state directly (bypasses confirmation dialog in closeEvent)
    // This must be done via invokeMethod since we're in signal handler context
    if (g_mainWindow) {
        QMetaObject::invokeMethod(g_mainWindow, "saveSettings", Qt::BlockingQueuedConnection);
    }

    // Force sync settings to disk
    TR4QT::AppSettings::instance().forceSync();

    // Quit the application gracefully (don't call close() - it shows confirmation dialog)
    if (g_app) {
        QMetaObject::invokeMethod(g_app, "quit", Qt::QueuedConnection);
    }
}

// Hamlib debug callback - routes hamlib debug output through our Logger
static int hamlibDebugCallback(enum rig_debug_level_e debug_level, rig_ptr_t /*user_data*/, const char *fmt, va_list ap) {
    // Format the hamlib message
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, ap);

    // Remove trailing newline if present (our logger adds its own)
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }

    // Skip empty messages
    if (strlen(buffer) == 0) {
        return 0;
    }

    // Map hamlib debug levels to our log levels
    QString msg = QString::fromUtf8(buffer);
    switch (debug_level) {
        case RIG_DEBUG_ERR:
            LOG_ERROR("Hamlib", msg);
            break;
        case RIG_DEBUG_WARN:
            LOG_WARN("Hamlib", msg);
            break;
        case RIG_DEBUG_VERBOSE:
            LOG_INFO("Hamlib", msg);
            break;
        case RIG_DEBUG_TRACE:
            LOG_DEBUG("Hamlib", msg);
            break;
        default:
            LOG_TRACE("Hamlib", msg);
            break;
    }

    return 0;  // Return 0 to indicate success
}

int main(int argc, char *argv[]) {
    // Set application name FIRST - required for QStandardPaths to return correct paths
    // QStandardPaths::writableLocation() uses applicationName to construct paths like:
    //   AppData\Local\TR4QT  (Windows)
    //   ~/Library/Application Support/TR4QT  (macOS)
    // Without this, paths would be: AppData\Local (missing app directory!)
    QCoreApplication::setOrganizationName("");  // Empty to avoid nested TR4QT\TR4QT
    QCoreApplication::setApplicationName(TR4QT::APP_NAME);

    // Initialize logger (now QStandardPaths will return correct TR4QT subdirectory)
    TR4QT::Logger& logger = TR4QT::Logger::instance();
    logger.initialize();

    // Load logger configuration from settings
    TR4QT::AppSettings& settings = TR4QT::AppSettings::instance();
    logger.setLogLevel(settings.getLogLevel());
    logger.setFileLoggingEnabled(settings.getFileLoggingEnabled());
    logger.setConsoleLoggingEnabled(settings.getConsoleLoggingEnabled());
    logger.setLogFilePath(settings.getLogFilePath());

    // Log startup banner FIRST (before any other output)
    LOG_INFO("TR4QTMain", "******************** PROGRAM STARTUP ************************");
    LOG_INFO_F("TR4QTMain", "TR4QT Version %s", TR4QT::APP_VERSION);

    // Migrate data from legacy ~/.tr4qt to platform-native location if needed
    // This must happen early, before any file operations
    TR4QT::PathManager::migrateFromLegacyPath();

    // Install Qt message handler (routes qDebug/qWarning/etc through our logger)
    qInstallMessageHandler(TR4QT::Logger::messageHandler);

    QApplication app(argc, argv);

    // Check for existing instance using lock file
    QString lockFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/tr4qt.lock";
    QLockFile lockFile(lockFilePath);
    lockFile.setStaleLockTime(0);  // Never consider lock file stale

    if (!lockFile.tryLock(100)) {  // 100ms timeout
        DialogHelper::warning(
            nullptr,
            "TR4QT Already Running",
            "Another instance of TR4QT is already running.\n\n"
            "Only one instance of TR4QT can run at a time.\n\n"
            "If you believe this is an error, delete the lock file at:\n" + lockFilePath
        );
        LOG_WARN("TR4QTMain", "Another instance is already running - exiting");
        return 1;
    }

    LOG_INFO("TR4QTMain", QString("Instance lock acquired: %1").arg(lockFilePath));

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("TR4QT - Ham Radio Contest Logger");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hamlibDebugOption("hamlib-debug",
        "Enable verbose hamlib debug output");
    parser.addOption(hamlibDebugOption);

    parser.process(app);

    // Set hamlib debug level (command-line overrides setting)
    // Install our callback to route hamlib output through our Logger
    rig_set_debug_callback(hamlibDebugCallback, nullptr);

    if (parser.isSet(hamlibDebugOption) || settings.getHamlibDebugEnabled()) {
        rig_set_debug(RIG_DEBUG_VERBOSE);
        LOG_INFO("Main", "Hamlib debug logging enabled - output will appear in log file");
    } else {
        rig_set_debug(RIG_DEBUG_NONE);
    }

    // Load all hamlib backends (required for radio enumeration)
    rig_load_all_backends();
    LOG_INFO("TR4QTMain", "Hamlib backends loaded for radio enumeration");

    // Register custom types for Qt meta-object system (required for queued connections)
    qRegisterMetaType<TR4QT::RadioConfig>("RadioConfig");
    qRegisterMetaType<TR4QT::RadioState>("RadioState");
    qRegisterMetaType<TR4QT::ModeType>("ModeType");
    qRegisterMetaType<TR4QT::VFO>("VFO");
    qRegisterMetaType<freq_t>("freq_t");  // Hamlib frequency type (required for cross-thread slot calls)

    // Note: Organization/Application names already set at top of main()
    // via QCoreApplication::setOrganizationName/setApplicationName (required for paths)
    app.setApplicationVersion(TR4QT::APP_VERSION);

    // Check if QSQLITE driver is available
    if (!TR4QT::GlobalDatabase::isSqliteDriverAvailable()) {
        LOG_ERROR("Main", "CRITICAL: QSQLITE database driver is not available!");
        LOG_ERROR("Main", "The application cannot run without database support");

        // Get startup logs for user to send to support
        QString logTail = logger.getLastLogLines(50);

        QString errorMessage = QString(
            "CRITICAL ERROR: Database driver not available\n\n"
            "TR4QT requires the QSQLITE driver to access databases.\n"
            "This driver is missing from your installation.\n\n"
            "Please copy the startup log below and report this issue:\n\n"
            "Startup Log:\n"
            "================\n%1"
        ).arg(logTail);

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle("Fatal Error - Database Driver Missing");
        msgBox.setText("TR4QT cannot start because the QSQLITE database driver is missing.");
        msgBox.setDetailedText(errorMessage);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        msgBox.exec();

        logger.shutdown();
        return 1;
    }

    // Initialize global database (for LOTW users and app-wide data)
    TR4QT::GlobalDatabase& globalDb = TR4QT::GlobalDatabase::instance();
    if (!globalDb.open()) {
        LOG_ERROR("Main", QString("CRITICAL: Failed to open global database: %1").arg(globalDb.lastError()));
        LOG_ERROR("Main", "The application cannot run without database support");

        // Get startup logs for user to send to support
        QString logTail = logger.getLastLogLines(50);

        QString errorMessage = QString(
            "CRITICAL ERROR: Failed to open database\n\n"
            "TR4QT could not open the global database.\n"
            "Error: %1\n\n"
            "Please copy the startup log below and report this issue:\n\n"
            "Startup Log:\n"
            "================\n%2"
        ).arg(globalDb.lastError(), logTail);

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle("Fatal Error - Database Failed");
        msgBox.setText("TR4QT cannot start because the database could not be opened.");
        msgBox.setDetailedText(errorMessage);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        msgBox.exec();

        logger.shutdown();
        return 1;
    }

    LOG_INFO("Main", QString("Global database opened: %1").arg(TR4QT::GlobalDatabase::defaultDatabasePath()));

    // Initialize country file on first run
    TR4QT::CountryFile countryFile;
    QString ctyPath = TR4QT::AppSettings::instance().getCountryFilePath();

    if (!QFile::exists(ctyPath)) {
        // First run: extract bundled cty.dat from resources
        LOG_DEBUG("Main", QString("No cty.dat found at %1, extracting from resources").arg(ctyPath));

        QFile resourceFile(":/data/cty.dat");
        if (resourceFile.open(QIODevice::ReadOnly)) {
            // Ensure directory exists
            QFileInfo fileInfo(ctyPath);
            QDir().mkpath(fileInfo.absolutePath());

            QFile outputFile(ctyPath);
            if (outputFile.open(QIODevice::WriteOnly)) {
                outputFile.write(resourceFile.readAll());
                outputFile.close();
                LOG_DEBUG("Main", QString("Extracted bundled cty.dat to %1").arg(ctyPath));
            } else {
                LOG_WARN("Main", QString("Failed to write cty.dat to %1: %2").arg(ctyPath).arg(outputFile.errorString()));
            }
            resourceFile.close();
        } else {
            LOG_WARN("Main", "Failed to open bundled cty.dat from resources");
        }
    }

    bool ctyLoaded = false;
    if (QFile::exists(ctyPath)) {
        LOG_DEBUG("Main", QString("Loading cty.dat from %1").arg(ctyPath));
        if (countryFile.loadFromFile(ctyPath)) {
            LOG_DEBUG("Main", QString("Loaded %1 countries").arg(countryFile.getAllCountries().size()));
            ctyLoaded = true;
        }
    }

    // If cty.dat failed to load, offer to download it
    if (!ctyLoaded) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Country File Not Available");
        msgBox.setText("The country database (cty.dat) is not available.");
        msgBox.setInformativeText("The country database is required for DXCC lookups, zone information, "
                                  "and contest scoring.\n\n"
                                  "Would you like to download it now?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);

        int ret = msgBox.exec();
        if (ret == QMessageBox::No) {
            LOG_INFO("Main", "User declined to download cty.dat, exiting");
            logger.shutdown();
            return 0;
        }

        // User chose to download - create window and trigger download
        LOG_INFO("Main", "User chose to download cty.dat");
    }

    // Create and show main window
    TR4QT::MainWindow mainWindow;

    // Set global pointers for signal handler
    g_app = &app;
    g_mainWindow = &mainWindow;

    // Install signal handlers for graceful shutdown
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    LOG_INFO("TR4QTMain", "Signal handlers installed for graceful shutdown");

    // Start settings auto-save timer (every 60 seconds)
    settings.startAutoSave();

    // If cty.dat wasn't loaded and user chose to download, trigger download
    if (!ctyLoaded) {
        // Use QTimer to trigger download after window is shown
        QTimer::singleShot(100, &mainWindow, [&mainWindow]() {
            mainWindow.triggerCountryFileDownload();
        });
    }

    mainWindow.show();

    int result = app.exec();

    // Clear global pointers
    g_mainWindow = nullptr;
    g_app = nullptr;

    // Stop auto-save timer and force final sync
    settings.stopAutoSave();
    settings.forceSync();
    LOG_INFO("TR4QTMain", "Final settings sync completed");

    // Shutdown logger before exit
    logger.shutdown();

    return result;
}
