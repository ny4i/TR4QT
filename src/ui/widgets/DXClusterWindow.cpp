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

#include "DXClusterWindow.h"
#include "../../core/Constants.h"
#include "../../logging/LogMacros.h"
#include "../../utils/DialogHelper.h"
#include "../../utils/AppSettings.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/FontManager.h"
#include "../../contests/ContestBase.h"
#include "../../data/Database.h"
#include "../../data/GlobalDatabase.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QFont>
#include <QSettings>
#include <QTimer>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextOption>
#include <QMouseEvent>
#include <QEvent>

namespace TR4QT {

// Default DX cluster servers (shown when no downloaded list or saved servers exist)
static const QStringList DEFAULT_CLUSTER_SERVERS = {
    "DXC.NC7J.COM:7373",
    "ve7cc.net:23",
    "dxc.ww2dx.com:7373"
};

static constexpr int DEFAULT_TELNET_PORT = 23;

// Alternating row contrast: subtle visual separation without being distracting
static constexpr int ALTERNATING_ROW_LIGHTNESS_SHIFT = 7;
static constexpr int LIGHTNESS_MIDPOINT = 128;  // 0=black, 255=white

DXClusterWindow::DXClusterWindow(QWidget* parent)
    : PersistentWindow<QWidget>("Windows/DXCluster", parent, "DXClusterWindow")
    , m_telnetThread(new TelnetThread(this))
    , m_telnetClient(nullptr)
    , m_isFrozen(false)
    , m_autoReconnect(false)
    , m_reconnectManager(new ReconnectionManager(RECONNECT_INTERVAL_MS, MAX_RECONNECT_ATTEMPTS, this))
    , m_spotRowCount(0)
    , m_spotWorkerThread(new QThread(this))
    , m_spotWorker(new SpotProcessorWorker())
{
    setupUI();
    loadSettings();

    // Setup spot processor worker thread
    m_spotWorker->moveToThread(m_spotWorkerThread);

    // Initialize worker's database connections when thread starts
    // Capture all paths and config by value on the main thread to avoid cross-thread access
    connect(m_spotWorkerThread, &QThread::started, this, [this]() {
        QString contestDbPath;
        QString globalDbPath;

        if (Database::instance().isOpen()) {
            contestDbPath = Database::instance().connection().databaseName();
        }
        if (GlobalDatabase::instance().isOpen()) {
            globalDbPath = GlobalDatabase::instance().connection().databaseName();
        }

        QString countryFilePath = AppSettings::instance().getCountryFilePath();
        SpotProcessorConfig config = buildConfig();

        // Post to worker thread — after initDatabase completes, signal back
        // to the main thread so we know the worker is ready for contest context.
        // Using QPointer to guard against DXClusterWindow being destroyed before
        // the callback fires.
        SpotProcessorWorker* worker = m_spotWorker;
        QPointer<DXClusterWindow> safeThis = this;
        QMetaObject::invokeMethod(worker, [worker, safeThis, contestDbPath, globalDbPath, countryFilePath, config]() {
            worker->initDatabase(contestDbPath, globalDbPath, countryFilePath);
            worker->setConfig(config);

            // Signal back to main thread that worker DB is ready
            QMetaObject::invokeMethod(safeThis, [safeThis]() {
                if (!safeThis) return;
                safeThis->onWorkerInitialized();
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    });

    // Connect worker's output to main thread display
    connect(m_spotWorker, &SpotProcessorWorker::spotProcessed,
            this, &DXClusterWindow::onSpotProcessed,
            Qt::QueuedConnection);

    // Clean up worker when thread finishes
    connect(m_spotWorkerThread, &QThread::finished, m_spotWorker, &QObject::deleteLater);

    m_spotWorkerThread->start();

    // Start telnet thread - use clientReady signal instead of msleep race
    connect(m_telnetThread, &TelnetThread::clientReady,
            this, &DXClusterWindow::onTelnetClientReady,
            Qt::QueuedConnection);

    m_telnetThread->start();

    // Setup reconnection manager (10 seconds, max 10 attempts)
    connect(m_reconnectManager, &ReconnectionManager::retryRequested, this, [this](int attempt) {
        if (m_autoReconnect) {
            appendText(QString("Reconnect attempt %1 of %2...")
                .arg(attempt).arg(MAX_RECONNECT_ATTEMPTS), Qt::darkYellow);
            LOG_DEBUG("DXClusterWindow", QString("Auto-reconnect: Attempt %1 of %2")
                .arg(attempt).arg(MAX_RECONNECT_ATTEMPTS));
            onConnectClicked();
        }
    });
    connect(m_reconnectManager, &ReconnectionManager::retriesExhausted, this, [this](int totalAttempts) {
        m_autoReconnect = false;
        appendText(QString("Failed to reconnect after %1 attempts. Please reconnect manually.")
            .arg(totalAttempts), Qt::red);
        LOG_WARN("DXClusterWindow", QString("Auto-reconnect failed after %1 attempts")
            .arg(totalAttempts));
    });

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &DXClusterWindow::applyTheme);
    applyTheme();  // Apply initial theme

    setWindowTitle("DX Cluster");

    // Auto-connect if enabled in settings
    if (AppSettings::instance().getDXClusterAutoConnect()) {
        QString server = AppSettings::instance().getDXClusterServer();
        if (!server.isEmpty()) {
            LOG_DEBUG("DXClusterWindow", QString("Auto-connect enabled, connecting to: %1").arg(server));
            // Use QTimer to delay connection slightly to ensure UI is fully initialized
            QTimer::singleShot(AUTO_CONNECT_DELAY_MS, this, [this, server]() {
                m_serverCombo->setCurrentText(server);
                onConnectClicked();
            });
        }
    }
}

DXClusterWindow::~DXClusterWindow() {
    saveSettings();

    // Stop spot worker thread
    if (m_spotWorkerThread->isRunning()) {
        m_spotWorkerThread->quit();
        m_spotWorkerThread->wait();
    }

    // Stop telnet thread
    if (m_telnetThread->isRunning()) {
        m_telnetThread->quit();
        m_telnetThread->wait();
    }
}

void DXClusterWindow::onTelnetClientReady(TelnetClient* client) {
    m_telnetClient = client;

    if (!m_telnetClient) {
        LOG_WARN("DXClusterWindow", "TelnetThread clientReady signal received but client is null");
        return;
    }

    // Now safe to connect signals (client is guaranteed to exist)
    connect(m_telnetClient, &TelnetClient::connected,
            this, &DXClusterWindow::onTelnetConnected,
            Qt::QueuedConnection);
    connect(m_telnetClient, &TelnetClient::disconnected,
            this, &DXClusterWindow::onTelnetDisconnected,
            Qt::QueuedConnection);
    connect(m_telnetClient, &TelnetClient::connectionError,
            this, &DXClusterWindow::onTelnetError,
            Qt::QueuedConnection);
    connect(m_telnetClient, &TelnetClient::dataReceived,
            this, &DXClusterWindow::onTelnetDataReceived,
            Qt::QueuedConnection);
    connect(m_telnetClient, &TelnetClient::spotReceived,
            this, &DXClusterWindow::onTelnetSpotReceived,
            Qt::QueuedConnection);

    LOG_DEBUG("DXClusterWindow", "Telnet client ready, signals connected");
}

void DXClusterWindow::setActiveContest(ContestBase* contest, int contestDbId) {
    if (m_spotWorker) {
        QList<MultiplierDefinition> multDefs;
        if (contest) {
            multDefs = contest->getMultiplierTypes();
        }

        // If worker hasn't finished init yet, defer until it does
        if (!m_workerInitialized) {
            m_pendingContestContext = ContestContext{contestDbId, multDefs};
            LOG_DEBUG("DXClusterWindow", "Contest context deferred until worker init completes");
            return;
        }

        // Capture worker pointer by value, not 'this'
        SpotProcessorWorker* worker = m_spotWorker;
        QMetaObject::invokeMethod(worker, [worker, contestDbId, multDefs]() {
            worker->setContestContext(contestDbId, multDefs);
        }, Qt::QueuedConnection);
    }
}

void DXClusterWindow::onWorkerInitialized() {
    m_workerInitialized = true;
    LOG_DEBUG("DXClusterWindow", "Worker initialized, DB connections ready");

    // Apply any contest context that arrived before init completed
    if (m_pendingContestContext) {
        auto ctx = *m_pendingContestContext;
        m_pendingContestContext.reset();
        SpotProcessorWorker* worker = m_spotWorker;
        QMetaObject::invokeMethod(worker, [worker, ctx]() {
            worker->setContestContext(ctx.contestDbId, ctx.multDefs);
        }, Qt::QueuedConnection);
    }
}

void DXClusterWindow::updateSpotProcessorConfig() {
    if (m_spotWorker) {
        SpotProcessorConfig config = buildConfig();

        // Cache row colors locally for appendRichText
        m_evenRowBackground = config.evenRowBackground;
        m_oddRowBackground = config.oddRowBackground;

        // Pass config to worker via queued connection
        SpotProcessorWorker* worker = m_spotWorker;
        QMetaObject::invokeMethod(worker, [worker, config]() {
            worker->setConfig(config);
        }, Qt::QueuedConnection);
    }
}

SpotProcessorConfig DXClusterWindow::buildConfig() {
    SpotProcessorConfig config;
    AppSettings& settings = AppSettings::instance();
    ThemeManager& theme = ThemeManager::instance();

    // Dupe/multiplier colors from user settings
    config.dupeColor = QColor(settings.getClusterDupeColor());
    config.multiplierColor = QColor(settings.getClusterMultiplierColor());
    config.lotwLookupEnabled = settings.getEnableLotwLookup();

    // Use ThemeManager colors where applicable
    config.defaultCallColor = theme.color(ColorRole::PrimaryText);
    config.spotterColor = theme.color(ColorRole::SecondaryText);
    config.commentColor = theme.color(ColorRole::SecondaryText);

    // Derive row backgrounds from theme
    QColor windowBg = theme.color(ColorRole::TextDisplayBackground);
    config.evenRowBackground = windowBg;
    // Slightly darker/lighter for alternating rows
    int lightnessShift = (windowBg.lightness() > LIGHTNESS_MIDPOINT)
        ? -ALTERNATING_ROW_LIGHTNESS_SHIFT
        :  ALTERNATING_ROW_LIGHTNESS_SHIFT;
    config.oddRowBackground = QColor(
        qBound(0, windowBg.red() + lightnessShift, 255),
        qBound(0, windowBg.green() + lightnessShift, 255),
        qBound(0, windowBg.blue() + lightnessShift, 255)
    );

    return config;
}


void DXClusterWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();

    m_connectButton = new QPushButton("Connect", this);
    m_disconnectButton = new QPushButton("Disconnect", this);
    m_freezeButton = new QPushButton("Freeze", this);
    m_clearButton = new QPushButton("Clear", this);
    m_commandsButton = new QPushButton("Commands", this);

    m_freezeButton->setCheckable(true);

    toolbarLayout->addWidget(m_connectButton);
    toolbarLayout->addWidget(m_disconnectButton);
    toolbarLayout->addWidget(m_freezeButton);
    toolbarLayout->addWidget(m_clearButton);
    toolbarLayout->addWidget(m_commandsButton);
    toolbarLayout->addStretch();

    mainLayout->addLayout(toolbarLayout);

    // Server selection and send button
    QHBoxLayout* serverLayout = new QHBoxLayout();

    m_serverCombo = new QComboBox(this);
    m_serverCombo->setEditable(true);
    m_serverCombo->setMinimumWidth(300);
    m_serverCombo->addItems(DEFAULT_CLUSTER_SERVERS);

    m_sendButton = new QPushButton("Send", this);

    serverLayout->addWidget(m_serverCombo);
    serverLayout->addStretch();
    serverLayout->addWidget(m_sendButton);

    mainLayout->addLayout(serverLayout);

    // Status label
    m_statusLabel = new QLabel("DISCONNECTED", this);
    // Initial style will be set by applyTheme()
    mainLayout->addWidget(m_statusLabel);

    // Text display
    m_textDisplay = new QTextEdit(this);
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setAcceptRichText(true);  // Enable rich text for modern styling

    // Use fixed-width font for proper alignment
    m_textDisplay->setFont(FontManager::instance().courierFont(10));

    // Disable word wrap for proper column alignment
    m_textDisplay->setLineWrapMode(QTextEdit::NoWrap);
    m_textDisplay->setWordWrapMode(QTextOption::NoWrap);

    // Background style will be set by applyTheme()
    m_textDisplay->viewport()->setCursor(Qt::PointingHandCursor);  // Show it's clickable

    // Install event filter to handle mouse clicks
    m_textDisplay->viewport()->installEventFilter(this);

    mainLayout->addWidget(m_textDisplay);

    // Command input
    QHBoxLayout* commandLayout = new QHBoxLayout();
    m_commandEdit = new QLineEdit(this);
    m_commandEdit->setPlaceholderText("Enter command...");
    commandLayout->addWidget(m_commandEdit);

    mainLayout->addLayout(commandLayout);

    // Connect button signals
    connect(m_connectButton, &QPushButton::clicked,
            this, &DXClusterWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked,
            this, &DXClusterWindow::onDisconnectClicked);
    connect(m_freezeButton, &QPushButton::clicked,
            this, &DXClusterWindow::onFreezeClicked);
    connect(m_clearButton, &QPushButton::clicked,
            this, &DXClusterWindow::onClearClicked);
    connect(m_commandsButton, &QPushButton::clicked,
            this, &DXClusterWindow::onCommandsClicked);
    connect(m_sendButton, &QPushButton::clicked,
            this, &DXClusterWindow::onSendClicked);

    // Enter key in command edit sends command
    connect(m_commandEdit, &QLineEdit::returnPressed,
            this, &DXClusterWindow::onSendClicked);

    // Initial button states
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);

    setMinimumSize(600, 400);
}

void DXClusterWindow::loadSettings() {
    QSettings settings(APP_ORG, APP_NAME);  // Must match AppSettings initialization

    // Load downloaded cluster list from AppSettings
    AppSettings& appSettings = AppSettings::instance();
    QStringList clusterList = appSettings.getDXClusterList();

    if (!clusterList.isEmpty()) {
        // Clear default servers and use downloaded list
        m_serverCombo->clear();
        for (const QString& server : clusterList) {
            m_serverCombo->addItem(server);
        }
    } else {
        // Load recent servers (fallback if no downloaded list)
        int serverCount = settings.value("DXCluster/ServerCount", 0).toInt();
        for (int i = 0; i < serverCount; ++i) {
            QString server = settings.value(QString("DXCluster/Server%1").arg(i)).toString();
            if (!server.isEmpty() && m_serverCombo->findText(server) == -1) {
                m_serverCombo->addItem(server);
            }
        }
    }

    // Load default server from preferences (always use on startup)
    QString defaultServer = appSettings.getDXClusterServer();
    if (!defaultServer.isEmpty()) {
        // Try to find exact match first
        int index = m_serverCombo->findText(defaultServer, Qt::MatchExactly);
        if (index >= 0) {
            m_serverCombo->setCurrentIndex(index);
        } else {
            // Try partial match (for display format vs plain format)
            index = m_serverCombo->findText(defaultServer, Qt::MatchContains);
            if (index >= 0) {
                m_serverCombo->setCurrentIndex(index);
            } else {
                // Not in list, set as custom text
                m_serverCombo->setCurrentText(defaultServer);
            }
        }
    }
}

void DXClusterWindow::saveSettings() {
    QSettings settings(APP_ORG, APP_NAME);  // Must match AppSettings initialization

    // Save current server
    settings.setValue("DXCluster/LastServer", m_serverCombo->currentText());

    // Save recent servers
    QStringList servers;
    for (int i = 0; i < m_serverCombo->count(); ++i) {
        servers.append(m_serverCombo->itemText(i));
    }
    settings.setValue("DXCluster/ServerCount", servers.size());
    for (int i = 0; i < servers.size(); ++i) {
        settings.setValue(QString("DXCluster/Server%1").arg(i), servers[i]);
    }
}

void DXClusterWindow::onConnectClicked() {
    if (!m_telnetClient) {
        LOG_WARN("DXClusterWindow", "Cannot connect: telnet client not yet ready");
        appendText("Waiting for telnet client to initialize...", Qt::darkYellow);
        return;
    }

    // Enable auto-reconnect when user initiates connection
    m_autoReconnect = true;
    m_reconnectManager->reset();  // Stop any pending reconnect and reset counter

    QString serverString = m_serverCombo->currentText().trimmed();
    if (serverString.isEmpty()) {
        DialogHelper::warning(this, "DX Cluster",
                           "Please enter a server address (host:port)");
        return;
    }

    // Extract host:port from display format: "W9ODD (134.48.91.82:23) - AR-Cluster"
    // Or handle plain format: "dxc.nc7j.com:7373"
    QString connectionString = serverString;

    QRegularExpression displayRegex(R"(\(([^:]+):(\d+)\))");
    QRegularExpressionMatch match = displayRegex.match(serverString);

    if (match.hasMatch()) {
        // Extracted from display format
        connectionString = match.captured(1) + ":" + match.captured(2);
    }

    // Parse server:port (default to port 23 if not specified)
    QStringList parts = connectionString.split(':');
    QString host;
    int port = DEFAULT_TELNET_PORT;

    if (parts.size() == 1) {
        // No port specified, use default
        host = parts[0].trimmed();
    } else if (parts.size() == 2) {
        // Port specified
        host = parts[0].trimmed();
        bool ok;
        port = parts[1].trimmed().toInt(&ok);

        if (!ok || port <= 0 || port > 65535) {
            DialogHelper::warning(this, "DX Cluster",
                               "Invalid port number");
            return;
        }
    } else {
        DialogHelper::warning(this, "DX Cluster",
                           QString("Invalid format. Use: hostname:port or just hostname (e.g., %1)")
                           .arg(DEFAULT_CLUSTER_SERVERS.first()));
        return;
    }

    if (host.isEmpty()) {
        DialogHelper::warning(this, "DX Cluster",
                           "Please enter a valid hostname or IP address");
        return;
    }

    // Set auto-login callsign from settings before connecting
    QString callsign = AppSettings::instance().getDXClusterCallsign();
    QMetaObject::invokeMethod(m_telnetClient, "setAutoLoginCallsign",
                             Qt::QueuedConnection,
                             Q_ARG(QString, callsign));

    // Connect using cross-thread signal
    QMetaObject::invokeMethod(m_telnetClient, "connectToServer",
                             Qt::QueuedConnection,
                             Q_ARG(QString, host),
                             Q_ARG(int, port));

    appendText(QString("Connecting to %1:%2...").arg(host).arg(port), Qt::blue);
}

void DXClusterWindow::onDisconnectClicked() {
    if (!m_telnetClient) {
        return;
    }

    // Disable auto-reconnect when user manually disconnects
    m_autoReconnect = false;
    m_reconnectManager->reset();

    QMetaObject::invokeMethod(m_telnetClient, "disconnectFromServer",
                             Qt::QueuedConnection);
}

void DXClusterWindow::onFreezeClicked() {
    m_isFrozen = m_freezeButton->isChecked();

    // Update button appearance to show frozen state
    ThemeManager& theme = ThemeManager::instance();

    if (m_isFrozen) {
        m_freezeButton->setText("FROZEN");
        m_freezeButton->setStyleSheet(QString("QPushButton { background-color: %1; font-weight: bold; }")
            .arg(theme.color(ColorRole::FrozenIndicator).name()));
    } else {
        m_freezeButton->setText("Freeze");
        m_freezeButton->setStyleSheet("");
    }
}

void DXClusterWindow::onClearClicked() {
    m_textDisplay->clear();
    m_spotRowCount = 0;  // Reset alternating row counter
    m_splitSpots.clear();  // Clear split spot cache
}

void DXClusterWindow::onCommandsClicked() {
    // Show common commands help
    QString help =
        "Common DX Cluster Commands:\n\n"
        "SH/DX         - Show recent spots\n"
        "SH/DX 20      - Show spots on 20m\n"
        "SH/DX/15      - Show last 15 spots\n"
        "SET/DX        - Enable spot announcements\n"
        "UNSET/DX      - Disable spot announcements\n"
        "SET/SKIMMER   - Enable skimmer spots\n"
        "UNSET/SKIMMER - Disable skimmer spots\n"
        "SH/SUN        - Show sunrise/sunset\n"
        "BYE           - Disconnect\n";

    DialogHelper::information(this, "DX Cluster Commands", help);
}

void DXClusterWindow::onSendClicked() {
    if (!m_telnetClient) {
        return;
    }

    QString command = m_commandEdit->text().trimmed();
    if (command.isEmpty()) {
        return;
    }

    // Send command using cross-thread signal
    QMetaObject::invokeMethod(m_telnetClient, "sendCommand",
                             Qt::QueuedConnection,
                             Q_ARG(QString, command));

    // Echo command locally
    appendText(QString("> %1").arg(command), Qt::darkGray);

    // Clear input
    m_commandEdit->clear();
}

void DXClusterWindow::onTelnetConnected() {
    m_isConnected = true;
    updateConnectionStatus(true);
    appendText("Connected!", Qt::darkGreen);

    // Stop reconnect timer on successful connection
    m_reconnectManager->recordSuccess();
}

void DXClusterWindow::onTelnetDisconnected() {
    m_isConnected = false;
    updateConnectionStatus(false);
    appendText("Disconnected.", Qt::darkRed);

    // Start auto-reconnect timer if enabled
    if (m_autoReconnect) {
        int reconnectSec = RECONNECT_INTERVAL_MS / 1000;
        LOG_DEBUG("DXClusterWindow", QString("Disconnected - will attempt reconnect in %1 seconds").arg(reconnectSec));
        appendText(QString("Will attempt to reconnect in %1 seconds...").arg(reconnectSec), Qt::darkYellow);
        m_reconnectManager->start();
    }
}

void DXClusterWindow::onTelnetError(const QString& error) {
    appendText(QString("Error: %1").arg(error), Qt::red);
    updateConnectionStatus(false);
}

void DXClusterWindow::onTelnetDataReceived(const QString& data) {
    // Don't update display if frozen
    if (m_isFrozen) {
        return;
    }

    // Skip DX spot lines - they'll be formatted by onTelnetSpotReceived()
    if (data.startsWith("DX de ", Qt::CaseInsensitive)) {
        return;
    }

    // Append data to display
    appendText(data);

    // Auto-scroll to bottom
    QTextCursor cursor = m_textDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textDisplay->setTextCursor(cursor);
}

void DXClusterWindow::onTelnetSpotReceived(const QString& callsign,
                                          double frequency,
                                          const QString& spotter,
                                          const QString& comment,
                                          const QString& timestamp) {
    // Forward to worker thread for processing (dupe/mult/LOTW checks, formatting)
    // Worker emits spotProcessed() which we handle in onSpotProcessed()
    QMetaObject::invokeMethod(m_spotWorker, "processSpot",
                             Qt::QueuedConnection,
                             Q_ARG(QString, callsign),
                             Q_ARG(double, frequency),
                             Q_ARG(QString, spotter),
                             Q_ARG(QString, comment),
                             Q_ARG(QString, timestamp));
}

void DXClusterWindow::onSpotProcessed(const ProcessedSpot& result) {
    // Don't update display if frozen (but still forward spot to band map)
    if (!m_isFrozen) {
        // Store split info for click-to-QSY handling
        if (result.isSplit) {
            SplitSpotInfo info;
            info.spotFrequency = result.spotFrequency;
            info.listenFrequency = result.listenFrequency;
            info.callsign = result.callsign;
            m_splitSpots[result.displayText.trimmed()] = info;

            // Prune if cache is getting too large
            if (m_splitSpots.size() > MAX_SPLIT_SPOTS_CACHED) {
                pruneSplitSpots();
            }
        }

        appendRichText(result.displayText, result.formats, result.isSplit);

        // Auto-scroll to bottom
        QTextCursor cursor = m_textDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_textDisplay->setTextCursor(cursor);
    }

    // Always forward processed spot to band map
    emit spotProcessed(result);
}

void DXClusterWindow::pruneSplitSpots() {
    // Remove first half of entries by key order to stay under MAX_SPLIT_SPOTS_CACHED
    int toRemove = m_splitSpots.size() / 2;
    auto it = m_splitSpots.begin();
    for (int i = 0; i < toRemove && it != m_splitSpots.end(); ++i) {
        it = m_splitSpots.erase(it);
    }
    LOG_DEBUG("DXClusterWindow", QString("Pruned split spots cache, %1 entries remaining")
        .arg(m_splitSpots.size()));
}

void DXClusterWindow::updateConnectionStatus(bool connected) {
    ThemeManager& theme = ThemeManager::instance();

    if (connected) {
        m_statusLabel->setText("CONNECTED");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
            .arg(theme.color(ColorRole::ConnectedStatus).name()));
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(true);
    } else {
        m_statusLabel->setText("DISCONNECTED");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
            .arg(theme.color(ColorRole::DisconnectedStatus).name()));
        m_connectButton->setEnabled(true);
        m_disconnectButton->setEnabled(false);
    }
}

