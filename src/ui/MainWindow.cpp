#include "MainWindow.h"
#include "dialogs/RadioConfigDialog.h"
#include "dialogs/PreferencesDialog.h"
#include "widgets/DXClusterWindow.h"
#include "widgets/BandMapWidget.h"
#include "widgets/RadioControlWidget.h"
#include "widgets/MultiplierWidget.h"
#include "../network/UdpBroadcastManager.h"
#include "../core/Constants.h"
#include "../utils/ADIFExporter.h"
#include "../utils/CabrilloExporter.h"
#include <QFile>
#include <QFileDialog>
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
    , m_dxClusterWindow(nullptr)
    , m_bandMapWindow(nullptr)
    , m_radioControlWindow(nullptr)
    , m_multiplierWindow(nullptr)
    , m_qsosThisHour(0)
    , m_hasActiveContest(false)
    , m_activeContest(nullptr)
    , m_nextSerialNumber(1)
    , m_udpBroadcastManager(new UdpBroadcastManager(this))
{
    setWindowTitle(QString("%1 v%2").arg(APP_NAME).arg(APP_VERSION));
    setWindowIcon(QIcon(":/icons/tr4qt.png"));

    // Load country file for exchange auto-population
    QString countryFilePath = AppSettings::instance().getCountryFilePath();
    if (QFile::exists(countryFilePath)) {
        if (!m_countryFile.loadFromFile(countryFilePath)) {
            qWarning() << "Failed to load country file:" << countryFilePath;
        }
    }

    setupUI();
    loadSettings();
    loadUdpBroadcastSettings();

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
    // Settings are already saved in closeEvent()
    // Don't save here as windows will be closed and visibility will be wrong

    // Clean up active contest
    if (m_activeContest) {
        delete m_activeContest;
        m_activeContest = nullptr;
    }
}

