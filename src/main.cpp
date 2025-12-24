#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCommandLineParser>
#include <hamlib/rig.h>
#include "core/Constants.h"
#include "utils/CountryFile.h"
#include "utils/CountryFileDownloader.h"
#include "utils/AppSettings.h"
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
        qDebug() << "Hamlib debug output enabled";
    } else {
        rig_set_debug(RIG_DEBUG_NONE);
    }

    // Register custom types for Qt meta-object system (required for queued connections)
    qRegisterMetaType<TR4QT::RadioConfig>("RadioConfig");
    qRegisterMetaType<TR4QT::RadioState>("RadioState");
    qRegisterMetaType<TR4QT::ModeType>("ModeType");
    qRegisterMetaType<TR4QT::VFO>("VFO");

    app.setOrganizationName(TR4QT::APP_ORG);
    app.setApplicationName(TR4QT::APP_NAME);
    app.setApplicationVersion(TR4QT::APP_VERSION);

    // Log startup banner (using TR4W-style format)
    LOG_INFO("TR4QTMain", "******************** PROGRAM STARTUP ************************");
    LOG_INFO_F("TR4QTMain", "TR4QT Version %s", TR4QT::APP_VERSION);
    qDebug() << "TR4QT Version" << TR4QT::APP_VERSION;

    // Initialize country file on first run
    TR4QT::CountryFile countryFile;
    QString ctyPath = TR4QT::AppSettings::instance().getCountryFilePath();

    if (QFile::exists(ctyPath)) {
        qDebug() << "Loading cty.dat from" << ctyPath;
        if (countryFile.loadFromFile(ctyPath)) {
            qDebug() << "Loaded" << countryFile.getAllCountries().size() << "countries";
        }
    } else {
        qDebug() << "No cty.dat found. Use Tools → Download Country File when implemented.";
    }

    // Create and show main window
    TR4QT::MainWindow mainWindow;
    mainWindow.show();

    int result = app.exec();

    // Shutdown logger before exit
    logger.shutdown();

    return result;
}
