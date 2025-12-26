#include "MainWindow.h"
#include "dialogs/RadioConfigDialog.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/BackupRestoreDialog.h"
#include "dialogs/OperatorDialog.h"
#include "dialogs/EditQSODialog.h"
#include "dialogs/ExportPreviewDialog.h"
#include "dialogs/SendMorseDialog.h"
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
#include "../utils/MessageBox.h"
#include <QCloseEvent>
#include <QApplication>
#include <QSettings>
#include <QPushButton>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QTextEdit>
#include <QtConcurrent/QtConcurrent>
#include <QThread>

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
    , m_qsosSinceLastIntegrityCheck(0)
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

    // Reopen last contest if available
    reopenLastContest();

    // Install event filter to raise all windows when any window is activated
    qApp->installEventFilter(this);

    // Setup update timer for time since last QSO and rate calculations
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateTimeDisplay);
    m_updateTimer->start(1000);  // Update every second

    // Setup periodic integrity check timer (Tier 2)
    m_integrityCheckTimer = new QTimer(this);
    connect(m_integrityCheckTimer, &QTimer::timeout, this, &MainWindow::onPeriodicIntegrityCheck);
    m_integrityCheckTimer->start(5 * 60 * 1000);  // Check every 5 minutes

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

    m_connectAction = radioMenu->addAction("C&onnect/Reconnect");
    m_connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_connectAction->setStatusTip("Connect or reconnect to radio");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::onRadioConnect);

    m_disconnectAction = radioMenu->addAction("&Disconnect");
    m_disconnectAction->setEnabled(false);
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::onRadioDisconnect);

    radioMenu->addSeparator();

    m_autoSendCWAction = radioMenu->addAction("&Auto Send CW");
    m_autoSendCWAction->setCheckable(true);
    m_autoSendCWAction->setChecked(AppSettings::instance().getAutoSendCW());
    m_autoSendCWAction->setStatusTip("Automatically send callsign via CW when Enter is pressed in CW mode");
    connect(m_autoSendCWAction, &QAction::toggled, this, [this](bool checked) {
        AppSettings::instance().setAutoSendCW(checked);
        LOG_DEBUG("MainWindow", QString("Auto Send CW %1").arg(checked ? "enabled" : "disabled"));
        updateRadioStatusGrid();  // Update WPM display
    });

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

    // Data integrity check
    QAction* integrityCheckAction = toolsMenu->addAction("Validate Log Integrity");
    integrityCheckAction->setShortcut(QKeySequence("Alt+I"));
    connect(integrityCheckAction, &QAction::triggered, this, &MainWindow::onFullIntegrityCheck);

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

    QAction* sendMorseAction = windowMenu->addAction("Send &Morse Code");
    sendMorseAction->setShortcut(QKeySequence("Alt+K"));
    connect(sendMorseAction, &QAction::triggered, this, &MainWindow::onSendMorse);

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
    m_bandSummaryGrid->setEnabled(true);  // Always enabled for manual band selection
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

    // WPM label for CW speed
    m_radioWpmLabel = new QLabel("-- WPM", radioStatusWidget);
    m_radioWpmLabel->setFont(labelFont);
    m_radioWpmLabel->setAlignment(Qt::AlignCenter);
    m_radioWpmLabel->setMinimumWidth(80);
    m_radioWpmLabel->setEnabled(false);  // Grayed out by default

    // Date/Time labels - stacked vertically to save width
    QFont dateTimeFont("Monospace", labelFont.pointSize());

    m_radioDateLabel = new QLabel("", radioStatusWidget);
    m_radioDateLabel->setFont(dateTimeFont);
    m_radioDateLabel->setAlignment(Qt::AlignCenter);
    m_radioDateLabel->setMinimumWidth(120);  // Narrower than single label

    m_radioTimeLabel = new QLabel("", radioStatusWidget);
    m_radioTimeLabel->setFont(dateTimeFont);
    m_radioTimeLabel->setAlignment(Qt::AlignCenter);
    m_radioTimeLabel->setMinimumWidth(120);

    // Vertical layout for date and time
    QVBoxLayout* dateTimeLayout = new QVBoxLayout();
    dateTimeLayout->setSpacing(2);
    dateTimeLayout->addWidget(m_radioDateLabel);
    dateTimeLayout->addWidget(m_radioTimeLabel);

    radioLayout->addLayout(freqLayout);
    radioLayout->addWidget(m_radioWpmLabel);
    radioLayout->addLayout(dateTimeLayout);

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

    // Duplicate warning label (row 0, column 3 - to right of call entry)
    m_dupeWarningLabel = new QLabel(this);
    m_dupeWarningLabel->setStyleSheet("QLabel { color: #ff6600; font-weight: bold; font-size: 11pt; }");
    m_dupeWarningLabel->setMinimumWidth(200);
    m_dupeWarningLabel->hide();  // Initially hidden
    entryLayout->addWidget(m_dupeWarningLabel, 0, 3);

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
            this, &MainWindow::onCallsignEnterPressed);
    connect(m_exchangeEntry, &QLineEdit::textChanged,
            this, &MainWindow::onExchangeTextChanged);
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