void MainWindow::setupUI() {
    createMenuBar();
    createCentralWidget();
    createStatusBar();

    // Set minimum size to prevent UI from becoming unusable
    setMinimumSize(800, 600);

    // Set initial size (user can resize larger or smaller, but not below minimum)
    resize(1024, 768);
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");

    QAction* newContestAction = fileMenu->addAction("&New/Open Contest...");
    newContestAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(newContestAction, &QAction::triggered, this, &MainWindow::onNewOpenContest);

    fileMenu->addSeparator();

    QAction* clearLogAction = fileMenu->addAction("&Clear Log...");
    connect(clearLogAction, &QAction::triggered, this, &MainWindow::onClearLog);

    fileMenu->addSeparator();

    // Export submenu
    QMenu* exportMenu = fileMenu->addMenu("&Export");
    QAction* exportADIFAction = exportMenu->addAction("Export &ADIF...");
    connect(exportADIFAction, &QAction::triggered, this, &MainWindow::onExportADIF);

    QAction* exportCabrilloAction = exportMenu->addAction("Export &Cabrillo...");
    exportCabrilloAction->setShortcut(QKeySequence("Ctrl+Alt+B"));
    connect(exportCabrilloAction, &QAction::triggered, this, &MainWindow::onExportCabrillo);

    fileMenu->addSeparator();

    QAction* preferencesAction = fileMenu->addAction("&Preferences...");
    preferencesAction->setShortcut(QKeySequence::Preferences);
    preferencesAction->setMenuRole(QAction::PreferencesRole);  // Explicitly set macOS menu role
    connect(preferencesAction, &QAction::triggered, this, [this]() {
        qDebug() << "*** Preferences action triggered ***";
        onPreferences();
    });
    qDebug() << "*** Preferences menu created with shortcut:" << preferencesAction->shortcut().toString()
             << "menuRole:" << preferencesAction->menuRole();

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // Radio menu
    QMenu* radioMenu = menuBar->addMenu("&Radio");

    QAction* configAction = radioMenu->addAction("&Configure...");
    configAction->setMenuRole(QAction::NoRole);  // Prevent macOS from moving this to app menu
    connect(configAction, &QAction::triggered, this, [this]() {
        qDebug() << "*** Radio Configure action triggered ***";
        onRadioConfigure();
    });
    qDebug() << "*** Radio Configure menu created with menuRole:" << configAction->menuRole();

    radioMenu->addSeparator();

    m_connectAction = radioMenu->addAction("C&onnect");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::onRadioConnect);

    m_disconnectAction = radioMenu->addAction("&Disconnect");
    m_disconnectAction->setEnabled(false);
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::onRadioDisconnect);

    // Window menu
    QMenu* windowMenu = menuBar->addMenu("&Window");

    QAction* dxClusterAction = windowMenu->addAction("DX &Cluster");
    dxClusterAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(dxClusterAction, &QAction::triggered, this, &MainWindow::onShowDXCluster);

    QAction* bandMapAction = windowMenu->addAction("&Band Map");
    bandMapAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
    connect(bandMapAction, &QAction::triggered, this, &MainWindow::onShowBandMap);

    QAction* radioControlAction = windowMenu->addAction("&Radio Control");
    radioControlAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(radioControlAction, &QAction::triggered, this, &MainWindow::onShowRadioControl);

    QAction* multipliersAction = windowMenu->addAction("&Multipliers");
    multipliersAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
    connect(multipliersAction, &QAction::triggered, this, &MainWindow::onShowMultipliers);

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
    connect(m_bandSummaryGrid, &BandSummaryGrid::bandClicked,
            this, &MainWindow::onBandClicked);
    mainLayout->addWidget(m_bandSummaryGrid);

    // Middle: QSO table (takes most space)
    m_qsoTableModel = new QSOTableModel(this);
    m_qsoTableView = new QTableView(this);
    m_qsoTableView->setModel(m_qsoTableModel);
    m_qsoTableView->setAlternatingRowColors(true);
    m_qsoTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_qsoTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_qsoTableView->horizontalHeader()->setStretchLastSection(true);  // Op column stretches
    m_qsoTableView->verticalHeader()->setVisible(false);
    m_qsoTableView->setFont(QFont("Monospace", 9));

    // Set column widths and resize modes for proper scaling
    QHeaderView* header = m_qsoTableView->horizontalHeader();

    // Fixed-width columns (small data)
    header->setSectionResizeMode(QSOTableModel::ColM, QHeaderView::Fixed);
    header->setSectionResizeMode(QSOTableModel::ColExch2, QHeaderView::Fixed);
    header->setSectionResizeMode(QSOTableModel::ColPts, QHeaderView::Fixed);
    header->setSectionResizeMode(QSOTableModel::ColExch1, QHeaderView::Fixed);
    header->setSectionResizeMode(QSOTableModel::ColMult, QHeaderView::Fixed);

    m_qsoTableView->setColumnWidth(QSOTableModel::ColM, 30);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColExch2, 30);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColPts, 40);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColExch1, 40);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColMult, 30);

    // Interactive columns (medium data, user can resize)
    header->setSectionResizeMode(QSOTableModel::ColBand, QHeaderView::Interactive);
    header->setSectionResizeMode(QSOTableModel::ColDate, QHeaderView::Interactive);
    header->setSectionResizeMode(QSOTableModel::ColUTC, QHeaderView::Interactive);
    header->setSectionResizeMode(QSOTableModel::ColQSOs, QHeaderView::Interactive);
    header->setSectionResizeMode(QSOTableModel::ColCallsign, QHeaderView::Interactive);
    header->setSectionResizeMode(QSOTableModel::ColFreq, QHeaderView::Interactive);

    m_qsoTableView->setColumnWidth(QSOTableModel::ColBand, 60);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColDate, 80);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColUTC, 50);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColQSOs, 50);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColCallsign, 100);
    m_qsoTableView->setColumnWidth(QSOTableModel::ColFreq, 80);

    // Op column stretches to fill remaining space (last column with stretchLastSection=true)
    header->setSectionResizeMode(QSOTableModel::ColOp, QHeaderView::Stretch);

    mainLayout->addWidget(m_qsoTableView, 1);  // Stretch factor 1 = takes remaining space

    // Bottom: Entry and stats panel
    QWidget* bottomPanel = createBottomPanel();
    mainLayout->addWidget(bottomPanel);

    // Radio status grid (at very bottom)
    QWidget* radioStatusGrid = createRadioStatusGrid();
    mainLayout->addWidget(radioStatusGrid);

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
    m_callsignEntry->setMaximumWidth(300);
    m_callsignEntry->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_callsignEntry->setFont(QFont("Monospace", 12));

    QLabel* exchLabel = new QLabel("Exch:", this);
    m_exchangeEntry = new QLineEdit(this);
    m_exchangeEntry->setPlaceholderText("RST + Zone");
    m_exchangeEntry->setMinimumWidth(150);
    m_exchangeEntry->setMaximumWidth(300);
    m_exchangeEntry->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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
    m_timeLabel->setMinimumWidth(70);  // Fixed width to prevent layout shifts
    m_timeLabel->setAlignment(Qt::AlignLeft);
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

    // Set size policy: prefer fixed width but can shrink if needed
    statsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    statsWidget->setMinimumWidth(200);
    statsWidget->setMaximumWidth(300);
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

