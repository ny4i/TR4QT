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

#include "WebServer.h"
#include "WebServerContext.h"  // For contest API structs
#include "../radio/RadioController.h"
#include "../utils/AppSettings.h"
#include "../utils/ADIFExporter.h"
#include "../logging/LogMacros.h"
#include "../core/Types.h"
#include "../core/Constants.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTcpServer>
#include <QFile>

namespace TR4QT {

WebServer::WebServer(IQSODataSource* qsoDataSource,
                     RadioController* radioController,
                     QObject* parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_qsoDataSource(qsoDataSource)
    , m_radioController(radioController)
{
    // Setup HTTP routes

    // Root - redirect to dashboard
    m_server->route("/", [this]() {
        return handleRoot();
    });

    // API endpoints (JSON)
    m_server->route("/api/status", [this]() {
        return handleApiStatus();
    });

    m_server->route("/api/qsos", [this]() {
        return handleApiQsos();
    });

    m_server->route("/api/radio", [this]() {
        return handleApiRadio();
    });

    m_server->route("/api/score", [this]() {
        return handleApiScore();
    });

    m_server->route("/api/worked-sections", [this]() {
        return handleApiWorkedSections();
    });

    m_server->route("/api/worked-states", [this]() {
        return handleApiWorkedStates();
    });

    m_server->route("/api/sections-geojson", [this]() {
        return handleApiSectionsGeoJSON();
    });

    m_server->route("/api/states-geojson", [this]() {
        return handleApiStatesGeoJSON();
    });

    // HTML dashboard
    m_server->route("/dashboard", [this]() {
        return handleDashboard();
    });

    m_server->route("/map", [this]() {
        return handleSectionsMap();
    });

    m_server->route("/states-map", [this]() {
        return handleStatesMap();
    });

    // Favicon routes
    m_server->route("/favicon.ico", [this]() {
        return handleFavicon();
    });

    m_server->route("/apple-touch-icon.png", [this]() {
        return handleAppleTouchIcon();
    });

    // === Command API endpoints (POST) ===

    // POST /api/log-qso - Log a QSO with acknowledgment
    m_server->route("/api/log-qso", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            return handlePostLogQSO(request);
        });

    // POST /api/command - Execute a command (CW, band change, etc.)
    m_server->route("/api/command", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            return handlePostCommand(request);
        });

    // GET /api/commands - Self-documentation of available commands
    m_server->route("/api/commands", [this]() {
        return handleGetCommands();
    });

    // === Contest API endpoints (for headless server) ===

    // POST /api/contest/create - Create a new contest
    m_server->route("/api/contest/create", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            return handlePostContestCreate(request);
        });

    // POST /api/contest/open - Open an existing contest
    m_server->route("/api/contest/open", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            return handlePostContestOpen(request);
        });

    // POST /api/contest/close - Close the active contest
    m_server->route("/api/contest/close", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            Q_UNUSED(request);
            return handlePostContestClose(request);
        });

    // GET /api/contest/status - Get current contest status
    m_server->route("/api/contest/status", [this]() {
        return handleGetContestStatus();
    });

    // GET /api/export/adif - Export all QSOs to ADIF format
    m_server->route("/api/export/adif", [this]() {
        return handleGetExportAdif();
    });

    // GET /api/export/cabrillo - Export all QSOs to Cabrillo format
    m_server->route("/api/export/cabrillo", [this]() {
        return handleGetExportCabrillo();
    });

    // GET /api/contest/score - Get detailed score with band breakdown
    m_server->route("/api/contest/score", [this]() {
        return handleGetContestScore();
    });
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start(quint16 port, const QHostAddress& address) {
    // Stop any existing server
    stop();

    // Create and configure TCP server
    m_tcpServer = new QTcpServer(this);

    if (!m_tcpServer->listen(address, port)) {
        QString error = QString("Failed to start web server on %1:%2: %3")
            .arg(address.toString()).arg(port).arg(m_tcpServer->errorString());
        LOG_ERROR("WebServer", error);
        emit errorOccurred(error);
        delete m_tcpServer;
        m_tcpServer = nullptr;
        return false;
    }

    // Bind HTTP server to TCP server
    // Note: bind() returns void on Windows Qt 6.7.2, bool on macOS - API inconsistency!
    m_server->bind(m_tcpServer);

    m_port = port;
    m_address = address;

    QString urlStr = url();
    LOG_INFO("WebServer", QString("Web server started: %1").arg(urlStr));
    emit serverStarted(urlStr);
    return true;
}

void WebServer::stop() {
    if (m_tcpServer) {
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
        LOG_INFO("WebServer", "Web server stopped");
        emit serverStopped();
    }
}

bool WebServer::isRunning() const {
    return m_tcpServer && m_tcpServer->isListening();
}

void WebServer::setPort(quint16 port) {
    m_port = port;
}

void WebServer::setAddress(const QHostAddress& address) {
    m_address = address;
}

QString WebServer::url() const {
    QString host = m_address == QHostAddress::Any || m_address == QHostAddress::AnyIPv4
        ? "localhost" : m_address.toString();
    return QString("http://%1:%2").arg(host).arg(m_port);
}

void WebServer::setContestName(const QString& contestName) {
    m_contestName = contestName;
}

void WebServer::setUsesZoneMultipliers(bool usesZones) {
    m_usesZoneMultipliers = usesZones;
}

void WebServer::setUsesModeGroupBreakdown(bool usesModeGroups) {
    m_usesModeGroupBreakdown = usesModeGroups;
}

// HTTP Route Handlers

QHttpServerResponse WebServer::handleRoot() {
    // HTML meta refresh redirect (works across all Qt versions and platforms)
    // Note: Qt HttpServer header APIs are inconsistent between versions
    // (setHeader doesn't exist in 6.9+, setHeaders(QHttpHeaders) only in 6.8+,
    //  bind() returns void on Windows 6.7 but bool on macOS)
    QString html = "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/dashboard\"></head></html>";
    return QHttpServerResponse("text/html", html.toUtf8());
}

