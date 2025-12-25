#include "MainWindow.h"
#include "dialogs/RadioConfigDialog.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/BackupRestoreDialog.h"
#include "dialogs/OperatorDialog.h"
#include "dialogs/EditQSODialog.h"
#include "widgets/DXClusterWindow.h"
#include "widgets/BandMapWidget.h"
#include "widgets/RadioControlWidget.h"
#include "widgets/MultiplierWidget.h"
#include "widgets/StatisticsWindow.h"
#include "../network/UdpBroadcastManager.h"
#include "../core/Constants.h"
#include "../logging/LogMacros.h"
#include "../utils/ThemeManager.h"
#include "../utils/ADIFExporter.h"
#include "../utils/CabrilloExporter.h"
#include "../utils/CountryFileDownloader.h"
#include "../utils/LOTWUserDownloader.h"
#include "../data/Database.h"
#include "../data/QSORepository.h"
#include "../data/LOTWUserRepository.h"
#include "../data/BackupManager.h"
#include "../data/ExchangeMemoryRepository.h"
#include <QFile>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QSettings>
#include <QPushButton>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QTextEdit>

namespace TR4QT {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_radio(new RadioController(this))
    , m_radioConnected(false)
    , m_dxClusterWindow(nullptr)
    , m_bandMapWindow(nullptr)
    , m_radioControlWindow(nullptr)
    , m_multiplierWindow(nullptr)
    , m_statisticsWindow(nullptr)
    , m_qsosThisHour(0)
    , m_hasActiveContest(false)
    , m_activeContest(nullptr)
    , m_currentContestDbId(-1)
    , m_nextSerialNumber(1)
    , m_udpBroadcastManager(new UdpBroadcastManager(this))
    , m_inRaiseAllWindows(false)
    , m_initialExchangePopulated(false)
{
    setWindowTitle(QString("%1 v%2").arg(APP_NAME).arg(APP_VERSION));
    setWindowIcon(QIcon(":/icons/tr4qt.png"));

    // Load country file for exchange auto-population
    QString countryFilePath = AppSettings::instance().getCountryFilePath();
    if (QFile::exists(countryFilePath)) {
        if (!m_countryFile.loadFromFile(countryFilePath)) {
            LOG_WARN("MainWindow", QString("Failed to load country file: %1").arg(countryFilePath));
        }
    }

    setupUI();
    loadSettings();
    loadUdpBroadcastSettings();

    // Initialize backup manager from settings
    loadBackupSettings();

    // Install event filter to raise all windows when any window is activated
    qApp->installEventFilter(this);

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

    // Check if grid square is configured (needed for azimuth/distance calculations)
    // Delay check to let UI fully initialize
    QTimer::singleShot(1000, this, [this]() {
        AppSettings& settings = AppSettings::instance();
        QString gridSquare = settings.getMyGridSquare();

        if (gridSquare.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setWindowTitle("Grid Square Not Configured");
            msgBox.setText("Your Maidenhead grid square is not configured.");
            msgBox.setInformativeText("The grid square is needed to calculate distance and azimuth "
                                     "to DX spots in the band map.\n\n"
                                     "Would you like to configure it now in Preferences?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);

            if (msgBox.exec() == QMessageBox::Yes) {
                onPreferences();
            }
        }
    });
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
        LOG_DEBUG("MainWindow", "*** Preferences action triggered ***");
        onPreferences();
    });
    LOG_DEBUG("MainWindow", QString("*** Preferences menu created with shortcut: %1 menuRole: %2").arg(preferencesAction->shortcut().toString()).arg(preferencesAction->menuRole()));

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // Radio menu
    QMenu* radioMenu = menuBar->addMenu("&Radio");

    QAction* configAction = radioMenu->addAction("&Configure...");
    configAction->setMenuRole(QAction::NoRole);  // Prevent macOS from moving this to app menu
    connect(configAction, &QAction::triggered, this, [this]() {
        LOG_DEBUG("MainWindow", "*** Radio Configure action triggered ***");
        onRadioConfigure();
    });
    LOG_DEBUG("MainWindow", QString("*** Radio Configure menu created with menuRole: %1").arg(configAction->menuRole()));

    radioMenu->addSeparator();