QWidget* MainWindow::createRadioStatusGrid() {
    QWidget* radioStatusWidget = new QWidget(this);
    QHBoxLayout* radioLayout = new QHBoxLayout(radioStatusWidget);
    radioLayout->setSpacing(20);
    radioLayout->setContentsMargins(10, 5, 10, 5);

    // Set background color to match TR4W style
    radioStatusWidget->setStyleSheet("QWidget { background-color: #f0f0f0; }");

    QFont labelFont("Monospace", 11);
    labelFont.setBold(true);

    // Band/Mode label (e.g., "15SSB")
    m_radioFreqBandLabel = new QLabel("--", this);
    m_radioFreqBandLabel->setFont(labelFont);
    m_radioFreqBandLabel->setMinimumWidth(80);
    m_radioFreqBandLabel->setAlignment(Qt::AlignCenter);
    m_radioFreqBandLabel->setStyleSheet("QLabel { background-color: white; padding: 5px; border: 1px solid #ccc; }");

    // Frequency label (below will be in vertical layout)
    QFont freqFont("Monospace", 10);
    m_radioFreqLabel = new QLabel("0.000 MHz", this);
    m_radioFreqLabel->setFont(freqFont);
    m_radioFreqLabel->setMinimumWidth(100);
    m_radioFreqLabel->setAlignment(Qt::AlignCenter);
    m_radioFreqLabel->setStyleSheet("QLabel { background-color: white; padding: 3px; border: 1px solid #ccc; }");

    // Vertical layout for band/mode and frequency
    QVBoxLayout* freqLayout = new QVBoxLayout();
    freqLayout->setSpacing(2);
    freqLayout->addWidget(m_radioFreqBandLabel);
    freqLayout->addWidget(m_radioFreqLabel);

    // Date/Time label
    m_radioDateTimeLabel = new QLabel("", this);
    m_radioDateTimeLabel->setFont(labelFont);
    m_radioDateTimeLabel->setAlignment(Qt::AlignCenter);
    m_radioDateTimeLabel->setStyleSheet("QLabel { background-color: white; padding: 5px; border: 1px solid #ccc; }");

    radioLayout->addLayout(freqLayout);
    radioLayout->addWidget(m_radioDateTimeLabel);
    radioLayout->addStretch();

    return radioStatusWidget;
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

    // Apply font settings
    applyFontSettings();

    // Restore child windows if they were visible
    qDebug() << "DX Cluster was visible:" << settings.getDXClusterVisible();
    if (settings.getDXClusterVisible()) {
        qDebug() << "Restoring DX Cluster window";
        onShowDXCluster();
        QByteArray dxGeometry = settings.loadDXClusterGeometry();
        if (!dxGeometry.isEmpty()) {
            m_dxClusterWindow->restoreGeometry(dxGeometry);
        }
    }

    qDebug() << "Band Map was visible:" << settings.getBandMapVisible();
    if (settings.getBandMapVisible()) {
        qDebug() << "Restoring Band Map window";
        onShowBandMap();
        QByteArray bmGeometry = settings.loadBandMapGeometry();
        if (!bmGeometry.isEmpty()) {
            m_bandMapWindow->restoreGeometry(bmGeometry);
        }
    }

    qDebug() << "Radio Control was visible:" << settings.getRadioControlVisible();
    if (settings.getRadioControlVisible()) {
        qDebug() << "Restoring Radio Control window";
        onShowRadioControl();
        QByteArray rcGeometry = settings.loadRadioControlGeometry();
        if (!rcGeometry.isEmpty()) {
            m_radioControlWindow->restoreGeometry(rcGeometry);
        }
    }

    qDebug() << "Multipliers was visible:" << settings.getMultipliersVisible();
    if (settings.getMultipliersVisible()) {
        qDebug() << "Restoring Multipliers window";
        onShowMultipliers();
        QByteArray multGeometry = settings.loadMultipliersGeometry();
        if (!multGeometry.isEmpty()) {
            m_multiplierWindow->restoreGeometry(multGeometry);
        }
    }
}