QHttpServerResponse WebServer::handleApiStatus() {
    // Pull fresh data from sources
    AppSettings& settings = AppSettings::instance();
    QString myCall = settings.getMyCallsign();
    QString currentOperator = settings.getCurrentOperator();

    // Calculate score from QSO model
    int qsoCount = m_qsoDataSource->qsoCount();
    int totalPoints = 0;
    int totalMults = 0;

    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoDataSource->qsoAt(row);
        totalPoints += qso.qsoPoints;
        if (qso.isMultiplier) {
            totalMults++;
        }
    }

    QJsonObject json;
    json["contest"] = m_contestName;
    json["myCall"] = myCall;
    json["operator"] = currentOperator.isEmpty() ? myCall : currentOperator;
    json["qsos"] = qsoCount;
    json["points"] = totalPoints;
    json["multipliers"] = totalMults;
    json["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleApiQsos() {
    QJsonArray qsos;

    // Pull recent QSOs from model (last 10)
    int qsoCount = m_qsoDataSource->qsoCount();
    int startIdx = qMax(0, qsoCount - MAX_RECENT_QSOS);

    for (int row = startIdx; row < qsoCount; ++row) {
        QSO qso = m_qsoDataSource->qsoAt(row);
        qsos.append(qsoToJson(qso));
    }

    QJsonObject json;
    json["qsos"] = qsos;
    json["count"] = qsos.size();

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleApiRadio() {
    // Pull radio state from RadioController
    RadioState radioState = m_radioController->getCurrentState();
    QJsonObject json = radioStateToJson(radioState);

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleApiScore() {
    // Use the same signal-based approach as handleGetContestScore()
    // This ensures consistent scoring using ScoreCalculationService
    ScoreResponse response;
    emit contestScoreRequested(&response);

    QJsonObject json;
    // Include both old and new field names for backward compatibility
    json["qsos"] = response.totalQsos;
    json["totalQsos"] = response.totalQsos;
    json["points"] = response.totalPoints;
    json["totalPoints"] = response.totalPoints;
    json["multipliers"] = response.totalMultipliers;
    json["totalMultipliers"] = response.totalMultipliers;
    json["score"] = response.score;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleApiWorkedSections() {
    // Build map of sections → QSO count
    QMap<QString, int> sectionCounts;

    int qsoCount = m_qsoDataSource->qsoCount();
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoDataSource->qsoAt(row);
        QString section = qso.arrlSection.trimmed().toUpper();

        if (!section.isEmpty()) {
            sectionCounts[section]++;
        }
    }

    // Convert to JSON array
    QJsonArray sectionsArray;
    for (auto it = sectionCounts.begin(); it != sectionCounts.end(); ++it) {
        QJsonObject sectionObj;
        sectionObj["section"] = it.key();
        sectionObj["count"] = it.value();
        sectionsArray.append(sectionObj);
    }

    QJsonObject json;
    json["sections"] = sectionsArray;
    json["totalSections"] = sectionCounts.size();
    json["totalQsos"] = qsoCount;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleApiWorkedStates() {
    // Build map of states → QSO count
    QMap<QString, int> stateCounts;

    int qsoCount = m_qsoDataSource->qsoCount();
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoDataSource->qsoAt(row);
        QString state = qso.state.trimmed().toUpper();

        if (!state.isEmpty()) {
            stateCounts[state]++;
        }
    }

    // Convert to JSON array
    QJsonArray statesArray;
    for (auto it = stateCounts.begin(); it != stateCounts.end(); ++it) {
        QJsonObject stateObj;
        stateObj["state"] = it.key();
        stateObj["count"] = it.value();
        statesArray.append(stateObj);
    }

    QJsonObject json;
    json["states"] = statesArray;
    json["totalStates"] = stateCounts.size();
    json["totalQsos"] = qsoCount;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleDashboard() {
    QString html = generateDashboardHtml();
    return QHttpServerResponse("text/html", html.toUtf8());
}

QHttpServerResponse WebServer::handleSectionsMap() {
    QString html = generateSectionsMapHtml();
    return QHttpServerResponse("text/html", html.toUtf8());
}

QHttpServerResponse WebServer::handleStatesMap() {
    QString html = generateStatesMapHtml();
    return QHttpServerResponse("text/html", html.toUtf8());
}

// Helper Methods

QString WebServer::generateDashboardHtml() {
    QString html = R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%1 - TR4QT Contest Dashboard</title>
    <link rel="icon" type="image/x-icon" href="/favicon.ico">
    <link rel="apple-touch-icon" sizes="256x256" href="/apple-touch-icon.png">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #fff;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 10px;
            backdrop-filter: blur(10px);
        }
        .header h1 { font-size: 2.5em; margin-bottom: 10px; }
        .header .callsign { font-size: 1.8em; color: #ffd700; }
        .header .operator { font-size: 1.2em; opacity: 0.9; }

        .grid {
            display: grid;
            grid-template-columns: 2fr 1fr;
            gap: 20px;
            margin-bottom: 30px;
        }
        @media (max-width: 900px) {
            .grid {
                grid-template-columns: 1fr;
            }
        }
        .card {
            background: rgba(255,255,255,0.15);
            padding: 25px;
            border-radius: 15px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
        }
        .card h2 {
            font-size: 1.3em;
            margin-bottom: 15px;
            border-bottom: 2px solid rgba(255,255,255,0.3);
            padding-bottom: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .card h2 label {
            font-size: 0.8em;
            font-weight: normal;
            opacity: 0.9;
            cursor: pointer;
            display: flex;
            align-items: center;
            gap: 5px;
        }
        .card h2 input[type="checkbox"] {
            cursor: pointer;
        }
        .stat {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .stat:last-child { border-bottom: none; }
        .stat-label { opacity: 0.8; }
        .stat-value {
            font-weight: bold;
            font-size: 1.2em;
            color: #ffd700;
        }

        .qso-list {
            background: rgba(0,0,0,0.2);
            border-radius: 10px;
            overflow: hidden;
        }
        .qso-header {
            padding: 10px 15px;
            display: grid;
            grid-template-columns: 70px 110px 90px 60px 1fr 90px;
            gap: 10px;
            font-weight: bold;
            font-size: 0.85em;
            border-bottom: 2px solid rgba(255,255,255,0.3);
            background: rgba(0,0,0,0.3);
        }
        .qso-item {
            padding: 12px 15px;
            border-bottom: 1px solid rgba(255,255,255,0.1);
            display: grid;
            grid-template-columns: 70px 110px 90px 60px 1fr 90px;
            gap: 10px;
            font-size: 0.90em;
        }
        .qso-item:last-child { border-bottom: none; }
        .qso-time { opacity: 0.7; font-size: 0.9em; }
        .qso-call { font-weight: bold; color: #ffd700; }
        .qso-freq { opacity: 0.8; font-size: 0.9em; }
        .qso-band { opacity: 0.8; }
        .qso-exchange { opacity: 0.9; }
        .qso-operator { opacity: 0.7; font-size: 0.9em; }

        .score-big {
            font-size: 3em;
            font-weight: bold;
            text-align: center;
            color: #ffd700;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
            margin: 20px 0;
        }
        .score-summary, .score-detail {
            transition: all 0.3s ease;
        }
        .score-summary.hidden, .score-detail.hidden {
            display: none;
        }
        .band-table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 15px;
        }
        .band-table th {
            background: rgba(0,0,0,0.3);
            padding: 8px;
            text-align: center;
            font-weight: bold;
            font-size: 0.85em;
            border-bottom: 2px solid rgba(255,255,255,0.3);
        }
        .band-table td {
            padding: 8px;
            text-align: center;
            border-bottom: 1px solid rgba(255,255,255,0.1);
            font-family: monospace;
        }
        .band-table tr:last-child td {
            border-bottom: none;
        }
        .band-table .row-label {
            text-align: left;
            font-weight: bold;
            opacity: 0.8;
        }
        .band-table .total-col {
            font-weight: bold;
            color: #ffd700;
        }
        .radio-status {
            background: rgba(0,0,0,0.2);
            padding: 15px;
            border-radius: 8px;
            margin: 20px 0;
            display: flex;
            justify-content: space-around;
            align-items: center;
            flex-wrap: wrap;
            gap: 15px;
        }
        .radio-status-item {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 5px;
        }
        .radio-status-label {
            font-size: 0.75em;
            opacity: 0.7;
            text-transform: uppercase;
        }
        .radio-status-value {
            font-size: 1.1em;
            font-weight: bold;
            color: #ffd700;
        }

        .refresh-info {
            text-align: center;
            opacity: 0.7;
            margin-top: 20px;
            font-size: 0.9em;
        }

        .refresh-card {
            background: rgba(255,255,255,0.15);
            padding: 25px;
            border-radius: 15px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            display: flex;
            flex-direction: column;
            gap: 15px;
            align-items: stretch;
        }

        .refresh-card h2 {
            font-size: 1.3em;
            margin-bottom: 10px;
            border-bottom: 2px solid rgba(255,255,255,0.3);
            padding-bottom: 10px;
        }

        .refresh-card button {
            background: #ffd700;
            color: #333;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 1em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s;
            width: 100%;
        }

        .refresh-card button:hover {
            background: #ffed4e;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(255,215,0,0.4);
        }

        .refresh-card label {
            font-size: 0.9em;
            opacity: 0.9;
            display: block;
            margin-bottom: 5px;
        }

        .refresh-card input[type="number"] {
            width: 100%;
            padding: 10px 12px;
            border-radius: 6px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1em;
            text-align: center;
        }

        .refresh-card input[type="number"]:focus {
            outline: none;
            border-color: #ffd700;
            background: rgba(255,255,255,0.2);
        }

        .refresh-card .countdown-text {
            text-align: center;
            opacity: 0.8;
            font-size: 0.9em;
        }

        #countdown {
            font-weight: bold;
            color: #ffd700;
        }

        footer {
            text-align: center;
            padding: 20px;
            opacity: 0.6;
            font-size: 0.9em;
        }
        footer a {
            color: #ffd700;
            text-decoration: none;
        }
        footer a:hover {
            text-decoration: underline;
        }

        @media (max-width: 768px) {
            .header h1 { font-size: 1.8em; }
            .grid { grid-template-columns: 1fr; }
            .qso-header { display: none; }
            .qso-item { grid-template-columns: 60px 1fr 60px; font-size: 0.85em; }
            .qso-freq, .qso-exchange, .qso-operator { display: none; }
            .controls { flex-direction: column; }
        }
    </style>
    <script>
        let refreshInterval = 60; // Default 60 seconds
        let remainingSeconds = refreshInterval;
        let timerId = null;
        let isServerDown = false;
        let reconnectAttempts = 0;

        function startCountdown() {
            if (timerId) clearInterval(timerId);
            remainingSeconds = refreshInterval;
            updateCountdown();

            timerId = setInterval(function() {
                remainingSeconds--;
                updateCountdown();

                if (remainingSeconds <= 0) {
                    checkServerAndReload();
                }
            }, 1000);
        }

        async function checkServerAndReload() {
            try {
                const response = await fetch('/api/status', {
                    method: 'GET',
                    cache: 'no-cache'
                });

                if (response.ok) {
                    // Server is up - reload page
                    isServerDown = false;
                    reconnectAttempts = 0;
                    location.reload();
                } else {
                    throw new Error('Server returned error');
                }
            } catch (error) {
                // Server is down or unreachable
                if (!isServerDown) {
                    isServerDown = true;
                    showReconnectingMessage();
                }
                reconnectAttempts++;

                // Retry every 5 seconds when server is down
                remainingSeconds = 5;
                startCountdown();
            }
        }

        function showReconnectingMessage() {
            const countdownEl = document.getElementById('countdown');
            if (countdownEl) {
                countdownEl.parentElement.innerHTML = '<div style="color: #ff6b6b; font-weight: bold;">Server connection lost. Reconnecting...</div>';
            }
        }

        function updateCountdown() {
            const countdownEl = document.getElementById('countdown');
            if (countdownEl) {
                countdownEl.textContent = remainingSeconds;
            }
        }

        function refreshNow() {
            location.reload();
        }

        function updateInterval() {
            const input = document.getElementById('refresh-interval');
            const newInterval = parseInt(input.value);

            if (newInterval >= 5 && newInterval <= 600) {
                refreshInterval = newInterval;
                startCountdown();
            } else {
                alert('Please enter a value between 5 and 600 seconds');
                input.value = refreshInterval;
            }
        }

        // Toggle score detail view
        function toggleScoreDetail() {
            const checkbox = document.getElementById('detail-toggle');
            const summaryView = document.querySelector('.score-summary');
            const detailView = document.querySelector('.score-detail');

            if (checkbox.checked) {
                // Show detail (band breakdown), hide summary
                summaryView.classList.add('hidden');
                detailView.classList.remove('hidden');
            } else {
                // Show summary (totals only), hide detail
                summaryView.classList.remove('hidden');
                detailView.classList.add('hidden');
            }

            // Save preference to localStorage
            localStorage.setItem('scoreDetailEnabled', checkbox.checked);
        }

        // Start countdown when page loads
        window.onload = function() {
            document.getElementById('refresh-interval').value = refreshInterval;

            // Restore score detail preference (default: unchecked = summary only)
            const detailEnabled = localStorage.getItem('scoreDetailEnabled');
            const checkbox = document.getElementById('detail-toggle');
            if (detailEnabled === 'true') {
                checkbox.checked = true;
            } else {
                checkbox.checked = false;  // Default to summary view
            }
            toggleScoreDetail();

            startCountdown();
        };
    </script>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>%2</h1>
            <div class="callsign">%3</div>
            <div class="operator">Operator: %4</div>
        </div>

        <div class="grid">
            <div class="card">
                <h2>
                    <span>Score Summary</span>
                    <label>
                        <input type="checkbox" id="detail-toggle" onchange="toggleScoreDetail()">
                        Show Detail
                    </label>
                </h2>
                <div class="score-big">%5</div>

                <!-- Summary View (default) - Shows totals only -->
                <div class="score-summary">
                    <div class="stat">
                        <span class="stat-label">QSOs</span>
                        <span class="stat-value">%6</span>
                    </div>
                    <div class="stat">
                        <span class="stat-label">Points</span>
                        <span class="stat-value">%7</span>
                    </div>
                    <div class="stat">
                        <span class="stat-label">Multipliers</span>
                        <span class="stat-value">%8</span>
                    </div>
                </div>

                <!-- Detail View - Shows band-by-band breakdown -->
                <div class="score-detail hidden">
                    %15
                </div>
            </div>

            <div class="refresh-card">
                <h2>🔄 Auto Refresh</h2>
                <button onclick="refreshNow()">&#8635; Refresh Now</button>
                <button onclick="window.location.href='/map'" style="margin-top: 10px;">&#127758; Sections Map</button>
                <div>
                    <label>Interval (seconds):</label>
                    <input type="number" id="refresh-interval" min="5" max="600" step="5" onchange="updateInterval()">
                </div>
                <div class="countdown-text">
                    Next refresh in <span id="countdown">60</span>s
                </div>
            </div>
        </div>

        <div class="radio-status">
            <div class="radio-status-item">
                <span class="radio-status-label">Radio Frequency</span>
                <span class="radio-status-value">%9</span>
            </div>
            <div class="radio-status-item">
                <span class="radio-status-label">Band</span>
                <span class="radio-status-value">%10</span>
            </div>
            <div class="radio-status-item">
                <span class="radio-status-label">Mode</span>
                <span class="radio-status-value">%11</span>
            </div>
        </div>

        <div class="card">
            <h2>Recent QSOs (Last 10)</h2>
            <div class="qso-list">
                <div class="qso-header">
                    <div>Time</div>
                    <div>Callsign</div>
                    <div>Frequency</div>
                    <div>Band</div>
                    <div>Exchange</div>
                    <div>Operator</div>
                </div>
                %12
            </div>
        </div>

        <div class="refresh-info">
            Last updated: %13
        </div>

        <footer>
            Generated by <a href="https://github.com/n6ml/TR4QT" target="_blank">TR4QT</a> v%14
        </footer>
    </div>
</body>
</html>
)HTML";

    // Pull fresh data from sources
    AppSettings& settings = AppSettings::instance();
    QString myCall = settings.getMyCallsign();
    QString currentOperator = settings.getCurrentOperator();
    QString operatorDisplay = (currentOperator != myCall) ? currentOperator : myCall;

    // Pull radio state
    RadioState radioState = m_radioController->getCurrentState();
    QString freqStr = radioState.frequencyA > 0
        ? QString::number(radioState.frequencyA / 1000000.0, 'f', 3) + " MHz"
        : "N/A";
    QString bandStr = bandToString(radioState.bandA);
    QString modeStr = modeToString(radioState.modeA);

    // Pull QSO count from model (thread-safe)
    int qsoCount = m_qsoDataSource->qsoCount();

    // Calculate scores and band data from QSOs
    int totalQSOPoints = 0;
    int totalMultQSOs = 0;
    QMap<BandType, int> bandQSOs;
    QMap<BandType, int> bandMults;
    QMap<BandType, int> bandZones;
    QMap<BandType, int> bandPoints;

    // Mode group tracking (for mixed-mode contests)
    QMap<ModeGroup, QMap<BandType, int>> modeGroupQSOs;  // [ModeGroup][Band] = count

    // Iterate through all QSOs to calculate scores
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoDataSource->qsoAt(row);
        if (qso.band == BandType::None) continue;

        // Count per band
        bandQSOs[qso.band]++;
        bandPoints[qso.band] += qso.qsoPoints;
        totalQSOPoints += qso.qsoPoints;

        // Count per mode group and band (for mixed-mode breakdown)
        if (m_usesModeGroupBreakdown) {
            ModeGroup group = modeTypeToModeGroup(qso.mode);
            modeGroupQSOs[group][qso.band]++;
        }

        // Count mults
        if (qso.isMultiplier) {
            bandMults[qso.band]++;
            totalMultQSOs++;
        }

        // Count zones
        if (qso.cqZone > 0) {
            bandZones[qso.band] = qMax(bandZones.value(qso.band, 0), 1);  // Simplified
        }
    }

    // Calculate final score (points × mults)
    int score = totalQSOPoints * totalMultQSOs;
    QString scoreStr = QString::number(score);

    // Format recent QSOs (last 10)
    QString qsoListHtml;
    int startIdx = qMax(0, qsoCount - MAX_RECENT_QSOS);
    for (int row = startIdx; row < qsoCount; ++row) {
        QSO qso = m_qsoDataSource->qsoAt(row);
        QString timeStr = qso.timestamp.toString("HH:mm:ss");
        QString bandQso = bandToString(qso.band);
        QString freqQso = qso.frequency > 0
            ? QString::number(qso.frequency / 1000000.0, 'f', 3)
            : "N/A";
        QString operatorQso = qso.operatorCall.isEmpty() ? myCall : qso.operatorCall;

        qsoListHtml += QString(R"(<div class="qso-item">
                    <div class="qso-time">%1</div>
                    <div class="qso-call">%2</div>
                    <div class="qso-freq">%3</div>
                    <div class="qso-band">%4</div>
                    <div class="qso-exchange">%5</div>
                    <div class="qso-operator">%6</div>
                </div>)")
            .arg(timeStr)
            .arg(qso.callsign)
            .arg(freqQso)
            .arg(bandQso)
            .arg(qso.exchangeReceived)
            .arg(operatorQso);
    }

    if (qsoListHtml.isEmpty()) {
        qsoListHtml = R"(<div class="qso-item">
            <div style="grid-column: 1 / -1; text-align: center; opacity: 0.6;">
                No QSOs logged yet
            </div>
        </div>)";
    }

    // Generate band breakdown table (like BandSummaryGrid)
    QString bandTableHtml = R"(
        <table class="band-table">
            <thead>
                <tr>
                    <th></th>
                    <th>160</th>
                    <th>80</th>
                    <th>40</th>
                    <th>20</th>
                    <th>15</th>
                    <th>10</th>
                    <th class="total-col">All</th>
                </tr>
            </thead>
            <tbody>
)";

    QList<BandType> bands = {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };

    // QSOs row(s) - either mode group breakdown or single row
    if (m_usesModeGroupBreakdown) {
        // Mixed-mode: Show Phone, CW, Digital rows
        QList<ModeGroup> modeGroups = {ModeGroup::Phone, ModeGroup::CW, ModeGroup::Digital};

        for (ModeGroup group : modeGroups) {
            bandTableHtml += QString("<tr><td class=\"row-label\">%1</td>")
                .arg(modeGroupToString(group));

            int modeGroupTotal = 0;
            for (BandType band : bands) {
                int count = modeGroupQSOs.value(group).value(band, 0);
                modeGroupTotal += count;
                bandTableHtml += QString("<td>%1</td>").arg(count);
            }
            bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(modeGroupTotal);
        }
    } else {
        // Single-mode: Show just QSOs row
        bandTableHtml += "<tr><td class=\"row-label\">QSOs</td>";
        int bandQSOTotal = 0;
        for (BandType band : bands) {
            int count = bandQSOs.value(band, 0);
            bandQSOTotal += count;
            bandTableHtml += QString("<td>%1</td>").arg(count);
        }
        bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandQSOTotal);
    }

    // Mults row
    bandTableHtml += "<tr><td class=\"row-label\">Mults</td>";
    int bandMultTotal = 0;
    for (BandType band : bands) {
        int count = bandMults.value(band, 0);
        bandMultTotal += count;
        bandTableHtml += QString("<td>%1</td>").arg(count);
    }
    bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandMultTotal);

    // Zones row (only if contest uses zone multipliers)
    if (m_usesZoneMultipliers) {
        bandTableHtml += "<tr><td class=\"row-label\">Zones</td>";
        int bandZoneTotal = 0;
        for (BandType band : bands) {
            int count = bandZones.value(band, 0);
            bandZoneTotal += count;
            bandTableHtml += QString("<td>%1</td>").arg(count);
        }
        bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandZoneTotal);
    }

    // Points row
    bandTableHtml += "<tr><td class=\"row-label\">Points</td>";
    int bandPointTotal = 0;
    for (BandType band : bands) {
        int points = bandPoints.value(band, 0);
        bandPointTotal += points;
        bandTableHtml += QString("<td>%1</td>").arg(points);
    }
    bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandPointTotal);

    bandTableHtml += "</tbody></table>";

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    return html
        .arg(m_contestName)                          // %1 title
        .arg(m_contestName)                          // %2 contest name header
        .arg(myCall)                                 // %3 callsign
        .arg(operatorDisplay)                        // %4 operator
        .arg(scoreStr)                               // %5 score big
        .arg(qsoCount)                               // %6 QSO count
        .arg(totalQSOPoints)                         // %7 points
        .arg(totalMultQSOs)                          // %8 multipliers
        .arg(freqStr)                                // %9 frequency
        .arg(bandStr)                                // %10 band
        .arg(modeStr)                                // %11 mode
        .arg(qsoListHtml)                            // %12 QSO list
        .arg(timestamp)                              // %13 timestamp
        .arg(APP_VERSION)                            // %14 version
        .arg(bandTableHtml);                         // %15 band breakdown table
}