    m_connectAction = radioMenu->addAction("C&onnect");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::onRadioConnect);

    m_disconnectAction = radioMenu->addAction("&Disconnect");
    m_disconnectAction->setEnabled(false);
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::onRadioDisconnect);

    // Edit menu (CTRL- shortcuts matching TR4W)
    QMenu* editMenu = menuBar->addMenu("&Edit");

    QAction* viewEditLogAction = editMenu->addAction("View/&Edit Log");
    viewEditLogAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(viewEditLogAction, &QAction::triggered, this, &MainWindow::onViewEditLog);

    QAction* clearDupesAction = editMenu->addAction("Clear &Dupes");
    clearDupesAction->setShortcut(QKeySequence("Ctrl+K"));
    connect(clearDupesAction, &QAction::triggered, this, &MainWindow::onClearDupes);

    QAction* noteAction = editMenu->addAction("&Note");
    noteAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(noteAction, &QAction::triggered, this, &MainWindow::onNote);

    QAction* recallLastAction = editMenu->addAction("&Recall Last Entry");
    recallLastAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(recallLastAction, &QAction::triggered, this, &MainWindow::onRecallLast);

    // Tools menu (ALT- shortcuts matching TR4W)
    QMenu* toolsMenu = menuBar->addMenu("&Tools");

    QAction* wkModeAction = toolsMenu->addAction("WK Mode (Re-initialize WinKeyer)");
    wkModeAction->setShortcut(QKeySequence("Alt+A"));
    connect(wkModeAction, &QAction::triggered, this, &MainWindow::onWKMode);

    QAction* backupLogAction = toolsMenu->addAction("Backup Log");
    backupLogAction->setShortcut(QKeySequence("Alt+F"));
    connect(backupLogAction, &QAction::triggered, this, &MainWindow::onBackupLog);

    QAction* downloadCTYAction = toolsMenu->addAction("Download CTY.dat");
    downloadCTYAction->setShortcut(QKeySequence("Alt+O"));
    connect(downloadCTYAction, &QAction::triggered, this, &MainWindow::onDownloadCTY);

    QAction* downloadLOTWAction = toolsMenu->addAction("Download LOTW Users");
    downloadLOTWAction->setShortcut(QKeySequence("Alt+L"));
    connect(downloadLOTWAction, &QAction::triggered, this, &MainWindow::onDownloadLOTW);

    QAction* setDateTimeAction = toolsMenu->addAction("Set System Date/Time");
    setDateTimeAction->setShortcut(QKeySequence("Alt+T"));
    connect(setDateTimeAction, &QAction::triggered, this, &MainWindow::onSetDateTime);

    QAction* initializeAction = toolsMenu->addAction("Initialize");
    initializeAction->setShortcut(QKeySequence("Alt+W"));
    connect(initializeAction, &QAction::triggered, this, &MainWindow::onInitialize);

    toolsMenu->addSeparator();

    QAction* optionsAction = toolsMenu->addAction("Options");
    optionsAction->setShortcut(QKeySequence("Ctrl+J"));
    connect(optionsAction, &QAction::triggered, this, &MainWindow::onPreferences);

    // Operating menu (ALT- shortcuts for operating functions)
    QMenu* operatingMenu = menuBar->addMenu("&Operating");

    QAction* autoCQAction = operatingMenu->addAction("Auto CQ");
    autoCQAction->setShortcut(QKeySequence("Alt+Q"));
    connect(autoCQAction, &QAction::triggered, this, &MainWindow::onAutoCQ);

    QAction* autoCQResumeAction = operatingMenu->addAction("Auto CQ Resume");
    autoCQResumeAction->setShortcut(QKeySequence("Alt+C"));
    connect(autoCQResumeAction, &QAction::triggered, this, &MainWindow::onAutoCQResume);

    QAction* killCWAction = operatingMenu->addAction("Kill CW");
    killCWAction->setShortcut(QKeySequence("Alt+K"));
    connect(killCWAction, &QAction::triggered, this, &MainWindow::onKillCW);

    operatingMenu->addSeparator();

    QAction* dupeCheckAction = operatingMenu->addAction("Dupe Check");
    dupeCheckAction->setShortcut(QKeySequence("Alt+D"));
    connect(dupeCheckAction, &QAction::triggered, this, &MainWindow::onDupeCheck);

    QAction* searchLogAction = operatingMenu->addAction("Search Log");
    searchLogAction->setShortcut(QKeySequence("Alt+L"));
    connect(searchLogAction, &QAction::triggered, this, &MainWindow::onSearchLog);

    QAction* deleteLastQSOAction = operatingMenu->addAction("Delete Last QSO");
    deleteLastQSOAction->setShortcut(QKeySequence("Alt+Y"));
    connect(deleteLastQSOAction, &QAction::triggered, this, &MainWindow::onDeleteLastQSO);

    operatingMenu->addSeparator();

    QAction* incNumberAction = operatingMenu->addAction("Inc Number");
    incNumberAction->setShortcut(QKeySequence("Alt+I"));
    connect(incNumberAction, &QAction::triggered, this, &MainWindow::onIncNumber);

    QAction* initialExchangeAction = operatingMenu->addAction("Initial Exchange");
    initialExchangeAction->setShortcut(QKeySequence("Alt+Z"));
    connect(initialExchangeAction, &QAction::triggered, this, &MainWindow::onInitialExchange);

    QAction* cwSpeedAction = operatingMenu->addAction("CW Speed");
    cwSpeedAction->setShortcut(QKeySequence("Alt+S"));
    connect(cwSpeedAction, &QAction::triggered, this, &MainWindow::onCWSpeed);

    operatingMenu->addSeparator();

    QAction* toggleSidetoneAction = operatingMenu->addAction("Toggle Sidetone");
    toggleSidetoneAction->setShortcut(QKeySequence("Alt+="));
    connect(toggleSidetoneAction, &QAction::triggered, this, &MainWindow::onToggleSidetone);

    QAction* toggleAutosendAction = operatingMenu->addAction("Toggle Autosend");
    toggleAutosendAction->setShortcut(QKeySequence("Alt+-"));
    connect(toggleAutosendAction, &QAction::triggered, this, &MainWindow::onToggleAutosend);

    // Window menu (window display functions)
    QMenu* windowMenu = menuBar->addMenu("&Window");

    QAction* bandMapAction = windowMenu->addAction("&Band Map");
    connect(bandMapAction, &QAction::triggered, this, &MainWindow::onShowBandMap);

    QAction* dxClusterAction = windowMenu->addAction("DX &Cluster");
    connect(dxClusterAction, &QAction::triggered, this, &MainWindow::onShowDXCluster);

    QAction* radioControlAction = windowMenu->addAction("&Radio Control");
    connect(radioControlAction, &QAction::triggered, this, &MainWindow::onShowRadioControl);

    QAction* multipliersAction = windowMenu->addAction("&Multipliers");
    connect(multipliersAction, &QAction::triggered, this, &MainWindow::onShowMultipliers);

    QAction* statisticsAction = windowMenu->addAction("&Statistics");
    connect(statisticsAction, &QAction::triggered, this, &MainWindow::onShowStatistics);

    windowMenu->addSeparator();

    QAction* swapMultViewAction = windowMenu->addAction("Swap Mult View");
    swapMultViewAction->setShortcut(QKeySequence("Alt+G"));
    connect(swapMultViewAction, &QAction::triggered, this, &MainWindow::onSwapMultView);

    QAction* missingMultsAction = windowMenu->addAction("Missing Mults Report");
    missingMultsAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(missingMultsAction, &QAction::triggered, this, &MainWindow::onMissingMultsReport);

    // Band menu (band changing shortcuts)
    QMenu* bandMenu = menuBar->addMenu("&Band");

    QAction* bandUpAction = bandMenu->addAction("Band Up");
    bandUpAction->setShortcut(QKeySequence("Alt+B"));
    connect(bandUpAction, &QAction::triggered, this, &MainWindow::onBandUp);

    QAction* bandDownAction = bandMenu->addAction("Band Down");
    bandDownAction->setShortcut(QKeySequence("Alt+V"));
    connect(bandDownAction, &QAction::triggered, this, &MainWindow::onBandDown);

    bandMenu->addSeparator();

    QAction* toggleRigsAction = bandMenu->addAction("Toggle Rigs (SO2R)");
    toggleRigsAction->setShortcut(QKeySequence("Alt+R"));
    connect(toggleRigsAction, &QAction::triggered, this, &MainWindow::onToggleRigs);

    QAction* editSO2RAction = bandMenu->addAction("Edit SO2R");
    editSO2RAction->setShortcut(QKeySequence("Alt+E"));
    connect(editSO2RAction, &QAction::triggered, this, &MainWindow::onEditSO2R);

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
    m_bandSummaryGrid->setEnabled(false);  // Start disabled (radio not connected)
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

    // Connect double-click to edit QSO
    connect(m_qsoTableView, &QTableView::doubleClicked, this, &MainWindow::onEditQSO);

    // Enable context menu
    m_qsoTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_qsoTableView, &QTableView::customContextMenuRequested, this, &MainWindow::onQSOTableContextMenu);

    mainLayout->addWidget(m_qsoTableView, 1);  // Stretch factor 1 = takes remaining space

    // Bottom: Entry and stats panel (with radio status on left)
    QWidget* bottomPanel = createBottomPanel();
    mainLayout->addWidget(bottomPanel);

    setCentralWidget(central);
}