void MainWindow::saveSettings() {
    AppSettings& settings = AppSettings::instance();
    settings.saveWindowGeometry(saveGeometry());
    settings.saveWindowState(saveState());

    // Save child window geometry and visibility states
    if (m_dxClusterWindow) {
        bool visible = m_dxClusterWindow->isVisible();
        qDebug() << "Saving DX Cluster window - visible:" << visible;
        settings.saveDXClusterGeometry(m_dxClusterWindow->saveGeometry());
        settings.setDXClusterVisible(visible);
    }

    if (m_bandMapWindow) {
        bool visible = m_bandMapWindow->isVisible();
        qDebug() << "Saving Band Map window - visible:" << visible;
        settings.saveBandMapGeometry(m_bandMapWindow->saveGeometry());
        settings.setBandMapVisible(visible);
    }

    if (m_radioControlWindow) {
        bool visible = m_radioControlWindow->isVisible();
        qDebug() << "Saving Radio Control window - visible:" << visible;
        settings.saveRadioControlGeometry(m_radioControlWindow->saveGeometry());
        settings.setRadioControlVisible(visible);
    }

    if (m_multiplierWindow) {
        bool visible = m_multiplierWindow->isVisible();
        qDebug() << "Saving Multipliers window - visible:" << visible;
        settings.saveMultipliersGeometry(m_multiplierWindow->saveGeometry());
        settings.setMultipliersVisible(visible);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Ask for confirmation before closing
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Exit",
                                  "Are you sure you want to exit TR4QT?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        event->ignore();
        return;
    }

    // Save settings BEFORE closing windows (so visibility state is correct)
    saveSettings();

    // Close all child windows
    if (m_dxClusterWindow) {
        m_dxClusterWindow->close();
    }
    if (m_bandMapWindow) {
        m_bandMapWindow->close();
    }
    if (m_radioControlWindow) {
        m_radioControlWindow->close();
    }
    if (m_multiplierWindow) {
        m_multiplierWindow->close();
    }

    // Disconnect radio before closing
    if (m_radioConnected) {
        m_radio->disconnectFromRadio();
    }

    event->accept();

    // Ensure application quits
    QApplication::quit();
}

void MainWindow::onRadioConfigure() {
    qDebug() << "*** onRadioConfigure() called - opening RadioConfigDialog ***";
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

void MainWindow::onNewOpenContest() {
    ContestChooserDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        ContestInfo contestInfo = dialog.getContestInfo();

        // Activate the contest (creates contest object, updates UI)
        activateContest(contestInfo);

        // Update status
        if (contestInfo.isExisting) {
            m_statusLabel->setText(QString("Resumed contest: %1").arg(contestInfo.contestName));
        } else {
            m_statusLabel->setText(QString("Created new contest: %1").arg(contestInfo.contestName));
        }

        // TODO: Initialize database for new contest
        // TODO: Load QSOs from database if resuming existing contest
    }
}

void MainWindow::onPreferences() {
    qDebug() << "*** onPreferences() called - opening PreferencesDialog ***";
    PreferencesDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        m_statusLabel->setText("Preferences saved");

        // Apply font size changes immediately
        applyFontSettings();

        // Reload UDP broadcast settings
        loadUdpBroadcastSettings();

        // If radio settings changed and radio is connected, ask to reconnect
        if (m_radioConnected) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Reconnect Radio?",
                "Radio settings may have changed. Reconnect to apply new settings?",
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                onRadioDisconnect();
                QTimer::singleShot(500, this, &MainWindow::onRadioConnect);
            }
        }

        // TODO: Reload contest settings if changed
    }
}