QJsonObject WebServer::radioStateToJson(const RadioState& state) {
    QJsonObject json;
    json["frequencyA"] = static_cast<qint64>(state.frequencyA);
    json["frequencyB"] = static_cast<qint64>(state.frequencyB);
    json["bandA"] = bandToString(state.bandA);
    json["bandB"] = bandToString(state.bandB);
    json["modeA"] = modeToString(state.modeA);
    json["modeB"] = modeToString(state.modeB);
    json["ptt"] = state.isTransmitting;
    json["split"] = state.isSplitEnabled;
    json["radioModel"] = state.radioModel;
    return json;
}

QJsonObject WebServer::qsoToJson(const QSO& qso) {
    QJsonObject json;
    json["id"] = qso.id;
    json["timestamp"] = qso.timestamp.toString(Qt::ISODate);
    json["callsign"] = qso.callsign;
    json["frequency"] = static_cast<qint64>(qso.frequency);
    json["band"] = bandToString(qso.band);
    json["mode"] = modeToString(qso.mode);
    json["rstSent"] = qso.rstSent;
    json["rstReceived"] = qso.rstReceived;
    json["exchangeSent"] = qso.exchangeSent;
    json["exchangeReceived"] = qso.exchangeReceived;
    json["points"] = qso.qsoPoints;
    return json;
}

