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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHostAddress>
#include "../models/QSO.h"
#include "../radio/RadioInterface.h"
#include "../core/Types.h"
#include "../interfaces/IQSODataSource.h"

class QTcpServer;

namespace TR4QT {

// Forward declarations
class RadioController;
class AppSettings;

/**
 * Request structure for POST /api/log-qso
 *
 * Used by external programs (StreamDeck, test harnesses) to log QSOs.
 * WebServer parses JSON and populates this struct, then emits a signal
 * for MainWindow to process.
 */
struct LogQSOWebRequest {
    QString callsign;                  // Required: Station callsign
    QString exchange;                  // Required: Received exchange
    freq_t frequency = 0;              // Optional: Frequency in Hz (0 = use current)
    BandType band = BandType::None;    // Optional: Band (None = use current)
    ModeType mode = ModeType::None;    // Optional: Mode (None = use current)
};

/**
 * Response structure for POST /api/log-qso
 *
 * Populated by MainWindow after processing the log request.
 * WebServer converts this to JSON for the HTTP response.
 */
struct LogQSOWebResponse {
    bool success = false;              // true if QSO logged successfully
    QString error;                     // Error message if success = false
    QString errorField;                // Field that caused error ("callsign", "exchange", etc.)

    // QSO details (if success = true)
    int qsoId = 0;                     // Database ID of logged QSO
    QString callsign;                  // Logged callsign
    QDateTime timestamp;               // QSO timestamp
    freq_t frequency = 0;              // Frequency in Hz
    QString band;                      // Band string ("20M", etc.)
    QString mode;                      // Mode string ("CW", "USB", etc.)
    QString exchangeSent;              // Exchange sent
    QString exchangeReceived;          // Exchange received
    int points = 0;                    // QSO points
    bool isMultiplier = false;         // true if new multiplier
    bool isDuplicate = false;          // true if duplicate QSO
    int serialNumber = 0;              // Serial number used
};

/**
 * Request structure for POST /api/command
 *
 * Used by external programs to execute commands (CW, band change, etc.)
 */
struct CommandWebRequest {
    QString command;                   // Command name (send-cw, set-band, etc.)
    QVariantMap params;                // Command parameters
};

/**
 * Response structure for POST /api/command
 */
struct CommandWebResponse {
    bool success = false;              // true if command executed successfully
    QString error;                     // Error message if success = false
    QString command;                   // Command that was executed
    QString message;                   // Success message or additional info
};

// Forward declarations for contest API structs (defined in WebServerContext.h)
struct CreateContestRequest;
struct CreateContestResponse;
struct OpenContestRequest;
struct OpenContestResponse;
struct ContestStatusResponse;
struct ScoreResponse;

/**
 * WebServer - HTTP server for contest information display
 *
 * Provides JSON API and HTML dashboard for viewing contest information
 * from remote browsers (phones, tablets, other PCs on network).
 *
 * Features:
 * - Configurable port and network interface
 * - JSON API endpoints for programmatic access
 * - HTML dashboard for human viewing
 * - Real-time contest statistics
 * - Recent QSO list
 * - Radio status
 * - Operator information
 *
 * Security:
 * - Runs on LAN only (no internet exposure)
 * - Default: localhost only (127.0.0.1)
 * - Optional: all interfaces (0.0.0.0) for network access
 * - No authentication (trust LAN environment)
 */
class WebServer : public QObject {
    Q_OBJECT

public:
    /**
     * Construct a WebServer
     * @param qsoDataSource QSO data source (QSOTableModel or WebServerContext)
     * @param radioController Radio controller for radio status (can be nullptr)
     * @param parent Parent QObject
     */
    explicit WebServer(IQSODataSource* qsoDataSource,
                      RadioController* radioController,
                      QObject* parent = nullptr);
    ~WebServer() override;

    // Server control
    bool start(quint16 port = 14140, const QHostAddress& address = QHostAddress::LocalHost);
    void stop();
    bool isRunning() const;

    // Configuration
    void setPort(quint16 port);
    void setAddress(const QHostAddress& address);
    quint16 port() const { return m_port; }
    QHostAddress address() const { return m_address; }
    QString url() const;

    // Contest state (set when contest changes)
    void setContestName(const QString& contestName);
    void setUsesZoneMultipliers(bool usesZones);
    void setUsesModeGroupBreakdown(bool usesModeGroups);

signals:
    void serverStarted(const QString& url);
    void serverStopped();
    void errorOccurred(const QString& error);

