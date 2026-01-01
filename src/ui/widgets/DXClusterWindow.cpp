#include "DXClusterWindow.h"
#include "../../core/Constants.h"
#include "../../logging/LogMacros.h"
#include "../../utils/DialogHelper.h"
#include "../../utils/AppSettings.h"
#include "../../utils/ThemeManager.h"
#include "../../data/LOTWUserRepository.h"
#include "../../contests/ContestBase.h"
#include "../../data/QSORepository.h"
#include "../../utils/CountryFile.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QFont>
#include <QSettings>
#include <QTimer>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextOption>
#include <QMouseEvent>
#include <QEvent>

namespace TR4QT {

// DX Cluster band colors
// Consistent color definitions for all bands (avoids magic RGB values)
static const QColor COLOR_BAND_160M(102, 51, 153);  // Purple
static const QColor COLOR_BAND_80M(153, 76, 0);     // Brown
static const QColor COLOR_BAND_40M(204, 0, 102);    // Magenta
static const QColor COLOR_BAND_20M(0, 102, 204);    // Blue
static const QColor COLOR_BAND_15M(0, 153, 0);      // Green
static const QColor COLOR_BAND_10M(204, 102, 0);    // Orange
static const QColor COLOR_BAND_DEFAULT(102, 102, 102);  // Gray for unhandled bands

/**
 * Map band to display color for DX cluster spots
 * Uses distinct colors to visually differentiate bands
 */
static QColor getBandColor(BandType band) {
    switch (band) {
        case BandType::Band160M: return COLOR_BAND_160M;
        case BandType::Band80M:  return COLOR_BAND_80M;
        case BandType::Band40M:  return COLOR_BAND_40M;
        case BandType::Band20M:  return COLOR_BAND_20M;
        case BandType::Band15M:  return COLOR_BAND_15M;
        case BandType::Band10M:  return COLOR_BAND_10M;
        default:                 return COLOR_BAND_DEFAULT;
    }
}

DXClusterWindow::DXClusterWindow(QWidget* parent)
    : QWidget(parent)
    , m_telnetThread(new TelnetThread(this))
    , m_telnetClient(nullptr)
    , m_isFrozen(false)
    , m_autoReconnect(false)
    , m_reconnectTimer(new QTimer(this))
    , m_reconnectAttempts(0)
    , m_spotRowCount(0)
    , m_activeContest(nullptr)
    , m_contestDbId(-1)
    , m_countryFile(nullptr)
{
    setupUI();
    loadSettings();

    // Start telnet thread
    m_telnetThread->start();

    // Wait for thread to create client
    QThread::msleep(100);
    m_telnetClient = m_telnetThread->client();

    // Connect signals from telnet client (cross-thread)
    if (m_telnetClient) {
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
    }

    // Setup reconnection timer (10 seconds between attempts)
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(10000);  // 10 seconds
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_autoReconnect && m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            m_reconnectAttempts++;
            appendText(QString("Reconnect attempt %1 of %2...")
                .arg(m_reconnectAttempts).arg(MAX_RECONNECT_ATTEMPTS), Qt::darkYellow);
            LOG_DEBUG("DXClusterWindow", QString("Auto-reconnect: Attempt %1 of %2")
                .arg(m_reconnectAttempts).arg(MAX_RECONNECT_ATTEMPTS));
            onConnectClicked();
        } else if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            m_autoReconnect = false;
            appendText(QString("Failed to reconnect after %1 attempts. Please reconnect manually.")
                .arg(MAX_RECONNECT_ATTEMPTS), Qt::red);
            LOG_WARN("DXClusterWindow", QString("Auto-reconnect failed after %1 attempts")
                .arg(MAX_RECONNECT_ATTEMPTS));
        }
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
            QTimer::singleShot(500, this, [this, server]() {
                m_serverCombo->setCurrentText(server);
                onConnectClicked();
            });
        }
    }
}

