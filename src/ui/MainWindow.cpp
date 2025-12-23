#include "MainWindow.h"
#include "dialogs/RadioConfigDialog.h"
#include "../core/Constants.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QPushButton>
#include <QFont>
#include <QHeaderView>

namespace TR4QT {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_radio(new RadioController(this))
    , m_radioConnected(false)
    , m_qsosThisHour(0)
{
    setWindowTitle(QString("%1 v%2").arg(APP_NAME).arg(APP_VERSION));

    setupUI();
    loadSettings();

    // Setup update timer for time since last QSO and rate calculations
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateTimeDisplay);
    m_updateTimer->start(1000);  // Update every second

    // Connect radio signals (RadioController automatically uses Qt::QueuedConnection for cross-thread)
    connect(m_radio, &RadioController::connectionStatusChanged,
            this, &MainWindow::onRadioConnected);
    connect(m_radio, &RadioController::stateUpdated,
            this, &MainWindow::onRadioStateUpdated);
    connect(m_radio, &RadioController::errorOccurred,
            this, &MainWindow::onRadioError);
    connect(m_radio, &RadioController::radioModelChanged,
            this, &MainWindow::onRadioModelChanged);

    // Try auto-connect if enabled and config exists
    AppSettings& settings = AppSettings::instance();
    if (settings.hasRadioConfig()) {
        if (settings.getRadioAutoConnect()) {
            // Auto-connect enabled - connect now
            m_statusLabel->setText("Auto-connecting to radio...");
            QTimer::singleShot(500, this, &MainWindow::onRadioConnect);  // Slight delay to let UI initialize
        } else {
            m_statusLabel->setText("Found saved radio configuration. Use Radio → Connect to connect.");
        }
    } else {
        m_statusLabel->setText("No radio configuration found. Use Radio → Configure.");
    }
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setupUI() {
    createMenuBar();
    createCentralWidget();
    createStatusBar();

    resize(1024, 768);
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // Radio menu
    QMenu* radioMenu = menuBar->addMenu("&Radio");

    QAction* configAction = radioMenu->addAction("&Configure...");
    connect(configAction, &QAction::triggered, this, &MainWindow::onRadioConfigure);

    radioMenu->addSeparator();

    m_connectAction = radioMenu->addAction("C&onnect");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::onRadioConnect);

    m_disconnectAction = radioMenu->addAction("&Disconnect");
    m_disconnectAction->setEnabled(false);
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::onRadioDisconnect);

    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createCentralWidget() {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Top: Band summary grid
    m_bandSummaryGrid = new BandSummaryGrid(this);
    mainLayout->addWidget(m_bandSummaryGrid);

    // Middle: QSO table (takes most space)
    m_qsoTableModel = new QSOTableModel(this);
    m_qsoTableView = new QTableView(this);
    m_qsoTableView->setModel(m_qsoTableModel);
    m_qsoTableView->setAlternatingRowColors(true);
    m_qsoTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_qsoTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_qsoTableView->horizontalHeader()->setStretchLastSection(false);
    m_qsoTableView->horizontalHeader()->setDefaultSectionSize(70);
    m_qsoTableView->verticalHeader()->setVisible(false);
    m_qsoTableView->setFont(QFont("Monospace", 9));

    // Set specific column widths for TR4W look
    m_qsoTableView->setColumnWidth(QSOTableModel::ColBand, 60);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColDate, 80);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColUTC, 50);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColQSOs, 50);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColCallsign, 100);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColDX, 40);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColZn, 30);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColPts, 40);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColM, 30);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColMult, 30);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColFreq, 80);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColOp, 60);

    mainLayout->addWidget(m_qsoTableView, 1);  // Stretch factor 1 = takes remaining space

    // Bottom: Entry and stats panel
    QWidget* bottomPanel = createBottomPanel();
    mainLayout->addWidget(bottomPanel);

    setCentralWidget(central);
}