QWidget* MainWindow::createBottomPanel() {
    QWidget* bottomPanel = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setSpacing(15);
    bottomLayout->setContentsMargins(10, 4, 10, 4);

    // LEFT: Radio status (frequency, band/mode, time) in container widget
    QWidget* radioStatusWidget = new QWidget(this);
    QHBoxLayout* radioLayout = new QHBoxLayout(radioStatusWidget);
    radioLayout->setSpacing(15);
    radioLayout->setContentsMargins(10, 5, 10, 5);

    QFont labelFont("Monospace", 11);
    labelFont.setBold(true);

    // Band/Mode label (e.g., "15SSB")
    m_radioFreqBandLabel = new QLabel("--", radioStatusWidget);
    m_radioFreqBandLabel->setFont(labelFont);
    m_radioFreqBandLabel->setMinimumWidth(80);
    m_radioFreqBandLabel->setAlignment(Qt::AlignCenter);

    // Frequency label
    QFont freqFont("Monospace", 10);
    m_radioFreqLabel = new QLabel("0.000 MHz", radioStatusWidget);
    m_radioFreqLabel->setFont(freqFont);
    m_radioFreqLabel->setMinimumWidth(100);
    m_radioFreqLabel->setAlignment(Qt::AlignCenter);

    // Vertical layout for band/mode and frequency
    QVBoxLayout* freqLayout = new QVBoxLayout();
    freqLayout->setSpacing(2);
    freqLayout->addWidget(m_radioFreqBandLabel);
    freqLayout->addWidget(m_radioFreqLabel);

    // Date/Time label - use monospace font and fixed width to prevent jumping
    m_radioDateTimeLabel = new QLabel("", radioStatusWidget);
    QFont dateTimeFont("Monospace", labelFont.pointSize());
    m_radioDateTimeLabel->setFont(dateTimeFont);
    m_radioDateTimeLabel->setAlignment(Qt::AlignCenter);
    m_radioDateTimeLabel->setMinimumWidth(220);  // Wide enough for "Wed 25-Dec-2025 13:45:30"

    radioLayout->addLayout(freqLayout);
    radioLayout->addWidget(m_radioDateTimeLabel);

    bottomLayout->addWidget(radioStatusWidget);

    // Stretch to push Call/Exch to center
    bottomLayout->addStretch(1);

    // CENTER: Entry fields (vertical layout)
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

    bottomLayout->addWidget(entryWidget);

    // Stretch to push Stats to right
    bottomLayout->addStretch(1);

    // Right side: Stats panel
    QWidget* statsWidget = new QWidget(this);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setSpacing(2);
    statsLayout->setContentsMargins(4, 4, 4, 4);

    AppSettings& settings = AppSettings::instance();
    int miscFontSize = settings.getMiscDisplayFontSize();
    QFont monoFont("Monospace", miscFontSize);

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
    m_operatorLabelStatic = new QLabel("Op:", this);
    m_operatorLabelStatic->setFont(monoFont);
    m_operatorLabel = new QLabel("", this);
    m_operatorLabel->setFont(monoFont);
    opRow->addWidget(m_operatorLabelStatic);
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

    // Connect to theme changes and apply initial theme
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::applyTheme);
    applyTheme();

    // Restore child windows if they were visible
    LOG_DEBUG("MainWindow", QString("DX Cluster was visible: %1").arg(settings.getDXClusterVisible() ? "true" : "false"));
    if (settings.getDXClusterVisible()) {
        LOG_DEBUG("MainWindow", "Restoring DX Cluster window");
        onShowDXCluster();
        QByteArray dxGeometry = settings.loadDXClusterGeometry();
        if (!dxGeometry.isEmpty()) {
            m_dxClusterWindow->restoreGeometry(dxGeometry);
        }
    }

    LOG_DEBUG("MainWindow", QString("Band Map was visible: %1").arg(settings.getBandMapVisible() ? "true" : "false"));
    if (settings.getBandMapVisible()) {
        LOG_DEBUG("MainWindow", "Restoring Band Map window");
        onShowBandMap();
        QByteArray bmGeometry = settings.loadBandMapGeometry();
        if (!bmGeometry.isEmpty()) {
            m_bandMapWindow->restoreGeometry(bmGeometry);
        }
    }

    LOG_DEBUG("MainWindow", QString("Radio Control was visible: %1").arg(settings.getRadioControlVisible() ? "true" : "false"));
    if (settings.getRadioControlVisible()) {
        LOG_DEBUG("MainWindow", "Restoring Radio Control window");
        onShowRadioControl();
        QByteArray rcGeometry = settings.loadRadioControlGeometry();
        if (!rcGeometry.isEmpty()) {
            m_radioControlWindow->restoreGeometry(rcGeometry);
        }
    }

    LOG_DEBUG("MainWindow", QString("Multipliers was visible: %1").arg(settings.getMultipliersVisible() ? "true" : "false"));
    if (settings.getMultipliersVisible()) {
        LOG_DEBUG("MainWindow", "Restoring Multipliers window");
        onShowMultipliers();
        QByteArray multGeometry = settings.loadMultipliersGeometry();
        if (!multGeometry.isEmpty()) {
            m_multiplierWindow->restoreGeometry(multGeometry);
        }
    }

    // Load and display current operator
    QString currentOperator = settings.getCurrentOperator();
    if (!currentOperator.isEmpty()) {
        m_operatorLabel->setText(currentOperator);
    }
}