QHttpServerResponse WebServer::handleFavicon() {
    // Serve the .ico file from resources
    QFile iconFile(":/icons/tr4qt.ico");
    if (!iconFile.open(QIODevice::ReadOnly)) {
        LOG_WARN("WebServer", "Failed to open favicon.ico from resources");
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }

    QByteArray iconData = iconFile.readAll();
    return QHttpServerResponse("image/x-icon", iconData);
}

QHttpServerResponse WebServer::handleAppleTouchIcon() {
    // Serve the 256x256 PNG for Apple devices (they prefer 180x180 but 256 is close enough)
    QFile iconFile(":/icons/icon_256x256.png");
    if (!iconFile.open(QIODevice::ReadOnly)) {
        LOG_WARN("WebServer", "Failed to open apple-touch-icon.png from resources");
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }

    QByteArray iconData = iconFile.readAll();
    return QHttpServerResponse("image/png", iconData);
}

QHttpServerResponse WebServer::handleApiSectionsGeoJSON() {
    // Serve the ARRL sections GeoJSON data from resources
    QFile geoJsonFile(":/data/arrl_sections.geojson");
    if (!geoJsonFile.open(QIODevice::ReadOnly)) {
        LOG_WARN("WebServer", "Failed to open arrl_sections.geojson from resources");
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }

    QByteArray geoJsonData = geoJsonFile.readAll();

    // Return with appropriate content type
    return QHttpServerResponse("application/json", geoJsonData);
}