QWidget* MainWindow::createBottomPanel() {
    QWidget* bottomPanel = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setSpacing(10);
    bottomLayout->setContentsMargins(0, 4, 0, 0);

    // Left side: Entry fields (vertical layout)
    QWidget* entryWidget = new QWidget(this);
    QGridLayout* entryLayout = new QGridLayout(entryWidget);
    entryLayout->setSpacing(4);
    entryLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* callLabel = new QLabel("Call:", this);
    m_callsignEntry = new QLineEdit(this);
    m_callsignEntry->setPlaceholderText("Callsign");
    m_callsignEntry->setMinimumWidth(150);
    m_callsignEntry->setFont(QFont("Monospace", 12));

    QLabel* exchLabel = new QLabel("Exch:", this);
    m_exchangeEntry = new QLineEdit(this);
    m_exchangeEntry->setPlaceholderText("RST + Zone");
    m_exchangeEntry->setMinimumWidth(150);
    m_exchangeEntry->setFont(QFont("Monospace", 12));

    m_logButton = new QPushButton("Log", this);
    m_logButton->setMinimumHeight(60);  // Span both rows

    // Row 0: Call label and entry
    entryLayout->addWidget(callLabel, 0, 0);
    entryLayout->addWidget(m_callsignEntry, 0, 1);

    // Row 1: Exch label and entry
    entryLayout->addWidget(exchLabel, 1, 0);
    entryLayout->addWidget(m_exchangeEntry, 1, 1);

    // Log button spans both rows
    entryLayout->addWidget(m_logButton, 0, 2, 2, 1);

    entryLayout->setColumnStretch(3, 1);  // Push to left

    bottomLayout->addWidget(entryWidget, 1);

    // Right side: Stats panel
    QWidget* statsWidget = new QWidget(this);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setSpacing(2);
    statsLayout->setContentsMargins(4, 4, 4, 4);

    QFont monoFont("Monospace", 9);

    // Time and rate
    QHBoxLayout* timeRow = new QHBoxLayout();
    m_timeLabel = new QLabel("00:00:00", this);
    m_timeLabel->setFont(monoFont);
    m_thisHrLabel = new QLabel("This Hr = 0", this);
    m_thisHrLabel->setFont(monoFont);
    m_rateLabel = new QLabel("Rate = 0", this);
    m_rateLabel->setFont(monoFont);
    timeRow->addWidget(m_timeLabel);
    timeRow->addWidget(m_thisHrLabel);
    timeRow->addWidget(m_rateLabel);
    timeRow->addStretch();

    // CQ and S&P counts
    QHBoxLayout* countsRow = new QHBoxLayout();
    m_cqCountLabel = new QLabel("CQ: 0", this);
    m_cqCountLabel->setFont(monoFont);
    m_spCountLabel = new QLabel("SP: 0", this);
    m_spCountLabel->setFont(monoFont);
    countsRow->addWidget(m_cqCountLabel);
    countsRow->addWidget(m_spCountLabel);
    countsRow->addStretch();

    // Operator
    QHBoxLayout* opRow = new QHBoxLayout();
    QLabel* opLabel = new QLabel("Op:", this);
    opLabel->setFont(monoFont);
    m_operatorLabel = new QLabel("", this);
    m_operatorLabel->setFont(monoFont);
    opRow->addWidget(opLabel);
    opRow->addWidget(m_operatorLabel);
    opRow->addStretch();

    statsLayout->addLayout(timeRow);
    statsLayout->addLayout(countsRow);
    statsLayout->addLayout(opRow);
    statsLayout->addStretch();

    statsWidget->setMaximumWidth(250);
    bottomLayout->addWidget(statsWidget);

    // Connect signals
    connect(m_callsignEntry, &QLineEdit::textChanged,
            this, &MainWindow::onCallsignChanged);
    connect(m_callsignEntry, &QLineEdit::returnPressed,
            this, &MainWindow::onLogQSO);
    connect(m_exchangeEntry, &QLineEdit::returnPressed,
            this, &MainWindow::onLogQSO);
    connect(m_logButton, &QPushButton::clicked,
            this, &MainWindow::onLogQSO);

    return bottomPanel;
}

void MainWindow::createStatusBar() {
    QStatusBar* status = new QStatusBar(this);
    setStatusBar(status);

    m_statusLabel = new QLabel("Ready", this);
    status->addWidget(m_statusLabel);

    m_radioStatusLabel = new QLabel("Radio: Not Connected", this);
    m_radioStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    status->addPermanentWidget(m_radioStatusLabel);
}

void MainWindow::loadSettings() {
    AppSettings& settings = AppSettings::instance();

    QByteArray geometry = settings.loadWindowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    QByteArray state = settings.loadWindowState();
    if (!state.isEmpty()) {
        restoreState(state);
    }
}