void DXClusterWindow::appendText(const QString& text, const QColor& color) {
    QTextCursor cursor = m_textDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(color);

    cursor.setCharFormat(format);
    cursor.insertText(text + "\n");
}

void DXClusterWindow::appendRichText(const QString& text, const QList<SpotFormatRange>& formats, bool isSplit) {
    QTextCursor cursor = m_textDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    // Alternating row backgrounds (theme-aware)
    bool isEvenRow = (m_spotRowCount % 2 == 0);
    QColor bgColor = isEvenRow ? m_evenRowBackground : m_oddRowBackground;

    QTextBlockFormat blockFormat;
    blockFormat.setBackground(bgColor);
    cursor.setBlockFormat(blockFormat);

    // Insert the plain text first (preserves exact character spacing)
    int textStart = cursor.position();
    cursor.insertText(text);

    // Now apply character formatting to specific ranges
    for (const SpotFormatRange& range : formats) {
        QTextCharFormat format;
        format.setForeground(range.color);
        if (range.bold) {
            format.setFontWeight(QFont::Bold);
        }

        // Select the range and apply format
        cursor.setPosition(textStart + range.start);
        cursor.setPosition(textStart + range.start + range.length, QTextCursor::KeepAnchor);
        cursor.setCharFormat(format);
    }

    // Move to end and insert newline
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n");

    m_spotRowCount++;
}