DXClusterWindow::~DXClusterWindow() {
    saveSettings();

    // Stop telnet thread
    if (m_telnetThread->isRunning()) {
        m_telnetThread->quit();
        m_telnetThread->wait();
    }
}

void DXClusterWindow::setActiveContest(ContestBase* contest, int contestDbId) {
    m_activeContest = contest;
    m_contestDbId = contestDbId;
}

void DXClusterWindow::setCountryFile(CountryFile* countryFile) {
    m_countryFile = countryFile;
}

QColor DXClusterWindow::getSpotColor(const QString& callsign, double frequency) const {
    // No contest active - return default color
    if (!m_activeContest || m_contestDbId < 0) {
        return Qt::black;
    }

    // Determine band and mode from frequency
    BandType band = frequencyToBand(static_cast<unsigned long>(frequency));
    ModeType mode = (frequency >= 1800000 && frequency < 10000000) ? ModeType::CW : ModeType::USB; // Simplified mode detection

    // Check if it's a dupe
    QSORepository repo;
    bool isDupe = repo.isDuplicate(callsign, band, mode, m_contestDbId);

    if (isDupe) {
        // Return dupe color from settings
        QString dupeColorStr = AppSettings::instance().getClusterDupeColor();
        return QColor(dupeColorStr);
    }

    // Check if it's a new multiplier
    // Create a temporary QSO for mult checking
    QSO tempQso;
    tempQso.callsign = callsign;
    tempQso.band = band;
    tempQso.mode = mode;
    tempQso.frequency = frequency;

    // Populate country/zone data from CountryFile (if available)
    if (m_countryFile) {
        CountryData countryData = m_countryFile->lookup(callsign);
        if (countryData.isValid()) {
            tempQso.dxccPrefix = countryData.primaryPrefix;
            tempQso.dxccEntity = countryData.name;
            tempQso.continent = continentToString(countryData.continent);
            tempQso.cqZone = countryData.cqZone;
            tempQso.ituZone = countryData.ituZone;
        }
    }

    // Get multiplier types for this contest
    QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();

    for (const MultiplierDefinition& multDef : multDefs) {
        // Determine band parameter based on multiplier scope
        QString bandParam = (multDef.scope == MultiplierScope::PerBand)
                            ? bandToString(band)
                            : QString();

        // Get worked multipliers for this type
        QStringList workedMults = repo.getWorkedMultipliers(multDef.type, bandParam, m_contestDbId);

        // Check if this spot is a new mult
        QString multValue = m_activeContest->getMultiplierValue(tempQso, multDef.type, workedMults);
        if (!multValue.isEmpty()) {
            // It's a new multiplier! Return multiplier color
            QString multColorStr = AppSettings::instance().getClusterMultiplierColor();
            return QColor(multColorStr);
        }
    }

    // Not a dupe, not a multiplier - return normal color
    return Qt::black;
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
    m_serverCombo->addItem("DXC.NC7J.COM:7373");
    m_serverCombo->addItem("ve7cc.net:23");
    m_serverCombo->addItem("dxc.ww2dx.com:7373");

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
    QFont monoFont("Courier", 10);
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    m_textDisplay->setFont(monoFont);

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
    QSettings settings(APP_ORG, APP_NAME);

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
    QSettings settings(APP_ORG, APP_NAME);

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
        return;
    }

    // Enable auto-reconnect when user initiates connection
    m_autoReconnect = true;
    m_reconnectTimer->stop();  // Stop any pending reconnect attempt
    m_reconnectAttempts = 0;   // Reset retry counter

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
    int port = 23;  // Default telnet port

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
                           "Invalid format. Use: hostname:port or just hostname (e.g., DXC.NC7J.COM:7373 or DXC.NC7J.COM)");
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
    m_reconnectTimer->stop();

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
    updateConnectionStatus(true);
    appendText("Connected!", Qt::darkGreen);

    // Stop reconnect timer on successful connection
    m_reconnectTimer->stop();
    m_reconnectAttempts = 0;  // Reset retry counter on success
}