QHttpServerResponse WebServer::handleApiStatesGeoJSON() {
    // Serve the US states GeoJSON data from resources
    QFile geoJsonFile(":/data/us_states.geojson");
    if (!geoJsonFile.open(QIODevice::ReadOnly)) {
        LOG_WARN("WebServer", "Failed to open us_states.geojson from resources");
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }

    QByteArray geoJsonData = geoJsonFile.readAll();

    // Return with appropriate content type
    return QHttpServerResponse("application/json", geoJsonData);
}

QString WebServer::generateSectionsMapHtml() {
    AppSettings& settings = AppSettings::instance();
    QString callsign = settings.getMyCallsign();
    QString operator_name = settings.getCurrentOperator();

    QString html = R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ARRL Sections Map - %1</title>
    <link rel="icon" type="image/x-icon" href="/favicon.ico">
    <link rel="apple-touch-icon" sizes="256x256" href="/apple-touch-icon.png">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #1a1a2e;
            color: #fff;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 15px 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.3);
        }
        .header h1 {
            font-size: 1.8em;
            margin-bottom: 5px;
        }
        .header .info {
            font-size: 0.9em;
            opacity: 0.9;
        }
        .container {
            display: flex;
            height: calc(100vh - 80px);
        }
        #map {
            flex: 1;
            height: 100%;
        }
        .sidebar {
            width: 300px;
            background: #16213e;
            padding: 20px;
            overflow-y: auto;
            box-shadow: -2px 0 10px rgba(0,0,0,0.3);
        }
        .stats {
            background: rgba(255,255,255,0.1);
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .stats h3 {
            margin-bottom: 10px;
            color: #ffd700;
        }
        .stat-row {
            display: flex;
            justify-content: space-between;
            margin: 8px 0;
            padding: 5px 0;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .legend {
            background: rgba(255,255,255,0.1);
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .legend h3 {
            margin-bottom: 10px;
            color: #ffd700;
        }
        .legend-item {
            display: flex;
            align-items: center;
            margin: 8px 0;
        }
        .legend-color {
            width: 30px;
            height: 20px;
            margin-right: 10px;
            border: 1px solid #fff;
            border-radius: 3px;
        }
        .worked-list {
            background: rgba(255,255,255,0.1);
            padding: 15px;
            border-radius: 8px;
            max-height: 400px;
            overflow-y: auto;
        }
        .worked-list h3 {
            margin-bottom: 10px;
            color: #ffd700;
            position: sticky;
            top: 0;
            background: #16213e;
            padding: 5px 0;
        }
        .section-item {
            padding: 5px;
            margin: 3px 0;
            background: rgba(255,255,255,0.05);
            border-radius: 3px;
        }
        .controls {
            text-align: center;
            padding: 15px;
            background: rgba(255,255,255,0.1);
            border-radius: 8px;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            margin: 5px;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
        }
        @media (max-width: 900px) {
            .container { flex-direction: column; }
            .sidebar { width: 100%; max-height: 300px; }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>ARRL Sections Worked - %2</h1>
        <div class="info">Callsign: %3 | Operator: %4</div>
    </div>

    <div class="container">
        <div id="map"></div>

        <div class="sidebar">
            <div class="stats">
                <h3>Statistics</h3>
                <div class="stat-row">
                    <span>Sections Worked:</span>
                    <span id="sections-worked">0</span>
                </div>
                <div class="stat-row">
                    <span>Total Sections:</span>
                    <span>83</span>
                </div>
                <div class="stat-row">
                    <span>Completion:</span>
                    <span id="completion">0%</span>
                </div>
                <div class="stat-row">
                    <span>Total QSOs:</span>
                    <span id="total-qsos">0</span>
                </div>
            </div>

            <div class="legend">
                <h3>QSO Count Legend</h3>
                <div class="legend-item">
                    <div class="legend-color" style="background: #cccccc;"></div>
                    <span>0 (Not Worked)</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #3498db;"></div>
                    <span>1 QSO</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #e74c3c;"></div>
                    <span>2 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #0d5d0d;"></div>
                    <span>3-9 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #137a13;"></div>
                    <span>10-19 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #1a9e1a;"></div>
                    <span>20-49 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #2ecc71;"></div>
                    <span>50-99 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #5ed68f;"></div>
                    <span>100-199 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #8ee0ad;"></div>
                    <span>200-499 QSOs</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #c8f0dc;"></div>
                    <span>500+ QSOs</span>
                </div>
            </div>

            <div class="controls">
                <button class="btn" onclick="refreshData()">Refresh Data</button>
                <button class="btn" onclick="window.location.href='/dashboard'">Dashboard</button>
            </div>

            <div class="worked-list">
                <h3>Worked Sections</h3>
                <div id="worked-sections-list"></div>
            </div>
        </div>
    </div>

    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script>
        // Initialize map centered on USA
        const map = L.map('map').setView([39.8, -98.6], 4);

        // Add tile layer
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '© OpenStreetMap contributors',
            maxZoom: 18,
        }).addTo(map);

        // Store data
        let workedSections = {};
        let geoJsonLayer = null;
        let sectionsGeoJSON = null;
        let initialLoadComplete = false;

        // Chloropleth color scheme
        function getColorForCount(count) {
            if (count === 0) return '#cccccc';      // Gray - not worked
            if (count === 1) return '#3498db';      // Blue - first contact
            if (count === 2) return '#e74c3c';      // Red - second contact
            if (count <= 9) return '#0d5d0d';       // Dark green - 3-9
            if (count <= 19) return '#137a13';      // Green - 10-19
            if (count <= 49) return '#1a9e1a';      // Medium green - 20-49
            if (count <= 99) return '#2ecc71';      // Bright green - 50-99
            if (count <= 199) return '#5ed68f';     // Light green - 100-199
            if (count <= 499) return '#8ee0ad';     // Lighter green - 200-499
            return '#c8f0dc';                        // Very light green - 500+
        }

        function styleFeature(feature) {
            const section = feature.properties.section;
            const count = workedSections[section] || 0;

            return {
                fillColor: getColorForCount(count),
                weight: 1,
                opacity: 1,
                color: '#ffffff',
                fillOpacity: 0.7
            };
        }

        function onEachFeature(feature, layer) {
            const section = feature.properties.section;
            const count = workedSections[section] || 0;

            layer.bindPopup(`
                <b>${section}</b><br>
                QSOs: ${count}<br>
                Status: ${count > 0 ? 'Worked' : 'Not Worked'}
            `);

            // Highlight on hover
            layer.on('mouseover', function() {
                this.setStyle({
                    weight: 3,
                    color: '#ffff00',
                    fillOpacity: 0.85
                });
            });

            layer.on('mouseout', function() {
                geoJsonLayer.resetStyle(this);
            });
        }

        async function loadGeoJSON() {
            try {
                const response = await fetch('/api/sections-geojson');
                sectionsGeoJSON = await response.json();
                console.log(`Loaded ${sectionsGeoJSON.features.length} section polygons`);
                return true;
            } catch (error) {
                console.error('Failed to load GeoJSON:', error);
                return false;
            }
        }

        async function renderSectionPolygons() {
            if (!sectionsGeoJSON) {
                console.error('GeoJSON data not loaded');
                return;
            }

            // Remove existing layer if present
            if (geoJsonLayer) {
                map.removeLayer(geoJsonLayer);
            }

            // Add GeoJSON layer with styling
            geoJsonLayer = L.geoJSON(sectionsGeoJSON, {
                style: styleFeature,
                onEachFeature: onEachFeature
            }).addTo(map);

            // Only fit bounds on initial load - preserve user's zoom/pan on refresh
            if (!initialLoadComplete && geoJsonLayer.getBounds().isValid()) {
                map.fitBounds(geoJsonLayer.getBounds(), { padding: [20, 20] });
                initialLoadComplete = true;
            }
        }

        async function loadWorkedSections() {
            try {
                const response = await fetch('/api/worked-sections');
                const data = await response.json();

                // Convert array to map
                workedSections = {};
                data.sections.forEach(section => {
                    workedSections[section.section] = section.count;
                });

                // Update UI
                updateStats(data);
                updateWorkedList(data.sections);

                // Re-render polygons with new data
                if (sectionsGeoJSON) {
                    renderSectionPolygons();
                }

            } catch (error) {
                console.error('Failed to load worked sections:', error);
            }
        }

        function updateStats(data) {
            document.getElementById('sections-worked').textContent = data.totalSections;
            document.getElementById('total-qsos').textContent = data.totalQsos;
            const completion = ((data.totalSections / 83) * 100).toFixed(1);
            document.getElementById('completion').textContent = completion + '%';
        }

        function updateWorkedList(sections) {
            const listEl = document.getElementById('worked-sections-list');

            if (sections.length === 0) {
                listEl.innerHTML = '<div style="opacity: 0.6; text-align: center; padding: 20px;">No sections worked yet</div>';
                return;
            }

            // Sort by count descending
            sections.sort((a, b) => b.count - a.count);

            listEl.innerHTML = sections.map(s => `
                <div class="section-item">
                    <strong>${s.section}</strong>: ${s.count} QSO${s.count !== 1 ? 's' : ''}
                </div>
            `).join('');
        }

        function refreshData() {
            loadWorkedSections();
        }

        // Load data on page load
        window.addEventListener('load', async () => {
            // First load GeoJSON polygons (one-time load)
            const geoJsonLoaded = await loadGeoJSON();

            if (geoJsonLoaded) {
                // Then load worked sections data and render
                await loadWorkedSections();

                // Auto-refresh worked sections every 30 seconds
                setInterval(loadWorkedSections, 30000);
            } else {
                console.error('Failed to initialize map - GeoJSON not loaded');
            }
        });
    </script>
</body>
</html>
)HTML";

    return html.arg(m_contestName.isEmpty() ? "Contest" : m_contestName)
               .arg(m_contestName.isEmpty() ? "TR4QT Contest Logger" : m_contestName)
               .arg(callsign.isEmpty() ? "N0CALL" : callsign)
               .arg(operator_name.isEmpty() ? "Unknown" : operator_name);
}