    /**
     * Emitted when a POST /api/log-qso request is received
     *
     * MainWindow should connect to this signal and:
     * 1. Process the request using QSOLoggingService
     * 2. Populate the response struct
     *
     * Signal is emitted synchronously (same thread), so the response
     * will be populated before the HTTP handler returns.
     *
     * @param request Parsed JSON request
     * @param response Pointer to response struct to populate
     */
    void logQSORequested(const LogQSOWebRequest& request, LogQSOWebResponse* response);

    /**
     * Emitted when a POST /api/command request is received
     *
     * MainWindow should connect to this signal and execute the command.
     *
     * @param request Parsed command request
     * @param response Pointer to response struct to populate
     */
    void commandRequested(const CommandWebRequest& request, CommandWebResponse* response);

    /**
     * Emitted when a POST /api/contest/create request is received
     *
     * @param request Parsed create contest request
     * @param response Pointer to response struct to populate
     */
    void createContestRequested(const CreateContestRequest& request, CreateContestResponse* response);

    /**
     * Emitted when a POST /api/contest/open request is received
     *
     * @param request Parsed open contest request
     * @param response Pointer to response struct to populate
     */
    void openContestRequested(const OpenContestRequest& request, OpenContestResponse* response);

    /**
     * Emitted when a POST /api/contest/close request is received
     */
    void closeContestRequested();

    /**
     * Emitted when a GET /api/contest/status request is received
     *
     * @param response Pointer to response struct to populate
     */
    void contestStatusRequested(ContestStatusResponse* response);

    /**
     * Emitted when a GET /api/contest/score request is received
     *
     * @param response Pointer to response struct to populate
     */
    void contestScoreRequested(ScoreResponse* response);

    /**
     * Emitted when a GET /api/export/cabrillo request is received
     *
     * @param content Pointer to QString to receive Cabrillo content
     */
    void cabrilloExportRequested(QString* content);

private:
    // HTTP route handlers
    QHttpServerResponse handleRoot();
    QHttpServerResponse handleApiStatus();
    QHttpServerResponse handleApiQsos();
    QHttpServerResponse handleApiRadio();
    QHttpServerResponse handleApiScore();
    QHttpServerResponse handleApiWorkedSections();
    QHttpServerResponse handleApiWorkedStates();
    QHttpServerResponse handleDashboard();
    QHttpServerResponse handleSectionsMap();
    QHttpServerResponse handleStatesMap();
    QHttpServerResponse handleFavicon();
    QHttpServerResponse handleAppleTouchIcon();
    QHttpServerResponse handleApiSectionsGeoJSON();
    QHttpServerResponse handleApiStatesGeoJSON();

    // POST endpoint handlers (command API)
    QHttpServerResponse handlePostLogQSO(const QHttpServerRequest& request);
    QHttpServerResponse handlePostCommand(const QHttpServerRequest& request);
    QHttpServerResponse handleGetCommands();

    // Contest API handlers
    QHttpServerResponse handlePostContestCreate(const QHttpServerRequest& request);
    QHttpServerResponse handlePostContestOpen(const QHttpServerRequest& request);
    QHttpServerResponse handlePostContestClose(const QHttpServerRequest& request);
    QHttpServerResponse handleGetContestStatus();

    // Export API handlers
    QHttpServerResponse handleGetExportAdif();
    QHttpServerResponse handleGetExportCabrillo();

    // Score API handlers
    QHttpServerResponse handleGetContestScore();

    // JSON response helpers
    QHttpServerResponse jsonError(int statusCode, const QString& message,
                                   const QString& field = QString());
    QHttpServerResponse jsonSuccess(const QJsonObject& data);

    // Helper methods
    QString generateDashboardHtml();
    QString generateSectionsMapHtml();
    QString generateStatesMapHtml();
    QJsonObject radioStateToJson(const RadioState& state);
    QJsonObject qsoToJson(const QSO& qso);

    QHttpServer* m_server;
    QTcpServer* m_tcpServer{nullptr};
    quint16 m_port{14140};  // Default: 14140 (ham radio frequency joke: 14.140 MHz)
    QHostAddress m_address{QHostAddress::LocalHost};

    // Data sources (not owned - pull model)
    IQSODataSource* m_qsoDataSource;
    RadioController* m_radioController;

    // Contest state (minimal - only what can't be pulled)
    QString m_contestName;
    bool m_usesZoneMultipliers{true};  // Default: show zones
    bool m_usesModeGroupBreakdown{false};  // Default: single QSOs row

    static constexpr int MAX_RECENT_QSOS = 10;
};

} // namespace TR4QT

#endif // WEBSERVER_H