void MainWindow::saveSettings() {
    AppSettings& settings = AppSettings::instance();
    settings.saveWindowGeometry(saveGeometry());
    settings.saveWindowState(saveState());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Disconnect radio before closing
    if (m_radioConnected) {
        m_radio->disconnectFromRadio();
    }

    saveSettings();
    event->accept();
}

void MainWindow::onRadioConfigure() {
    RadioConfigDialog dialog(this);

    // Load existing config if available
    AppSettings& settings = AppSettings::instance();
    if (settings.hasRadioConfig()) {
        dialog.setConfig(settings.loadRadioConfig());
    }

    if (dialog.exec() == QDialog::Accepted) {
        RadioConfig config = dialog.getConfig();
        settings.saveRadioConfig(config);

        // Save auto-connect setting
        settings.setRadioAutoConnect(dialog.getAutoConnect());

        m_statusLabel->setText(QString("Radio configuration saved: Model %1, Port %2")
                                  .arg(config.hamlibModelId)
                                  .arg(config.port));

        // If currently connected, ask to reconnect
        if (m_radioConnected) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Reconnect Radio?",
                "Radio configuration changed. Reconnect with new settings?",
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                onRadioDisconnect();
                onRadioConnect();
            }
        }
    }
}

void MainWindow::onRadioConnect() {
    AppSettings& settings = AppSettings::instance();

    if (!settings.hasRadioConfig()) {
        QMessageBox::warning(this, "No Configuration",
                           "Please configure your radio first (Radio → Configure).");
        onRadioConfigure();
        return;
    }

    RadioConfig config = settings.loadRadioConfig();

    m_statusLabel->setText(QString("Connecting to radio: Model %1, Port %2...")
                              .arg(config.hamlibModelId)
                              .arg(config.port));
    QApplication::processEvents();  // Update UI

    // Connect happens asynchronously in worker thread
    // connectionStatusChanged signal will indicate success/failure
    m_radio->connectToRadio(config);
}

void MainWindow::onRadioDisconnect() {
    m_statusLabel->setText("Disconnecting from radio...");
    m_radio->disconnectFromRadio();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About TR4QT",
                      QString("<h2>%1 v%2</h2>"
                              "<p>Amateur Radio Contest Logger</p>"
                              "<p>Cross-platform Qt/C++ port of TR4W</p>"
                              "<p><b>Phase 1 Development Build</b></p>"
                              "<p>Features:</p>"
                              "<ul>"
                              "<li>Radio control via Hamlib</li>"
                              "<li>Support for K4, IC-7610, IC-7760</li>"
                              "<li>DXCC country lookup (cty.dat)</li>"
                              "</ul>")
                          .arg(APP_NAME)
                          .arg(APP_VERSION));
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onRadioConnected(bool connected) {
    qDebug() << "MainWindow::onRadioConnected called with connected =" << connected;
    m_radioConnected = connected;
    updateConnectionStatus(connected);

    if (connected) {
        m_statusLabel->setText("Connected to radio (waiting for state...)");
    } else {
        m_statusLabel->setText("Radio disconnected");
    }
}

void MainWindow::onRadioStateUpdated(const RadioState& state) {
    // Log radio model if it changed
    static QString lastModel;
    if (!state.radioModel.isEmpty() && state.radioModel != lastModel) {
        qDebug() << "MainWindow: Radio model from state:" << state.radioModel;
        m_statusLabel->setText(QString("Radio: %1").arg(state.radioModel));
        lastModel = state.radioModel;
    }

    m_currentState = state;
    // Radio state is cached for use when logging QSOs
}

void MainWindow::onRadioError(const QString& error) {
    m_statusLabel->setText(QString("Radio error: %1").arg(error));
}

void MainWindow::onRadioModelChanged(const QString& model) {
    qDebug() << "MainWindow::onRadioModelChanged:" << model;
    if (m_radioConnected) {
        m_radioStatusLabel->setText(QString("Radio: %1").arg(model));
    }
}


void MainWindow::updateConnectionStatus(bool connected) {
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);

    if (connected) {
        // DO NOT call m_radio->getRadioModel() here - that's a cross-thread blocking call!
        // Radio model will be updated via onRadioModelChanged() signal
        m_radioStatusLabel->setText("Radio: Connected");
        m_radioStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        m_radioStatusLabel->setText("Radio: Not Connected");
        m_radioStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}

// ===== Logging Actions =====