void MainWindow::onExportADIF() {
    // Get all QSOs from the table model
    QList<QSO> qsos;
    for (int i = 0; i < m_qsoTableModel->count(); ++i) {
        qsos.append(m_qsoTableModel->getQSO(i));
    }

    if (qsos.isEmpty()) {
        QMessageBox::information(this, "Export ADIF", "No QSOs to export.");
        return;
    }

    // Get file name from user
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export ADIF File",
        QDir::homePath() + "/log.adi",
        "ADIF Files (*.adi *.adif);;All Files (*)");

    if (fileName.isEmpty()) {
        return;  // User cancelled
    }

    // Export to file
    ADIFExporter exporter;
    QString contestName = m_hasActiveContest ? m_currentContest.contestName : QString();
    QString operatorCall = AppSettings::instance().getMyCallsign();

    if (exporter.exportToFile(qsos, fileName, contestName, operatorCall)) {
        m_statusLabel->setText(QString("Exported %1 QSOs to %2")
                                  .arg(qsos.size())
                                  .arg(QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::critical(this, "Export Error",
                            QString("Failed to export ADIF: %1")
                                .arg(exporter.lastError()));
    }
}

void MainWindow::onExportCabrillo() {
    // Check if we have QSOs to export
    if (m_qsoTableModel->count() == 0) {
        QMessageBox::information(this, "Export Cabrillo", "No QSOs to export.");
        return;
    }

    // Warn if no contest is active
    if (!m_hasActiveContest || !m_activeContest) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Export Cabrillo",
            "No active contest selected. Export anyway with generic formatting?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (reply == QMessageBox::No) {
            return;
        }
    }

    // Get all QSOs from the table model
    QList<QSO> qsos;
    for (int i = 0; i < m_qsoTableModel->count(); ++i) {
        qsos.append(m_qsoTableModel->getQSO(i));
    }

    if (qsos.isEmpty()) {
        QMessageBox::information(this, "Export Cabrillo", "No QSOs to export.");
        return;
    }

    // Get file name from user
    QString defaultFileName;
    if (m_hasActiveContest && !m_currentContest.contestName.isEmpty()) {
        defaultFileName = QString("%1.cbr").arg(m_currentContest.contestName.replace(' ', '_'));
    } else {
        defaultFileName = "log.cbr";
    }
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export Cabrillo File",
        QDir::homePath() + "/" + defaultFileName,
        "Cabrillo Files (*.cbr *.log);;All Files (*)");

    if (fileName.isEmpty()) {
        return;  // User cancelled
    }

    // Set up exporter with station information
    CabrilloExporter exporter;
    exporter.setStationInfo(
        AppSettings::instance().getMyCallsign(),
        AppSettings::instance().getMyGridSquare(),
        AppSettings::instance().getMyCallsign(),  // Name
        "",  // Address
        "",  // City
        "",  // State/Province
        "",  // Postal Code
        "",  // Country
        ""   // Email
    );

    // TODO: Get category information from contest dialog
    exporter.setCategory(
        "NON-ASSISTED",  // Assisted
        "ALL",           // Band
        "MIXED",         // Mode
        "SINGLE-OP",     // Operator
        "LOW",           // Power
        "FIXED",         // Station
        "",              // Time
        "ONE",           // Transmitter
        ""               // Overlay
    );

    // Calculate claimed score
    // TODO: Get actual score from scoring engine
    int claimedScore = 0;
    exporter.setClaimedScore(claimedScore);
    exporter.setOperators(AppSettings::instance().getMyCallsign());

    // Export to file
    if (exporter.exportToFile(qsos, m_activeContest, fileName)) {
        m_statusLabel->setText(QString("Exported %1 QSOs to %2")
                                  .arg(qsos.size())
                                  .arg(QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::critical(this, "Export Error",
                            QString("Failed to export Cabrillo: %1")
                                .arg(exporter.lastError()));
    }
}

void MainWindow::onClearLog() {
    if (m_qsoTableModel->count() == 0) {
        QMessageBox::information(this, "Clear Log", "Log is already empty.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Log",
        QString("Are you sure you want to clear all %1 QSOs from the log?\n\nThis action cannot be undone.")
            .arg(m_qsoTableModel->count()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_qsoTableModel->clear();
        m_lastQSOTime = QDateTime();  // Reset time tracking
        m_qsosThisHour = 0;
        updateScoreDisplay();
        updateTimeDisplay();
        m_statusLabel->setText("Log cleared");
    }
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
    // Send UDP broadcast for radio state change (throttled in manager)
    QString stationCall = AppSettings::instance().getMyCallsign();
    m_udpBroadcastManager->onRadioStateChanged(state, stationCall);

    // Log radio model if it changed
    static QString lastModel;
    if (!state.radioModel.isEmpty() && state.radioModel != lastModel) {
        qDebug() << "MainWindow: Radio model from state:" << state.radioModel;
        m_statusLabel->setText(QString("Radio: %1").arg(state.radioModel));
        lastModel = state.radioModel;
    }

    m_currentState = state;
    // Radio state is cached for use when logging QSOs

    // Update radio status grid with new state
    updateRadioStatusGrid();

    // Update radio control window if it's open
    if (m_radioControlWindow && m_radioControlWindow->isVisible()) {
        m_radioControlWindow->updateRadioState(state);
    }
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

    // Send UDP broadcast for new QSO
    QString stationCall = AppSettings::instance().getMyCallsign();
    QString contestName = m_hasActiveContest ? m_currentContest.contestName : "Unknown";
    m_udpBroadcastManager->onQSOLogged(qso, stationCall, contestName);

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
    // Auto-populate exchange based on callsign lookup (e.g., zone from cty.dat)
    autoPopulateExchange(callsign);

    // TODO: Real-time dupe checking
    // TODO: Callsign lookup/prediction from previous QSOs
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

    // Update radio status grid (date/time updates every second)
    updateRadioStatusGrid();
}

void MainWindow::updateRadioStatusGrid() {
    // Update band/mode (e.g., "15SSB")
    if (m_radioConnected && m_currentState.frequencyA > 0) {
        QString bandStr = bandToString(m_currentState.bandA).remove('M');  // Remove 'M' from "15M" -> "15"
        QString modeStr = modeToString(m_currentState.modeA);
        m_radioFreqBandLabel->setText(QString("%1%2").arg(bandStr).arg(modeStr));

        // Update frequency (in MHz with 3 decimal places)
        double freqMHz = m_currentState.frequencyA / 1000000.0;
        m_radioFreqLabel->setText(QString("%1 MHz").arg(freqMHz, 0, 'f', 3));
    } else {
        m_radioFreqBandLabel->setText("--");
        m_radioFreqLabel->setText("0.000 MHz");
    }

    // Update date/time (current local time)
    QDateTime now = QDateTime::currentDateTime();
    QString dateTimeStr = now.toString("ddd dd-MMM-yyyy hh:mm:ss");
    m_radioDateTimeLabel->setText(dateTimeStr);
}

void MainWindow::applyFontSettings() {
    AppSettings& settings = AppSettings::instance();

    // Apply entry field font sizes
    int entryFontSize = settings.getEntryFontSize();
    QFont entryFont("Monospace", entryFontSize);
    m_callsignEntry->setFont(entryFont);
    m_exchangeEntry->setFont(entryFont);

    // Apply QSO table font size
    int tableFontSize = settings.getTableFontSize();
    QFont tableFont("Monospace", tableFontSize);
    m_qsoTableView->setFont(tableFont);

    // Apply band summary grid font size
    int gridFontSize = settings.getGridFontSize();
    if (m_bandSummaryGrid) {
        m_bandSummaryGrid->setFontSize(gridFontSize);
    }
}

void MainWindow::loadUdpBroadcastSettings() {
    AppSettings& settings = AppSettings::instance();

    // Configure UDP broadcast manager
    m_udpBroadcastManager->setEnabled(settings.getUDPBroadcastEnabled());
    m_udpBroadcastManager->setRadioInfoEnabled(settings.getUDPRadioInfoEnabled());
    m_udpBroadcastManager->setContactInfoEnabled(settings.getUDPContactInfoEnabled());
    m_udpBroadcastManager->setThrottleInterval(settings.getUDPThrottleInterval());

    // Load destinations
    QList<UdpDestination> destinations = settings.getUDPDestinations();
    m_udpBroadcastManager->setDestinations(destinations);

    qDebug() << "UDP Broadcast settings loaded:"
             << "Enabled=" << settings.getUDPBroadcastEnabled()
             << "RadioInfo=" << settings.getUDPRadioInfoEnabled()
             << "ContactInfo=" << settings.getUDPContactInfoEnabled()
             << "Destinations=" << destinations.size();
}

void MainWindow::activateContest(const ContestInfo& contestInfo) {
    // Clean up previous contest if any
    if (m_activeContest) {
        delete m_activeContest;
        m_activeContest = nullptr;
    }

    // Create appropriate contest instance based on type
    if (contestInfo.contestType == "CQWW_CW") {
        m_activeContest = new CQWWContest(ModeType::CW);
    } else if (contestInfo.contestType == "CQWW_SSB") {
        m_activeContest = new CQWWContest(ModeType::USB);
    } else if (contestInfo.contestType == "CQWPX_CW") {
        m_activeContest = new CQWPXContest(ModeType::CW);
    } else if (contestInfo.contestType == "CQWPX_SSB") {
        m_activeContest = new CQWPXContest(ModeType::USB);
    } else if (contestInfo.contestType == "WFD") {
        m_activeContest = new WinterFieldDayContest();
    } else {
        qWarning() << "Unknown contest type:" << contestInfo.contestType;
        return;
    }

    // Store contest info
    m_currentContest = contestInfo;
    m_hasActiveContest = true;

    // Reset serial number (will be loaded from DB if resuming existing contest)
    m_nextSerialNumber = 1;

    // Update window title to include contest name
    setWindowTitle(QString("%1 v%2 - %3")
                      .arg(APP_NAME)
                      .arg(APP_VERSION)
                      .arg(contestInfo.contestName));

    // Update exchange fields for this contest
    updateExchangeFieldsForContest();
}

void MainWindow::updateExchangeFieldsForContest() {
    if (!m_activeContest) {
        m_exchangeEntry->setPlaceholderText("No contest selected");
        return;
    }

    // Get exchange fields from contest
    QList<ExchangeField> receivedFields = m_activeContest->getReceivedExchangeFields();

    // Update table model column headers
    m_qsoTableModel->setContestExchangeFields(receivedFields);

    // Build hint text from non-auto fields
    QStringList hints;
    for (const ExchangeField& field : receivedFields) {
        if (!field.autoFill) {
            hints.append(field.hint);
        }
    }

    // Update exchange entry placeholder
    if (hints.isEmpty()) {
        m_exchangeEntry->setPlaceholderText("Exchange");
    } else {
        m_exchangeEntry->setPlaceholderText(hints.join(" "));
    }

    // Clear any existing exchange text
    m_exchangeEntry->clear();
}

void MainWindow::autoPopulateExchange(const QString& callsign) {
    if (!m_activeContest || callsign.isEmpty()) {
        return;
    }

    // Look up callsign in country file
    CountryData countryData = m_countryFile.lookup(callsign);

    // Get exchange fields from contest
    QList<ExchangeField> receivedFields = m_activeContest->getReceivedExchangeFields();

    // Check what fields the contest uses and auto-populate if applicable
    QStringList exchangeParts;

    for (const ExchangeField& field : receivedFields) {
        if (field.name == "Zone" && !field.autoFill) {
            // CQ WW uses CQ Zone - populate from country file
            exchangeParts.append(QString::number(countryData.cqZone));
        } else if (field.name == "ITU Zone" && !field.autoFill) {
            // Some contests might use ITU zone
            exchangeParts.append(QString::number(countryData.ituZone));
        }
        // Note: Other fields like Serial Number, Class, Section cannot be auto-populated
        // from country file - they must be entered by the operator
    }

    // If we have auto-populated data, set it in the exchange field
    if (!exchangeParts.isEmpty()) {
        m_exchangeEntry->setText(exchangeParts.join(" "));
    }
}

// Window menu slot implementations

void MainWindow::onShowDXCluster() {
    if (!m_dxClusterWindow) {
        m_dxClusterWindow = new DXClusterWindow();
        m_dxClusterWindow->setWindowTitle("DX Cluster");
        m_dxClusterWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Connect spot signal to forward spots to band map
        connect(m_dxClusterWindow, &DXClusterWindow::spotReceived,
                this, &MainWindow::onDXSpotReceived);

        // Connect QSY signal to tune radio to clicked frequency
        connect(m_dxClusterWindow, &DXClusterWindow::qsyRequested,
                this, [this](double frequency) {
                    if (m_radioConnected) {
                        qDebug() << "DX Cluster click-to-QSY:" << QString::number(frequency) << "Hz";
                        m_radio->setFrequency(static_cast<freq_t>(frequency));
                    } else {
                        qDebug() << "DX Cluster click-to-QSY: Radio not connected, cannot QSY to" << frequency;
                    }
                });
    }
    m_dxClusterWindow->show();
    m_dxClusterWindow->raise();
    m_dxClusterWindow->activateWindow();
}

void MainWindow::onShowBandMap() {
    if (!m_bandMapWindow) {
        m_bandMapWindow = new BandMapWidget();
        m_bandMapWindow->setWindowTitle("Band Map");
        m_bandMapWindow->setWindowFlags(Qt::Window);
        m_bandMapWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Connect band map signals
        connect(m_bandMapWindow, &BandMapWidget::qsyRequested,
                this, [this](freq_t frequency) {
                    if (m_radioConnected) {
                        qDebug() << "Band Map QSY to" << QString::number(frequency) << "Hz";
                        m_radio->setFrequency(frequency);
                    }
                });

        connect(m_bandMapWindow, &BandMapWidget::callsignSelected,
                this, [this](const QString& callsign) {
                    qDebug() << "Band Map selected callsign:" << callsign;
                    m_callsignEntry->setText(callsign);
                    m_callsignEntry->setFocus();
                });
    }
    m_bandMapWindow->show();
    m_bandMapWindow->raise();
    m_bandMapWindow->activateWindow();
}

void MainWindow::onShowRadioControl() {
    if (!m_radioControlWindow) {
        m_radioControlWindow = new RadioControlWidget();
        m_radioControlWindow->setWindowTitle("Radio Control");
        m_radioControlWindow->setWindowFlags(Qt::Window);
        m_radioControlWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Update with current radio state
        if (m_radioConnected) {
            m_radioControlWindow->updateRadioState(m_currentState);
        }
    }
    m_radioControlWindow->show();
    m_radioControlWindow->raise();
    m_radioControlWindow->activateWindow();
}

void MainWindow::onShowMultipliers() {
    if (!m_multiplierWindow) {
        m_multiplierWindow = new MultiplierWidget();
        m_multiplierWindow->setWindowTitle("Multipliers");
        m_multiplierWindow->setWindowFlags(Qt::Window);
        m_multiplierWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_multiplierWindow->show();
    m_multiplierWindow->raise();
    m_multiplierWindow->activateWindow();
}

void MainWindow::onDXSpotReceived(const QString& callsign,
                                   double frequency,
                                   const QString& spotter,
                                   const QString& comment) {
    qDebug() << "DX Spot received:" << callsign << "at" << frequency << "Hz from" << spotter;

    // If band map window exists, forward the spot to it
    if (m_bandMapWindow) {
        Spot spot;
        spot.callsign = callsign;
        spot.frequency = static_cast<freq_t>(frequency);  // Already in Hz from TelnetClient
        spot.timestamp = QDateTime::currentDateTime();
        spot.isMultiplier = false;  // TODO: Check if this is a needed multiplier
        spot.isWorked = false;       // TODO: Check if we've worked this station
        spot.source = QString("DX Cluster (%1)").arg(spotter);

        m_bandMapWindow->addSpot(spot);
        qDebug() << "Added spot to band map:" << callsign;
    } else {
        qDebug() << "Band map window not open - spot not added";
    }
}

void MainWindow::onBandClicked(BandType band) {
    if (!m_radioConnected) {
        qDebug() << "Cannot change band: radio not connected";
        return;
    }

    // Get frequency for the clicked band based on current mode
    freq_t frequency = getFrequencyForBand(band, m_currentState.modeA);

    qDebug() << "Band clicked:" << static_cast<int>(band)
             << "Setting frequency to:" << QString::number(frequency) << "Hz";

    // Send frequency change to radio
    m_radio->setFrequency(frequency);
}

void MainWindow::onBandUp() {
    // TODO: Implement band up
    qDebug() << "Band up";
}

void MainWindow::onBandDown() {
    // TODO: Implement band down
    qDebug() << "Band down";
}

freq_t MainWindow::getFrequencyForBand(BandType band, ModeType mode) const {
    // Determine if we're in a CW mode
    bool isCW = (mode == ModeType::CW || mode == ModeType::RTTY);

    // Return appropriate frequency for band and mode
    // CW portions are typically lower in frequency than SSB
    switch (band) {
    case BandType::Band160M:
        return isCW ? 1830000 : 1850000;  // 1.830 MHz CW, 1.850 MHz SSB
    case BandType::Band80M:
        return isCW ? 3530000 : 3750000;  // 3.530 MHz CW, 3.750 MHz SSB
    case BandType::Band40M:
        return isCW ? 7030000 : 7200000;  // 7.030 MHz CW, 7.200 MHz SSB
    case BandType::Band20M:
        return isCW ? 14030000 : 14200000;  // 14.030 MHz CW, 14.200 MHz SSB
    case BandType::Band15M:
        return isCW ? 21030000 : 21300000;  // 21.030 MHz CW, 21.300 MHz SSB
    case BandType::Band10M:
        return isCW ? 28030000 : 28400000;  // 28.030 MHz CW, 28.400 MHz SSB
    default:
        return 14030000;  // Default to 20m CW
    }
}

} // namespace TR4QT