QString WebServer::generateStatesMapHtml() {
    AppSettings& settings = AppSettings::instance();
    QString callsign = settings.getMyCallsign();
    QString operator_name = settings.getCurrentOperator();

    // Re-use the exact same HTML as sections map, but with states terminology
    // Just search-replace: sections→states, Sections→States, ARRL Sections→US States
    QString html = generateSectionsMapHtml();

    // Replace sections terminology with states terminology
    html.replace("ARRL Sections", "US States");
    html.replace("Sections Worked", "States Worked");
    html.replace("sections-worked", "states-worked");
    html.replace("Total Sections:", "Total States:");
    html.replace("<span>83</span>", "<span>50</span>");  // 50 states
    html.replace("/api/worked-sections", "/api/worked-states");
    html.replace("/api/sections-geojson", "/api/states-geojson");
    html.replace("sectionsGeoJSON", "statesGeoJSON");
    html.replace("workedSections", "workedStates");
    html.replace("feature.properties.section", "feature.properties.state");
    html.replace("const section =", "const state =");
    html.replace("section polygons", "state polygons");
    html.replace("Worked Sections", "Worked States");
    html.replace("worked-sections-list", "worked-states-list");
    html.replace("`Loaded ${", "`Loaded ${");  // Keep this line intact
    html.replace("Section:", "State:");
    html.replace("forEach(section =>", "forEach(state =>");
    html.replace("(section.section", "(state.state");
    html.replace("section.count", "state.count");
    html.replace("${section.section}", "${state.state}");
    html.replace("${section.count}", "${state.count}");
    html.replace("'/api/sections-geojson'", "'/api/states-geojson'");
    html.replace("'/api/worked-sections'", "'/api/worked-states'");
    html.replace("console.log(`Loaded ${sectionsGeoJSON", "console.log(`Loaded ${statesGeoJSON");
    html.replace(".sections", ".states");
    html.replace("sections:", "states:");
    html.replace("totalSections", "totalStates");

    return html;
}

// === Command API handlers (POST endpoints) ===

QHttpServerResponse WebServer::jsonError(int statusCode, const QString& message,
                                          const QString& field) {
    QJsonObject json;
    json["success"] = false;
    json["error"] = message;
    if (!field.isEmpty()) {
        json["field"] = field;
    }

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson(),
                               static_cast<QHttpServerResponse::StatusCode>(statusCode));
}