void MainWindow::saveSettings() {
    AppSettings& settings = AppSettings::instance();
    settings.saveWindowGeometry(saveGeometry());
    settings.saveWindowState(saveState());

    // Save child window geometry and visibility states
    if (m_dxClusterWindow) {
        bool visible = m_dxClusterWindow->isVisible();
        LOG_DEBUG("MainWindow", QString("Saving DX Cluster window - visible: %1").arg(visible ? "true" : "false"));
        settings.saveDXClusterGeometry(m_dxClusterWindow->saveGeometry());
        settings.setDXClusterVisible(visible);
    }

    if (m_bandMapWindow) {
        bool visible = m_bandMapWindow->isVisible();
        LOG_DEBUG("MainWindow", QString("Saving Band Map window - visible: %1").arg(visible ? "true" : "false"));
        settings.saveBandMapGeometry(m_bandMapWindow->saveGeometry());
        settings.setBandMapVisible(visible);
    }

    if (m_radioControlWindow) {
        bool visible = m_radioControlWindow->isVisible();
        LOG_DEBUG("MainWindow", QString("Saving Radio Control window - visible: %1").arg(visible ? "true" : "false"));
        settings.saveRadioControlGeometry(m_radioControlWindow->saveGeometry());
        settings.setRadioControlVisible(visible);
    }

    if (m_multiplierWindow) {
        bool visible = m_multiplierWindow->isVisible();
        LOG_DEBUG("MainWindow", QString("Saving Multipliers window - visible: %1").arg(visible ? "true" : "false"));
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

    // Save band map spots to database before closing
    if (m_bandMapWindow) {
        m_bandMapWindow->saveSpotsToDatabase();
    }

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

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // Catch WindowActivate events on any of our windows
    if (event->type() == QEvent::WindowActivate) {
        // Check if the activated window belongs to our application
        QWidget* widget = qobject_cast<QWidget*>(obj);
        if (widget && widget->isWindow()) {
            // Only raise windows when one of the CHILD windows is activated
            // Don't do it when MainWindow itself is activated (to avoid stealing focus)
            if (widget == m_dxClusterWindow ||
                widget == m_bandMapWindow ||
                widget == m_radioControlWindow ||
                widget == m_multiplierWindow ||
                widget == m_statisticsWindow) {
                // Raise all windows to bring them all to front
                raiseAllWindows();
            }
        }
    }

    // Pass event to base class
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::raiseAllWindows() {
    // Prevent infinite recursion - raising windows triggers WindowActivate events
    // which would call this function again
    if (m_inRaiseAllWindows) {
        return;
    }

    m_inRaiseAllWindows = true;

    // Raise main window
    raise();
    activateWindow();

    // Raise all child windows that are visible
    if (m_dxClusterWindow && m_dxClusterWindow->isVisible()) {
        m_dxClusterWindow->raise();
        m_dxClusterWindow->activateWindow();
    }

    if (m_bandMapWindow && m_bandMapWindow->isVisible()) {
        m_bandMapWindow->raise();
        m_bandMapWindow->activateWindow();
    }

    if (m_radioControlWindow && m_radioControlWindow->isVisible()) {
        m_radioControlWindow->raise();
        m_radioControlWindow->activateWindow();
    }

    if (m_multiplierWindow && m_multiplierWindow->isVisible()) {
        m_multiplierWindow->raise();
        m_multiplierWindow->activateWindow();
    }

    if (m_statisticsWindow && m_statisticsWindow->isVisible()) {
        m_statisticsWindow->raise();
        m_statisticsWindow->activateWindow();
    }

    m_inRaiseAllWindows = false;
}

void MainWindow::onRadioConfigure() {
    LOG_DEBUG("MainWindow", "*** onRadioConfigure() called - opening RadioConfigDialog ***");
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
    LOG_DEBUG("MainWindow", "*** onPreferences() called - opening PreferencesDialog ***");

    // Save current radio config to detect changes
    AppSettings& settings = AppSettings::instance();
    RadioConfig oldConfig;
    bool hadRadioConfig = settings.hasRadioConfig();
    if (hadRadioConfig) {
        oldConfig = settings.loadRadioConfig();
    }
    bool oldAutoConnect = settings.getRadioAutoConnect();

    PreferencesDialog dialog(this);

    // Connect LOTW settings change to refresh Band Map in real-time
    connect(&dialog, &PreferencesDialog::lotwSettingsChanged,
            this, [this]() {
                if (m_bandMapWindow) {
                    LOG_DEBUG("MainWindow", "LOTW settings changed - refreshing Band Map");
                    m_bandMapWindow->refreshLotwStatus();
                }
            });

    if (dialog.exec() == QDialog::Accepted) {
        m_statusLabel->setText("Preferences saved");

        // Apply font size changes immediately
        applyFontSettings();

        // Reload UDP broadcast settings
        loadUdpBroadcastSettings();

        // Check if radio settings actually changed
        bool radioSettingsChanged = false;
        if (settings.hasRadioConfig()) {
            RadioConfig newConfig = settings.loadRadioConfig();
            bool autoConnectChanged = (oldAutoConnect != settings.getRadioAutoConnect());

            // Compare configs
            if (!hadRadioConfig) {
                radioSettingsChanged = true;  // New config added
            } else if (oldConfig.hamlibModelId != newConfig.hamlibModelId ||
                       oldConfig.port != newConfig.port ||
                       oldConfig.baudRate != newConfig.baudRate ||
                       oldConfig.civAddress != newConfig.civAddress ||
                       oldConfig.pollInterval != newConfig.pollInterval ||
                       autoConnectChanged) {
                radioSettingsChanged = true;
            }
        } else if (hadRadioConfig) {
            radioSettingsChanged = true;  // Config was removed
        }

        // Only ask to reconnect if radio settings actually changed
        if (radioSettingsChanged && m_radioConnected) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Reconnect Radio?",
                "Radio settings have changed. Reconnect to apply new settings?",
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
    LOG_DEBUG("MainWindow", QString("MainWindow::onRadioConnected called with connected = %1").arg(connected ? "true" : "false"));
    m_radioConnected = connected;
    updateConnectionStatus(connected);

    if (connected) {
        m_statusLabel->setText("Connected to radio (waiting for state...)");

        // Enable band buttons when radio connected
        if (m_bandSummaryGrid) {
            m_bandSummaryGrid->setEnabled(true);
        }
    } else {
        m_statusLabel->setText("Radio disconnected");

        // Clear radio control display when disconnected
        if (m_radioControlWindow) {
            m_radioControlWindow->clearDisplay();
        }

        // Disable band buttons when radio disconnected
        if (m_bandSummaryGrid) {
            m_bandSummaryGrid->setEnabled(false);
        }
    }
}

void MainWindow::onRadioStateUpdated(const RadioState& state) {
    // Send UDP broadcast for radio state change (throttled in manager)
    QString stationCall = AppSettings::instance().getMyCallsign();
    m_udpBroadcastManager->onRadioStateChanged(state, stationCall);

    // Log radio model if it changed
    static QString lastModel;
    if (!state.radioModel.isEmpty() && state.radioModel != lastModel) {
        LOG_DEBUG("MainWindow", QString("MainWindow: Radio model from state: %1").arg(state.radioModel));
        m_statusLabel->setText(QString("Radio: %1").arg(state.radioModel));
        lastModel = state.radioModel;
    }

    m_currentState = state;
    // Radio state is cached for use when logging QSOs

    // Update radio status grid with new state
    updateRadioStatusGrid();

    // Update radio control window if it exists (even if not visible, so state is current when shown)
    if (m_radioControlWindow) {
        m_radioControlWindow->updateRadioState(state);
    }

    // Update Band Map with current frequency for band filtering
    if (m_bandMapWindow) {
        m_bandMapWindow->setCurrentFrequency(state.frequencyA);
    }
}

void MainWindow::onRadioError(const QString& error) {
    m_statusLabel->setText(QString("Radio error: %1").arg(error));
}

void MainWindow::onRadioModelChanged(const QString& model) {
    LOG_DEBUG("MainWindow", QString("MainWindow::onRadioModelChanged: %1").arg(model));
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

    // Check for OPON command (change operator)
    if (callsign == "OPON") {
        OperatorDialog dialog(this);

        // Pre-populate with current operator
        AppSettings& settings = AppSettings::instance();
        dialog.setOperatorCallsign(settings.getCurrentOperator());

        if (dialog.exec() == QDialog::Accepted) {
            QString newOperator = dialog.getOperatorCallsign();
            if (!newOperator.isEmpty()) {
                settings.setCurrentOperator(newOperator);
                m_operatorLabel->setText(newOperator);  // Update operator display
                m_statusLabel->setText(QString("Operator changed to: %1").arg(newOperator));
                LOG_INFO("MainWindow", QString("Operator changed to: %1").arg(newOperator));
            } else {
                m_statusLabel->setText("Operator change cancelled (empty callsign)");
            }
        } else {
            m_statusLabel->setText("Operator change cancelled");
        }

        // Clear entry fields and focus callsign
        onClearEntry();
        return;
    }

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

    // Validate exchange against contest rules
    if (m_activeContest) {
        QString errorMsg;
        if (!m_activeContest->validateReceivedExchange(exchange, errorMsg)) {
            m_statusLabel->setText(QString("Invalid exchange: %1").arg(errorMsg));
            m_exchangeEntry->setFocus();
            m_exchangeEntry->selectAll();
            return;
        }
    }

    // Create QSO object with current radio state (snapshot!)
    QSO qso;
    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.callsign = callsign;
    qso.operatorCall = AppSettings::instance().getCurrentOperator();

    // Snapshot radio state
    qso.frequency = m_currentState.frequencyA;
    qso.mode = m_currentState.modeA;
    qso.band = m_currentState.bandA;

    // Exchange
    qso.rstSent = (qso.mode == ModeType::CW) ? "599" : "59";
    qso.exchangeReceived = exchange;

    // Parse exchange into components
    if (m_activeContest) {
        qso.parsedExchange = m_activeContest->parseReceivedExchange(exchange);

        // Extract RST from parsed exchange if present
        if (qso.parsedExchange.contains("RST")) {
            qso.rstReceived = qso.parsedExchange["RST"];
        } else {
            // Use default if RST not in exchange
            qso.rstReceived = (qso.mode == ModeType::CW) ? "599" : "59";
        }
    } else {
        // No active contest - use default RST
        qso.rstReceived = (qso.mode == ModeType::CW) ? "599" : "59";
    }

    // Serial number handling
    if (m_activeContest && m_activeContest->usesSerialNumbers()) {
        qso.serialNumber = m_nextSerialNumber++;
        qso.exchangeSent = QString::number(qso.serialNumber);
    } else {
        qso.exchangeSent = "";  // Will be set by contest rules later
    }

    // TODO: Lookup country/zone from cty.dat
    // TODO: Check for dupe
    // TODO: Calculate points via contest
    // TODO: Check for new multipliers

    // Add to table model (UI)
    m_qsoTableModel->addQSO(qso);

    // Scroll to show the newly logged QSO
    m_qsoTableView->scrollToBottom();

    // Save to database if contest is active
    if (m_hasActiveContest) {
        QSORepository repo;
        if (!repo.saveQSO(qso, m_currentContestDbId)) {
            LOG_WARN("MainWindow", QString("Failed to save QSO to database: %1").arg(repo.lastError()));
            m_statusLabel->setText("Warning: QSO logged but not saved to database");
            // Continue anyway - QSO is in table model
        } else {
            LOG_DEBUG("MainWindow", QString("QSO saved to database with ID: %1").arg(qso.id));

            // Save exchange to memory for future auto-population
            if (m_activeContest && !qso.exchangeReceived.isEmpty()) {
                ExchangeMemoryEntry memEntry;
                memEntry.callsign = qso.callsign;
                memEntry.exchange = qso.exchangeReceived;
                memEntry.contestType = m_activeContest->getContestId();
                memEntry.mode = qso.mode;
                memEntry.timestamp = QDateTime::currentDateTime();
                memEntry.source = m_initialExchangePopulated ? "auto" : "manual";
                memEntry.hitCount = 0;

                ExchangeMemoryRepository memRepo;
                if (!memRepo.save(memEntry)) {
                    LOG_WARN("MainWindow", QString("Failed to save exchange memory: %1")
                             .arg(memRepo.lastError()));
                }
            }

            // Auto-backup check (if enabled)
            int currentQSOCount = repo.getQSOCount(m_currentContestDbId);
            BackupManager::instance().autoBackupIfNeeded(
                m_currentContest.databasePath,
                currentQSOCount);
        }
    }

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
    m_initialExchangePopulated = false;
}

void MainWindow::onEditQSO(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    int row = index.row();

    // Get the QSO at the selected row
    QSO qso = m_qsoTableModel->getQSO(row);
    if (qso.id < 0) {
        QMessageBox::warning(this, "Error", "Cannot edit QSO: Invalid QSO ID");
        return;
    }

    // Open edit dialog
    EditQSODialog dialog(qso, this);
    if (dialog.exec() == QDialog::Accepted) {
        QSO editedQSO = dialog.getEditedQSO();

        // Update in database
        QSORepository repo;

        if (repo.updateQSO(editedQSO)) {
            // Update the table model at the specific row
            m_qsoTableModel->updateQSO(row, editedQSO);

            LOG_INFO("MainWindow", QString("Updated QSO #%1 (%2)")
                .arg(editedQSO.id)
                .arg(editedQSO.callsign));
        } else {
            QMessageBox::warning(this, "Error",
                QString("Failed to update QSO: %1").arg(repo.lastError()));
        }
    }
}

void MainWindow::onQSOTableContextMenu(const QPoint& pos) {
    QModelIndex index = m_qsoTableView->indexAt(pos);
    if (!index.isValid()) {
        return;  // No QSO at click position
    }

    // Create context menu
    QMenu menu(this);
    QAction* editAction = menu.addAction("Edit QSO");
    connect(editAction, &QAction::triggered, [this, index]() {
        onEditQSO(index);
    });

    // Show menu at cursor position
    menu.exec(m_qsoTableView->viewport()->mapToGlobal(pos));
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
    QDateTime hourStart = QDateTime(now.date(), QTime(now.time().hour(), 0), QTimeZone::UTC);

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

    // Apply misc display font size (stats panel: This Hr, Rate, Op, etc.)
    int miscFontSize = settings.getMiscDisplayFontSize();
    QFont miscFont("Monospace", miscFontSize);
    m_timeLabel->setFont(miscFont);
    m_thisHrLabel->setFont(miscFont);
    m_rateLabel->setFont(miscFont);
    m_cqCountLabel->setFont(miscFont);
    m_spCountLabel->setFont(miscFont);
    m_operatorLabelStatic->setFont(miscFont);
    m_operatorLabel->setFont(miscFont);
}

void MainWindow::applyTheme() {
    ThemeManager& theme = ThemeManager::instance();

    // Update radio status widget background
    QWidget* radioStatusWidget = m_radioFreqBandLabel->parentWidget();
    if (radioStatusWidget) {
        radioStatusWidget->setStyleSheet(QString("QWidget { background-color: %1; }")
            .arg(theme.color(ColorRole::WindowBackground).name()));
    }

    // Update radio status labels
    QString labelStyle = QString("QLabel { background-color: %1; padding: 5px; border: 1px solid %2; border-radius: 3px; }")
        .arg(theme.color(ColorRole::TextDisplayBackground).name())
        .arg(theme.color(ColorRole::BorderColor).name());

    m_radioFreqBandLabel->setStyleSheet(labelStyle);
    m_radioDateTimeLabel->setStyleSheet(labelStyle);

    QString freqLabelStyle = QString("QLabel { background-color: %1; padding: 3px; border: 1px solid %2; border-radius: 3px; }")
        .arg(theme.color(ColorRole::TextDisplayBackground).name())
        .arg(theme.color(ColorRole::BorderColor).name());

    m_radioFreqLabel->setStyleSheet(freqLabelStyle);
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

    LOG_DEBUG("MainWindow", QString("UDP Broadcast settings loaded: Enabled=%1 RadioInfo=%2 ContactInfo=%3 Destinations=%4")
        .arg(settings.getUDPBroadcastEnabled() ? "true" : "false")
        .arg(settings.getUDPRadioInfoEnabled() ? "true" : "false")
        .arg(settings.getUDPContactInfoEnabled() ? "true" : "false")
        .arg(destinations.size()));
}

void MainWindow::loadBackupSettings() {
    AppSettings& settings = AppSettings::instance();
    BackupManager& backup = BackupManager::instance();

    // Configure backup manager from settings
    backup.setAutoBackupEnabled(settings.getAutoBackupEnabled());
    backup.setAutoBackupInterval(settings.getAutoBackupInterval());
    backup.setBackupDirectory(settings.getBackupDirectory());
    backup.setMaxBackups(settings.getMaxBackups());

    LOG_DEBUG("MainWindow", QString("Backup settings loaded: Enabled=%1 Interval=%2 Directory=%3 MaxBackups=%4")
        .arg(settings.getAutoBackupEnabled() ? "true" : "false")
        .arg(settings.getAutoBackupInterval())
        .arg(settings.getBackupDirectory())
        .arg(settings.getMaxBackups()));
}

void MainWindow::activateContest(const ContestInfo& contestInfo) {
    // Clean up previous contest if any
    if (m_activeContest) {
        delete m_activeContest;
        m_activeContest = nullptr;
    }

    // Open database
    Database& db = Database::instance();
    if (!db.open(contestInfo.databasePath)) {
        QMessageBox::critical(this, "Database Error",
                            QString("Failed to open database:\n%1").arg(db.lastError()));
        return;
    }

    LOG_DEBUG("MainWindow", QString("Database opened: %1").arg(contestInfo.databasePath));

    // Find or create contest record
    QSqlQuery query = db.execute(
        "SELECT id, current_serial FROM contests WHERE contest_id = ?",
        {contestInfo.contestId});

    if (query.next()) {
        // Existing contest - load data
        m_currentContestDbId = query.value(0).toInt();
        m_nextSerialNumber = query.value(1).toInt();
        LOG_DEBUG("MainWindow", QString("Resumed contest with DB ID: %1 next serial: %2")
            .arg(m_currentContestDbId)
            .arg(m_nextSerialNumber));

        // Load existing QSOs into table
        QSORepository repo;
        QList<QSO> existingQSOs = repo.findByContest(m_currentContestDbId);
        m_qsoTableModel->clear();
        for (const QSO& qso : existingQSOs) {
            m_qsoTableModel->addQSO(qso);
        }
        LOG_DEBUG("MainWindow", QString("Loaded %1 existing QSOs").arg(existingQSOs.size()));

    } else {
        // New contest - create record
        AppSettings& settings = AppSettings::instance();
        QDateTime now = QDateTime::currentDateTimeUtc();

        query = db.execute(
            "INSERT INTO contests (contest_id, contest_name, start_time, my_call, "
            "my_grid, my_continent, my_cq_zone, my_itu_zone, "
            "current_serial, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            {contestInfo.contestId, contestInfo.contestName,
             contestInfo.startDate.toSecsSinceEpoch(),
             settings.getMyCallsign(), settings.getMyGridSquare(),
             settings.getMyContinent(),
             settings.getMyCQZone(), settings.getMyITUZone(),
             1, now.toSecsSinceEpoch()});

        if (!query.isActive()) {
            QMessageBox::critical(this, "Database Error",
                                QString("Failed to create contest record:\n%1").arg(db.lastError()));
            return;
        }

        m_currentContestDbId = db.lastInsertId();
        m_nextSerialNumber = 1;
        m_qsoTableModel->clear();
        LOG_DEBUG("MainWindow", QString("Created new contest with DB ID: %1").arg(m_currentContestDbId));
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
        LOG_WARN("MainWindow", QString("Unknown contest type: %1").arg(contestInfo.contestType));
        return;
    }

    // Store contest info
    m_currentContest = contestInfo;
    m_hasActiveContest = true;

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
    if (callsign.length() < 2 || !m_activeContest) {
        return;
    }

    // Use InitialExchangeManager for sophisticated exchange prediction
    QString prediction = InitialExchangeManager::instance().predictExchange(
        callsign,
        m_activeContest,
        m_currentState.modeA
    );

    if (!prediction.isEmpty()) {
        m_exchangeEntry->setText(prediction);
        m_exchangeEntry->selectAll();  // Overwrite mode: first key replaces
        m_initialExchangePopulated = true;
    } else {
        m_initialExchangePopulated = false;
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
                        LOG_DEBUG("MainWindow", QString("DX Cluster click-to-QSY: %1 Hz").arg(QString::number(frequency)));
                        m_radio->setFrequency(static_cast<freq_t>(frequency));
                    } else {
                        LOG_DEBUG("MainWindow", QString("DX Cluster click-to-QSY: Radio not connected, cannot QSY to %1").arg(frequency));
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
                        LOG_DEBUG("MainWindow", QString("Band Map QSY to %1 Hz").arg(QString::number(frequency)));
                        m_radio->setFrequency(frequency);
                    }
                });

        connect(m_bandMapWindow, &BandMapWidget::callsignSelected,
                this, [this](const QString& callsign) {
                    LOG_DEBUG("MainWindow", QString("Band Map selected callsign: %1").arg(callsign));
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

void MainWindow::onShowStatistics() {
    if (!m_statisticsWindow) {
        m_statisticsWindow = new StatisticsWindow();
        m_statisticsWindow->setWindowTitle("Statistics");
        m_statisticsWindow->setWindowFlags(Qt::Window);
        m_statisticsWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_statisticsWindow->show();
    m_statisticsWindow->raise();
    m_statisticsWindow->activateWindow();
}

// Window menu placeholder implementations
void MainWindow::onSwapMultView() {
    LOG_DEBUG("MainWindow", "Swap Mult View (Alt+G) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Swap Mult View feature will be implemented in a future version.\n\n"
                           "This will toggle between different multiplier display modes.");
}

void MainWindow::onMissingMultsReport() {
    LOG_DEBUG("MainWindow", "Missing Mults Report (Ctrl+O) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Missing Mults Report will be implemented in a future version.\n\n"
                           "This will show a report of multipliers still needed.");
}

// Edit menu placeholder implementations
void MainWindow::onViewEditLog() {
    LOG_DEBUG("MainWindow", "View/Edit Log (Ctrl+L) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "View/Edit Log will be implemented in a future version.\n\n"
                           "This will show all logged QSOs in a table for viewing and editing.");
}

void MainWindow::onClearDupes() {
    LOG_DEBUG("MainWindow", "Clear Dupes (Ctrl+K) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Clear Dupes will be implemented in a future version.\n\n"
                           "This will remove duplicate QSOs from the log.");
}

void MainWindow::onNote() {
    LOG_DEBUG("MainWindow", "Note (Ctrl+N) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Note feature will be implemented in a future version.\n\n"
                           "This will allow adding notes to the log.");
}

void MainWindow::onRecallLast() {
    LOG_DEBUG("MainWindow", "Recall Last Entry (Ctrl+R) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Recall Last Entry will be implemented in a future version.\n\n"
                           "This will recall the last deleted log entry.");
}

// Tools menu placeholder implementations
void MainWindow::onWKMode() {
    LOG_DEBUG("MainWindow", "WK Mode (Alt+A) - Re-initialize WinKeyer - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "WinKeyer re-initialization will be implemented in a future version.\n\n"
                           "This will re-initialize the WinKeyer for CW keying.");
}

void MainWindow::onBackupLog() {
    if (!m_hasActiveContest) {
        QMessageBox::warning(this, "No Active Contest",
            "No contest is currently active. Please open or create a contest first.");
        return;
    }

    BackupRestoreDialog dialog(m_currentContest, this);
    dialog.exec();
}

void MainWindow::onDownloadCTY(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download CTY.dat (Alt+O) - Starting download (headless=%1)").arg(headless));

    // Get the save directory (e.g., ~/.tr4qt)
    QString saveDir = QDir::homePath() + "/.tr4qt";

    // Create progress dialog (only if not headless)
    QProgressDialog* progressDialog = nullptr;
    if (!headless) {
        progressDialog = new QProgressDialog("Downloading country file...", "Cancel", 0, 100, this);
        progressDialog->setWindowTitle("Download CTY.dat");
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setValue(0);
    }

    // Create downloader
    CountryFileDownloader* downloader = new CountryFileDownloader(this);

    // Connect progress signal (only if not headless)
    if (!headless) {
        connect(downloader, &CountryFileDownloader::downloadProgress,
                this, [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
                    if (progressDialog && bytesTotal > 0) {
                        int percentage = (bytesReceived * 100) / bytesTotal;
                        progressDialog->setValue(percentage);
                        progressDialog->setLabelText(QString("Downloading country file... %1 KB / %2 KB")
                                                    .arg(bytesReceived / 1024)
                                                    .arg(bytesTotal / 1024));
                    }
                });
    }

    // Connect finished signal
    connect(downloader, &CountryFileDownloader::downloadFinished,
            this, [this, progressDialog, downloader, headless](bool success, const QString& filePath, const QString& version) {
                if (progressDialog) {
                    progressDialog->close();
                    progressDialog->deleteLater();
                }

                if (success) {
                    LOG_DEBUG("MainWindow", QString("Download successful: %1 Version: %2").arg(filePath).arg(version));

                    if (!headless) {
                        // Ask user if they want to reload the country file
                        QMessageBox::StandardButton reply = QMessageBox::question(
                            this,
                            "Download Complete",
                            "Country file downloaded successfully!\n\n"
                            "Do you want to reload it now?",
                            QMessageBox::Yes | QMessageBox::No
                        );

                        if (reply == QMessageBox::Yes) {
                            // Reload the country file
                            if (m_countryFile.loadFromFile(filePath)) {
                                // Set the version from the download
                                m_countryFile.setVersion(version);
                                LOG_DEBUG("MainWindow", QString("Country file reloaded successfully. Version: %1")
                                    .arg(m_countryFile.getVersion()));
                                QMessageBox::information(this, "Success",
                                    QString("Country file reloaded successfully!\n\n"
                                           "Version: %1").arg(m_countryFile.getVersion()));
                            } else {
                                QMessageBox::warning(this, "Reload Failed",
                                    "Failed to reload the country file.\n\n"
                                    "Please restart the application.");
                            }
                        }
                    } else {
                        // Headless mode: auto-reload without prompts
                        if (m_countryFile.loadFromFile(filePath)) {
                            m_countryFile.setVersion(version);
                            LOG_DEBUG("MainWindow", QString("Country file reloaded successfully (headless). Version: %1")
                                .arg(m_countryFile.getVersion()));
                        } else {
                            LOG_WARN("MainWindow", "Failed to reload country file (headless)");
                        }
                    }
                } else {
                    if (!headless) {
                        QMessageBox::critical(this, "Download Failed",
                            "Failed to download country file.\n\n"
                            "Please check your internet connection and try again.");
                    } else {
                        LOG_WARN("MainWindow", "Failed to download country file (headless)");
                    }
                }

                downloader->deleteLater();
            });

    // Connect error signal (only if not headless)
    if (!headless) {
        connect(downloader, &CountryFileDownloader::errorOccurred,
                this, [progressDialog](const QString& error) {
                    LOG_DEBUG("MainWindow", QString("Download error: %1").arg(error));
                    if (progressDialog) {
                        progressDialog->setLabelText("Error: " + error);
                    }
                });

        // Connect cancel button
        connect(progressDialog, &QProgressDialog::canceled,
                downloader, &CountryFileDownloader::cancel);
    }

    // Start download
    downloader->downloadLatest(saveDir);
}

void MainWindow::onDownloadLOTW(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download LOTW Users (Alt+L) - Starting download (headless=%1)").arg(headless));

    // Create progress dialog (only if not headless)
    QProgressDialog* progressDialog = nullptr;
    if (!headless) {
        progressDialog = new QProgressDialog("Downloading LOTW user list...", "Cancel", 0, 100, this);
        progressDialog->setWindowTitle("Download LOTW Users");
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setValue(0);
    }

    // Create downloader
    LOTWUserDownloader* downloader = new LOTWUserDownloader(this);

    // Connect progress signal (only if not headless)
    if (!headless) {
        connect(downloader, &LOTWUserDownloader::downloadProgress,
                this, [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
                    if (progressDialog && bytesTotal > 0) {
                        int percentage = (bytesReceived * 100) / bytesTotal;
                        progressDialog->setValue(percentage);
                        progressDialog->setLabelText(QString("Downloading LOTW user list... %1 KB / %2 KB")
                                                    .arg(bytesReceived / 1024)
                                                    .arg(bytesTotal / 1024));
                    }
                });
    }

    // Connect finished signal
    connect(downloader, &LOTWUserDownloader::downloadFinished,
            this, [this, progressDialog, downloader, headless](bool success, int userCount, const QString& error) {
                if (progressDialog) {
                    progressDialog->close();
                    progressDialog->deleteLater();
                }

                if (success) {
                    LOG_DEBUG("MainWindow", QString("LOTW download successful: %1 users imported").arg(userCount));

                    // Update last update timestamp
                    AppSettings& settings = AppSettings::instance();
                    settings.setLotwLastUpdateTime(QDateTime::currentDateTime());

                    if (!headless) {
                        QMessageBox::information(this, "Download Complete",
                            QString("LOTW user list downloaded successfully!\n\n"
                                   "%1 users imported.").arg(userCount));
                    }
                } else {
                    if (!headless) {
                        QMessageBox::critical(this, "Download Failed",
                            QString("Failed to download LOTW user list.\n\n%1\n\n"
                                   "Please check your internet connection and try again.").arg(error));
                    } else {
                        LOG_WARN("MainWindow", QString("Failed to download LOTW user list (headless): %1").arg(error));
                    }
                }

                downloader->deleteLater();
            });

    // Connect error signal (only if not headless)
    if (!headless) {
        connect(downloader, &LOTWUserDownloader::errorOccurred,
                this, [progressDialog](const QString& error) {
                    LOG_DEBUG("MainWindow", QString("LOTW download error: %1").arg(error));
                    if (progressDialog) {
                        progressDialog->setLabelText("Error: " + error);
                    }
                });

        // Connect cancel button
        connect(progressDialog, &QProgressDialog::canceled,
                downloader, &LOTWUserDownloader::cancel);
    }

    // Start download
    downloader->downloadLatest();
}

void MainWindow::onSetDateTime() {
    LOG_DEBUG("MainWindow", "Set System Date/Time (Alt+T) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Set System Date/Time will be implemented in a future version.\n\n"
                           "This will allow setting the system date and time.");
}

void MainWindow::onInitialize() {
    LOG_DEBUG("MainWindow", "Initialize (Alt+W) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Initialize will be implemented in a future version.\n\n"
                           "This will initialize/reset contest parameters.");
}

// Operating menu placeholder implementations
void MainWindow::onAutoCQ() {
    LOG_DEBUG("MainWindow", "Auto CQ (Alt+Q) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Auto CQ will be implemented in a future version.\n\n"
                           "This will enable automatic CQ sending.");
}

void MainWindow::onAutoCQResume() {
    LOG_DEBUG("MainWindow", "Auto CQ Resume (Alt+C) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Auto CQ Resume will be implemented in a future version.\n\n"
                           "This will resume automatic CQ after an interruption.");
}

void MainWindow::onKillCW() {
    LOG_DEBUG("MainWindow", "Kill CW (Alt+K) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Kill CW will be implemented in a future version.\n\n"
                           "This will immediately stop CW transmission.");
}

void MainWindow::onDupeCheck() {
    LOG_DEBUG("MainWindow", "Dupe Check (Alt+D) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Dupe Check will be implemented in a future version.\n\n"
                           "This will check if the entered callsign is a duplicate.");
}

void MainWindow::onSearchLog() {
    LOG_DEBUG("MainWindow", "Search Log (Alt+L) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Search Log will be implemented in a future version.\n\n"
                           "This will search the log for a specific callsign.");
}

void MainWindow::onDeleteLastQSO() {
    LOG_DEBUG("MainWindow", "Delete Last QSO (Alt+Y) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Delete Last QSO will be implemented in a future version.\n\n"
                           "This will delete the most recent QSO from the log.");
}

void MainWindow::onIncNumber() {
    LOG_DEBUG("MainWindow", "Inc Number (Alt+I) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Inc Number will be implemented in a future version.\n\n"
                           "This will increment the serial number.");
}

void MainWindow::onInitialExchange() {
    LOG_DEBUG("MainWindow", "Initial Exchange (Alt+Z) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Initial Exchange will be implemented in a future version.\n\n"
                           "This will set/reset the initial exchange information.");
}

void MainWindow::onCWSpeed() {
    LOG_DEBUG("MainWindow", "CW Speed (Alt+S) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "CW Speed will be implemented in a future version.\n\n"
                           "This will adjust the CW sending speed.");
}

void MainWindow::onToggleSidetone() {
    LOG_DEBUG("MainWindow", "Toggle Sidetone (Alt+=) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Toggle Sidetone will be implemented in a future version.\n\n"
                           "This will turn CW sidetone on/off.");
}

void MainWindow::onToggleAutosend() {
    LOG_DEBUG("MainWindow", "Toggle Autosend (Alt+-) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Toggle Autosend will be implemented in a future version.\n\n"
                           "This will enable/disable automatic sending.");
}

// Band menu placeholder implementations
void MainWindow::onToggleRigs() {
    LOG_DEBUG("MainWindow", "Toggle Rigs (Alt+R) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Toggle Rigs will be implemented in a future version.\n\n"
                           "This will switch between radios in SO2R mode.");
}

void MainWindow::onEditSO2R() {
    LOG_DEBUG("MainWindow", "Edit SO2R (Alt+E) - Not yet implemented");
    QMessageBox::information(this, "Not Implemented",
                           "Edit SO2R will be implemented in a future version.\n\n"
                           "This will configure SO2R (two-radio) settings.");
}

void MainWindow::onDXSpotReceived(const QString& callsign,
                                   double frequency,
                                   const QString& spotter,
                                   const QString& comment) {
    LOG_DEBUG("MainWindow", QString("DX Spot received: %1 at %2 Hz from %3, comment: \"%4\"")
        .arg(callsign)
        .arg(QString::number(static_cast<qint64>(frequency)))
        .arg(spotter)
        .arg(comment));

    // If band map window exists, forward the spot to it
    if (m_bandMapWindow) {
        Spot spot;
        spot.callsign = callsign;
        spot.frequency = static_cast<freq_t>(frequency);  // Already in Hz from TelnetClient
        spot.timestamp = QDateTime::currentDateTime();
        spot.isMultiplier = false;  // TODO: Check if this is a needed multiplier
        spot.isWorked = false;       // TODO: Check if we've worked this station
        spot.comment = comment;      // DX cluster comment

        // Parse split frequency from comment
        // Supports: "UP 5" (offset in kHz) or "QSX 210" (fragment or full frequency)
        // UP: offset from spot frequency (e.g., "UP 5" = spot + 5 kHz)
        // QSX: frequency fragment (e.g., "QSX 210" with spot 14.200 = 14.210 MHz)

        // Try QSX pattern first (e.g., "QSX 210" or "QSX 14.210")
        static QRegularExpression qsxFragmentRegex(R"(\bQSX\s+(\d+(?:\.\d+)?)\b)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch qsxMatch = qsxFragmentRegex.match(comment);

        if (qsxMatch.hasMatch()) {
            double qsxValue = qsxMatch.captured(1).toDouble();

            // If value < 1000, treat as kHz fragment (e.g., 210 = 14.210 MHz on 20m)
            if (qsxValue < 1000) {
                // Get MHz part of spot frequency (e.g., 14200000 Hz -> 14 MHz)
                freq_t spotMHz = (spot.frequency / 1000000) * 1000000;
                // Add kHz fragment (e.g., 210 kHz = 210000 Hz)
                spot.qsx = spotMHz + static_cast<freq_t>(qsxValue * 1000);
                LOG_DEBUG("MainWindow", QString("Parsed QSX fragment: %1 kHz on %2 MHz band = %3 Hz")
                    .arg(qsxValue).arg(spotMHz / 1000000).arg(spot.qsx));
            } else {
                // Full frequency in MHz (e.g., 14.210)
                spot.qsx = static_cast<freq_t>(qsxValue * 1000000);
                LOG_DEBUG("MainWindow", QString("Parsed QSX full frequency: %1 MHz = %2 Hz")
                    .arg(qsxValue).arg(spot.qsx));
            }
        } else {
            // Try UP pattern (offset in kHz)
            static QRegularExpression upRegex(R"(\bUP\s+(\d+(?:\.\d+)?)\b)", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch upMatch = upRegex.match(comment);

            if (upMatch.hasMatch()) {
                double offsetKHz = upMatch.captured(1).toDouble();
                // spot.frequency is in Hz, offset is in kHz
                // QSX = transmit frequency + offset (for VFO B)
                spot.qsx = spot.frequency + static_cast<freq_t>(offsetKHz * 1000);
                LOG_DEBUG("MainWindow", QString("Parsed UP offset: TX=%1 Hz + %2 kHz = RX=%3 Hz")
                    .arg(spot.frequency).arg(offsetKHz).arg(spot.qsx));
            }
        }

        // Check if LOTW user (only if enabled in settings)
        AppSettings& settings = AppSettings::instance();
        if (settings.getEnableLotwLookup()) {
            LOTWUserRepository lotwRepo;
            spot.isLotwUser = lotwRepo.isLotwUser(callsign);

            if (spot.isLotwUser) {
                LOTWUser lotwUser = lotwRepo.findByCallsign(callsign);
                LOG_DEBUG("MainWindow", QString("DX Spot: %1 is an LOTW user (last upload: %2 %3)")
                    .arg(callsign)
                    .arg(lotwUser.lastUploadDate)
                    .arg(lotwUser.lastUploadTime));
            } else {
                LOG_DEBUG("MainWindow", QString("DX Spot: %1 is NOT an LOTW user").arg(callsign));
            }
        } else {
            spot.isLotwUser = false;  // LOTW lookup disabled
            LOG_DEBUG("MainWindow", QString("DX Spot: %1 - LOTW lookup disabled").arg(callsign));
        }

        spot.source = QString("DX Cluster (%1)").arg(spotter);

        m_bandMapWindow->addSpot(spot);

        // Comprehensive logging of spot details
        QString logMsg = QString("Added spot to band map: %1").arg(callsign);
        logMsg += QString(" | TX: %1 Hz (%2 MHz)").arg(spot.frequency).arg(spot.frequency / 1000000.0, 0, 'f', 3);
        if (spot.qsx > 0) {
            logMsg += QString(" | RX (QSX): %1 Hz (%2 MHz)").arg(spot.qsx).arg(spot.qsx / 1000000.0, 0, 'f', 3);
        }
        logMsg += QString(" | LOTW: %1").arg(spot.isLotwUser ? "YES" : "NO");
        if (!spot.comment.isEmpty()) {
            logMsg += QString(" | Comment: \"%1\"").arg(spot.comment);
        }
        LOG_DEBUG("MainWindow", logMsg);
    } else {
        LOG_DEBUG("MainWindow", "Band map window not open - spot not added");
    }
}

void MainWindow::onBandClicked(BandType band) {
    if (!m_radioConnected) {
        LOG_DEBUG("MainWindow", "Cannot change band: radio not connected");
        return;
    }

    // Get frequency for the clicked band based on current mode
    freq_t frequency = getFrequencyForBand(band, m_currentState.modeA);

    LOG_DEBUG("MainWindow", QString("Band clicked: %1 Setting frequency to: %2 Hz").arg(QString::number(static_cast<int>(band))).arg(QString::number(frequency)));

    // Send frequency change to radio
    m_radio->setFrequency(frequency);
}

void MainWindow::onBandUp() {
    // TODO: Implement band up
    LOG_DEBUG("MainWindow", "Band up");
}

void MainWindow::onBandDown() {
    // TODO: Implement band down
    LOG_DEBUG("MainWindow", "Band down");
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