void MainWindow::keyPressEvent(QKeyEvent* event) {
    // PgUp: Increase WPM by configurable increment
    if (event->key() == Qt::Key_PageUp) {
        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = AppSettings::instance().getMorseWPM();
        int newWpm = qMin(currentWpm + increment, 60);  // Max 60 WPM
        AppSettings::instance().setMorseWPM(newWpm);

        // Update display
        updateRadioStatusGrid();

        // Send to radio if in CW mode
        bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
        if (isCWMode && m_radioConnected) {
            m_radio->setCWSpeed(newWpm);
        }

        m_statusLabel->setText(QString("CW Speed: %1 WPM").arg(newWpm));
        LOG_DEBUG("MainWindow", QString("WPM increased to %1 (PgUp)").arg(newWpm));
        event->accept();
        return;
    }

    // PgDown: Decrease WPM by configurable increment
    if (event->key() == Qt::Key_PageDown) {
        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = AppSettings::instance().getMorseWPM();
        int newWpm = qMax(currentWpm - increment, 5);  // Min 5 WPM
        AppSettings::instance().setMorseWPM(newWpm);

        // Update display
        updateRadioStatusGrid();

        // Send to radio if in CW mode
        bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
        if (isCWMode && m_radioConnected) {
            m_radio->setCWSpeed(newWpm);
        }

        m_statusLabel->setText(QString("CW Speed: %1 WPM").arg(newWpm));
        LOG_DEBUG("MainWindow", QString("WPM decreased to %1 (PgDn)").arg(newWpm));
        event->accept();
        return;
    }

    // ESC: Abort CW transmission
    if (event->key() == Qt::Key_Escape) {
        if (m_radioConnected) {
            m_radio->stopCW();
            m_statusLabel->setText("CW transmission aborted");
            LOG_DEBUG("MainWindow", "CW transmission aborted via ESC key");
        }
        event->accept();
        return;
    }

    // Let parent handle other keys
    QMainWindow::keyPressEvent(event);
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

void MainWindow::setStatusMessage(const QString& message) {
    // Log all status messages for debugging
    LOG_WARN("MainWindow", QString("Status: %1").arg(message));
    m_statusLabel->setText(message);
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

        // Set focus to callsign entry for immediate logging
        m_callsignEntry->setFocus();

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

    // Generate ADIF content
    ADIFExporter exporter;
    QString operatorCall = AppSettings::instance().getMyCallsign();
    QString adifContent = exporter.generateADIF(qsos, m_activeContest, operatorCall);

    // Generate default filename
    QString defaultFileName = m_hasActiveContest ?
        m_currentContest.contestName.toLower().replace(" ", "_") + ".adi" :
        "log.adi";

    // Show preview dialog
    ExportPreviewDialog preview(
        QString("ADIF Export Preview - %1 QSOs").arg(qsos.size()),
        adifContent,
        "ADIF Files (*.adi *.adif);;All Files (*)",
        defaultFileName,
        this);

    preview.exec();

    // Update status if saved
    if (preview.wasSaved()) {
        m_statusLabel->setText(QString("Exported %1 QSOs to %2")
                                  .arg(qsos.size())
                                  .arg(QFileInfo(preview.getSaveFilePath()).fileName()));
    }
}

void MainWindow::onExportCabrillo() {
    // Check if we have QSOs to export
    if (m_qsoTableModel->count() == 0) {
        QMessageBox::information(this, "Export Cabrillo", "No QSOs to export.");
        return;
    }

    // Tier 4: Run integrity check before export
    if (m_hasActiveContest) {
        if (!quickIntegrityCheck()) {
            QMessageBox::StandardButton reply = QMessageBox::warning(
                this,
                "Data Integrity Warning",
                "Log integrity check failed!\n\n"
                "There is a mismatch between memory and database.\n"
                "Exporting may result in incomplete or incorrect data.\n\n"
                "Continue with export anyway?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (reply == QMessageBox::No) {
                return;  // Abort export
            }
        } else {
            LOG_INFO("MainWindow", "Pre-export integrity check passed");
        }
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

    // Generate default filename
    QString defaultFileName;
    if (m_hasActiveContest && !m_currentContest.contestName.isEmpty()) {
        defaultFileName = QString("%1.cbr").arg(m_currentContest.contestName.replace(' ', '_'));
    } else {
        defaultFileName = "log.cbr";
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

    // Generate Cabrillo content
    QString cabrilloContent = exporter.generateCabrillo(qsos, m_activeContest);

    // Show preview dialog
    ExportPreviewDialog preview(
        QString("Cabrillo Export Preview - %1 QSOs").arg(qsos.size()),
        cabrilloContent,
        "Cabrillo Files (*.cbr *.log);;All Files (*)",
        defaultFileName,
        this);

    preview.exec();

    // Update status if saved
    if (preview.wasSaved()) {
        m_statusLabel->setText(QString("Exported %1 QSOs to %2")
                                  .arg(qsos.size())
                                  .arg(QFileInfo(preview.getSaveFilePath()).fileName()));
    }
}

void MainWindow::onClearLog() {
    if (m_qsoTableModel->count() == 0) {
        QMessageBox::information(this, "Clear Log", "Log is already empty.");
        return;
    }

    // Ask if user wants to create a backup first
    QMessageBox::StandardButton backupReply = QMessageBox::question(
        this, "Create Backup?",
        QString("Would you like to create a backup before clearing %1 QSOs?\n\n"
                "The backup will be saved as an archived copy for safety.")
            .arg(m_qsoTableModel->count()),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);

    if (backupReply == QMessageBox::Cancel) {
        return;  // User cancelled
    }

    // Create backup if requested
    if (backupReply == QMessageBox::Yes) {
        BackupManager& backupMgr = BackupManager::instance();
        QString backupPath;
        QString backupDir = QDir::homePath() + "/.tr4qt/backups";

        if (!backupMgr.createBackup(m_currentContest.databasePath, backupDir, backupPath)) {
            QMessageBox::StandardButton continueReply = QMessageBox::warning(
                this, "Backup Failed",
                QString("Failed to create backup: %1\n\nDo you still want to clear the log?")
                    .arg(backupMgr.lastError()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (continueReply != QMessageBox::Yes) {
                return;  // User chose not to continue
            }
        } else {
            m_statusLabel->setText(QString("Backup created: %1")
                .arg(QFileInfo(backupPath).fileName()));
        }
    }

    // Confirm clear
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Log",
        QString("Are you sure you want to clear all %1 QSOs from the log?\n\nThis action cannot be undone.")
            .arg(m_qsoTableModel->count()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Clear QSOs and multipliers from database
        QSORepository repo;
        if (!repo.deleteAllQSOs(m_currentContestDbId)) {
            QMessageBox::critical(this, "Error",
                QString("Failed to clear log from database: %1").arg(repo.lastError()));
            return;
        }

        // Clear exchange memory for this contest
        ExchangeMemoryRepository memRepo;
        QString contestType = m_activeContest ? m_activeContest->getContestId() : QString();
        memRepo.clearForContest(contestType);

        // Clear in-memory model
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

    // Sync CW speed from radio to program settings (when in CW mode)
    static int lastCwSpeed = 0;
    if ((state.modeA == ModeType::CW || state.modeA == ModeType::CWR) && state.cwSpeed > 0) {
        if (state.cwSpeed != lastCwSpeed) {
            AppSettings::instance().setMorseWPM(state.cwSpeed);
            lastCwSpeed = state.cwSpeed;
            LOG_DEBUG("MainWindow", QString("Synced CW speed from radio: %1 WPM").arg(state.cwSpeed));
        }
    }

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

    // Check for UDP command (rebroadcast entire log)
    if (callsign == "UDP") {
        onRebroadcastLog();

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
            setStatusMessage(QString("Invalid exchange: %1").arg(errorMsg));
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

    // Lookup country/zone from cty.dat
    CountryData countryData = m_countryFile.lookup(callsign);
    if (countryData.isValid()) {
        qso.cqZone = countryData.cqZone;
        qso.ituZone = countryData.ituZone;
        qso.dxccEntity = countryData.name;
        qso.dxccPrefix = countryData.primaryPrefix;
        qso.dxccEntityCode = countryData.dxccEntity;  // ADIF DXCC Entity Code
        qso.continent = continentToString(countryData.continent);

        LOG_DEBUG("MainWindow", QString("Looked up %1: %2 (Zone %3, %4)")
            .arg(callsign)
            .arg(countryData.name)
            .arg(countryData.cqZone)
            .arg(continentToString(countryData.continent)));
    } else {
        LOG_WARN("MainWindow", QString("Country lookup failed for callsign: %1").arg(callsign));
    }

    // Calculate QSO points via contest scoring
    if (m_activeContest) {
        StationInfo myStation;
        myStation.callsign = AppSettings::instance().getMyCallsign();
        myStation.continent = AppSettings::instance().getMyContinent();
        myStation.cqZone = AppSettings::instance().getMyCQZone();

        // Lookup my country from callsign via cty.dat
        CountryData myCountryData = m_countryFile.lookup(myStation.callsign);
        if (myCountryData.isValid()) {
            myStation.country = myCountryData.name;
        }

        qso.qsoPoints = m_activeContest->calculateQSOPoints(qso, myStation);

        LOG_DEBUG("MainWindow", QString("QSO points calculated: %1").arg(qso.qsoPoints));
    } else {
        qso.qsoPoints = 1;  // Default 1 point if no contest
    }

    // TODO: Check for dupe
    // TODO: Check for new multipliers

    // Add to table model (UI)
    m_qsoTableModel->addQSO(qso);

    // Update band summary grid with new scores
    updateScoreDisplay();

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

            // Update the table model with the QSO now that it has a database ID
            // This allows Edit QSO to work properly (needs valid ID)
            int lastRow = m_qsoTableModel->count() - 1;
            m_qsoTableModel->updateQSO(lastRow, qso);

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

    // Tier 2 Integrity Check: Check after every 50 QSOs
    m_qsosSinceLastIntegrityCheck++;
    if (m_qsosSinceLastIntegrityCheck >= 50) {
        quickIntegrityCheck();
        m_qsosSinceLastIntegrityCheck = 0;
    }

    // Clear entry fields and focus callsign
    onClearEntry();

    // Update score display and time display
    updateScoreDisplay();
    updateTimeDisplay();  // Immediate update after logging
}

void MainWindow::onCallsignChanged(const QString& callsign) {
    Q_UNUSED(callsign);
    // Exchange auto-population now happens on Enter key press, not while typing
    // Duplicate checking happens on Enter key press
}

void MainWindow::onExchangeTextChanged(const QString& text) {
    // Clear styling if empty or no active contest
    if (text.isEmpty() || !m_activeContest) {
        m_exchangeEntry->setStyleSheet("");
        m_exchangeEntry->setToolTip("");
        return;
    }

    // Validate exchange against contest rules
    QString errorMsg;
    bool isValid = m_activeContest->validateReceivedExchange(text, errorMsg);

    QString styleSheet;
    QString tooltip;

    if (isValid) {
        // Green border for valid exchange
        styleSheet = "QLineEdit { border: 2px solid #00aa00; background-color: #f0fff0; }";
        tooltip = "✓ Valid exchange";
    } else {
        // Check if this could become valid with more input (partial/incomplete)
        QString trimmed = text.trimmed();
        QList<ExchangeField> fields = m_activeContest->getReceivedExchangeFields();

        // Count fields entered vs expected
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        bool couldBePartial = (parts.size() < fields.size()) && !trimmed.isEmpty();

        if (couldBePartial) {
            // Yellow border for incomplete exchange
            styleSheet = "QLineEdit { border: 2px solid #ffaa00; background-color: #fffef0; }";
            tooltip = "⚠ Incomplete - " + errorMsg;
        } else {
            // Red border for invalid exchange
            styleSheet = "QLineEdit { border: 2px solid #ff0000; background-color: #fff0f0; }";
            tooltip = "✗ " + errorMsg;
        }
    }

    m_exchangeEntry->setStyleSheet(styleSheet);
    m_exchangeEntry->setToolTip(tooltip);
}

void MainWindow::onCallsignEnterPressed() {
    QString callsign = m_callsignEntry->text().trimmed().toUpper();

    if (callsign.isEmpty()) {
        return;
    }

    LOG_DEBUG("MainWindow", QString("onCallsignEnterPressed - callsign: '%1'").arg(callsign));

    // Check for numeric frequency entry
    // If callsign is a number, treat it as frequency change command
    bool isNumeric = false;
    unsigned long freqValue = callsign.toULong(&isNumeric);

    LOG_DEBUG("MainWindow", QString("Numeric check - isNumeric: %1, freqValue: %2")
        .arg(isNumeric).arg(freqValue));

    if (isNumeric && freqValue > 0) {
        // Determine if this is an offset or absolute frequency
        unsigned long targetFreqKHz = 0;

        if (freqValue < 1000) {
            // Small number - treat as offset from band edge
            // e.g., "300" on 15m -> 21000 + 300 = 21300 kHz
            unsigned long bandEdge = bandToBaseFrequency(m_currentState.bandA);
            if (bandEdge > 0) {
                targetFreqKHz = bandEdge + freqValue;
                LOG_DEBUG("MainWindow", QString("Frequency offset entry: %1 + %2 = %3 kHz")
                    .arg(bandEdge).arg(freqValue).arg(targetFreqKHz));
            } else {
                m_statusLabel->setText("Error: Cannot determine band edge for current band");
                onClearEntry();
                return;
            }
        } else {
            // Large number - treat as absolute frequency in kHz
            // e.g., "14210" -> 14210 kHz
            targetFreqKHz = freqValue;
            LOG_DEBUG("MainWindow", QString("Absolute frequency entry: %1 kHz").arg(targetFreqKHz));
        }

        // Convert kHz to Hz for hamlib
        freq_t targetFreqHz = static_cast<freq_t>(targetFreqKHz) * 1000;

        // Set radio frequency
        if (m_radio && m_radioConnected) {
            m_radio->setFrequency(targetFreqHz);
            m_statusLabel->setText(QString("Frequency set to %1 kHz").arg(targetFreqKHz));
            LOG_INFO("MainWindow", QString("Frequency changed to %1 kHz via numeric entry").arg(targetFreqKHz));
        } else {
            m_statusLabel->setText("Error: Radio not connected");
        }

        // Clear entry and return (don't process as callsign)
        onClearEntry();
        return;
    }

    // Check for OPON command (change operator)
    if (callsign == "OPON") {
        onLogQSO();  // Handle OPON in onLogQSO which has the full implementation
        return;
    }

    // Check for UDP command (rebroadcast entire log)
    if (callsign == "UDP") {
        onLogQSO();  // Handle UDP in onLogQSO which has the full implementation
        return;
    }

    // Check for duplicate QSO
    QString dupeInfo;
    bool isDupe = checkForDuplicate(callsign, m_currentState.bandA, m_currentState.modeA, dupeInfo);

    if (isDupe) {
        // Show warning in status bar (allows logging to proceed)
        m_statusLabel->setText("⚠ " + dupeInfo);
        m_statusLabel->setStyleSheet("QLabel { color: #ff6600; font-weight: bold; }");
    } else {
        // Clear warning for non-duplicates
        m_statusLabel->setText("Ready");
        m_statusLabel->setStyleSheet("");  // Reset style
    }

    // Auto-populate exchange based on callsign
    autoPopulateExchange(callsign);

    // Auto-send callsign via CW when in CW mode (if enabled)
    bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
    bool autoSendEnabled = AppSettings::instance().getAutoSendCW();
    if (isCWMode && m_radioConnected && m_radio && autoSendEnabled) {
        // Set CW speed from settings
        int wpm = AppSettings::instance().getMorseWPM();
        m_radio->setCWSpeed(wpm);

        // Send the callsign
        LOG_DEBUG("MainWindow", QString("Auto-sending callsign via CW: '%1' at %2 WPM").arg(callsign).arg(wpm));
        m_radio->sendCW(callsign);
    }

    // Move focus to exchange field
    m_exchangeEntry->setFocus();
    m_exchangeEntry->selectAll();  // Select all for easy overwrite
}

void MainWindow::onClearEntry() {
    m_callsignEntry->clear();
    m_exchangeEntry->clear();
    m_callsignEntry->setFocus();
    m_initialExchangePopulated = false;

    // Reset status
    m_statusLabel->setText("Ready");
    m_statusLabel->setStyleSheet("");
}

void MainWindow::onEditQSO(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    int row = index.row();

    // Get the QSO at the selected row
    QSO qso = m_qsoTableModel->getQSO(row);
    if (qso.id < 0) {
        MessageBox::warning(this, "Error", "Cannot edit QSO: Invalid QSO ID");
        return;
    }

    // Open edit dialog with contest for validation
    EditQSODialog dialog(qso, m_activeContest, this);
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
    if (!m_bandSummaryGrid) {
        return;
    }

    // Initialize per-band counters
    QMap<BandType, int> qsosPerBand;
    QMap<BandType, int> pointsPerBand;
    QMap<BandType, QSet<QString>> multsPerBand;    // Track unique mults
    QMap<BandType, QSet<int>> zonesPerBand;         // Track unique zones

    int totalQSOs = 0;
    int totalPoints = 0;

    // Iterate through all QSOs in the table model
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);

        if (qso.band == BandType::None) {
            continue;  // Skip QSOs with no band
        }

        // Count QSOs per band
        qsosPerBand[qso.band]++;
        totalQSOs++;

        // Sum points per band
        pointsPerBand[qso.band] += qso.qsoPoints;
        totalPoints += qso.qsoPoints;

        // Track unique multipliers per band (DXCC prefix)
        if (!qso.dxccPrefix.isEmpty()) {
            multsPerBand[qso.band].insert(qso.dxccPrefix);
        }

        // Track unique zones per band
        if (qso.cqZone > 0) {
            zonesPerBand[qso.band].insert(qso.cqZone);
        }
    }

    // Update band summary grid with calculated values
    QList<BandType> bands = {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };

    int totalMults = 0;
    int totalZones = 0;

    for (BandType band : bands) {
        int qsos = qsosPerBand.value(band, 0);
        int points = pointsPerBand.value(band, 0);
        int mults = multsPerBand.value(band).size();
        int zones = zonesPerBand.value(band).size();

        m_bandSummaryGrid->setQSOCount(band, qsos);
        m_bandSummaryGrid->setPointsCount(band, points);
        m_bandSummaryGrid->setMultCount(band, mults);
        m_bandSummaryGrid->setZoneCount(band, zones);

        totalMults += mults;
        totalZones += zones;
    }

    // Update "All" column totals
    m_bandSummaryGrid->setAllQSOs(totalQSOs);
    m_bandSummaryGrid->setAllMults(totalMults);
    m_bandSummaryGrid->setAllZones(totalZones);
    m_bandSummaryGrid->setTotalPoints(totalPoints);

    // Update status bar
    m_statusLabel->setText(QString("%1 QSOs, %2 Points").arg(totalQSOs).arg(totalPoints));
}

void MainWindow::recalculateAllPoints() {
    if (!m_activeContest || !m_qsoTableModel) {
        LOG_WARN("MainWindow", "Cannot recalculate points - no active contest or table model");
        return;
    }

    LOG_INFO("MainWindow", QString("Recalculating points for %1 QSOs").arg(m_qsoTableModel->count()));

    // Setup station info
    StationInfo myStation;
    myStation.callsign = AppSettings::instance().getMyCallsign();
    myStation.continent = AppSettings::instance().getMyContinent();
    myStation.cqZone = AppSettings::instance().getMyCQZone();

    CountryData myCountryData = m_countryFile.lookup(myStation.callsign);
    if (myCountryData.isValid()) {
        myStation.country = myCountryData.name;
    }

    int updatedCount = 0;
    QSORepository repo;

    // Iterate through all QSOs and recalculate points
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);

        // Calculate new points
        int newPoints = m_activeContest->calculateQSOPoints(qso, myStation);

        // Update if different
        if (qso.qsoPoints != newPoints) {
            qso.qsoPoints = newPoints;

            // Update in database
            if (repo.updateQSO(qso)) {
                // Update in table model
                m_qsoTableModel->updateQSO(row, qso);
                updatedCount++;
            } else {
                LOG_ERROR("MainWindow", QString("Failed to update QSO %1 in database").arg(qso.id));
            }
        }
    }

    LOG_INFO("MainWindow", QString("Recalculated points for %1 QSOs").arg(updatedCount));

    // Update display
    updateScoreDisplay();

    // Show result to user
    m_statusLabel->setText(QString("Recalculated points for %1 QSOs").arg(updatedCount));
}

// Tier 2: Periodic lightweight integrity check
void MainWindow::onPeriodicIntegrityCheck() {
    if (!m_hasActiveContest || !m_qsoTableModel) {
        return;  // No contest active, skip check
    }

    if (!quickIntegrityCheck()) {
        LOG_WARN("MainWindow", "Periodic integrity check failed");
    }
}

bool MainWindow::quickIntegrityCheck() {
    if (!m_hasActiveContest || !m_qsoTableModel) {
        return true;  // Nothing to check
    }

    // Quick count comparison
    int memoryCount = m_qsoTableModel->count();
    QSORepository repo;

    // Count non-deleted QSOs in database
    Database& db = Database::instance();
    QSqlQuery query = db.execute(
        "SELECT COUNT(*) FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_currentContestDbId});

    int dbCount = 0;
    if (query.next()) {
        dbCount = query.value(0).toInt();
    }

    if (memoryCount != dbCount) {
        LOG_ERROR("MainWindow", QString("INTEGRITY CHECK FAILED: Memory=%1 DB=%2")
            .arg(memoryCount).arg(dbCount));

        handleIntegrityMismatch(memoryCount, dbCount);
        return false;
    }

    LOG_DEBUG("MainWindow", QString("Integrity check passed: %1 QSOs").arg(memoryCount));
    return true;
}

void MainWindow::handleIntegrityMismatch(int memoryCount, int dbCount) {
    QString message = QString(
        "Data integrity mismatch detected!\n\n"
        "QSOs in memory: %1\n"
        "QSOs in database: %2\n\n"
        "This may indicate a database write failure.\n"
        "Recommend reloading the contest to synchronize data.\n\n"
        "Reload contest now?")
        .arg(memoryCount)
        .arg(dbCount);

    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        "Data Integrity Warning",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (reply == QMessageBox::Yes) {
        // Reload contest from database
        LOG_INFO("MainWindow", "User requested contest reload after integrity mismatch");
        reopenLastContest();
    }
}

// Tier 3: Full detailed integrity check
void MainWindow::onFullIntegrityCheck() {
    if (!m_hasActiveContest || !m_qsoTableModel) {
        QMessageBox::information(this, "Integrity Check",
            "No active contest to validate.");
        return;
    }

    m_statusLabel->setText("Running full integrity check...");
    QApplication::processEvents();  // Update UI

    QString report = fullIntegrityCheck();

    // Display report
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Log Integrity Check Report");
    dialog->resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QTextEdit* reportText = new QTextEdit(dialog);
    reportText->setReadOnly(true);
    reportText->setPlainText(report);
    reportText->setFont(QFont("Monospace", 10));
    layout->addWidget(reportText);

    QPushButton* closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);

    dialog->exec();
    delete dialog;

    m_statusLabel->setText("Integrity check complete");
}

QString MainWindow::fullIntegrityCheck() {
    QString report;
    report += "=== LOG INTEGRITY CHECK REPORT ===\n\n";
    report += QString("Contest: %1\n").arg(m_currentContest.contestName);
    report += QString("Database: %1\n").arg(m_currentContest.databasePath);
    report += QString("Check time: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));

    int memoryCount = m_qsoTableModel->count();
    Database& db = Database::instance();

    // Count database QSOs
    QSqlQuery countQuery = db.execute(
        "SELECT COUNT(*) FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_currentContestDbId});
    int dbCount = 0;
    if (countQuery.next()) {
        dbCount = countQuery.value(0).toInt();
    }

    report += QString("QSOs in memory: %1\n").arg(memoryCount);
    report += QString("QSOs in database: %1\n\n").arg(dbCount);

    // Check 1: Count match
    if (memoryCount == dbCount) {
        report += "✓ QSO count matches\n\n";
    } else {
        report += QString("✗ QSO COUNT MISMATCH (diff: %1)\n\n")
            .arg(qAbs(memoryCount - dbCount));
    }

    // Check 2: Verify all memory QSOs exist in database
    QStringList missingInDB;
    QSORepository repo;
    for (int row = 0; row < memoryCount; ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);
        if (qso.id < 0) {
            missingInDB.append(QString("Row %1: %2 (no database ID)")
                .arg(row).arg(qso.callsign));
        } else {
            QSO dbQso = repo.findById(qso.id);
            if (dbQso.id < 0) {
                missingInDB.append(QString("Row %1: %2 (ID=%3 not found in DB)")
                    .arg(row).arg(qso.callsign).arg(qso.id));
            }
        }
    }

    if (missingInDB.isEmpty()) {
        report += "✓ All memory QSOs exist in database\n\n";
    } else {
        report += QString("✗ %1 QSOs in memory not found in database:\n")
            .arg(missingInDB.size());
        for (const QString& item : missingInDB) {
            report += QString("  - %1\n").arg(item);
        }
        report += "\n";
    }

    // Check 3: Look for orphaned QSOs in database
    QSqlQuery dbQuery = db.execute(
        "SELECT id, callsign FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_currentContestDbId});

    QList<int> dbIds;
    QMap<int, QString> dbCallsigns;
    while (dbQuery.next()) {
        int id = dbQuery.value(0).toInt();
        QString callsign = dbQuery.value(1).toString();
        dbIds.append(id);
        dbCallsigns[id] = callsign;
    }

    QSet<int> memoryIds;
    for (int row = 0; row < memoryCount; ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);
        if (qso.id >= 0) {
            memoryIds.insert(qso.id);
        }
    }

    QStringList orphanedInDB;
    for (int dbId : dbIds) {
        if (!memoryIds.contains(dbId)) {
            orphanedInDB.append(QString("ID=%1: %2")
                .arg(dbId).arg(dbCallsigns[dbId]));
        }
    }

    if (orphanedInDB.isEmpty()) {
        report += "✓ No orphaned QSOs in database\n\n";
    } else {
        report += QString("✗ %1 QSOs in database not loaded in memory:\n")
            .arg(orphanedInDB.size());
        for (const QString& item : orphanedInDB) {
            report += QString("  - %1\n").arg(item);
        }
        report += "\n";
    }

    // Check 4: Verify critical fields match
    int fieldMismatches = 0;
    for (int row = 0; row < qMin(memoryCount, 100); ++row) {  // Sample first 100
        QSO memQso = m_qsoTableModel->getQSO(row);
        if (memQso.id < 0) continue;

        QSO dbQso = repo.findById(memQso.id);
        if (dbQso.id < 0) continue;

        if (memQso.callsign != dbQso.callsign ||
            memQso.qsoPoints != dbQso.qsoPoints ||
            memQso.band != dbQso.band) {
            fieldMismatches++;
        }
    }

    if (fieldMismatches == 0) {
        report += QString("✓ Field values match (sampled first 100 QSOs)\n\n");
    } else {
        report += QString("✗ %1 field mismatches detected in sample\n\n")
            .arg(fieldMismatches);
    }

    // Summary
    report += "=== SUMMARY ===\n";
    bool allPassed = (memoryCount == dbCount) &&
                     missingInDB.isEmpty() &&
                     orphanedInDB.isEmpty() &&
                     (fieldMismatches == 0);

    if (allPassed) {
        report += "✓ ALL CHECKS PASSED - Log integrity verified\n";
    } else {
        report += "✗ ISSUES DETECTED - See details above\n";
        report += "\nRecommendation: Consider reloading contest from database\n";
    }

    LOG_INFO("MainWindow", QString("Full integrity check: %1")
        .arg(allPassed ? "PASSED" : "FAILED"));

    return report;
}

// UDP command: Rebroadcast entire log
void MainWindow::onRebroadcastLog() {
    if (!m_hasActiveContest || !m_qsoTableModel) {
        m_statusLabel->setText("Error: No active contest to rebroadcast");
        return;
    }

    if (!m_udpBroadcastManager->isEnabled()) {
        m_statusLabel->setText("Error: UDP broadcasting is disabled");
        return;
    }

    int totalQSOs = m_qsoTableModel->count();
    if (totalQSOs == 0) {
        m_statusLabel->setText("No QSOs to rebroadcast");
        return;
    }

    LOG_INFO("MainWindow", QString("Starting UDP rebroadcast of %1 QSOs").arg(totalQSOs));
    m_statusLabel->setText(QString("Starting UDP rebroadcast of %1 QSOs...").arg(totalQSOs));

    // Run in separate thread to avoid blocking UI
    // Note: Rebroadcast is read-only (no database writes), so it doesn't interfere with QSO logging
    auto future = QtConcurrent::run([this, totalQSOs]() {
        QString stationCall = AppSettings::instance().getMyCallsign();
        QString contestName = m_currentContest.contestName;

        int quarter = qMax(1, totalQSOs / 4);  // For progress updates
        int sent = 0;

        for (int row = 0; row < totalQSOs; ++row) {
            QSO qso = m_qsoTableModel->getQSO(row);

            // Broadcast this QSO
            m_udpBroadcastManager->onQSOLogged(qso, stationCall, contestName);
            sent++;

            // Progress updates at 25%, 50%, 75%
            if (sent == quarter) {
                QMetaObject::invokeMethod(this, [this, sent, totalQSOs]() {
                    m_statusLabel->setText(QString("UDP rebroadcast: %1/%2 (25%)")
                        .arg(sent).arg(totalQSOs));
                }, Qt::QueuedConnection);
            } else if (sent == quarter * 2) {
                QMetaObject::invokeMethod(this, [this, sent, totalQSOs]() {
                    m_statusLabel->setText(QString("UDP rebroadcast: %1/%2 (50%)")
                        .arg(sent).arg(totalQSOs));
                }, Qt::QueuedConnection);
            } else if (sent == quarter * 3) {
                QMetaObject::invokeMethod(this, [this, sent, totalQSOs]() {
                    m_statusLabel->setText(QString("UDP rebroadcast: %1/%2 (75%)")
                        .arg(sent).arg(totalQSOs));
                }, Qt::QueuedConnection);
            }

            // Wait after every 5 QSOs
            if ((sent % 5) == 0 && sent < totalQSOs) {
                QThread::msleep(100);  // 100ms wait between batches
            }
        }

        // Final status
        QMetaObject::invokeMethod(this, [this, totalQSOs]() {
            m_statusLabel->setText(QString("UDP rebroadcast complete: %1 QSOs sent").arg(totalQSOs));
            LOG_INFO("MainWindow", QString("UDP rebroadcast complete: %1 QSOs sent").arg(totalQSOs));
        }, Qt::QueuedConnection);
    });
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
    // Update band/mode (e.g., "15SSB") and frequency
    // Display even when radio not connected if we have valid band/mode/frequency from manual selection
    if (m_currentState.frequencyA > 0 && m_currentState.bandA != BandType::None) {
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

    // Update WPM label (only enabled in CW mode AND when auto-send is enabled)
    bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
    bool autoSendEnabled = AppSettings::instance().getAutoSendCW();
    int wpm = AppSettings::instance().getMorseWPM();
    m_radioWpmLabel->setText(QString("%1 WPM").arg(wpm));
    m_radioWpmLabel->setEnabled(isCWMode && autoSendEnabled);  // Gray out when not in CW mode or auto-send disabled

    // Update date/time (current local time)
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("ddd dd-MMM-yyyy");
    QString timeStr = now.toString("hh:mm:ss");
    m_radioDateLabel->setText(dateStr);
    m_radioTimeLabel->setText(timeStr);
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
    m_radioDateLabel->setStyleSheet(labelStyle);
    m_radioTimeLabel->setStyleSheet(labelStyle);

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

void MainWindow::reopenLastContest() {
    AppSettings& settings = AppSettings::instance();
    QString lastContestPath = settings.getLastContestPath();

    // Check if we have a last contest path and if the file exists
    if (lastContestPath.isEmpty() || !QFile::exists(lastContestPath)) {
        LOG_DEBUG("MainWindow", "No last contest to reopen");
        return;
    }

    LOG_DEBUG("MainWindow", QString("Attempting to reopen last contest: %1").arg(lastContestPath));

    // Open the database to read contest info
    Database& db = Database::instance();
    if (!db.open(lastContestPath)) {
        LOG_WARN("MainWindow", QString("Failed to reopen last contest database: %1").arg(db.lastError()));
        return;
    }

    // Read contest info from database
    QSqlQuery query = db.execute("SELECT contest_id, contest_name, start_time FROM contests LIMIT 1", {});
    if (!query.next()) {
        LOG_WARN("MainWindow", "Last contest database has no contest record");
        db.close();
        return;
    }

    // Build ContestInfo from database
    ContestInfo contestInfo;
    contestInfo.contestId = query.value(0).toString();
    contestInfo.contestName = query.value(1).toString();
    contestInfo.startDate = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());
    contestInfo.databasePath = lastContestPath;
    contestInfo.isExisting = true;

    // Determine contest type from contest_id (strip date suffix)
    // contest_id format: "CQWW_CW_2025_12_25" → contestType: "CQWW_CW"
    QString contestId = contestInfo.contestId;
    if (contestId.contains("CQWW_CW")) {
        contestInfo.contestType = "CQWW_CW";
    } else if (contestId.contains("CQWW_SSB")) {
        contestInfo.contestType = "CQWW_SSB";
    } else if (contestId.contains("CQWPX_CW")) {
        contestInfo.contestType = "CQWPX_CW";
    } else if (contestId.contains("CQWPX_SSB")) {
        contestInfo.contestType = "CQWPX_SSB";
    } else if (contestId.contains("WFD")) {
        contestInfo.contestType = "WFD";
    } else {
        LOG_WARN("MainWindow", QString("Could not determine contest type from ID: %1").arg(contestId));
        contestInfo.contestType = contestId;  // Fallback to full ID
    }

    // Determine mode from contest type
    if (contestInfo.contestType.contains("CW")) {
        contestInfo.mode = "CW";
    } else if (contestInfo.contestType.contains("SSB")) {
        contestInfo.mode = "SSB";
    } else {
        contestInfo.mode = "Mixed";
    }

    // Close database - activateContest will reopen it
    db.close();

    // Activate the contest
    activateContest(contestInfo);

    LOG_DEBUG("MainWindow", QString("Reopened last contest: %1").arg(contestInfo.contestName));
    m_statusLabel->setText(QString("Reopened: %1").arg(contestInfo.contestName));

    // Set focus to callsign entry for immediate logging
    m_callsignEntry->setFocus();
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
        for (QSO& qso : existingQSOs) {
            m_qsoTableModel->addQSO(qso);
        }
        LOG_DEBUG("MainWindow", QString("Loaded %1 existing QSOs").arg(existingQSOs.size()));

        // Update band summary grid with loaded QSOs
        updateScoreDisplay();

        // Scroll to bottom to show latest QSO and ensure scroll bars are visible
        if (!existingQSOs.isEmpty()) {
            m_qsoTableView->scrollToBottom();
            // Select the last row (most recent QSO)
            int lastRow = m_qsoTableModel->rowCount() - 1;
            m_qsoTableView->selectRow(lastRow);
        }

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
    } else if (contestInfo.contestType == "ARRL_SS_CW") {
        m_activeContest = new ARRLSweepstakesContest(ModeType::CW);
    } else if (contestInfo.contestType == "ARRL_SS_SSB") {
        m_activeContest = new ARRLSweepstakesContest(ModeType::USB);
    } else {
        LOG_WARN("MainWindow", QString("Unknown contest type: %1").arg(contestInfo.contestType));
        return;
    }

    // Reparse all loaded QSOs to populate parsedExchange field
    // (parsedExchange is not stored in database, must be regenerated)
    if (m_activeContest && m_qsoTableModel->count() > 0) {
        for (int i = 0; i < m_qsoTableModel->count(); ++i) {
            QSO qso = m_qsoTableModel->getQSO(i);
            qso.parsedExchange = m_activeContest->parseReceivedExchange(qso.exchangeReceived);
            m_qsoTableModel->updateQSO(i, qso);
        }
        LOG_DEBUG("MainWindow", QString("Reparsed exchange fields for %1 QSOs").arg(m_qsoTableModel->count()));
    }

    // Store contest info
    m_currentContest = contestInfo;
    m_hasActiveContest = true;

    // Save as last opened contest for auto-reopen on next startup
    AppSettings::instance().setLastContestPath(contestInfo.databasePath);

    // Update window title to include contest name
    setWindowTitle(QString("%1 v%2 - %3")
                      .arg(APP_NAME)
                      .arg(APP_VERSION)
                      .arg(contestInfo.contestName));

    // Set default band and mode from contest if radio not connected
    if (!m_radioConnected) {
        // Set mode based on contest type
        if (contestInfo.contestType.contains("CW")) {
            m_currentState.modeA = ModeType::CW;
        } else if (contestInfo.contestType.contains("SSB")) {
            m_currentState.modeA = ModeType::USB;
        } else {
            m_currentState.modeA = ModeType::CW;  // Default for mixed mode
        }

        // Set default band (20M is a good starting point)
        m_currentState.bandA = BandType::Band20M;

        // Set frequency for the default band/mode
        m_currentState.frequencyA = getFrequencyForBand(m_currentState.bandA, m_currentState.modeA);

        // Update display
        updateRadioStatusGrid();

        LOG_DEBUG("MainWindow", QString("Set default band/mode/freq: %1 %2 %3 Hz (radio not connected)")
            .arg(bandToString(m_currentState.bandA))
            .arg(modeToString(m_currentState.modeA))
            .arg(m_currentState.frequencyA));
    }

    // Recalculate points for all QSOs (fixes old QSOs with 0 points)
    // Must be called after m_activeContest is created
    if (m_qsoTableModel->count() > 0) {
        recalculateAllPoints();
    }

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
    // Try to get table column definitions first (for contests with custom display)
    QList<TableColumn> tableColumns = m_activeContest->getTableColumns();
    if (!tableColumns.isEmpty()) {
        // Contest provides custom column definitions
        m_qsoTableModel->setTableColumns(tableColumns);
    } else {
        // Use legacy method (auto-converts to TableColumn)
        m_qsoTableModel->setContestExchangeFields(receivedFields);
    }

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

    // Skip exchange memory for contests with serial numbers
    // Serial numbers auto-increment and should not be predicted from history
    if (m_activeContest->usesSerialNumbers()) {
        m_initialExchangePopulated = false;
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

bool MainWindow::checkForDuplicate(const QString& callsign, BandType band, ModeType mode, QString& dupeInfo) const {
    if (!m_activeContest) {
        return false;
    }

    // Get duplicate checking rule from contest
    DuplicateCheckingRule rule = m_activeContest->getDuplicateCheckingRule();

    // Convert band/mode enums to strings (database stores as TEXT)
    QString bandStr = bandToString(band);
    QString modeStr = modeToString(mode);

    // Build SQL query based on duplicate rule
    QString sql = "SELECT band, mode, timestamp FROM qsos WHERE callsign = ?";
    QVariantList params;
    params << callsign;

    // Add additional filters based on duplicate rule
    switch (rule) {
        case DuplicateCheckingRule::PerBandMode:
            sql += " AND band = ? AND mode = ?";
            params << bandStr << modeStr;
            break;
        case DuplicateCheckingRule::AllBandMode:
            sql += " AND mode = ?";
            params << modeStr;
            break;
        case DuplicateCheckingRule::PerBand:
            sql += " AND band = ?";
            params << bandStr;
            break;
        case DuplicateCheckingRule::AllBand:
            // No additional filter - any contact with this callsign is a dupe
            break;
    }

    sql += " LIMIT 1";

    Database& db = Database::instance();
    QSqlQuery query = db.execute(sql, params);

    if (query.next()) {
        // Found a duplicate - build info string
        QDateTime timestamp = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());

        switch (rule) {
            case DuplicateCheckingRule::PerBandMode:
                dupeInfo = QString("DUPE - Worked on %1 at %2")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
            case DuplicateCheckingRule::AllBandMode:
                dupeInfo = QString("DUPE - Worked on %1 at %2 (same mode, different band)")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
            case DuplicateCheckingRule::PerBand:
                dupeInfo = QString("DUPE - Worked on %1 at %2 (same band, different mode)")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
            case DuplicateCheckingRule::AllBand:
                dupeInfo = QString("DUPE - Worked on %1 at %2 (once-per-contest)")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
        }

        return true;
    }

    return false;
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

void MainWindow::onSendMorse() {
    if (!m_radioConnected) {
        QMessageBox::warning(this, "Radio Not Connected",
            "Radio must be connected to send morse code.\n\n"
            "Please connect to your radio first.");
        return;
    }

    SendMorseDialog dialog(m_radio, this);
    dialog.exec();
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
    if (m_radioConnected) {
        // Radio connected: Change radio frequency
        freq_t frequency = getFrequencyForBand(band, m_currentState.modeA);
        LOG_DEBUG("MainWindow", QString("Band clicked: %1 Setting frequency to: %2 Hz")
            .arg(QString::number(static_cast<int>(band)))
            .arg(QString::number(frequency)));

        // Send frequency change to radio
        m_radio->setFrequency(frequency);
    } else {
        // Radio not connected: Manual band selection for logging
        LOG_DEBUG("MainWindow", QString("Manual band selection: %1").arg(bandToString(band)));

        // Update current state with manually selected band and frequency
        m_currentState.bandA = band;

        // Set frequency to low end of band for logging purposes
        m_currentState.frequencyA = getFrequencyForBand(band, m_currentState.modeA);

        // Update radio status display
        updateRadioStatusGrid();

        // Update status message
        m_statusLabel->setText(QString("Band: %1 (manual)").arg(bandToString(band)));
    }
}

void MainWindow::onBandUp() {
    BandType currentBand = m_currentState.bandA;
    BandType nextBand = getNextBand(currentBand);

    if (nextBand != currentBand) {
        LOG_DEBUG("MainWindow", QString("Band up: %1 -> %2")
            .arg(bandToString(currentBand))
            .arg(bandToString(nextBand)));

        // Use band click handler (works for both connected and disconnected radio)
        onBandClicked(nextBand);
    } else {
        LOG_DEBUG("MainWindow", "Already at highest band");
    }
}

void MainWindow::onBandDown() {
    BandType currentBand = m_currentState.bandA;
    BandType prevBand = getPreviousBand(currentBand);

    if (prevBand != currentBand) {
        LOG_DEBUG("MainWindow", QString("Band down: %1 -> %2")
            .arg(bandToString(currentBand))
            .arg(bandToString(prevBand)));

        // Use band click handler (works for both connected and disconnected radio)
        onBandClicked(prevBand);
    } else {
        LOG_DEBUG("MainWindow", "Already at lowest band");
    }
}

freq_t MainWindow::getFrequencyForBand(BandType band, ModeType mode) const {
    Q_UNUSED(mode);  // Not used - we return band edge for manual selection

    // Return low band edge as visual reminder this is manually set, not from radio
    // Real radio would show frequency within CW/SSB segments
    switch (band) {
    case BandType::Band160M:
        return 1800000;   // 1.800 MHz (band edge)
    case BandType::Band80M:
        return 3500000;   // 3.500 MHz (band edge)
    case BandType::Band40M:
        return 7000000;   // 7.000 MHz (band edge)
    case BandType::Band20M:
        return 14000000;  // 14.000 MHz (band edge)
    case BandType::Band15M:
        return 21000000;  // 21.000 MHz (band edge)
    case BandType::Band10M:
        return 28000000;  // 28.000 MHz (band edge)
    default:
        return 14000000;  // Default to 20m band edge
    }
}

BandType MainWindow::getNextBand(BandType currentBand) const {
    // Contest bands in order from low to high frequency
    static const QList<BandType> contestBands = {
        BandType::Band160M,
        BandType::Band80M,
        BandType::Band40M,
        BandType::Band20M,
        BandType::Band15M,
        BandType::Band10M
    };

    int currentIndex = contestBands.indexOf(currentBand);
    if (currentIndex == -1 || currentIndex >= contestBands.size() - 1) {
        return currentBand;  // Already at highest or invalid band
    }

    return contestBands[currentIndex + 1];
}

BandType MainWindow::getPreviousBand(BandType currentBand) const {
    // Contest bands in order from low to high frequency
    static const QList<BandType> contestBands = {
        BandType::Band160M,
        BandType::Band80M,
        BandType::Band40M,
        BandType::Band20M,
        BandType::Band15M,
        BandType::Band10M
    };

    int currentIndex = contestBands.indexOf(currentBand);
    if (currentIndex <= 0) {
        return currentBand;  // Already at lowest or invalid band
    }

    return contestBands[currentIndex - 1];
}

} // namespace TR4QT
