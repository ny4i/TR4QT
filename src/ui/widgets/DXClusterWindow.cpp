#include "DXClusterWindow.h"
#include "../../core/Constants.h"
#include "../../logging/LogMacros.h"
#include "../../utils/DialogHelper.h"
#include "../../utils/AppSettings.h"
#include "../../utils/ThemeManager.h"
#include "../../data/LOTWUserRepository.h"
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

DXClusterWindow::DXClusterWindow(QWidget* parent)
    : QWidget(parent)
    , m_telnetThread(new TelnetThread(this))
    , m_telnetClient(nullptr)
    , m_isFrozen(false)
    , m_autoReconnect(false)
    , m_reconnectTimer(new QTimer(this))
    , m_reconnectAttempts(0)
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

    // Add split indicator if this is a split spot
    QString splitIndicator = isSplit ? "[SPLIT] " : "        ";

    QString formattedSpot = QString("%1%2 %3 %4 %5Z %6")
        .arg(splitIndicator)      // Split indicator (8 chars)
        .arg(spotter, -12)        // Left-aligned, 12 chars
        .arg(freqStr, 10)         // Right-aligned, 10 chars
        .arg(callsign, -12)       // Left-aligned, 12 chars
        .arg(timestamp, 4)        // 4 chars (HHMM)
        .arg(comment);            // No padding, remainder

    // Store split info for click-to-QSY handling
    if (isSplit) {
        SplitSpotInfo info;
        info.spotFrequency = frequency;
        info.listenFrequency = listenFrequency;
        info.callsign = callsign;
        m_splitSpots[formattedSpot.trimmed()] = info;
    }

    // Highlight split spots with different color
    QColor spotColor = isSplit ? Qt::darkCyan : Qt::black;
    appendText(formattedSpot, spotColor);

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