void MainWindow::onLogQSO() {
    QString callsign = m_callsignEntry->text().trimmed().toUpper();
    QString exchange = m_exchangeEntry->text().trimmed();

    if (callsign.isEmpty()) {
        m_statusLabel->setText("Error: Callsign is required");
        m_callsignEntry->setFocus();
        return;
    }

    if (exchange.isEmpty()) {
        m_statusLabel->setText("Error: Exchange is required");
        m_exchangeEntry->setFocus();
        return;
    }

    // Create QSO object with current radio state (snapshot!)
    QSO qso;
    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.callsign = callsign;

    // Snapshot radio state
    qso.frequency = m_currentState.frequencyA;
    qso.mode = m_currentState.modeA;
    qso.band = m_currentState.bandA;

    // Exchange
    qso.rstSent = (qso.mode == ModeType::CW) ? "599" : "59";
    qso.rstReceived = (qso.mode == ModeType::CW) ? "599" : "59";
    qso.exchangeReceived = exchange;

    // TODO: Lookup country/zone from cty.dat
    // TODO: Check for dupe
    // TODO: Calculate points via contest
    // TODO: Check for new multipliers
    // TODO: Save to database

    // For now, just add to table
    m_qsoTableModel->addQSO(qso);

    // Update last QSO time for time since last QSO calculation
    m_lastQSOTime = qso.timestamp;

    m_statusLabel->setText(QString("Logged: %1 on %2 %3")
                              .arg(callsign)
                              .arg(bandToString(qso.band))
                              .arg(modeToString(qso.mode)));

    // Clear entry fields and focus callsign
    onClearEntry();

    // Update score display and time display
    updateScoreDisplay();
    updateTimeDisplay();  // Immediate update after logging
}

void MainWindow::onCallsignChanged(const QString& callsign) {
    // TODO: Real-time dupe checking
    // TODO: Callsign lookup/prediction from previous QSOs
    Q_UNUSED(callsign);
}

void MainWindow::onClearEntry() {
    m_callsignEntry->clear();
    m_exchangeEntry->clear();
    m_callsignEntry->setFocus();
}

void MainWindow::updateScoreDisplay() {
    // TODO: Calculate actual scores per band and update band summary grid
    // For now, just update the status bar with QSO count
    int qsoCount = m_qsoTableModel->count();
    m_statusLabel->setText(QString("%1 QSOs logged").arg(qsoCount));
}

void MainWindow::updateTimeDisplay() {
    // Calculate time since last QSO
    QString timeStr = "00:00:00";
    if (m_lastQSOTime.isValid()) {
        qint64 elapsed = m_lastQSOTime.secsTo(QDateTime::currentDateTimeUtc());
        int hours = elapsed / 3600;
        int mins = (elapsed % 3600) / 60;
        int secs = elapsed % 60;
        timeStr = QString("%1:%2:%3")
                      .arg(hours, 2, 10, QChar('0'))
                      .arg(mins, 2, 10, QChar('0'))
                      .arg(secs, 2, 10, QChar('0'));
    }
    m_timeLabel->setText(timeStr);

    // Calculate QSOs this hour
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime hourStart = QDateTime(now.date(), QTime(now.time().hour(), 0), Qt::UTC);

    m_qsosThisHour = 0;
    for (int i = 0; i < m_qsoTableModel->count(); ++i) {
        QSO qso = m_qsoTableModel->getQSO(i);
        if (qso.timestamp >= hourStart && qso.timestamp <= now) {
            m_qsosThisHour++;
        }
    }

    // Calculate rate (QSOs per hour based on last 10 QSOs or last 10 minutes)
    int rate = 0;
    if (m_qsoTableModel->count() >= 2) {
        int lookback = qMin(10, m_qsoTableModel->count());
        QSO firstQSO = m_qsoTableModel->getQSO(m_qsoTableModel->count() - lookback);
        QSO lastQSO = m_qsoTableModel->getQSO(m_qsoTableModel->count() - 1);

        qint64 periodSecs = firstQSO.timestamp.secsTo(lastQSO.timestamp);
        if (periodSecs > 0) {
            rate = (lookback - 1) * 3600 / periodSecs;
        }
    }

    // Update labels
    m_thisHrLabel->setText(QString("This Hr = %1").arg(m_qsosThisHour));
    m_rateLabel->setText(QString("Rate = %1").arg(rate));
}

} // namespace TR4QT