void DXClusterWindow::onTelnetDisconnected() {
    updateConnectionStatus(false);
    appendText("Disconnected.", Qt::darkRed);

    // Start auto-reconnect timer if enabled
    if (m_autoReconnect) {
        LOG_DEBUG("DXClusterWindow", "Disconnected - will attempt reconnect in 10 seconds");
        appendText("Will attempt to reconnect in 10 seconds...", Qt::darkYellow);
        m_reconnectTimer->start();
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
    // Don't update display if frozen
    if (m_isFrozen) {
        // Still forward to band map even if display is frozen
        emit spotReceived(callsign, frequency, spotter, comment);
        return;
    }

    // Check for split operation (QSX/UP/DOWN in comment)
    double listenFrequency = parseSplitInfo(comment, frequency);
    bool isSplit = (listenFrequency > 0);

    // Format spot into fixed-width columns for better readability
    // Format: [SPLIT] Spotter(12) Freq(10) Callsign(12) Time(5) Comment(remaining)

    // Convert frequency from Hz to kHz for display
    double freqKHz = frequency / 1000.0;
    QString freqStr = QString::number(freqKHz, 'f', 1);  // 1 decimal place

    // Spot formatting constants
    const int SPLIT_INDICATOR_WIDTH = 2;  // Width of split indicator ("● " or "  ")
    const int CALLSIGN_INDENT = 3;         // Spaces between frequency and callsign

    // Split indicator (consistent 2-character width for both display and formatting)
    QString splitIcon = isSplit ? "● " : "  ";

    // Build plain text version for split spot lookup and display
    QString plainSpot = QString("%1%2 %3%4%5 %6Z %7")
        .arg(splitIcon)
        .arg(spotter, -12)
        .arg(freqStr, 10)
        .arg(QString(CALLSIGN_INDENT, ' '))
        .arg(callsign, -12)
        .arg(timestamp, 4)
        .arg(comment);

    // Store split info for click-to-QSY handling
    if (isSplit) {
        SplitSpotInfo info;
        info.spotFrequency = frequency;
        info.listenFrequency = listenFrequency;
        info.callsign = callsign;
        m_splitSpots[plainSpot.trimmed()] = info;
    }

    // Determine frequency color based on band (using central band determination logic)
    BandType band = frequencyToBand(static_cast<unsigned long>(frequency));
    QColor freqColor = getBandColor(band);

    // Build format info for character-based formatting (preserves alignment)
    QList<FormatRange> formats;

    // Calculate character positions in the line
    // Format: [● ] Spotter(12) Freq(10) [3 spaces] Callsign(12) TimeZ(5) Comment
    int pos = 0;

    // Split indicator (already defined above as splitIcon)
    if (isSplit) {
        formats.append({pos, SPLIT_INDICATOR_WIDTH, QColor(0, 206, 209), false});  // Cyan dot
    }
    pos += SPLIT_INDICATOR_WIDTH;

    // Spotter (12 chars) - gray
    formats.append({pos, 12, QColor(102, 102, 102), false});
    pos += 12;

    // Space
    pos += 1;

    // Frequency (10 chars, right-aligned) - color-coded by band
    const int FREQUENCY_FIELD_WIDTH = 10;
    int freqPadding = FREQUENCY_FIELD_WIDTH - freqStr.length();
    int freqStart = pos + freqPadding;  // Adjust for right-alignment
    formats.append({freqStart, static_cast<int>(freqStr.length()), freqColor, false});
    pos += FREQUENCY_FIELD_WIDTH;

    // Indent (3 spaces)
    pos += CALLSIGN_INDENT;

    // Callsign (12 chars) - bold, color based on dupe/multiplier status
    int callsignPos = pos;
    int callsignLen = callsign.length();
    QColor callsignColor = getSpotColor(callsign, frequency);
    formats.append({callsignPos, callsignLen, callsignColor, true});
    pos += 12;

    // Space
    pos += 1;

    // Timestamp (4 chars + Z) - light gray
    formats.append({pos, 5, QColor(153, 153, 153), false});
    pos += 5;

    // Space
    pos += 1;

    // Comment - dark gray
    formats.append({pos, static_cast<int>(comment.length()), QColor(51, 51, 51), false});

    // Use the plain text version we already built
    appendRichText(plainSpot, formats, isSplit);

    // Auto-scroll to bottom
    QTextCursor cursor = m_textDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textDisplay->setTextCursor(cursor);

    // Forward spot to band map (signal will be connected by MainWindow)
    emit spotReceived(callsign, frequency, spotter, comment);
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

void DXClusterWindow::appendRichText(const QString& text, const QList<FormatRange>& formats, bool isSplit) {
    QTextCursor cursor = m_textDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    // Alternating row backgrounds (very subtle)
    // Note: QTextBlockFormat applies to entire line including leading/trailing whitespace
    bool isEvenRow = (m_spotRowCount % 2 == 0);
    QColor bgColor = isEvenRow ? QColor(255, 255, 255) : QColor(248, 248, 248);

    QTextBlockFormat blockFormat;
    blockFormat.setBackground(bgColor);
    cursor.setBlockFormat(blockFormat);

    // Insert the plain text first (preserves exact character spacing)
    int textStart = cursor.position();
    cursor.insertText(text);

    // Now apply character formatting to specific ranges
    for (const FormatRange& range : formats) {
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

bool DXClusterWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_textDisplay->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            // Single click - show spot info in status label
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // Get the line under cursor
                QTextCursor cursor = m_textDisplay->cursorForPosition(mouseEvent->pos());
                cursor.select(QTextCursor::LineUnderCursor);
                QString line = cursor.selectedText().trimmed();

                if (!line.isEmpty()) {
                    // Parse formatted spot line:
                    // Format: [SPLIT] Spotter(12) Freq(10) Callsign(12) Time(5) Comment

                    // Check if this is a split spot
                    bool isSplit = line.startsWith("[SPLIT]");
                    QString spotLine = isSplit ? line.mid(8).trimmed() : line;

                    // Extract frequency (after spotter, before callsign)
                    QRegularExpression freqRegex(R"(\s+(\d+\.\d+)\s+)");
                    QRegularExpressionMatch freqMatch = freqRegex.match(spotLine);

                    if (freqMatch.hasMatch()) {
                        QString freqStr = freqMatch.captured(1);
                        double freqKHz = freqStr.toDouble();

                        // Extract callsign (after frequency)
                        int freqEnd = freqMatch.capturedEnd();
                        QString remainder = spotLine.mid(freqEnd).trimmed();
                        QStringList parts = remainder.split(QRegularExpression("\\s+"));
                        QString callsign = parts.isEmpty() ? "" : parts[0];

                        // Build status message
                        QString statusMsg;
                        if (isSplit) {
                            // Look up split info from m_splitSpots
                            if (m_splitSpots.contains(line)) {
                                SplitSpotInfo info = m_splitSpots[line];
                                double txKHz = info.spotFrequency / 1000.0;
                                double rxKHz = info.listenFrequency / 1000.0;
                                statusMsg = QString("SPLIT - %1 - TX: %2 kHz, RX: %3 kHz - Double-click to QSY")
                                    .arg(callsign)
                                    .arg(QString::number(txKHz, 'f', 1))
                                    .arg(QString::number(rxKHz, 'f', 1));
                            } else {
                                statusMsg = QString("SPLIT - %1 @ %2 kHz - Double-click to QSY")
                                    .arg(callsign)
                                    .arg(freqStr);
                            }
                        } else {
                            statusMsg = QString("%1 @ %2 kHz - Double-click to QSY")
                                .arg(callsign)
                                .arg(freqStr);
                        }

                        m_statusLabel->setText(statusMsg);
                        LOG_DEBUG("DXClusterWindow", QString("Single-click: %1").arg(statusMsg));
                    }
                }
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            // Double click - QSY to spot
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // Get the line under cursor
                QTextCursor cursor = m_textDisplay->cursorForPosition(mouseEvent->pos());
                cursor.select(QTextCursor::LineUnderCursor);
                QString line = cursor.selectedText().trimmed();

                if (!line.isEmpty()) {
                    // Check if this is a split spot
                    bool isSplit = line.startsWith("[SPLIT]");

                    if (isSplit && m_splitSpots.contains(line)) {
                        // Emit split QSY signal
                        SplitSpotInfo info = m_splitSpots[line];
                        LOG_DEBUG("DXClusterWindow", QString("Double-click split QSY: TX=%1 Hz, RX=%2 Hz")
                            .arg(QString::number(info.spotFrequency, 'f', 0))
                            .arg(QString::number(info.listenFrequency, 'f', 0)));
                        emit splitQsyRequested(info.spotFrequency, info.listenFrequency);
                    } else {
                        // Parse frequency for simplex QSY
                        QString spotLine = isSplit ? line.mid(8).trimmed() : line;
                        QRegularExpression freqRegex(R"(\s+(\d+\.\d+)\s+)");
                        QRegularExpressionMatch freqMatch = freqRegex.match(spotLine);

                        if (freqMatch.hasMatch()) {
                            QString freqStr = freqMatch.captured(1);
                            double freqKHz = freqStr.toDouble();
                            double freqHz = freqKHz * 1000.0;  // Convert kHz to Hz

                            LOG_DEBUG("DXClusterWindow", QString("Double-click simplex QSY: %1 Hz")
                                .arg(QString::number(freqHz, 'f', 0)));
                            emit qsyRequested(freqHz);
                        }
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
    bool isConnected = (m_statusLabel->text() == "CONNECTED");
    updateConnectionStatus(isConnected);

    // Update freeze button if frozen
    if (m_isFrozen) {
        m_freezeButton->setStyleSheet(QString("QPushButton { background-color: %1; font-weight: bold; }")
            .arg(theme.color(ColorRole::FrozenIndicator).name()));
    }
}

double DXClusterWindow::parseSplitInfo(const QString& comment, double spotFrequency) {
    // Parse split operation from comment:
    // - "QSX 14205" or "QSX14205" - DX listening on 14205 kHz (absolute)
    // - "UP 10" or "UP10" - DX listening 10 kHz up from spot frequency
    // - "DOWN 5" or "DN5" - DX listening 5 kHz down from spot frequency

    QString upperComment = comment.toUpper();

    // Check for QSX (absolute frequency)
    QRegularExpression qsxRegex(R"(QSX\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch qsxMatch = qsxRegex.match(upperComment);
    if (qsxMatch.hasMatch()) {
        double listenKHz = qsxMatch.captured(1).toDouble();
        // Convert kHz to Hz
        return listenKHz * 1000.0;
    }

    // Check for UP (relative offset, positive)
    QRegularExpression upRegex(R"(\bUP\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch upMatch = upRegex.match(upperComment);
    if (upMatch.hasMatch()) {
        double offsetKHz = upMatch.captured(1).toDouble();
        // Add offset to spot frequency
        return spotFrequency + (offsetKHz * 1000.0);
    }

    // Check for DOWN/DN (relative offset, negative)
    QRegularExpression downRegex(R"(\b(?:DOWN|DN)\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch downMatch = downRegex.match(upperComment);
    if (downMatch.hasMatch()) {
        double offsetKHz = downMatch.captured(1).toDouble();
        // Subtract offset from spot frequency
        return spotFrequency - (offsetKHz * 1000.0);
    }

    // Not a split spot
    return 0;
}

} // namespace TR4QT
