#include "DXClusterWindow.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QFont>
#include <QSettings>

namespace TR4QT {

DXClusterWindow::DXClusterWindow(QWidget* parent)
    : QWidget(parent)
    , m_telnetThread(new TelnetThread(this))
    , m_telnetClient(nullptr)
    , m_isFrozen(false)
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

    setWindowTitle("DX Cluster");
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
    m_statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    mainLayout->addWidget(m_statusLabel);

    // Text display
    m_textDisplay = new QTextEdit(this);
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setFont(QFont("Monospace", 9));
    m_textDisplay->setStyleSheet("QTextEdit { background-color: white; color: black; }");
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

    // Load recent servers
    int serverCount = settings.value("DXCluster/ServerCount", 0).toInt();
    for (int i = 0; i < serverCount; ++i) {
        QString server = settings.value(QString("DXCluster/Server%1").arg(i)).toString();
        if (!server.isEmpty() && m_serverCombo->findText(server) == -1) {
            m_serverCombo->addItem(server);
        }
    }

    // Load last server
    QString lastServer = settings.value("DXCluster/LastServer").toString();
    if (!lastServer.isEmpty()) {
        int index = m_serverCombo->findText(lastServer);
        if (index >= 0) {
            m_serverCombo->setCurrentIndex(index);
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

    QString serverString = m_serverCombo->currentText().trimmed();
    if (serverString.isEmpty()) {
        QMessageBox::warning(this, "DX Cluster",
                           "Please enter a server address (host:port)");
        return;
    }

    // Parse server:port
    QStringList parts = serverString.split(':');
    if (parts.size() != 2) {
        QMessageBox::warning(this, "DX Cluster",
                           "Invalid format. Use: hostname:port (e.g., DXC.NC7J.COM:7373)");
        return;
    }

    QString host = parts[0].trimmed();
    bool ok;
    int port = parts[1].trimmed().toInt(&ok);

    if (!ok || port <= 0 || port > 65535) {
        QMessageBox::warning(this, "DX Cluster",
                           "Invalid port number");
        return;
    }

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

    QMetaObject::invokeMethod(m_telnetClient, "disconnectFromServer",
                             Qt::QueuedConnection);
}

void DXClusterWindow::onFreezeClicked() {
    m_isFrozen = m_freezeButton->isChecked();
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

    QMessageBox::information(this, "DX Cluster Commands", help);
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
}

void DXClusterWindow::onTelnetDisconnected() {
    updateConnectionStatus(false);
    appendText("Disconnected.", Qt::darkRed);
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
    // Forward spot to band map (signal will be connected by MainWindow)
    emit spotReceived(callsign, frequency, spotter, comment);
}

void DXClusterWindow::updateConnectionStatus(bool connected) {
    if (connected) {
        m_statusLabel->setText("CONNECTED");
        m_statusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(true);
    } else {
        m_statusLabel->setText("DISCONNECTED");
        m_statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
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

} // namespace TR4QT