QHttpServerResponse WebServer::jsonSuccess(const QJsonObject& data) {
    QJsonObject json = data;
    json["success"] = true;

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handlePostLogQSO(const QHttpServerRequest& request) {
    LOG_DEBUG("WebServer", "Received POST /api/log-qso");

    // Parse JSON body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("WebServer", QString("Invalid JSON in log-qso request: %1")
                 .arg(parseError.errorString()));
        return jsonError(400, QString("Invalid JSON: %1").arg(parseError.errorString()));
    }

    if (!doc.isObject()) {
        return jsonError(400, "Request body must be a JSON object");
    }

    QJsonObject json = doc.object();

    // Validate required fields
    if (!json.contains("callsign") || json["callsign"].toString().trimmed().isEmpty()) {
        return jsonError(400, "Missing required field: callsign", "callsign");
    }

    // Build request struct
    LogQSOWebRequest req;
    req.callsign = json["callsign"].toString().trimmed().toUpper();
    req.exchange = json.value("exchange").toString().trimmed().toUpper();

    // Optional fields
    if (json.contains("frequency")) {
        req.frequency = static_cast<freq_t>(json["frequency"].toInteger());
    }

    if (json.contains("band")) {
        QString bandStr = json["band"].toString().toUpper();
        req.band = stringToBand(bandStr);
        if (req.band == BandType::None && !bandStr.isEmpty()) {
            return jsonError(400, QString("Invalid band: %1").arg(bandStr), "band");
        }
    }

    if (json.contains("mode")) {
        QString modeStr = json["mode"].toString().toUpper();
        req.mode = stringToMode(modeStr);
        if (req.mode == ModeType::None && !modeStr.isEmpty()) {
            return jsonError(400, QString("Invalid mode: %1").arg(modeStr), "mode");
        }
    }

    // Parse timestamp (ISO 8601 format: "2025-03-29T17:23:00Z")
    // If not provided or invalid, QDateTime will be invalid and logging service uses current UTC
    if (json.contains("datetime")) {
        QString dtStr = json["datetime"].toString();
        req.timestamp = QDateTime::fromString(dtStr, Qt::ISODate);
        if (!req.timestamp.isValid() && !dtStr.isEmpty()) {
            return jsonError(400, QString("Invalid datetime format: %1 (expected ISO 8601)").arg(dtStr), "datetime");
        }
        // Ensure timestamp is in UTC
        if (req.timestamp.isValid()) {
            req.timestamp.setTimeSpec(Qt::UTC);
        }
    }

    LOG_INFO("WebServer", QString("Log QSO request: %1 exchange=%2%3")
             .arg(req.callsign)
             .arg(req.exchange)
             .arg(req.timestamp.isValid() ? QString(" at %1").arg(req.timestamp.toString(Qt::ISODate)) : ""));

    // Emit signal for MainWindow to handle (synchronous - same thread)
    LogQSOWebResponse response;
    emit logQSORequested(req, &response);

    // Build JSON response
    if (!response.success) {
        int statusCode = 400;  // Bad request by default

        // Map specific errors to HTTP status codes
        if (response.error.contains("No active contest", Qt::CaseInsensitive)) {
            statusCode = 503;  // Service unavailable
        } else if (response.isDuplicate) {
            statusCode = 409;  // Conflict (duplicate)
        }

        return jsonError(statusCode, response.error, response.errorField);
    }

    // Build success response with QSO details
    QJsonObject qsoJson;
    qsoJson["id"] = response.qsoId;
    qsoJson["callsign"] = response.callsign;
    qsoJson["timestamp"] = response.timestamp.toString(Qt::ISODate);
    qsoJson["frequency"] = static_cast<qint64>(response.frequency);
    qsoJson["band"] = response.band;
    qsoJson["mode"] = response.mode;
    qsoJson["exchangeSent"] = response.exchangeSent;
    qsoJson["exchangeReceived"] = response.exchangeReceived;
    qsoJson["points"] = response.points;
    qsoJson["isMultiplier"] = response.isMultiplier;
    qsoJson["isDuplicate"] = response.isDuplicate;

    QJsonObject result;
    result["qso"] = qsoJson;
    result["serialNumber"] = response.serialNumber;

    LOG_INFO("WebServer", QString("QSO logged via API: %1 (id=%2)")
             .arg(response.callsign).arg(response.qsoId));

    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handlePostCommand(const QHttpServerRequest& request) {
    LOG_DEBUG("WebServer", "Received POST /api/command");

    // Parse JSON body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("WebServer", QString("Invalid JSON in command request: %1")
                 .arg(parseError.errorString()));
        return jsonError(400, QString("Invalid JSON: %1").arg(parseError.errorString()));
    }

    if (!doc.isObject()) {
        return jsonError(400, "Request body must be a JSON object");
    }

    QJsonObject json = doc.object();

    // Validate required fields
    if (!json.contains("command") || json["command"].toString().trimmed().isEmpty()) {
        return jsonError(400, "Missing required field: command", "command");
    }

    // Build request struct
    CommandWebRequest req;
    req.command = json["command"].toString().trimmed().toLower();

    // Parse params object if present
    if (json.contains("params") && json["params"].isObject()) {
        QJsonObject paramsObj = json["params"].toObject();
        for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it) {
            req.params[it.key()] = it.value().toVariant();
        }
    }

    LOG_INFO("WebServer", QString("Command request: %1").arg(req.command));

    // Emit signal for MainWindow to handle (synchronous - same thread)
    CommandWebResponse response;
    emit commandRequested(req, &response);

    // Build JSON response
    if (!response.success) {
        return jsonError(400, response.error);
    }

    QJsonObject result;
    result["command"] = response.command;
    result["message"] = response.message;

    LOG_INFO("WebServer", QString("Command executed: %1 - %2")
             .arg(response.command).arg(response.message));

    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handleGetCommands() {
    LOG_DEBUG("WebServer", "Received GET /api/commands");

    QJsonArray commands;

    // Helper to add radio parameter to a params array
    auto addRadioParam = [](QJsonArray& params) {
        QJsonObject param;
        param["name"] = "radio";
        param["type"] = "string";
        param["description"] = "Target radio: 'active' (default), 'standby', '1', or '2'";
        param["required"] = false;
        params.append(param);
    };

    // send-cw command
    {
        QJsonObject cmd;
        cmd["command"] = "send-cw";
        cmd["description"] = "Send a CW message to specified radio";
        QJsonArray params;
        {
            QJsonObject param;
            param["name"] = "message";
            param["type"] = "string";
            param["description"] = "CW message text to send";
            param["required"] = false;
            params.append(param);
        }
        {
            QJsonObject param;
            param["name"] = "fkey";
            param["type"] = "integer";
            param["description"] = "Function key number (1-12) to send";
            param["required"] = false;
            params.append(param);
        }
        addRadioParam(params);
        cmd["params"] = params;
        cmd["notes"] = "Either 'message' or 'fkey' must be provided, not both";
        commands.append(cmd);
    }

    // set-frequency command
    {
        QJsonObject cmd;
        cmd["command"] = "set-frequency";
        cmd["description"] = "Set the operating frequency";
        QJsonArray params;
        {
            QJsonObject param;
            param["name"] = "frequency";
            param["type"] = "integer";
            param["description"] = "Frequency in Hz (e.g., 14025000 for 14.025 MHz)";
            param["required"] = true;
            params.append(param);
        }
        addRadioParam(params);
        cmd["params"] = params;
        commands.append(cmd);
    }

    // set-band command
    {
        QJsonObject cmd;
        cmd["command"] = "set-band";
        cmd["description"] = "Change the operating band";
        QJsonArray params;
        {
            QJsonObject param;
            param["name"] = "band";
            param["type"] = "string";
            param["description"] = "Band name (160M, 80M, 40M, 20M, 15M, 10M, 6M, 2M, etc.)";
            param["required"] = true;
            params.append(param);
        }
        addRadioParam(params);
        cmd["params"] = params;
        commands.append(cmd);
    }

    // set-mode command
    {
        QJsonObject cmd;
        cmd["command"] = "set-mode";
        cmd["description"] = "Change the operating mode";
        QJsonArray params;
        {
            QJsonObject param;
            param["name"] = "mode";
            param["type"] = "string";
            param["description"] = "Mode name (CW, USB, LSB, FM, AM, RTTY, FT8, etc.)";
            param["required"] = true;
            params.append(param);
        }
        addRadioParam(params);
        cmd["params"] = params;
        commands.append(cmd);
    }

    // toggle-run-mode command
    {
        QJsonObject cmd;
        cmd["command"] = "toggle-run-mode";
        cmd["description"] = "Toggle between CQ (Run) and S&P (Search & Pounce) modes";
        cmd["params"] = QJsonArray();
        commands.append(cmd);
    }

    // clear-entry command
    {
        QJsonObject cmd;
        cmd["command"] = "clear-entry";
        cmd["description"] = "Clear the callsign and exchange entry fields";
        cmd["params"] = QJsonArray();
        commands.append(cmd);
    }

    QJsonObject result;
    result["commands"] = commands;
    result["version"] = APP_VERSION;
    result["apiVersion"] = "1.0";

    return jsonSuccess(result);
}

// === Contest API handlers ===

