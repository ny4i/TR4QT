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

/**
 * TR4QT Headless Server - main_server.cpp
 *
 * Headless entry point for TR4QT that exposes the WebServer API without a GUI.
 * This validates architecture modularity and enables automated testing.
 *
 * Features:
 * - Contest API (create, open, close, status)
 * - QSO logging API (log-qso)
 * - Command API (set-frequency, set-band, set-mode, toggle-run-mode)
 * - Radio status API (optional, if radio configured)
 *
 * Usage:
 *   tr4qt_server [options]
 *
 * Options:
 *   --port <port>      Web server port (default: 14141)
 *   --address <addr>   Bind address (default: 127.0.0.1)
 *   --all-interfaces   Bind to all interfaces (0.0.0.0)
 *   --help             Show help
 *
 * Example:
 *   tr4qt_server --port 14141 --all-interfaces
 *
 * Test with curl:
 *   # Create contest
 *   curl -X POST http://localhost:14141/api/contest/create \
 *     -H "Content-Type: application/json" \
 *     -d '{"contestType":"CQWW_CW","callsign":"K1TEST","exchangeSent":"599 05"}'
 *
 *   # Log QSO
 *   curl -X POST http://localhost:14141/api/log-qso \
 *     -H "Content-Type: application/json" \
 *     -d '{"callsign":"W1AW","exchange":"599 5"}'
 *
 *   # Get status
 *   curl http://localhost:14141/api/contest/status
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>

#include "../network/WebServer.h"
#include "../network/WebServerContext.h"
#include "../utils/PathManager.h"
#include "../utils/AppSettings.h"
#include "../logging/Logger.h"
#include "../logging/LogMacros.h"
#include "../core/Constants.h"

using namespace TR4QT;

int main(int argc, char *argv[])
{
    // Create non-GUI application
    QCoreApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(QString("%1-server").arg(APP_VERSION));
    QCoreApplication::setOrganizationName(APP_ORG);

    // Initialize logging
    Logger::instance().setLogLevel(LogLevel::Debug);
    Logger::instance().setConsoleLoggingEnabled(true);
    Logger::instance().initialize();

    LOG_INFO("Server", QString("TR4QT Headless Server %1").arg(APP_VERSION));
    LOG_INFO("Server", QString("Data directory: %1").arg(PathManager::getAppDataDir()));

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("TR4QT Headless Contest Server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        "Web server port (default: 14141)",
        "port",
        "14141"
    );
    parser.addOption(portOption);

    QCommandLineOption addressOption(
        QStringList() << "a" << "address",
        "Bind address (default: 127.0.0.1)",
        "address",
        "127.0.0.1"
    );
    parser.addOption(addressOption);

    QCommandLineOption allInterfacesOption(
        "all-interfaces",
        "Bind to all network interfaces (0.0.0.0)"
    );
    parser.addOption(allInterfacesOption);

    parser.process(app);

    // Get configuration
    quint16 port = static_cast<quint16>(parser.value(portOption).toUInt());
    QHostAddress address;

    if (parser.isSet(allInterfacesOption)) {
        address = QHostAddress::Any;
        LOG_INFO("Server", "Binding to all interfaces");
    } else {
        QString addrStr = parser.value(addressOption);
        if (!address.setAddress(addrStr)) {
            LOG_ERROR("Server", QString("Invalid address: %1").arg(addrStr));
            return 1;
        }
    }

    // Validate port
    if (port < 1024 || port > 65535) {
        LOG_ERROR("Server", QString("Invalid port: %1 (must be 1024-65535)").arg(port));
        return 1;
    }

    // Create WebServerContext (headless context with all services)
    WebServerContext::Config contextConfig;
    contextConfig.appDataDir = PathManager::getAppDataDir();
    contextConfig.radioHandler = nullptr;  // No radio control in headless mode

    WebServerContext context(contextConfig);

    // Create WebServer
    // - Uses WebServerContext as IQSODataSource for QSO data
    // - No RadioController in headless mode
    WebServer server(&context, nullptr);

    // Connect signals from WebServer to WebServerContext

    // Log QSO
    QObject::connect(&server, &WebServer::logQSORequested,
                     &context, &WebServerContext::onLogQSOFromWeb);

    // Command
    QObject::connect(&server, &WebServer::commandRequested,
                     &context, &WebServerContext::onCommandFromWeb);

    // Contest API - create contest
    QObject::connect(&server, &WebServer::createContestRequested,
        [&context](const CreateContestRequest& req, CreateContestResponse* resp) {
            *resp = context.createContest(req);
        });

    // Contest API - open contest
    QObject::connect(&server, &WebServer::openContestRequested,
        [&context](const OpenContestRequest& req, OpenContestResponse* resp) {
            *resp = context.openContest(req);
        });

    // Contest API - close contest
    QObject::connect(&server, &WebServer::closeContestRequested,
        [&context]() {
            context.closeContest();
        });

    // Contest API - get status
    QObject::connect(&server, &WebServer::contestStatusRequested,
        [&context](ContestStatusResponse* resp) {
            *resp = context.getContestStatus();
        });

    // Contest API - get score with band breakdown
    QObject::connect(&server, &WebServer::contestScoreRequested,
        [&context](ScoreResponse* resp) {
            *resp = context.getScore();
        });

    // Export API - Cabrillo
    QObject::connect(&server, &WebServer::cabrilloExportRequested,
        [&context](QString* content) {
            *content = context.generateCabrillo();
        });

    // Connect context signals for logging
    QObject::connect(&context, &WebServerContext::contestActivated,
        [](const QString& name) {
            LOG_INFO("Server", QString("Contest activated: %1").arg(name));
        });

    QObject::connect(&context, &WebServerContext::contestClosed,
        []() {
            LOG_INFO("Server", "Contest closed");
        });

    QObject::connect(&context, &WebServerContext::qsoLogged,
        [](const QSO& qso) {
            LOG_INFO("Server", QString("QSO logged: %1 on %2")
                     .arg(qso.callsign)
                     .arg(bandToString(qso.band)));
        });

    // Start the web server
    if (!server.start(port, address)) {
        LOG_ERROR("Server", "Failed to start web server");
        return 1;
    }

    QString url = server.url();
    LOG_INFO("Server", QString("Web server started: %1").arg(url));

    // Print usage info to console
    qInfo() << "";
    qInfo() << "TR4QT Headless Server running at:" << url;
    qInfo() << "";
    qInfo() << "API Endpoints:";
    qInfo() << "  POST /api/contest/create  - Create new contest";
    qInfo() << "  POST /api/contest/open    - Open existing contest";
    qInfo() << "  POST /api/contest/close   - Close active contest";
    qInfo() << "  GET  /api/contest/status  - Get contest status";
    qInfo() << "  POST /api/log-qso         - Log a QSO";
    qInfo() << "  POST /api/command         - Execute command";
    qInfo() << "  GET  /api/commands        - List available commands";
    qInfo() << "";
    qInfo() << "Press Ctrl+C to stop";
    qInfo() << "";

    // Run the event loop
    return app.exec();
}
