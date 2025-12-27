#include "WebServer.h"
#include "../ui/models/QSOTableModel.h"
#include "../radio/RadioController.h"
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"
#include "../core/Types.h"
#include "../core/Constants.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTcpServer>
#include <QHttpHeaders>

namespace TR4QT {

WebServer::WebServer(QSOTableModel* qsoModel,
                     RadioController* radioController,
                     QObject* parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_qsoModel(qsoModel)
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

    // HTML dashboard
    m_server->route("/dashboard", [this]() {
        return handleDashboard();
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
    if (!m_server->bind(m_tcpServer)) {
        QString error = QString("Failed to bind HTTP server to TCP server");
        LOG_ERROR("WebServer", error);
        emit errorOccurred(error);
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
        return false;
    }

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

// HTTP Route Handlers

QHttpServerResponse WebServer::handleRoot() {
    // Redirect to dashboard
    QHttpServerResponse response(QHttpServerResponder::StatusCode::MovedPermanently);
    QHttpHeaders headers;
    headers.append("Location", "/dashboard");
    response.setHeaders(headers);
    return response;
}

QHttpServerResponse WebServer::handleApiStatus() {
    // Pull fresh data from sources
    AppSettings& settings = AppSettings::instance();
    QString myCall = settings.getMyCallsign();
    QString currentOperator = settings.getCurrentOperator();

    // Calculate score from QSO model
    int qsoCount = m_qsoModel->count();
    int totalPoints = 0;
    int totalMults = 0;

    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoModel->getQSO(row);
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
    int qsoCount = m_qsoModel->count();
    int startIdx = qMax(0, qsoCount - MAX_RECENT_QSOS);

    for (int row = startIdx; row < qsoCount; ++row) {
        QSO qso = m_qsoModel->getQSO(row);
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
    // Calculate score from QSO model
    int qsoCount = m_qsoModel->count();
    int totalPoints = 0;
    int totalMults = 0;

    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoModel->getQSO(row);
        totalPoints += qso.qsoPoints;
        if (qso.isMultiplier) {
            totalMults++;
        }
    }

    QJsonObject json;
    json["qsos"] = qsoCount;
    json["points"] = totalPoints;
    json["multipliers"] = totalMults;
    json["score"] = totalPoints * totalMults;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonDocument doc(json);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse WebServer::handleDashboard() {
    QString html = generateDashboardHtml();
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

        function startCountdown() {
            if (timerId) clearInterval(timerId);
            remainingSeconds = refreshInterval;
            updateCountdown();

            timerId = setInterval(function() {
                remainingSeconds--;
                updateCountdown();

                if (remainingSeconds <= 0) {
                    location.reload();
                }
            }, 1000);
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
                    <span>📊 Score Summary</span>
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
                <span class="radio-status-label">📻 Frequency</span>
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
    int qsoCount = m_qsoModel->count();

    // Calculate scores and band data from QSOs
    int totalQSOPoints = 0;
    int totalMultQSOs = 0;
    QMap<BandType, int> bandQSOs;
    QMap<BandType, int> bandMults;
    QMap<BandType, int> bandZones;
    QMap<BandType, int> bandPoints;

    // Iterate through all QSOs to calculate scores
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = m_qsoModel->getQSO(row);
        if (qso.band == BandType::None) continue;

        // Count per band
        bandQSOs[qso.band]++;
        bandPoints[qso.band] += qso.qsoPoints;
        totalQSOPoints += qso.qsoPoints;

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
        QSO qso = m_qsoModel->getQSO(row);
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

    // QSOs row
    bandTableHtml += "<tr><td class=\"row-label\">QSOs</td>";
    int bandQSOTotal = 0;
    for (BandType band : bands) {
        int count = bandQSOs.value(band, 0);
        bandQSOTotal += count;
        bandTableHtml += QString("<td>%1</td>").arg(count);
    }
    bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandQSOTotal);

    // Mults row
    bandTableHtml += "<tr><td class=\"row-label\">Mults</td>";
    int bandMultTotal = 0;
    for (BandType band : bands) {
        int count = bandMults.value(band, 0);
        bandMultTotal += count;
        bandTableHtml += QString("<td>%1</td>").arg(count);
    }
    bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandMultTotal);

    // Zones row
    bandTableHtml += "<tr><td class=\"row-label\">Zones</td>";
    int bandZoneTotal = 0;
    for (BandType band : bands) {
        int count = bandZones.value(band, 0);
        bandZoneTotal += count;
        bandTableHtml += QString("<td>%1</td>").arg(count);
    }
    bandTableHtml += QString("<td class=\"total-col\">%1</td></tr>").arg(bandZoneTotal);

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

} // namespace TR4QT