QHttpServerResponse WebServer::handlePostContestCreate(const QHttpServerRequest& request) {
    LOG_DEBUG("WebServer", "Received POST /api/contest/create");

    // Parse JSON body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("WebServer", QString("Invalid JSON in contest/create request: %1")
                 .arg(parseError.errorString()));
        return jsonError(400, QString("Invalid JSON: %1").arg(parseError.errorString()));
    }

    if (!doc.isObject()) {
        return jsonError(400, "Request body must be a JSON object");
    }

    QJsonObject json = doc.object();

    // Validate required fields
    if (!json.contains("contestType") || json["contestType"].toString().trimmed().isEmpty()) {
        return jsonError(400, "Missing required field: contestType", "contestType");
    }

    // Build request struct
    CreateContestRequest req;
    req.contestType = json["contestType"].toString().trimmed().toUpper();
    req.callsign = json.value("callsign").toString().trimmed().toUpper();
    req.exchangeSent = json.value("exchangeSent").toString().trimmed();
    req.mode = json.value("mode").toString().trimmed().toUpper();
    req.category = json.value("category").toString().trimmed().toUpper();
    req.powerClass = json.value("powerClass").toString().trimmed().toUpper();
    req.operatorName = json.value("operatorName").toString().trimmed();

    LOG_INFO("WebServer", QString("Create contest request: type=%1 callsign=%2")
             .arg(req.contestType).arg(req.callsign));

    // Emit signal for handler (WebServerContext or MainWindow)
    CreateContestResponse response;
    emit createContestRequested(req, &response);

    // Build JSON response
    if (!response.success) {
        return jsonError(500, response.error);
    }

    QJsonObject result;
    result["contestDbId"] = response.contestDbId;
    result["serialNumber"] = response.serialNumber;
    result["contestName"] = response.contestName;
    result["databasePath"] = response.databasePath;

    LOG_INFO("WebServer", QString("Contest created: %1 (dbId=%2)")
             .arg(response.contestName).arg(response.contestDbId));

    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handlePostContestOpen(const QHttpServerRequest& request) {
    LOG_DEBUG("WebServer", "Received POST /api/contest/open");

    // Parse JSON body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("WebServer", QString("Invalid JSON in contest/open request: %1")
                 .arg(parseError.errorString()));
        return jsonError(400, QString("Invalid JSON: %1").arg(parseError.errorString()));
    }

    if (!doc.isObject()) {
        return jsonError(400, "Request body must be a JSON object");
    }

    QJsonObject json = doc.object();

    // Validate required fields
    if (!json.contains("databasePath") || json["databasePath"].toString().trimmed().isEmpty()) {
        return jsonError(400, "Missing required field: databasePath", "databasePath");
    }

    // Build request struct
    OpenContestRequest req;
    req.databasePath = json["databasePath"].toString().trimmed();

    LOG_INFO("WebServer", QString("Open contest request: path=%1").arg(req.databasePath));

    // Emit signal for handler
    OpenContestResponse response;
    emit openContestRequested(req, &response);

    // Build JSON response
    if (!response.success) {
        return jsonError(500, response.error);
    }

    QJsonObject result;
    result["contestDbId"] = response.contestDbId;
    result["contestName"] = response.contestName;
    result["contestType"] = response.contestType;
    result["qsoCount"] = response.qsoCount;
    result["serialNumber"] = response.serialNumber;

    LOG_INFO("WebServer", QString("Contest opened: %1 (%2 QSOs)")
             .arg(response.contestName).arg(response.qsoCount));

    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handlePostContestClose(const QHttpServerRequest& request) {
    Q_UNUSED(request);
    LOG_DEBUG("WebServer", "Received POST /api/contest/close");

    // Emit signal for handler
    emit closeContestRequested();

    QJsonObject result;
    result["message"] = "Contest closed";

    LOG_INFO("WebServer", "Contest closed via API");

    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handleGetContestStatus() {
    LOG_DEBUG("WebServer", "Received GET /api/contest/status");

    // Emit signal for handler to populate response
    ContestStatusResponse response;
    emit contestStatusRequested(&response);

    QJsonObject result;
    result["active"] = response.active;

    if (response.active) {
        result["contestId"] = response.contestId;
        result["contestName"] = response.contestName;
        result["contestType"] = response.contestType;
        result["qsoCount"] = response.qsoCount;
        result["serialNumber"] = response.serialNumber;
        result["totalPoints"] = response.totalPoints;
        result["totalMultipliers"] = response.totalMultipliers;
        result["score"] = response.totalPoints * response.totalMultipliers;
    }

    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handleGetExportAdif() {
    LOG_DEBUG("WebServer", "Received GET /api/export/adif");

    if (!m_qsoDataSource) {
        return jsonError(500, "No QSO data source configured");
    }

    // Get contest info for filename
    ContestStatusResponse status;
    emit contestStatusRequested(&status);

    // Generate filename: CONTESTTYPE_YYYYMMDD_NQSOs.adi
    QString filename;
    if (status.active && !status.contestType.isEmpty()) {
        QString dateStr = QDateTime::currentDateTimeUtc().toString("yyyyMMdd");
        filename = QString("%1_%2_%3qsos.adi")
            .arg(status.contestType)
            .arg(dateStr)
            .arg(status.qsoCount);
    } else {
        filename = "export.adi";
    }

    // Get all QSOs from the data source
    QList<QSO> qsos = m_qsoDataSource->allQSOs();

    // Generate ADIF content
    ADIFExporter exporter;
    QString adifContent = exporter.generateADIF(qsos);

    // Return JSON with filename and content
    QJsonObject result;
    result["success"] = true;
    result["filename"] = filename;
    result["qsoCount"] = qsos.size();
    result["content"] = adifContent;

    LOG_INFO("WebServer", QString("ADIF export: %1 (%2 QSOs)").arg(filename).arg(qsos.size()));
    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handleGetExportCabrillo() {
    LOG_DEBUG("WebServer", "Received GET /api/export/cabrillo");

    // Get contest info for filename
    ContestStatusResponse status;
    emit contestStatusRequested(&status);

    // Generate filename: CONTESTTYPE_YYYYMMDD.cbr
    QString filename;
    if (status.active && !status.contestType.isEmpty()) {
        QString dateStr = QDateTime::currentDateTimeUtc().toString("yyyyMMdd");
        filename = QString("%1_%2.cbr")
            .arg(status.contestType)
            .arg(dateStr);
    } else {
        filename = "export.cbr";
    }

    // Emit signal for handler to generate Cabrillo
    QString cabrilloContent;
    emit cabrilloExportRequested(&cabrilloContent);

    if (cabrilloContent.isEmpty()) {
        return jsonError(400, "No active contest or no QSOs to export");
    }

    // Return JSON with filename and content
    QJsonObject result;
    result["success"] = true;
    result["filename"] = filename;
    result["qsoCount"] = status.qsoCount;
    result["content"] = cabrilloContent;

    LOG_INFO("WebServer", QString("Cabrillo export: %1").arg(filename));
    return jsonSuccess(result);
}

QHttpServerResponse WebServer::handleGetContestScore() {
    LOG_DEBUG("WebServer", "Received GET /api/contest/score");

    // Emit signal for handler to populate response
    ScoreResponse response;
    emit contestScoreRequested(&response);

    QJsonObject result;
    result["active"] = response.active;

    if (response.active) {
        result["contestName"] = response.contestName;
        result["totalQsos"] = response.totalQsos;
        result["totalPoints"] = response.totalPoints;
        result["totalMultipliers"] = response.totalMultipliers;
        result["score"] = response.score;

        // Band breakdown array
        QJsonArray breakdown;
        for (const BandBreakdown& entry : response.bandBreakdown) {
            QJsonObject bandObj;
            bandObj["band"] = entry.band;
            bandObj["mode"] = entry.mode;
            bandObj["qsos"] = entry.qsos;
            bandObj["points"] = entry.points;
            bandObj["multipliers"] = entry.multipliers;
            breakdown.append(bandObj);
        }
        result["bandBreakdown"] = breakdown;
    }

    return jsonSuccess(result);
}

} // namespace TR4QT