DXClusterWindow::ParsedSpotLine DXClusterWindow::parseSpotLine(const QString& line) {
    ParsedSpotLine result;
    if (line.isEmpty()) return result;

    result.isSplit = line.startsWith("[SPLIT]");
    QString spotLine = result.isSplit ? line.mid(8).trimmed() : line;

    static const QRegularExpression freqRegex(R"(\s+(\d+\.\d+)\s+)");
    QRegularExpressionMatch freqMatch = freqRegex.match(spotLine);

    if (freqMatch.hasMatch()) {
        result.valid = true;
        result.freqKHz = freqMatch.captured(1).toDouble();

        int freqEnd = freqMatch.capturedEnd();
        QString remainder = spotLine.mid(freqEnd).trimmed();
        QStringList parts = remainder.split(QRegularExpression("\\s+"));
        result.callsign = parts.isEmpty() ? QString() : parts[0];
    }

    return result;
}

bool DXClusterWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_textDisplay->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            // Single click - show spot info in status label
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QTextCursor cursor = m_textDisplay->cursorForPosition(mouseEvent->pos());
                cursor.select(QTextCursor::LineUnderCursor);
                QString line = cursor.selectedText().trimmed();

                ParsedSpotLine parsed = parseSpotLine(line);
                if (parsed.valid) {
                    QString freqStr = QString::number(parsed.freqKHz, 'f', 1);
                    QString statusMsg;

                    if (parsed.isSplit && m_splitSpots.contains(line)) {
                        SplitSpotInfo info = m_splitSpots[line];
                        double txKHz = info.spotFrequency / 1000.0;
                        double rxKHz = info.listenFrequency / 1000.0;
                        statusMsg = QString("SPLIT - %1 - TX: %2 kHz, RX: %3 kHz - Double-click to QSY")
                            .arg(parsed.callsign)
                            .arg(QString::number(txKHz, 'f', 1))
                            .arg(QString::number(rxKHz, 'f', 1));
                    } else if (parsed.isSplit) {
                        statusMsg = QString("SPLIT - %1 @ %2 kHz - Double-click to QSY")
                            .arg(parsed.callsign, freqStr);
                    } else {
                        statusMsg = QString("%1 @ %2 kHz - Double-click to QSY")
                            .arg(parsed.callsign, freqStr);
                    }

                    m_statusLabel->setText(statusMsg);
                    LOG_DEBUG("DXClusterWindow", QString("Single-click: %1").arg(statusMsg));
                }
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            // Double click - QSY to spot
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QTextCursor cursor = m_textDisplay->cursorForPosition(mouseEvent->pos());
                cursor.select(QTextCursor::LineUnderCursor);
                QString line = cursor.selectedText().trimmed();

                // Split spots with cached info get split QSY
                if (line.startsWith("[SPLIT]") && m_splitSpots.contains(line)) {
                    SplitSpotInfo info = m_splitSpots[line];
                    LOG_DEBUG("DXClusterWindow", QString("Double-click split QSY: TX=%1 Hz, RX=%2 Hz")
                        .arg(QString::number(info.spotFrequency, 'f', 0))
                        .arg(QString::number(info.listenFrequency, 'f', 0)));
                    emit splitQsyRequested(info.spotFrequency, info.listenFrequency);
                } else {
                    // Simplex QSY - parse frequency from line
                    ParsedSpotLine parsed = parseSpotLine(line);
                    if (parsed.valid) {
                        double freqHz = parsed.freqKHz * 1000.0;
                        LOG_DEBUG("DXClusterWindow", QString("Double-click simplex QSY: %1 Hz")
                            .arg(QString::number(freqHz, 'f', 0)));
                        emit qsyRequested(freqHz);
                    }
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void DXClusterWindow::applyTheme() {
    ThemeManager& theme = ThemeManager::instance();

    // Update text display background
    m_textDisplay->setStyleSheet(QString("QTextEdit { background-color: %1; }")
        .arg(theme.color(ColorRole::TextDisplayBackground).name()));

    // Update status label (maintain current connection state)
    updateConnectionStatus(m_isConnected);

    // Update freeze button if frozen
    if (m_isFrozen) {
        m_freezeButton->setStyleSheet(QString("QPushButton { background-color: %1; font-weight: bold; }")
            .arg(theme.color(ColorRole::FrozenIndicator).name()));
    }

    // Push updated colors to spot processor worker
    updateSpotProcessorConfig();
}

} // namespace TR4QT
