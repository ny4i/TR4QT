#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCommandLineParser>
#include <QMessageBox>
#include <hamlib/rig.h>
#include "core/Constants.h"
#include "utils/CountryFile.h"
#include "utils/CountryFileDownloader.h"
#include "utils/AppSettings.h"
#include "data/GlobalDatabase.h"
#include "radio/RadioInterface.h"
#include "logging/Logger.h"
#include "logging/LogMacros.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    // Initialize logger FIRST (before QApplication) to capture all startup messages
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

    // Install Qt message handler (routes qDebug/qWarning/etc through our logger)
    qInstallMessageHandler(TR4QT::Logger::messageHandler);

    QApplication app(argc, argv);

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("TR4QT - Ham Radio Contest Logger");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hamlibDebugOption("hamlib-debug",
        "Enable verbose hamlib debug output");
    parser.addOption(hamlibDebugOption);

    parser.process(app);

    // Set hamlib debug level (default: none, unless --hamlib-debug specified)
    if (parser.isSet(hamlibDebugOption)) {
        rig_set_debug(RIG_DEBUG_VERBOSE);
        LOG_DEBUG("Main", "Hamlib debug output enabled");
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

    app.setOrganizationName(TR4QT::APP_ORG);
    app.setApplicationName(TR4QT::APP_NAME);
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

    if (QFile::exists(ctyPath)) {
        LOG_DEBUG("Main", QString("Loading cty.dat from %1").arg(ctyPath));
        if (countryFile.loadFromFile(ctyPath)) {
            LOG_DEBUG("Main", QString("Loaded %1 countries").arg(countryFile.getAllCountries().size()));
        }
    } else {
        LOG_DEBUG("Main", "No cty.dat found. Use Tools → Download Country File when implemented.");
    }

    // Create and show main window
    TR4QT::MainWindow mainWindow;
    mainWindow.show();

    int result = app.exec();

    // Shutdown logger before exit
    logger.shutdown();

    return result;
}
