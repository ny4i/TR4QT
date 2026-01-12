#include "MainWindow.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/BackupRestoreDialog.h"
#include "dialogs/OperatorDialog.h"
#include "dialogs/EditQSODialog.h"
#include "dialogs/ADIFImportDialog.h"
#include "dialogs/ExportPreviewDialog.h"
#include "dialogs/SendMorseDialog.h"
#include "dialogs/CWMessageEditorDialog.h"
#include "dialogs/FunctionKeysWindow.h"
#include "dialogs/GraylineMapDialog.h"
#include "widgets/DXClusterWindow.h"
#include "widgets/BandMapWidget.h"
#include "widgets/RadioControlWidget.h"
#include "widgets/MultiplierWidget.h"
#include "widgets/StatisticsWindow.h"
#include "NativeMapViewer.h"
#include "../network/UdpBroadcastManager.h"
#include "../network/WebServer.h"
#include "../controllers/ImportExportManager.h"
#include "../controllers/CWMessageManager.h"
#include "../controllers/BandSwitchingManager.h"
#include "../core/Constants.h"
#include "../core/BandConstants.h"
#include "../logging/LogMacros.h"
#include "../logging/Logger.h"
#include "../utils/ThemeManager.h"
#include "../utils/DialogHelper.h"
#include "../utils/AppSettings.h"
#include "../utils/PerformanceProfiler.h"
#include "../utils/ADIFExporter.h"
#include "../utils/CabrilloExporter.h"
#include "../utils/CallsignValidator.h"
#include "../utils/CountryFileDownloader.h"
#include "../utils/LOTWUserDownloader.h"
#include "../utils/SCPDownloader.h"
#include "../utils/GeographicUtils.h"
#include "../utils/PathManager.h"
#include "../data/Database.h"
#include "../data/QSORepository.h"
#include "../data/LOTWUserRepository.h"
#include "../data/BackupManager.h"
#include "../data/ExchangeMemoryRepository.h"
#include "../data/SCPRepository.h"
#include "../commands/CommandDispatcher.h"
#include "../contests/RSTValidator.h"
#include "../cw/CWTemplateEngine.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
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
#include <QInputDialog>
#include "../utils/SelectableMessageBox.h"
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
#include <QTimeZone>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>
#include <QAbstractButton>
#include <QProcess>
#include <QDateTime>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileInfo>
#include <hamlib/rig.h>

namespace TR4QT {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_radio(nullptr)  // Will be set from RadioManager after it's created
    , m_radioConnected(false)
    , m_radioAutoReconnect(false)
    , m_radioReconnectTimer(new QTimer(this))
    , m_radioReconnectAttempts(0)
    , m_radioFlashTimer(new QTimer(this))
    , m_radioFlashState(false)
    , m_hamPrivileges(nullptr)
    , m_dxClusterWindow(nullptr)
    , m_bandMapWindow(nullptr)
    , m_radioControlWindow(nullptr)
    , m_multiplierWindow(nullptr)
    , m_statisticsWindow(nullptr)
    , m_functionKeysWindow(nullptr)
    , m_sectionsMapViewer(nullptr)
    , m_statesMapViewer(nullptr)
    , m_graylineMapDialog(nullptr)
    , m_qsosThisHour(0)
    , m_qsosSinceLastIntegrityCheck(0)
    , m_hasActiveContest(false)
    , m_activeContest(nullptr)
    , m_currentContestDbId(-1)
    , m_nextSerialNumber(1)
    , m_qsoLogger(nullptr)
    , m_integrityManager(nullptr)
    , m_contestManager(nullptr)
    , m_contestService(nullptr)
    , m_menuManager(nullptr)
    , m_settingsManager(nullptr)
    , m_windowManager(nullptr)
    , m_importExportManager(nullptr)
    , m_downloadManager(nullptr)
    , m_radioManager(nullptr)
    , m_bandSwitchingManager(nullptr)
    , m_cwMessageManager(nullptr)
    , m_qsoTableModel(new QSOTableModel(this))
    , m_scpMatcher(new SCPMatcher())
    , m_countryFileDownloader(new CountryFileDownloader(this))
    , m_latestCTYVersion(0)
    , m_udpBroadcastManager(new UdpBroadcastManager(this))
    , m_webServer(new WebServer(m_qsoTableModel, m_radio, this))
    , m_inRaiseAllWindows(false)
    , m_initialExchangePopulated(false)
    , m_operatingMode(OperatingMode::CQ)
    , m_operatingModeLabel(nullptr)
    , m_lastFrequency(0)
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

    // Create ContestManager with country file
    ContestManager::Config contestManagerConfig;
    contestManagerConfig.countryFile = &m_countryFile;
    m_contestManager = new ContestManager(contestManagerConfig);
    LOG_DEBUG("MainWindow", "ContestManager created");

    // Create MenuManager (will be used in setupUI -> createMenuBar)
    m_menuManager = new MenuManager(this);
    LOG_DEBUG("MainWindow", "MenuManager created");

    setupUI();

    // Create managers after setupUI (some need UI widgets)
    m_downloadManager = new DownloadManager({&m_countryFile}, this);
    m_radioManager = new RadioManager(this);
    m_radio = m_radioManager->radioController();  // Get RadioController from RadioManager
    m_bandSwitchingManager = new BandSwitchingManager(this);
    m_cwMessageManager = new CWMessageManager({m_radio, nullptr});  // Contest set later
    m_windowManager = new WindowManager(this);
    m_settingsManager = new SettingsManager();
    LOG_DEBUG("MainWindow", "Controllers and UI managers created");

    loadSettings();
    loadUdpBroadcastSettings();

    // Initialize ham radio privileges validator with license class from settings
    QString licenseClassStr = AppSettings::instance().getLicenseClass();
    HamRadioPrivileges::LicenseClass licenseClass =
        HamRadioPrivileges::stringToLicenseClass(licenseClassStr);
    m_hamPrivileges = new HamRadioPrivileges(licenseClass);

    // Initialize backup manager from settings
    loadBackupSettings();

    // Reopen last contest if available
    reopenLastContest();

    // Create ImportExportManager (needs contest context)
    ImportExportManager::Config importExportConfig;
    importExportConfig.countryFile = &m_countryFile;
    importExportConfig.qsoTableModel = m_qsoTableModel;
    importExportConfig.activeContest = m_activeContest;
    importExportConfig.currentContestDbId = m_currentContestDbId;
    importExportConfig.currentContestName = m_currentContest.contestName;
    importExportConfig.hasActiveContest = m_hasActiveContest;
    m_importExportManager = new ImportExportManager(importExportConfig, this);
    LOG_DEBUG("MainWindow", "ImportExportManager created");

    // Initialize web server with current operator (only if explicitly set)
    AppSettings& webSettings = AppSettings::instance();
    QString currentOperator = webSettings.getCurrentOperator();
    QString stationCall = webSettings.getMyCallsign();

    // WebServer now pulls operator from AppSettings - no need to push

    // Auto-start web server if enabled
    if (webSettings.getWebServerAutoStart()) {
        quint16 port = webSettings.getWebServerPort();
        QString addressStr = webSettings.getWebServerAddress();
        QHostAddress address(addressStr);

        if (m_webServer->start(port, address)) {
            m_webServerAction->setText("Stop Web Server");
            LOG_INFO("MainWindow", QString("Web server auto-started: %1").arg(m_webServer->url()));
        } else {
            // Auto-start failed - just log error, don't show dialog
            LOG_ERROR("MainWindow", QString("Web server auto-start failed on %1:%2 (port may be in use)")
                .arg(addressStr).arg(port));
        }
    }

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

    // Connect radio signals from RadioManager (already connected to RadioController internally)
    connect(m_radioManager, &RadioManager::connectionStatusChanged,
            this, &MainWindow::onRadioConnected);
    connect(m_radioManager, &RadioManager::radioStateUpdated,
            this, &MainWindow::onRadioStateUpdated);
    connect(m_radioManager, &RadioManager::radioErrorOccurred,
            this, &MainWindow::onRadioError);
    connect(m_radioManager, &RadioManager::radioModelChanged,
            this, &MainWindow::onRadioModelChanged);
    connect(m_radioManager, &RadioManager::statusMessage,
            this, [this](const QString& message) { m_statusLabel->setText(message); });
    connect(m_radioManager, &RadioManager::flashStateChanged,
            this, [this](bool flashState) {
                m_radioFlashState = flashState;
                updateRadioStatusFlash();
            });

    // NOTE: Radio reconnection and flash timers are now handled by RadioManager
    // These local timers and variables (m_radioReconnectTimer, m_radioFlashTimer, etc.)
    // can be removed in a future cleanup

    // Connect CTY.DAT update notification
    connect(m_countryFileDownloader, &CountryFileDownloader::updateAvailable,
            this, &MainWindow::onCTYUpdateAvailable);

    // Check for CTY.DAT updates 2 seconds after startup (async, non-blocking)
    QTimer::singleShot(2000, this, [this]() {
        LOG_DEBUG("MainWindow", "Checking for CTY.DAT updates...");
        m_countryFileDownloader->checkLatestVersion();
    });

    // Initialize radio status display (date/time, band/mode/freq defaults)
    updateRadioStatusGrid();

    // Try auto-connect if enabled and config exists
    AppSettings& settings = AppSettings::instance();
    if (settings.hasRadioConfig()) {
        RadioConfig config = settings.loadRadioConfig();
        // Only auto-connect if a valid radio model is selected (not "Select radio...")
        if (config.hamlibModelId > 0 && settings.getRadioAutoConnect()) {
            // Auto-connect enabled - connect now
            m_statusLabel->setText("Auto-connecting to radio...");
            QTimer::singleShot(500, this, &MainWindow::onRadioConnect);  // Slight delay to let UI initialize
        } else if (config.hamlibModelId > 0) {
            m_statusLabel->setText("Found saved radio configuration. Use Radio → Connect to connect.");
        } else {
            m_statusLabel->setText("No valid radio model selected. Use Radio → Configure.");
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
            QMessageBox::StandardButton reply = DialogHelper::question(
                this,
                "Grid Square Not Configured",
                "Your Maidenhead grid square is not configured.\n\n"
                "The grid square is needed to calculate distance and azimuth "
                "to DX spots in the band map.\n\n"
                "Would you like to configure it now in Preferences?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes);

            if (reply == QMessageBox::Yes) {
                onPreferences();
            }
        }
    });

    // Check auto-send status and show message if disabled
    QTimer::singleShot(100, this, [this]() {
        if (!AppSettings::instance().getCWAutoSendEnabled()) {
            m_statusLabel->setText("⚠ CW Auto-Send is OFF - Enable in Radio menu");
            m_statusLabel->setStyleSheet("QLabel { color: #ff6600; font-weight: bold; }");
            LOG_INFO("MainWindow", "Auto-Send is disabled at startup");
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

void MainWindow::triggerCountryFileDownload() {
    onDownloadCTY(false);
}

void MainWindow::setupUI() {
    createMenuBar();
    createCentralWidget();  // Also sets minimum width dynamically
    createStatusBar();

    // Set minimum height
    setMinimumHeight(UIDefaults::MAIN_WINDOW_MIN_HEIGHT);

    // Set initial size (user can resize larger or smaller, but not below minimum)
    resize(UIDefaults::MAIN_WINDOW_WIDTH, UIDefaults::MAIN_WINDOW_HEIGHT);
}

void MainWindow::createMenuBar() {
    // Configure MenuManager callbacks
    MenuManager::Config config;
    
    // File menu
    config.onNewOpenContest = [this]() { onNewOpenContest(); };
    config.onClearLog = [this]() { onClearLog(); };
    config.onImportADIF = [this]() { onImportADIF(); };
    config.onExportADIF = [this]() { onExportADIF(); };
    config.onExportCabrillo = [this]() { onExportCabrillo(); };
    config.onPreferences = [this]() { onPreferences(); };
    config.onExit = [this]() { onExit(); };
    
    // Radio menu
    config.onRadioConfigure = [this]() { onRadioConfigure(); };
    config.onRadioConnect = [this]() { onRadioConnect(); };
    config.onRadioDisconnect = [this]() { onRadioDisconnect(); };
    config.onAutoSendCWToggled = [this](bool checked) {
        AppSettings::instance().setCWAutoSendEnabled(checked);
        LOG_DEBUG("MainWindow", QString("Auto Send CW %1").arg(checked ? "enabled" : "disabled"));
        updateRadioStatusGrid();
    };
    config.onUpdateRadioStatusGrid = [this]() { updateRadioStatusGrid(); };
    
    // Edit menu
    config.onViewEditLog = [this]() { onViewEditLog(); };
    config.onClearDupes = [this]() { onClearDupes(); };
    config.onNote = [this]() { onNote(); };
    config.onRecallLast = [this]() { onRecallLast(); };
    
    // Tools menu
    config.onWKMode = [this]() { onWKMode(); };
    config.onBackupLog = [this]() { onBackupLog(); };
    config.onDownloadCTY = [this]() { onDownloadCTY(); };
    config.onDownloadLOTW = [this]() { onDownloadLOTW(); };
    config.onDownloadSCP = [this]() { onDownloadSCP(); };
    config.onInitialize = [this]() { onInitialize(); };
    config.onRescoreContest = [this]() { onRescoreContest(); };
    config.onEditContestSettings = [this]() { onEditContestSettings(); };
    config.onFullIntegrityCheck = [this]() { onFullIntegrityCheck(); };
    config.onToggleWebServer = [this]() { onToggleWebServer(); };
    config.onResetWindowPositions = [this]() { onResetWindowPositions(); };
    
    // Operating menu
    config.onAutoCQ = [this]() { onAutoCQ(); };
    config.onAutoCQResume = [this]() { onAutoCQResume(); };
    config.onKillCW = [this]() { onKillCW(); };
    config.onDupeCheck = [this]() { onDupeCheck(); };
    config.onSearchLog = [this]() { onSearchLog(); };
    config.onDeleteLastQSO = [this]() { onDeleteLastQSO(); };
    config.onIncNumber = [this]() { onIncNumber(); };
    config.onInitialExchange = [this]() { onInitialExchange(); };
    config.onToggleSidetone = [this]() { onToggleSidetone(); };
    config.onToggleAutosend = [this]() { onToggleAutosend(); };
    
    // Commands menu
    config.onCQMode = [this]() { onCQMode(); };
    config.onSPMode = [this]() { onSPMode(); };
    
    // Automation menu
    config.onAutoSPEnableToggled = [this](bool checked) {
        AppSettings::instance().setAutoSPEnable(checked);
        LOG_DEBUG("MainWindow", QString("AUTO S&P %1").arg(checked ? "enabled" : "disabled"));
    };
    config.onAutoSPSensitivity = [this]() {
        bool ok;
        int currentSensitivity = AppSettings::instance().getAutoSPSensitivity();
        int newSensitivity = QInputDialog::getInt(
            this,
            "AUTO S&P Sensitivity",
            "Frequency change threshold (Hz):",
            currentSensitivity,
            AUTO_SP_SENSITIVITY_MIN_HZ,
            AUTO_SP_SENSITIVITY_MAX_HZ,
            AUTO_SP_SENSITIVITY_STEP_HZ,
            &ok
        );
        if (ok) {
            AppSettings::instance().setAutoSPSensitivity(newSensitivity);
            LOG_DEBUG("MainWindow", QString("AUTO S&P sensitivity: %1 Hz").arg(newSensitivity));
        }
    };
    
    // Window menu
    config.onShowBandMap = [this]() { onShowBandMap(); };
    config.onShowDXCluster = [this]() { onShowDXCluster(); };
    config.onShowRadioControl = [this]() { onShowRadioControl(); };
    config.onSendMorse = [this]() { onSendMorse(); };
    config.onEditCWMessages = [this]() { onEditCWMessages(); };
    config.onShowFunctionKeysRef = [this]() { onShowFunctionKeysRef(); };
    config.onShowMultipliers = [this]() { onShowMultipliers(); };
    config.onShowStatistics = [this]() { onShowStatistics(); };
    config.onShowSectionsMap = [this]() { onShowSectionsMap(); };
    config.onShowStatesMap = [this]() { onShowStatesMap(); };
    config.onShowGraylineMap = [this]() { onShowGraylineMap(); };
    config.onSwapMultView = [this]() { onSwapMultView(); };
    config.onMissingMultsReport = [this]() { onMissingMultsReport(); };
    
    // Band menu
    config.onBandUp = [this]() { onBandUp(); };
    config.onBandDown = [this]() { onBandDown(); };
    config.onToggleRigs = [this]() { onToggleRigs(); };
    config.onEditSO2R = [this]() { onEditSO2R(); };
    
    // Help menu
    config.onAbout = [this]() { onAbout(); };
#ifdef ENABLE_PERFORMANCE_PROFILING
    config.onShowPerformanceReport = [this]() { onShowPerformanceReport(); };
#endif
    config.onEmailLogsToSupport = [this]() { onEmailLogsToSupport(); };
    
    // Create menu bar using MenuManager
    QMenuBar* menuBar = m_menuManager->createMenuBar(config);
    setMenuBar(menuBar);
    
    // Store action references for MainWindow to update
    m_connectAction = m_menuManager->connectAction();
    m_disconnectAction = m_menuManager->disconnectAction();
    m_autoSendCWAction = m_menuManager->autoSendCWAction();
    m_webServerAction = m_menuManager->webServerAction();
    m_bandMapAction = m_menuManager->bandMapAction();
    m_dxClusterAction = m_menuManager->dxClusterAction();
    m_radioControlAction = m_menuManager->radioControlAction();
    m_multipliersAction = m_menuManager->multipliersAction();
    m_statisticsAction = m_menuManager->statisticsAction();
    m_sectionsMapAction = m_menuManager->sectionsMapAction();
    m_statesMapAction = m_menuManager->statesMapAction();
    m_graylineMapAction = m_menuManager->graylineMapAction();
}

void MainWindow::createCentralWidget() {
    QWidget* central = new QWidget(this);
    central->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Top: Band summary grid and needs display (horizontal layout)
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(4);

    // Left: Band summary grid (takes 75% of width)
    m_bandSummaryGrid = new BandSummaryGrid(this);
    m_bandSummaryGrid->setEnabled(true);  // Widget always enabled for visibility
    // Disable band selection buttons until radio connects
    // (will be enabled in onRadioConnectionChanged when radio connects)
    m_bandSummaryGrid->setBandSelectionEnabled(false);
    connect(m_bandSummaryGrid, &BandSummaryGrid::bandClicked,
            this, &MainWindow::onBandClicked);
    topLayout->addWidget(m_bandSummaryGrid, 3);  // Stretch factor 3

    // Right: Vertical layout for needs display and station info
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(4);

    // Needs display widget (top)
    m_needsDisplayWidget = new NeedsDisplayWidget(this);
    m_needsDisplayWidget->setMinimumWidth(UIDefaults::NEEDS_DISPLAY_MIN_WIDTH);
    rightLayout->addWidget(m_needsDisplayWidget);

    rightLayout->addStretch();  // Push needs widget to top

    // Station info labels (at bottom of right column)
    int miscFontSize = AppSettings::instance().getMiscDisplayFontSize();
    QFont stationInfoFont("Monospace", miscFontSize);
    QString stationInfoColor = ThemeManager::instance().colorName(ColorRole::LotwUserText);

    // Line 1: Country name
    m_countryNameLabel = new QLabel(this);
    m_countryNameLabel->setFont(stationInfoFont);
    m_countryNameLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(stationInfoColor));
    m_countryNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_countryNameLabel->setText("");  // Empty initially
    rightLayout->addWidget(m_countryNameLabel);

    // Line 2: Station info (prefix, bearing, distance)
    m_stationInfoLabel = new QLabel(this);
    m_stationInfoLabel->setFont(stationInfoFont);
    m_stationInfoLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(stationInfoColor));
    m_stationInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_stationInfoLabel->setText("");  // Empty initially
    rightLayout->addWidget(m_stationInfoLabel);

    topLayout->addLayout(rightLayout, 1);  // Stretch factor 1

    mainLayout->addLayout(topLayout);

    // Middle: QSO table (takes most space)
    // m_qsoTableModel already initialized in initializer list
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

    // Make all columns user-resizable (except last which stretches)
    for (int i = 0; i < QSOTableModel::ColCount - 1; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Interactive);
    }

    // Op column stretches to fill remaining space (last column)
    header->setSectionResizeMode(QSOTableModel::ColOp, QHeaderView::Stretch);

    // Initially resize all columns to contents to trim whitespace
    m_qsoTableView->resizeColumnsToContents();

    // Calculate minimum column widths based on font metrics (no magic numbers!)
    // Add padding for column margins and sort indicator
    const int COLUMN_PADDING = 20;          // Base padding for margins
    const int HEADER_SORT_INDICATOR = 15;   // Extra space for sort arrow in header
    QFontMetrics fm = m_qsoTableView->fontMetrics();

    // Calculate widths based on typical/maximum content for each column
    // For Date/Time columns, add extra padding for header sort indicator
    int bandWidth = fm.horizontalAdvance("160SSB") + COLUMN_PADDING;                            // Longest band/mode combo
    int dateWidth = fm.horizontalAdvance("12-31-2025") + COLUMN_PADDING + HEADER_SORT_INDICATOR;  // Date format: MM-dd-yyyy
    int utcWidth = fm.horizontalAdvance("23:59") + COLUMN_PADDING + HEADER_SORT_INDICATOR;        // Time format: HH:mm
    int qsosWidth = fm.horizontalAdvance("99999") + COLUMN_PADDING;       // QSO number (up to 5 digits)
    int callsignWidth = fm.horizontalAdvance("WW1ABC/QRP") + COLUMN_PADDING; // Longest typical callsign
    int exchWidth = fm.horizontalAdvance("WWWW") + COLUMN_PADDING;        // Exchange field (4 chars typical)
    int ptsWidth = fm.horizontalAdvance("999") + COLUMN_PADDING;          // Points (up to 3 digits)
    int mWidth = fm.horizontalAdvance("xdzp") + COLUMN_PADDING;           // Multiplier indicators (4 chars max)
    int idWidth = fm.horizontalAdvance("A") + COLUMN_PADDING;             // Computer ID (1 char)
    int multWidth = fm.horizontalAdvance("$") + COLUMN_PADDING;           // S&P indicator (1 char)
    int dupeWidth = fm.horizontalAdvance("D") + COLUMN_PADDING;           // Dupe indicator (1 char)
    int freqWidth = fm.horizontalAdvance("14350.0") + COLUMN_PADDING;     // Frequency (kHz with 1 decimal)

    // Set minimum widths to ensure no truncation at startup
    m_qsoTableView->setColumnWidth(QSOTableModel::ColBand, qMax(bandWidth, m_qsoTableView->columnWidth(QSOTableModel::ColBand)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColDate, qMax(dateWidth, m_qsoTableView->columnWidth(QSOTableModel::ColDate)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColUTC, qMax(utcWidth, m_qsoTableView->columnWidth(QSOTableModel::ColUTC)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColQSOs, qMax(qsosWidth, m_qsoTableView->columnWidth(QSOTableModel::ColQSOs)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColCallsign, qMax(callsignWidth, m_qsoTableView->columnWidth(QSOTableModel::ColCallsign)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColExch1, qMax(exchWidth, m_qsoTableView->columnWidth(QSOTableModel::ColExch1)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColExch2, qMax(exchWidth, m_qsoTableView->columnWidth(QSOTableModel::ColExch2)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColPts, qMax(ptsWidth, m_qsoTableView->columnWidth(QSOTableModel::ColPts)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColM, qMax(mWidth, m_qsoTableView->columnWidth(QSOTableModel::ColM)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColId, qMax(idWidth, m_qsoTableView->columnWidth(QSOTableModel::ColId)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColMult, qMax(multWidth, m_qsoTableView->columnWidth(QSOTableModel::ColMult)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColDupe, qMax(dupeWidth, m_qsoTableView->columnWidth(QSOTableModel::ColDupe)));
    m_qsoTableView->setColumnWidth(QSOTableModel::ColFreq, qMax(freqWidth, m_qsoTableView->columnWidth(QSOTableModel::ColFreq)));

    // Connect signal to save column widths when user resizes
    connect(header, &QHeaderView::sectionResized, this, &MainWindow::onQSOTableColumnResized);

    // Note: Column widths are loaded in updateExchangeFieldsForContest() after contest is activated

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

    // Calculate minimum window width based on bottom panel's sizeHint
    // This ensures all widgets fit without overlap
    int bottomPanelMinWidth = bottomPanel->minimumSizeHint().width();
    if (bottomPanelMinWidth < 900) {
        bottomPanelMinWidth = 900;  // Ensure reasonable minimum
    }
    setMinimumWidth(bottomPanelMinWidth + 40);  // Add margins
}

QWidget* MainWindow::createBottomPanel() {
    QWidget* bottomPanel = new QWidget(this);
    bottomPanel->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setSpacing(15);
    bottomLayout->setContentsMargins(10, 4, 10, 4);

    // LEFT: Radio status (frequency, band/mode, time) in container widget
    QWidget* radioStatusWidget = new QWidget(this);
    radioStatusWidget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QHBoxLayout* radioLayout = new QHBoxLayout(radioStatusWidget);
    radioLayout->setSpacing(15);
    radioLayout->setContentsMargins(10, 5, 10, 5);

    QFont labelFont("Monospace", 11);
    labelFont.setBold(true);

    // Band/Mode label (e.g., "15SSB")
    m_radioFreqBandLabel = new QLabel("--", radioStatusWidget);
    m_radioFreqBandLabel->setFont(labelFont);
    m_radioFreqBandLabel->setMinimumWidth(UIDefaults::RADIO_FREQ_BAND_LABEL_WIDTH);
    m_radioFreqBandLabel->setAlignment(Qt::AlignCenter);

    // Frequency label
    QFont freqFont("Monospace", 10);
    m_radioFreqLabel = new QLabel("0.000 MHz", radioStatusWidget);
    m_radioFreqLabel->setFont(freqFont);
    m_radioFreqLabel->setMinimumWidth(UIDefaults::RADIO_FREQ_LABEL_WIDTH);
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
    m_radioWpmLabel->setMinimumWidth(UIDefaults::RADIO_WPM_LABEL_WIDTH);
    m_radioWpmLabel->setEnabled(false);  // Grayed out by default

    // Date/Time labels - stacked vertically to save width
    QFont dateTimeFont("Monospace", labelFont.pointSize());

    m_radioDateLabel = new QLabel("", radioStatusWidget);
    m_radioDateLabel->setFont(dateTimeFont);
    m_radioDateLabel->setAlignment(Qt::AlignCenter);
    m_radioDateLabel->setMinimumWidth(UIDefaults::RADIO_DATE_LABEL_WIDTH);  // Narrower than single label

    m_radioTimeLabel = new QLabel("", radioStatusWidget);
    m_radioTimeLabel->setFont(dateTimeFont);
    m_radioTimeLabel->setAlignment(Qt::AlignCenter);
    m_radioTimeLabel->setMinimumWidth(UIDefaults::RADIO_TIME_LABEL_WIDTH);

    // Vertical layout for date and time
    QVBoxLayout* dateTimeLayout = new QVBoxLayout();
    dateTimeLayout->setSpacing(2);
    dateTimeLayout->addWidget(m_radioDateLabel);
    dateTimeLayout->addWidget(m_radioTimeLabel);

    radioLayout->addLayout(freqLayout);
    radioLayout->addWidget(m_radioWpmLabel);
    radioLayout->addLayout(dateTimeLayout);

    // Calculate minimum width for radio status widget based on child components
    int radioStatusMinWidth = m_radioFreqLabel->minimumWidth() +  // 100px
                              m_radioWpmLabel->minimumWidth() +   // 80px
                              m_radioDateLabel->minimumWidth() +  // 120px
                              radioLayout->spacing() * 2 +        // Spacing between 3 items
                              radioLayout->contentsMargins().left() +
                              radioLayout->contentsMargins().right();
    radioStatusWidget->setMinimumWidth(radioStatusMinWidth);

    bottomLayout->addWidget(radioStatusWidget);

    // Smaller stretch to move Call/Exch closer to radio info
    bottomLayout->addStretch(0);  // Changed from 1 to 0 (half distance)

    // CENTER: Entry fields (vertical layout)
    QWidget* entryWidget = new QWidget(this);
    entryWidget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    entryWidget->setMinimumWidth(UIDefaults::ENTRY_WIDGET_MIN_WIDTH);  // Prevent SCP label from overlapping entry fields
    QGridLayout* entryLayout = new QGridLayout(entryWidget);
    entryLayout->setSpacing(4);
    entryLayout->setContentsMargins(0, 0, 0, 0);

    AppSettings& settings = AppSettings::instance();
    int miscFontSize = settings.getMiscDisplayFontSize();

    // Entry field width - both callsign and exchange use same width for consistent layout
    const int ENTRY_FIELD_WIDTH = 200;

    QLabel* callLabel = new QLabel("Call:", this);
    m_callsignEntry = new QLineEdit(this);
    m_callsignEntry->setPlaceholderText("Callsign");
    m_callsignEntry->setFixedWidth(ENTRY_FIELD_WIDTH);
    m_callsignEntry->setFont(QFont("Monospace", miscFontSize));

    QLabel* exchLabel = new QLabel("Exch:", this);
    m_exchangeEntry = new QLineEdit(this);
    m_exchangeEntry->setPlaceholderText("RST + Zone");
    m_exchangeEntry->setFixedWidth(ENTRY_FIELD_WIDTH);
    m_exchangeEntry->setFont(QFont("Monospace", miscFontSize));

    // Row 0: Call label and entry
    entryLayout->addWidget(callLabel, 0, 0);
    entryLayout->addWidget(m_callsignEntry, 0, 1);

    // Row 1: Exch label and entry
    entryLayout->addWidget(exchLabel, 1, 0);
    entryLayout->addWidget(m_exchangeEntry, 1, 1);

    // Add spacing between entry fields and SCP labels
    entryLayout->setColumnMinimumWidth(2, 20);  // 20px gap

    // SCP matches label (column 3, spans both rows - 2-column grid)
    int scpFontSize = settings.getSCPFontSize();
    m_scpMatchesLabel = new QLabel(this);
    m_scpMatchesLabel->setStyleSheet(QString("QLabel { color: #0066cc; font-size: %1pt; }").arg(scpFontSize));
    m_scpMatchesLabel->setMinimumWidth(UIDefaults::SCP_MATCHES_LABEL_WIDTH);
    m_scpMatchesLabel->setMaximumWidth(UIDefaults::SCP_MATCHES_LABEL_WIDTH);

    // Calculate max height based on entry field heights (spans 2 rows)
    int entryRowHeight = m_callsignEntry->sizeHint().height();
    int scpMaxHeight = (entryRowHeight * 2) + entryLayout->verticalSpacing();
    m_scpMatchesLabel->setMinimumHeight(scpMaxHeight);
    m_scpMatchesLabel->setMaximumHeight(scpMaxHeight);  // Fixed height - prevents layout shift when matches change

    m_scpMatchesLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_scpMatchesLabel->setWordWrap(true);
    m_scpMatchesLabel->setText("");  // Always visible, just empty when no matches
    entryLayout->addWidget(m_scpMatchesLabel, 0, 3, 2, 1);  // Span both rows, column 3

    // Calculate minimum width for entry widget based on child components
    // Column 0: labels (estimate ~50px), Column 1: entry fields (150px min),
    // Column 2: spacing (20px), Column 3: SCP label (120px)
    QLabel tempLabel("Exch:");  // Temporary label to measure width
    int labelWidth = tempLabel.sizeHint().width() + 10;  // Add padding
    int entryMinWidth = labelWidth +                      // Column 0: labels
                        m_callsignEntry->minimumWidth() + // Column 1: entry field
                        20 +                               // Column 2: spacing
                        m_scpMatchesLabel->minimumWidth() + // Column 3: SCP label
                        entryLayout->horizontalSpacing() * 3 + // Spacing between columns
                        entryLayout->contentsMargins().left() +
                        entryLayout->contentsMargins().right();
    entryWidget->setMinimumWidth(entryMinWidth);

    bottomLayout->addWidget(entryWidget);

    // Stretch to push Stats to right
    bottomLayout->addStretch(1);

    // Right side: Stats panel
    QWidget* statsWidget = new QWidget(this);
    statsWidget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setSpacing(2);
    statsLayout->setContentsMargins(4, 4, 4, 4);

    // settings and miscFontSize already declared above
    QFont monoFont("Monospace", miscFontSize);

    // Time and rate
    QHBoxLayout* timeRow = new QHBoxLayout();
    m_timeLabel = new QLabel("00:00:00", this);
    m_timeLabel->setFont(monoFont);
    m_timeLabel->setMinimumWidth(UIDefaults::TIME_LABEL_MIN_WIDTH);  // Fixed width to prevent layout shifts
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
    statsWidget->setMinimumWidth(UIDefaults::STATS_WIDGET_MIN_WIDTH);
    statsWidget->setMaximumWidth(UIDefaults::STATS_WIDGET_MAX_WIDTH);
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

    // Install event filter for ESC key handling
    m_callsignEntry->installEventFilter(this);
    m_exchangeEntry->installEventFilter(this);

    return bottomPanel;
}

void MainWindow::createStatusBar() {
    QStatusBar* status = new QStatusBar(this);
    setStatusBar(status);

    m_statusLabel = new QLabel("Ready", this);
    status->addWidget(m_statusLabel);

    // Operating mode indicator (CQ vs S&P)
    m_operatingModeLabel = new QLabel("CQ", this);
    m_operatingModeLabel->setStyleSheet("color: green; font-weight: bold; padding: 0 10px;");
    m_operatingModeLabel->setToolTip("Operating Mode: CQ (running) or S&P (search and pounce)");
    status->addPermanentWidget(m_operatingModeLabel);

    m_radioStatusLabel = new QLabel("Radio: Not Connected", this);
    m_radioStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    status->addPermanentWidget(m_radioStatusLabel);
}

void MainWindow::loadSettings() {
    if (!m_settingsManager) {
        return;
    }

    // Load and restore window geometry
    WindowGeometry geometry = m_settingsManager->loadWindowGeometry();

    if (!geometry.mainWindowGeometry.isNull()) {
        restoreGeometry(geometry.mainWindowGeometry);
    }
    if (!geometry.mainWindowState.isNull()) {
        restoreState(geometry.mainWindowState);
    }

    // Restore operator
    if (!geometry.currentOperator.isEmpty()) {
        if (m_operatorLabel) {
            m_operatorLabel->setText(geometry.currentOperator);
        }
    }

    // Apply font settings
    applyFontSettings();

    // Connect to theme changes and apply initial theme
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::applyTheme);
    applyTheme();

    // Restore child windows if they were visible
    if (geometry.dxClusterVisible) {
        LOG_DEBUG("MainWindow", "Restoring DX Cluster window (was visible on exit)");
        onShowDXCluster();
        if (m_dxClusterWindow && !geometry.dxClusterGeometry.isEmpty()) {
            m_dxClusterWindow->restoreGeometry(geometry.dxClusterGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring DX Cluster window (was hidden on exit)");
    }

    if (geometry.bandMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring Band Map window (was visible on exit)");
        onShowBandMap();
        if (m_bandMapWindow && !geometry.bandMapGeometry.isEmpty()) {
            m_bandMapWindow->restoreGeometry(geometry.bandMapGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Band Map window (was hidden on exit)");
    }

    if (geometry.radioControlVisible) {
        LOG_DEBUG("MainWindow", "Restoring Radio Control window (was visible on exit)");
        onShowRadioControl();
        if (m_radioControlWindow && !geometry.radioControlGeometry.isEmpty()) {
            m_radioControlWindow->restoreGeometry(geometry.radioControlGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Radio Control window (was hidden on exit)");
    }

    if (geometry.multipliersVisible) {
        LOG_DEBUG("MainWindow", "Restoring Multipliers window (was visible on exit)");
        onShowMultipliers();
        if (m_multiplierWindow && !geometry.multipliersGeometry.isEmpty()) {
            m_multiplierWindow->restoreGeometry(geometry.multipliersGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Multipliers window (was hidden on exit)");
    }

    if (geometry.statisticsVisible) {
        LOG_DEBUG("MainWindow", "Restoring Statistics window (was visible on exit)");
        onShowStatistics();
        if (m_statisticsWindow && !geometry.statisticsGeometry.isEmpty()) {
            m_statisticsWindow->restoreGeometry(geometry.statisticsGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Statistics window (was hidden on exit)");
    }

    if (geometry.sectionsMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring Sections Map window (was visible on exit)");
        onShowSectionsMap();
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Sections Map window (was hidden on exit)");
    }

    if (geometry.statesMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring States Map window (was visible on exit)");
        onShowStatesMap();
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring States Map window (was hidden on exit)");
    }

    if (geometry.graylineMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring Grayline Map window (was visible on exit)");
        onShowGraylineMap();
        if (m_graylineMapDialog && !geometry.graylineMapGeometry.isEmpty()) {
            m_graylineMapDialog->restoreGeometry(geometry.graylineMapGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Grayline Map window (was hidden on exit)");
    }
}

void MainWindow::saveSettings() {
    if (!m_settingsManager) {
        return;
    }

    // Build geometry struct
    WindowGeometry geometry;
    geometry.mainWindowGeometry = saveGeometry();
    geometry.mainWindowState = saveState();
    geometry.currentOperator = AppSettings::instance().getCurrentOperator();

    // Save child window geometries and visibility
    if (m_dxClusterWindow) {
        geometry.dxClusterGeometry = m_dxClusterWindow->saveGeometry();
        geometry.dxClusterVisible = m_dxClusterWindow->isVisible();
    }
    if (m_bandMapWindow) {
        geometry.bandMapGeometry = m_bandMapWindow->saveGeometry();
        geometry.bandMapVisible = m_bandMapWindow->isVisible();
    }
    if (m_radioControlWindow) {
        geometry.radioControlGeometry = m_radioControlWindow->saveGeometry();
        geometry.radioControlVisible = m_radioControlWindow->isVisible();
    }
    if (m_multiplierWindow) {
        geometry.multipliersGeometry = m_multiplierWindow->saveGeometry();
        geometry.multipliersVisible = m_multiplierWindow->isVisible();
    }
    if (m_statisticsWindow) {
        geometry.statisticsGeometry = m_statisticsWindow->saveGeometry();
        geometry.statisticsVisible = m_statisticsWindow->isVisible();
    }
    if (m_sectionsMapViewer) {
        geometry.sectionsMapVisible = m_sectionsMapViewer->isVisible();
    }
    if (m_statesMapViewer) {
        geometry.statesMapVisible = m_statesMapViewer->isVisible();
    }
    if (m_graylineMapDialog) {
        geometry.graylineMapGeometry = m_graylineMapDialog->saveGeometry();
        geometry.graylineMapVisible = m_graylineMapDialog->isVisible();
    }

    // Delegate save to SettingsManager
    m_settingsManager->saveWindowGeometry(geometry);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Ask for confirmation before closing
    QMessageBox::StandardButton reply;
    reply = DialogHelper::question(this, "Confirm Exit",
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
        // Only allow CW speed change in CW mode with radio connected
        bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
        if (!isCWMode || !m_radioConnected) {
            m_statusLabel->setText("CW speed adjust requires CW mode and radio connection");
            event->accept();
            return;
        }

        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = m_currentState.cwSpeed;  // Read from radio's actual speed
        const int MAX_WPM = 100;  // K4 maximum
        int newWpm = qMin(currentWpm + increment, MAX_WPM);

        // Send to radio - display will update when radio responds via stateUpdated
        m_radio->setCWSpeed(newWpm);

        m_statusLabel->setText(QString("CW Speed: %1 WPM").arg(newWpm));
        LOG_DEBUG("MainWindow", QString("WPM increased to %1 (PgUp)").arg(newWpm));
        event->accept();
        return;
    }

    // PgDown: Decrease WPM by configurable increment
    if (event->key() == Qt::Key_PageDown) {
        // Only allow CW speed change in CW mode with radio connected
        bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
        if (!isCWMode || !m_radioConnected) {
            m_statusLabel->setText("CW speed adjust requires CW mode and radio connection");
            event->accept();
            return;
        }

        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = m_currentState.cwSpeed;  // Read from radio's actual speed
        const int MIN_WPM = 8;  // K4 minimum
        int newWpm = qMax(currentWpm - increment, MIN_WPM);

        // Send to radio - display will update when radio responds via stateUpdated
        m_radio->setCWSpeed(newWpm);

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
    // Intercept PgUp/PgDn globally for CW speed control
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // PgUp: Increase CW speed
        if (keyEvent->key() == Qt::Key_PageUp) {
            // Only allow CW speed change in CW mode with radio connected
            bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
            if (!isCWMode || !m_radioConnected || !m_radio) {
                m_statusLabel->setText("CW speed adjust requires CW mode and radio connection");
                return true;  // Event handled, don't propagate
            }

            int increment = AppSettings::instance().getMorseWPMIncrement();
            int currentWpm = m_currentState.cwSpeed;  // Read from radio's actual speed
            const int MAX_WPM = 100;  // K4 maximum
            int newWpm = qMin(currentWpm + increment, MAX_WPM);

            // Send to radio - display will update when radio responds via stateUpdated
            m_radio->setCWSpeed(newWpm);

            m_statusLabel->setText(QString("CW Speed: %1 WPM").arg(newWpm));
            LOG_DEBUG("MainWindow", QString("WPM increased to %1 (PgUp - eventFilter)").arg(newWpm));
            return true;  // Event handled, don't propagate
        }

        // PgDown: Decrease CW speed
        if (keyEvent->key() == Qt::Key_PageDown) {
            // Only allow CW speed change in CW mode with radio connected
            bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
            if (!isCWMode || !m_radioConnected || !m_radio) {
                m_statusLabel->setText("CW speed adjust requires CW mode and radio connection");
                return true;  // Event handled, don't propagate
            }

            int increment = AppSettings::instance().getMorseWPMIncrement();
            int currentWpm = m_currentState.cwSpeed;  // Read from radio's actual speed
            const int MIN_WPM = 8;  // K4 minimum
            int newWpm = qMax(currentWpm - increment, MIN_WPM);

            // Send to radio - display will update when radio responds via stateUpdated
            m_radio->setCWSpeed(newWpm);

            m_statusLabel->setText(QString("CW Speed: %1 WPM").arg(newWpm));
            LOG_DEBUG("MainWindow", QString("WPM decreased to %1 (PgDn - eventFilter)").arg(newWpm));
            return true;  // Event handled, don't propagate
        }

        // Tab key in callsign field: Switch to S&P mode
        if (keyEvent->key() == Qt::Key_Tab && obj == m_callsignEntry) {
            onSPMode();
            LOG_DEBUG("MainWindow", "Tab pressed in callsign field - switched to S&P mode");
            return true;  // Event handled, don't tab to next field
        }

        // ESC key handling for callsign and exchange fields
        if (keyEvent->key() == Qt::Key_Escape) {
            // ALWAYS stop CW transmission first, regardless of where focus is
            if (m_radioConnected && m_radio) {
                m_radio->stopCW();
                m_statusLabel->setText("CW transmission aborted");
                LOG_DEBUG("MainWindow", "CW transmission aborted via ESC key");
            }

            // ESC in callsign field: clear if not empty, or switch to CQ if empty & in S&P
            if (obj == m_callsignEntry) {
                if (!m_callsignEntry->text().isEmpty()) {
                    // First ESC: Clear callsign (stay in current mode)
                    m_callsignEntry->clear();
                    LOG_DEBUG("MainWindow", "ESC pressed in callsign field - cleared");
                } else if (m_operatingMode == OperatingMode::SP) {
                    // Second ESC (empty field in S&P mode): Return to CQ mode
                    onCQMode();
                    LOG_DEBUG("MainWindow", "ESC pressed in empty callsign field (S&P mode) - switched to CQ mode");
                }
                return true;  // Event handled
            }

            // ESC in exchange field: clear exchange and return focus to callsign
            if (obj == m_exchangeEntry) {
                m_exchangeEntry->clear();
                m_callsignEntry->setFocus();
                LOG_DEBUG("MainWindow", "ESC pressed in exchange field - cleared and returned to callsign");
                return true;  // Event handled
            }
        }

        // F1-F12 keys: TR4W-style CW messages (with optional Ctrl/Alt modifiers)
        if (keyEvent->key() >= Qt::Key_F1 && keyEvent->key() <= Qt::Key_F12) {
            int fKey = keyEvent->key() - Qt::Key_F1 + 1;  // Convert to 1-12

            Qt::KeyboardModifiers mods = keyEvent->modifiers();
            bool ctrlPressed = mods & Qt::ControlModifier;
            bool altPressed = mods & Qt::AltModifier;

            handleFunctionKey(fKey, ctrlPressed, altPressed);
            return true;  // Event handled
        }

        // = key: Repeat last CW message
        if (keyEvent->key() == Qt::Key_Equal) {
            if (!m_radioConnected || !m_radio) {
                LOG_WARN("MainWindow", "Cannot send CW: radio not connected");
                m_statusLabel->setText("CW requires radio connection");
                return true;
            }

            bool isCWMode = (m_currentState.modeA == ModeType::CW ||
                             m_currentState.modeA == ModeType::CWR);
            if (!isCWMode) {
                LOG_WARN("MainWindow", "Cannot send CW: not in CW mode");
                m_statusLabel->setText("CW requires CW mode");
                return true;
            }

            if (m_lastCWMessage.isEmpty()) {
                LOG_INFO("MainWindow", "= key pressed but no previous CW message to repeat");
                m_statusLabel->setText("No previous CW message to repeat");
                return true;
            }

            // Resend the last message
            int wpm = AppSettings::instance().getMorseWPM();
            m_radio->setCWSpeed(wpm);
            m_radio->sendCW(m_lastCWMessage);

            m_statusLabel->setText(QString("Repeating CW: %1").arg(m_lastCWMessage));
            LOG_INFO("MainWindow", QString("Repeated CW: %1 (via = key)").arg(m_lastCWMessage));
            return true;  // Event handled
        }
    }

    // Clear callsign warning when exchange field gets focus
    // This prevents status bar message stacking (callsign warning hiding exchange errors)
    if (event->type() == QEvent::FocusIn && obj == m_exchangeEntry) {
        statusBar()->clearMessage();  // Clear callsign validation warning
        return false;  // Let the event propagate normally
    }

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
    if (!m_windowManager) {
        return;
    }

    // Raise main window first
    raise();

    // Delegate child window raising to WindowManager
    m_windowManager->raiseAllWindows();
}

void MainWindow::setStatusMessage(const QString& message) {
    // Log all status messages for debugging
    LOG_WARN("MainWindow", QString("Status: %1").arg(message));
    m_statusLabel->setText(message);
}

void MainWindow::onRadioConfigure() {
    LOG_DEBUG("MainWindow", "*** onRadioConfigure() called - opening Preferences with Radio tab ***");
    PreferencesDialog dialog(this);
    dialog.selectCategory("Radio");
    dialog.setRadioConnected(m_radioConnected);

    if (dialog.exec() == QDialog::Accepted) {
        m_statusLabel->setText("Radio configuration saved");

        // If currently connected, ask to reconnect
        if (m_radioConnected) {
            QMessageBox::StandardButton reply = DialogHelper::question(
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
    if (!m_radioManager) {
        return;
    }

    // RadioManager validates configuration and initiates connection
    bool success = m_radioManager->connectToRadio();

    // If validation failed, show configuration dialog
    if (!success) {
        AppSettings& settings = AppSettings::instance();
        if (!settings.hasRadioConfig() || settings.loadRadioConfig().hamlibModelId <= 0) {
            DialogHelper::warning(this, "Radio Configuration Required",
                               "Please configure your radio first (Radio → Configure).");
            onRadioConfigure();
        }
    }

    // RadioManager will emit statusMessage signal for UI feedback
    // MainWindow slots will handle UI updates via those signals
}

void MainWindow::onRadioDisconnect() {
    if (!m_radioManager) {
        return;
    }

    // Delegate disconnection to RadioManager
    m_radioManager->disconnectFromRadio();
}

void MainWindow::onAbout() {
    DialogHelper::about(this, "About TR4QT",
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

#ifdef ENABLE_PERFORMANCE_PROFILING
void MainWindow::onShowPerformanceReport() {
    QString report = PerformanceProfiler::instance().generateReport();

    // Log the report
    LOG_INFO("MainWindow", "\n" + report);

    // Show in a dialog
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Radio Performance Report");
    msgBox.setText("Performance comparison between K4 Direct and Hamlib interfaces:");
    msgBox.setDetailedText(report);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    // Make the dialog larger to show more text
    msgBox.setStyleSheet("QTextEdit { min-width: 600px; min-height: 400px; }");

    msgBox.exec();
}
#endif

void MainWindow::onEmailLogsToSupport() {
    // Get logs from last "PROGRAM STARTUP" banner forward
    QString logs = Logger::instance().getLastLogLines();

    // Get configured radio model from settings (not just connected radio)
    RadioConfig radioConfig = AppSettings::instance().loadRadioConfig();
    QString configuredRadio = "None";
    QString connectionType = "None";
    QString connectionDetails = "";

    if (radioConfig.hamlibModelId > 0) {
        // Get radio model name from Hamlib
        const struct rig_caps* caps = rig_get_caps(radioConfig.hamlibModelId);
        if (caps) {
            configuredRadio = QString("%1 %2").arg(caps->mfg_name).arg(caps->model_name);
        } else {
            configuredRadio = QString("Unknown (ID: %1)").arg(radioConfig.hamlibModelId);
        }

        // Determine connection type without revealing IP addresses
        if (radioConfig.port.contains(':')) {
            connectionType = "Network (TCP)";
            // Don't include actual IP:port for privacy
        } else if (!radioConfig.port.isEmpty()) {
            connectionType = "Serial";
            connectionDetails = QString("Port: %1, Baud: %2, %3%4%5")
                .arg(radioConfig.port)
                .arg(radioConfig.baudRate)
                .arg(radioConfig.dataBits)
                .arg(radioConfig.parity == 0 ? "N" : radioConfig.parity == 1 ? "O" : "E")
                .arg(radioConfig.stopBits);

            // Add CI-V address if configured (Icom radios)
            if (radioConfig.civAddress > 0) {
                connectionDetails += QString(", CI-V: 0x%1").arg(radioConfig.civAddress, 2, 16, QChar('0')).toUpper();
            }
        }
    }

    // Collect system information
    QString systemInfo = QString(
        "TR4QT Version: %1\n"
        "Platform: %2 %3\n"
        "Qt Version: %4\n"
        "Radio Model (Configured): %5\n"
        "Connection Type: %6\n"
        "%7"
        "Poll Interval: %8 ms\n"
        "Radio Connected: %9\n"
        "\n"
    ).arg(APP_VERSION)
     .arg(QSysInfo::productType())
     .arg(QSysInfo::productVersion())
     .arg(QT_VERSION_STR)
     .arg(configuredRadio)
     .arg(connectionType)
     .arg(connectionDetails.isEmpty() ? "" : connectionDetails + "\n")
     .arg(radioConfig.pollInterval)
     .arg(m_radio->isConnected() ? "Yes" : "No");

    // Build full log content
    QString logContent = systemInfo + "=== LOG (from last startup) ===\n\n" + logs;

    // Show preview dialog BEFORE creating zip file
    QMessageBox preview;
    preview.setWindowTitle("Email Logs to Support - Preview");
    preview.setIcon(QMessageBox::Question);
    preview.setText(
        QString("This will create a zip file with your support logs (%1 characters).\n\n"
                "Click 'Show Details' below to review what will be included.\n\n"
                "The zip file will be saved to your temp directory for you to attach to an email.")
        .arg(logContent.length()));
    preview.setDetailedText(logContent);
    preview.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    preview.setDefaultButton(QMessageBox::Ok);

    // Auto-expand "Show Details" so user can see the content immediately
    foreach (QAbstractButton *button, preview.buttons()) {
        if (preview.buttonRole(button) == QMessageBox::ActionRole) {
            button->click();
            break;
        }
    }

    // If user cancels, abort the operation
    if (preview.exec() != QMessageBox::Ok) {
        return;
    }

    // Save to Desktop for easy access (instead of temp folder)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        // Fallback to home directory if Desktop not available
        desktopPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    // Generate filename with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
    QString logFileName = QString("tr4qt-logs-%1.txt").arg(timestamp);
    QString zipFileName = QString("tr4qt-logs-%1.zip").arg(timestamp);
    QString logFilePath = QFileInfo(desktopPath, logFileName).filePath();
    QString zipFilePath = QFileInfo(desktopPath, zipFileName).filePath();

    // Write log content to file
    QFile logFile(logFilePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        DialogHelper::critical(this, "Error",
            QString("Failed to create temporary log file: %1\n\nError: %2")
            .arg(logFilePath)
            .arg(logFile.errorString()));
        return;
    }

    QTextStream out(&logFile);
    out << logContent;
    logFile.close();

    // Zip the log file using Qt's QProcess to call system zip command
    QProcess zipProcess;
    zipProcess.setWorkingDirectory(desktopPath);

#ifdef Q_OS_WIN
    // Windows: Use PowerShell Compress-Archive
    zipProcess.start("powershell", QStringList()
        << "-Command"
        << QString("Compress-Archive -Path '%1' -DestinationPath '%2' -Force")
           .arg(logFileName).arg(zipFileName));
#else
    // macOS/Linux: Use zip command
    zipProcess.start("zip", QStringList() << "-j" << zipFileName << logFileName);
#endif

    if (!zipProcess.waitForFinished(5000)) {
        DialogHelper::critical(this, "Error",
            "Failed to create zip file.\n\n"
            "Please manually attach the log file to your email:\n" + logFilePath);
        return;
    }

    if (zipProcess.exitCode() != 0) {
        DialogHelper::critical(this, "Error",
            QString("Zip command failed with exit code %1.\n\n"
                    "Please manually attach the log file to your email:\n%2")
            .arg(zipProcess.exitCode())
            .arg(logFilePath));
        return;
    }

    // Delete the uncompressed log file (keep only the zip)
    QFile::remove(logFilePath);

    // Show success dialog with instructions
    QMessageBox instructions;
    instructions.setWindowTitle("Support Logs Ready");
    instructions.setIcon(QMessageBox::Information);
    instructions.setText(
        QString("Support logs saved to your Desktop:\n\n"
                "%1\n\n"
                "What would you like to do?")
        .arg(zipFileName));  // Just show filename, not full path

    QPushButton* bothButton = instructions.addButton(
#ifdef Q_OS_MAC
        "Show in Finder && Open Email",
#else
        "Show in Explorer && Open Email",
#endif
        QMessageBox::AcceptRole);
    QPushButton* revealButton = instructions.addButton(
#ifdef Q_OS_MAC
        "Show in Finder Only",
#else
        "Show in Explorer Only",
#endif
        QMessageBox::ActionRole);
    QPushButton* emailButton = instructions.addButton("Open Email Only", QMessageBox::ActionRole);
    QPushButton* closeButton = instructions.addButton("Close", QMessageBox::RejectRole);
    instructions.setDefaultButton(bothButton);

    int result = instructions.exec();
    QAbstractButton* clicked = instructions.clickedButton();

    // Reveal file in Finder/Explorer if requested
    bool shouldReveal = (clicked == revealButton || clicked == bothButton);
    bool shouldEmail = (clicked == emailButton || clicked == bothButton);

    if (shouldReveal) {
#ifdef Q_OS_MAC
        // macOS: Use 'open -R' to reveal file in Finder
        QProcess::startDetached("open", QStringList() << "-R" << zipFilePath);
#elif defined(Q_OS_WIN)
        // Windows: Use 'explorer /select,' to highlight file in Explorer
        QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(zipFilePath));
#else
        // Linux: Open file manager at directory (can't select specific file universally)
        QDesktopServices::openUrl(QUrl::fromLocalFile(desktopPath));
#endif
        LOG_INFO("MainWindow", QString("Revealed support zip file: %1").arg(zipFilePath));
    }

    if (shouldEmail) {
        // Open email client with instructions
        QString subject = QString("TR4QT Support Request - v%1 (%2)")
            .arg(APP_VERSION)
            .arg(QSysInfo::productType());

        QString body = QString(
            "Please describe your issue:\n\n\n\n"
            "---\n"
            "Logs attached: %1\n"
            "TR4QT Version: %2\n"
            "Platform: %3 %4")
            .arg(zipFileName)
            .arg(APP_VERSION)
            .arg(QSysInfo::productType())
            .arg(QSysInfo::productVersion());

        QString mailto = QString("mailto:support@ny4i.com?subject=%1&body=%2")
            .arg(QUrl::toPercentEncoding(subject))
            .arg(QUrl::toPercentEncoding(body));

        if (!QDesktopServices::openUrl(QUrl(mailto))) {
            DialogHelper::critical(this, "Error",
                QString("Failed to open email client.\n\n"
                        "Please manually email the zip file to: support@ny4i.com\n\n"
                        "The file is on your Desktop:\n%1").arg(zipFileName));
        } else {
            // Show brief reminder (only if we didn't already show Finder)
            if (!shouldReveal) {
                DialogHelper::information(this, "Don't Forget!",
                    QString("Remember to attach the zip file from your Desktop:\n\n%1")
                    .arg(zipFileName));
            }

            LOG_INFO("MainWindow", QString("Created support zip file: %1").arg(zipFilePath));
        }
    }
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
    dialog.setRadioConnected(m_radioConnected);

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

        // Reload license class for phone privilege validation
        QString licenseClassStr = settings.getLicenseClass();
        HamRadioPrivileges::LicenseClass licenseClass =
            HamRadioPrivileges::stringToLicenseClass(licenseClassStr);
        delete m_hamPrivileges;
        m_hamPrivileges = new HamRadioPrivileges(licenseClass);
        LOG_DEBUG("MainWindow", QString("License class updated to: %1").arg(licenseClassStr));

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
            QMessageBox::StandardButton reply = DialogHelper::question(
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

void MainWindow::onImportADIF() {
    if (!m_importExportManager) {
        LOG_ERROR("MainWindow", "ImportExportManager is null");
        return;
    }

    // Delegate to ImportExportManager
    ImportResult result = m_importExportManager->importADIF();

    // Handle result: reload QSOs if successful
    if (result.success && result.successCount > 0) {
        // Reload QSOs from database to refresh UI
        QSORepository repo;
        QList<QSO> allQSOs = repo.findByContest(m_currentContestDbId);
        m_qsoTableModel->clear();
        for (const QSO& qso : allQSOs) {
            m_qsoTableModel->addQSO(qso);
        }

        // Update score display
        updateScoreDisplay();
    }

    // Update status label
    if (!result.statusMessage.isEmpty()) {
        m_statusLabel->setText(result.statusMessage);
    }
}

void MainWindow::onExportADIF() {
    if (!m_importExportManager) {
        LOG_ERROR("MainWindow", "ImportExportManager is null");
        return;
    }

    // Delegate to ImportExportManager
    ExportResult result = m_importExportManager->exportADIF();

    // Update status label
    if (!result.statusMessage.isEmpty()) {
        m_statusLabel->setText(result.statusMessage);
    }
}

void MainWindow::onExportCabrillo() {
    if (!m_importExportManager) {
        LOG_ERROR("MainWindow", "ImportExportManager is null");
        return;
    }

    // Delegate to ImportExportManager
    ExportResult result = m_importExportManager->exportCabrillo();

    // Update status label
    if (!result.statusMessage.isEmpty()) {
        m_statusLabel->setText(result.statusMessage);
    }
}

void MainWindow::onClearLog() {
    if (m_qsoTableModel->count() == 0) {
        DialogHelper::information(this, "Clear Log", "Log is already empty.");
        return;
    }

    // Ask if user wants to create a backup first
    QMessageBox::StandardButton backupReply = DialogHelper::question(
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
        QString backupDir = PathManager::getBackupsDir();

        if (!backupMgr.createBackup(m_currentContest.databasePath, backupDir, backupPath)) {
            QMessageBox::StandardButton continueReply = DialogHelper::warning(
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
    QMessageBox::StandardButton reply = DialogHelper::question(
        this, "Clear Log",
        QString("Are you sure you want to clear all %1 QSOs from the log?\n\nThis action cannot be undone.")
            .arg(m_qsoTableModel->count()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Clear QSOs and multipliers from database
        QSORepository repo;
        if (!repo.deleteAllQSOs(m_currentContestDbId)) {
            DialogHelper::critical(this, "Error",
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
        LOG_INFO("MainWindow", "Radio connected successfully");

        // Enable band selection buttons when radio connected
        if (m_bandSummaryGrid) {
            m_bandSummaryGrid->setBandSelectionEnabled(true);
        }
    } else {
        // Clear radio control display when disconnected
        if (m_radioControlWindow) {
            m_radioControlWindow->clearDisplay();
        }

        // Disable band selection buttons when radio disconnected
        if (m_bandSummaryGrid) {
            m_bandSummaryGrid->setBandSelectionEnabled(false);
        }
    }
}

void MainWindow::onRadioStateUpdated(const RadioState& state) {
    // Update cached state
    bool frequencyChanged = (state.frequencyA != m_currentState.frequencyA);
    bool bandChanged = (state.bandA != m_currentState.bandA);
    m_currentState = state;

    // Send UDP broadcast for radio state change (throttled in manager)
    QString stationCall = AppSettings::instance().getMyCallsign();
    m_udpBroadcastManager->onRadioStateChanged(state, stationCall);

    // Validate phone mode privileges (US only)
    if (m_hamPrivileges && state.frequencyA > 0) {
        QString warning = m_hamPrivileges->validatePhoneMode(
            state.frequencyA,
            state.bandA,
            state.modeA
        );
        if (!warning.isEmpty()) {
            // Display privilege warning in status bar with orange color
            m_statusLabel->setText(QString("⚠ %1").arg(warning));
            m_statusLabel->setStyleSheet("color: orange; font-weight: bold;");
            LOG_WARN("MainWindow", QString("Phone privilege violation: %1").arg(warning));
        }
    }

    // Check for AUTO S&P mode trigger (VFO movement)
    if (state.frequencyA > 0 && frequencyChanged) {
        checkAutoSP(state.frequencyA);
    }

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

    // Update radio control window if it exists
    if (m_radioControlWindow) {
        m_radioControlWindow->updateRadioState(state);
    }

    // Emit signals for frequency/band changes
    if (frequencyChanged) {
        emit currentFrequencyChanged(state.frequencyA);
    }
    if (bandChanged) {
        emit currentBandChanged(state.bandA);
    }
}

void MainWindow::onRadioError(const QString& error) {
    LOG_ERROR("MainWindow", QString("Radio error: %1").arg(error));
    // Status message already set by RadioManager via statusMessage signal
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
    QString exchange = m_exchangeEntry->text().trimmed().toUpper();

    // Check for commands (OPON, UDP) - Phase 1 extraction
    CommandDispatcher::CommandResult cmd = CommandDispatcher::parseCommand(callsign);
    if (cmd.wasCommand) {
        if (cmd.type == CommandDispatcher::ChangeOperator) {
            // OPON command - change operator
            OperatorDialog dialog(this);

            // Pre-populate with current operator
            AppSettings& settings = AppSettings::instance();
            dialog.setOperatorCallsign(settings.getCurrentOperator());

            if (dialog.exec() == QDialog::Accepted) {
                QString newOperator = dialog.getOperatorCallsign();
                if (!newOperator.isEmpty()) {
                    settings.setCurrentOperator(newOperator);
                    m_operatorLabel->setText(newOperator);
                    m_statusLabel->setText(QString("Operator changed to: %1").arg(newOperator));
                    LOG_INFO("MainWindow", QString("Operator changed to: %1").arg(newOperator));
                } else {
                    m_statusLabel->setText("Operator change cancelled (empty callsign)");
                }
            } else {
                m_statusLabel->setText("Operator change cancelled");
            }

            onClearEntry();
            return;
        }
        else if (cmd.type == CommandDispatcher::RebroadcastLog) {
            // UDP command - rebroadcast log
            onRebroadcastLog();
            onClearEntry();
            return;
        }
    }

    // Verify service is initialized
    if (!m_loggingService) {
        m_statusLabel->setText("Error: No active contest - open a contest first");
        QApplication::beep();
        return;
    }

    // Build request for QSO logging service
    QSOLoggingService::LogQSORequest request;
    request.callsign = callsign;
    request.exchange = exchange;
    request.radioState = m_currentState;
    request.operatorCallsign = AppSettings::instance().getCurrentOperator();
    request.serialNumber = m_nextSerialNumber;
    request.operatingMode = m_operatingMode;

    // Get existing QSOs for duplicate/multiplier checking
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        request.existingQSOs.append(m_qsoTableModel->getQSO(row));
    }

    request.saveExchangeMemory = true;
    request.autoPopulated = m_initialExchangePopulated;

    // Context for post-logging actions
    request.stationCallsign = AppSettings::instance().getMyCallsign();
    request.contestName = m_hasActiveContest ? m_currentContest.contestName : "Unknown";
    request.contestId = m_activeContest ? m_activeContest->getContestId() : "";
    request.databasePath = m_currentContest.databasePath;
    request.totalQSOCount = m_qsoTableModel->count() + 1;
    request.qsosSinceLastCheck = m_qsosSinceLastIntegrityCheck + 1;
    request.contestDbId = m_currentContestDbId;
    request.memoryQSOCount = m_qsoTableModel->count() + 1;

    // Execute QSO logging workflow (Phase 5 integration service)
    QSOLoggingService::LogQSOResult result = m_loggingService->logQSO(request);

    // Handle validation errors
    if (!result.success) {
        m_statusLabel->setText(result.errorMessage);
        m_statusLabel->setStyleSheet("QLabel { color: #ff0000; font-weight: bold; }");
        QApplication::beep();

        // Set focus to appropriate field
        if (result.errorMessage.contains("Callsign")) {
            m_callsignEntry->setFocus();
        } else if (result.errorMessage.contains("Exchange")) {
            m_exchangeEntry->setFocus();
        }
        return;
    }

    // Extract results
    QSO qso = result.qso;
    bool isDuplicate = result.isDuplicate;
    QStringList multiplierValues = result.multiplierValues;

    // Update serial number
    m_nextSerialNumber = result.updatedSerialNumber;

    // Log duplicate info if present
    if (isDuplicate) {
        LOG_INFO("MainWindow", QString("Duplicate QSO detected: %1 - %2").arg(callsign, result.dupeInfo));
    }

    // Add to table model (UI)
    m_qsoTableModel->addQSO(qso);

    // Update band summary grid with new scores
    updateScoreDisplay();

    // Update multiplier window
    if (m_multiplierWindow && m_activeContest) {
        QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();
        if (!multDefs.isEmpty()) {
            MultiplierType primaryMultType = multDefs.first().type;
            QString multValue = m_activeContest->getMultiplierValue(qso, primaryMultType, QStringList());
            if (!multValue.isEmpty()) {
                m_multiplierWindow->setMultiplierWorked(multValue, qso.band);
                LOG_DEBUG("MainWindow", QString("Updated multiplier window: %1 on %2")
                    .arg(multValue).arg(bandToString(qso.band)));
            }
        }
    }

    // Scroll to show newly logged QSO
    m_qsoTableView->scrollToBottom();

    // Handle persistence result
    if (result.persistenceResult.status == QSOPersistenceService::SaveResult::SavedToDatabase) {
        LOG_DEBUG("MainWindow", QString("QSO saved to database with ID: %1").arg(qso.id));

        // Update table model with database ID
        int lastRow = m_qsoTableModel->count() - 1;
        m_qsoTableModel->updateQSO(lastRow, qso);
    }
    else if (result.persistenceResult.status == QSOPersistenceService::SaveResult::SavedToEmergencyFile) {
        // Emergency file fallback
        DialogHelper::information(this, "QSO Saved to Emergency File",
            QString("Database save failed. QSO saved to emergency file:\n%1\n\n"
                    "You can import this file later using File → Import ADIF")
            .arg(result.persistenceResult.emergencyFilePath));
    }
    else if (result.persistenceResult.status == QSOPersistenceService::SaveResult::Failed) {
        // Complete failure
        DialogHelper::critical(this, "QSO Save Failed",
            "Could not save QSO to database or emergency file!\n\n"
            "The QSO is only in memory and will be lost if TR4QT crashes.");
    }
    else if (result.persistenceResult.status == QSOPersistenceService::SaveResult::NeedsUserDecision) {
        // User needs to decide (retry/emergency/stop)
        // This shouldn't happen with QSOLoggingService (it handles retries internally)
        // But handle it just in case
        DialogHelper::warning(this, "QSO Save Issue",
            QString("QSO save needs attention:\n%1")
            .arg(result.persistenceResult.errorMessage));
    }

    // Update last QSO time
    m_lastQSOTime = qso.timestamp;

    // Update status
    QString statusMsg = QString("Logged: %1 on %2 %3")
        .arg(callsign)
        .arg(bandToString(qso.band))
        .arg(modeToString(qso.mode));

    // Append post-logging actions if any
    if (!result.postLoggingActions.isEmpty()) {
        statusMsg += " | " + result.postLoggingActions.join(", ");
    }

    m_statusLabel->setText(statusMsg);
    m_statusLabel->setStyleSheet("");  // Reset color

    // Update integrity check counter
    m_qsosSinceLastIntegrityCheck = result.postLoggingActions.contains("Integrity check passed") ||
                                    result.postLoggingActions.contains("Integrity check FAILED") ? 0 : m_qsosSinceLastIntegrityCheck + 1;

    // Auto-send QSL message after logging
    bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
    bool autoSendEnabled = m_autoSendCWAction->isChecked();
    if (isCWMode && m_radioConnected && m_radio && autoSendEnabled) {
        QString qslMessage = AppSettings::instance().getQSLCWMessage();
        sendCWMessage(qslMessage);
        LOG_DEBUG("MainWindow", QString("Auto-sent QSL message: %1").arg(qslMessage));
    }

    // Clear entry fields and focus callsign
    onClearEntry();

    // Update displays
    updateScoreDisplay();
    updateTimeDisplay();
}


void MainWindow::onCallsignChanged(const QString& callsign) {
    // Update needs display as user types
    if (callsign.length() < 2 || !m_activeContest) {
        m_needsDisplayWidget->clear();
        m_scpMatchesLabel->setText("");  // Clear but keep visible to prevent layout shift
        m_stationInfoLabel->setText("");  // Clear station info when callsign cleared
        statusBar()->clearMessage();  // Clear any status messages
        return;
    }

    // Get worked bands for this callsign
    QList<BandType> workedBands = getWorkedBandsForCallsign(callsign);

    // Get multiplier value and worked mult bands
    QString multValue = getMultiplierValueForCallsign(callsign);
    QList<BandType> workedMultBands;

    if (!multValue.isEmpty()) {
        // Get the primary multiplier type and scope
        QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();
        if (!multDefs.isEmpty()) {
            MultiplierType primaryMultType = multDefs.first().type;
            MultiplierScope multScope = multDefs.first().scope;

            workedMultBands = getWorkedBandsForMultiplier(multValue, primaryMultType);

            // For AllBands multipliers (like CQ WPX prefix):
            // If worked on ANY band, consider it worked on ALL bands
            // (no mult needs to show)
            if (multScope == MultiplierScope::AllBands && !workedMultBands.isEmpty()) {
                // Get all valid bands from the contest
                QList<BandType> allBands = {
                    BandType::Band160M, BandType::Band80M, BandType::Band40M,
                    BandType::Band20M, BandType::Band15M, BandType::Band10M
                };
                if (AppSettings::instance().getVHFBandsEnabled()) {
                    allBands.append(BandType::Band6M);
                    allBands.append(BandType::Band2M);
                }
                // Mark all bands as worked for display purposes
                workedMultBands = allBands;
            }
        }
    }

    // Update the needs display widget
    m_needsDisplayWidget->updateForCallsign(
        callsign, m_activeContest, workedBands, workedMultBands);

    // Update SCP matches display (2-column grid format)
    // Shows SCP matches that are in the log, with duplicate highlighting
    bool scpEnabled = AppSettings::instance().getSCPEnabled();
    LOG_DEBUG("MainWindow", QString("SCP: enabled=%1, callsign='%2'")
        .arg(scpEnabled ? "true" : "false").arg(callsign));

    if (scpEnabled) {
        // Pass current contest database path for contest-specific prioritization
        // This allows SCP to query worked calls from contest DB + MASTER.SCP from global DB
        QString contestDbPath = m_hasActiveContest ? m_currentContest.databasePath : QString();
        QStringList matches = m_scpMatcher->findMatches(callsign, contestDbPath);
        LOG_DEBUG("MainWindow", QString("SCP: found %1 matches: %2")
            .arg(matches.size()).arg(matches.join(", ")));

        if (!matches.isEmpty()) {
            // Get colors for different states from ThemeManager
            QColor dupeColor = ThemeManager::instance().color(ColorRole::DupeText);  // Red for dupes
            QString dupeColorStr = dupeColor.name();
            QColor workedColor = ThemeManager::instance().color(ColorRole::WorkedStationText);  // Gray for worked
            QString workedColorStr = workedColor.name();
            QColor notWorkedColor = ThemeManager::instance().color(ColorRole::MultiplierText);  // Blue for not worked (potential new mult)
            QString notWorkedColorStr = notWorkedColor.name();

            // Get all worked callsigns for checking if a call was worked
            QSet<QString> workedCallsigns = getWorkedCallsigns();

            // Sort matches: worked/dupe calls FIRST, then not-worked calls
            // This prioritizes showing important information (dupes, worked calls) at the top
            QStringList workedMatches;  // Red (dupes) and Blue (worked but not dupe)
            QStringList notWorkedMatches;  // Gray (not worked yet)

            for (const QString& match : matches) {
                if (workedCallsigns.contains(match)) {
                    workedMatches.append(match);  // Worked or dupe - show first
                } else {
                    notWorkedMatches.append(match);  // Not worked - show after
                }
            }

            // Combine: worked calls first, then not-worked
            QStringList sortedMatches = workedMatches + notWorkedMatches;

            LOG_DEBUG("MainWindow", QString("SCP: sorted %1 worked first, %2 not worked after")
                .arg(workedMatches.size()).arg(notWorkedMatches.size()));

            // Format matches in 2 columns with color coding:
            // RED = duplicate on current band/mode
            // BLUE = worked but not a duplicate
            // GRAY = not worked yet (only in MASTER.SCP)
            QStringList rows;
            for (int i = 0; i < sortedMatches.size(); i += 2) {
                // Check first match
                QString dupeInfo;
                bool isWorked1 = workedCallsigns.contains(sortedMatches[i]);
                bool isDupe1 = isWorked1 && checkForDuplicate(sortedMatches[i], m_currentState.bandA, m_currentState.modeA, dupeInfo);

                QString color1;
                if (isDupe1) {
                    color1 = dupeColorStr;  // RED - dupe
                } else if (isWorked1) {
                    color1 = workedColorStr;  // GRAY - worked but not dupe
                } else {
                    color1 = notWorkedColorStr;  // BLUE - not worked (potential mult)
                }

                QString call1 = QString("<span style='color: %1;'>%2</span>")
                    .arg(color1)
                    .arg(sortedMatches[i]);

                QString row = call1;

                // Check second match if exists
                if (i + 1 < sortedMatches.size()) {
                    bool isWorked2 = workedCallsigns.contains(sortedMatches[i + 1]);
                    bool isDupe2 = isWorked2 && checkForDuplicate(sortedMatches[i + 1], m_currentState.bandA, m_currentState.modeA, dupeInfo);

                    QString color2;
                    if (isDupe2) {
                        color2 = dupeColorStr;  // RED - dupe
                    } else if (isWorked2) {
                        color2 = workedColorStr;  // GRAY - worked but not dupe
                    } else {
                        color2 = notWorkedColorStr;  // BLUE - not worked (potential mult)
                    }

                    QString call2 = QString("<span style='color: %1;'>%2</span>")
                        .arg(color2)
                        .arg(sortedMatches[i + 1]);
                    row += "  " + call2;
                }

                rows.append(row);
            }

            m_scpMatchesLabel->setTextFormat(Qt::RichText);  // Enable HTML formatting
            m_scpMatchesLabel->setText(rows.join("<br>"));  // Use <br> for line breaks in HTML
            m_scpMatchesLabel->show();  // Make sure label is visible
            LOG_DEBUG("MainWindow", QString("SCP: displaying %1 rows (%2 total calls)")
                .arg(rows.size()).arg(sortedMatches.size()));
        } else {
            m_scpMatchesLabel->setText("");  // Clear but don't hide
            m_scpMatchesLabel->show();  // Keep visible even when empty
            LOG_DEBUG("MainWindow", "SCP: no matches, clearing display");
        }
    } else {
        m_scpMatchesLabel->setText("");  // Clear but don't hide
        m_scpMatchesLabel->show();  // Keep visible even when empty
    }

    // Update station info display (country name, prefix, bearing, distance)
    // Wait for at least one digit to avoid matching incomplete prefixes (e.g., "KA" before "KA6")
    if (callsign.length() >= 2 && callsign.contains(QRegularExpression("\\d"))) {
        updateStationInfo(callsign);
    } else {
        m_countryNameLabel->setText("");
        m_stationInfoLabel->setText("");
    }

    // Exchange auto-population now happens on Enter key press, not while typing
    // Duplicate checking happens on Enter key press
}

void MainWindow::updateStationInfo(const QString& callsign) {
    // Lookup country data from CTY.DAT
    CountryData countryData = m_countryFile.lookup(callsign);
    if (!countryData.isValid()) {
        m_countryNameLabel->setText("");
        m_stationInfoLabel->setText("");
        return;
    }

    // Always show country name
    m_countryNameLabel->setText(countryData.name);

    // Get my station's grid square from settings
    AppSettings& settings = AppSettings::instance();
    QString myGrid = settings.getMyGridSquare();
    if (myGrid.isEmpty()) {
        // No grid square configured, just show prefix (cannot calculate distance/bearing)
        m_stationInfoLabel->setText(countryData.primaryPrefix);
        return;
    }

    // Convert my grid square to lat/lon
    double myLat, myLon;
    if (!GeographicUtils::gridToLatLon(myGrid, myLat, myLon)) {
        // Invalid grid square, just show prefix (cannot calculate distance/bearing)
        m_stationInfoLabel->setText(countryData.primaryPrefix);
        return;
    }

    // Calculate distance and bearing from my grid to target location
    double distance = 0.0;
    double bearing = 0.0;
    bool useKilometers = settings.getUseMetricDistance();

    // For US mainland callsigns (DXCC 291), use call area grid squares (more precise)
    double targetLat, targetLon;
    if (CountryFile::getUSCallAreaCoordinates(callsign, countryData.dxccEntity, targetLat, targetLon)) {
        // US mainland callsign - use call area center grid (W1→FN43, W2→FN22, etc.)
        distance = GeographicUtils::haversineDistance(myLat, myLon,
                                                      targetLat, targetLon,
                                                      useKilometers);
        bearing = GeographicUtils::calculateBearing(myLat, myLon,
                                                    targetLat, targetLon);
    } else {
        // Non-US or Alaska/Hawaii - use country center from CTY.DAT
        targetLat = countryData.latitude;
        targetLon = countryData.longitude;
        distance = GeographicUtils::haversineDistance(myLat, myLon,
                                                      targetLat, targetLon,
                                                      useKilometers);
        bearing = GeographicUtils::calculateBearing(myLat, myLon,
                                                    targetLat, targetLon);
    }

    // Calculate sunrise/sunset for DX station
    QDate today = QDate::currentDate();
    QTime dxSunrise = GeographicUtils::calculateSunrise(targetLat, targetLon, today);
    QTime dxSunset = GeographicUtils::calculateSunset(targetLat, targetLon, today);

    // Calculate sunrise/sunset for Home station
    QTime homeSunrise = GeographicUtils::calculateSunrise(myLat, myLon, today);
    QTime homeSunset = GeographicUtils::calculateSunset(myLat, myLon, today);

    // Check grayline status
    QDateTime now = QDateTime::currentDateTimeUtc();
    QTime currentTime = now.time();

    // Check which specific times are in grayline
    bool homeInGrayline = GeographicUtils::isInGraylineWindow(now, homeSunrise, homeSunset, GRAYLINE_WINDOW_MINUTES);

    // Check DX sunrise grayline
    bool dxSunriseInGrayline = false;
    if (dxSunrise.isValid()) {
        int secondsToSunrise = currentTime.secsTo(dxSunrise);
        dxSunriseInGrayline = (std::abs(secondsToSunrise) <= GRAYLINE_WINDOW_MINUTES * 60);
    }

    // Check DX sunset grayline
    bool dxSunsetInGrayline = false;
    if (dxSunset.isValid()) {
        int secondsToSunset = currentTime.secsTo(dxSunset);
        dxSunsetInGrayline = (std::abs(secondsToSunset) <= GRAYLINE_WINDOW_MINUTES * 60);
    }

    bool dxInGrayline = dxSunriseInGrayline || dxSunsetInGrayline;

    // Format station info: "PREFIX  BEARING°  DISTANCE  SR/SS"
    // Example: "KP4  121°  1263mi  10:45z/22:15z" or "HV  045°  4521km  04:52z/17:10z"
    QString distUnit = useKilometers ? "km" : "mi";
    QString info = QString("%1  %2°  %3%4")
        .arg(countryData.primaryPrefix, -6)  // Left-align in 6 char field
        .arg(static_cast<int>(bearing), 3)   // Bearing (3 digits)
        .arg(static_cast<int>(distance), 4)  // Distance (4 digits)
        .arg(distUnit);

    // Add sunrise/sunset times if valid (with rich text for grayline highlighting)
    QString tooltip;
    if (dxSunrise.isValid() && dxSunset.isValid()) {
        QString srText = dxSunrise.toString("HH:mm") + "z";
        QString ssText = dxSunset.toString("HH:mm") + "z";

        // Color highlight if in grayline (orange for enhanced propagation)
        QString graylineColor = ThemeManager::instance().colorName(ColorRole::AgingSpotText);

        if (dxSunriseInGrayline) {
            srText = QString("<span style='color:%1;font-weight:bold;'>%2</span>")
                .arg(graylineColor).arg(srText);
            tooltip = "DX station in sunrise grayline window (enhanced propagation)";
        }

        if (dxSunsetInGrayline) {
            ssText = QString("<span style='color:%1;font-weight:bold;'>%2</span>")
                .arg(graylineColor).arg(ssText);
            if (!tooltip.isEmpty()) {
                tooltip = "DX station in sunrise/sunset grayline window (enhanced propagation)";
            } else {
                tooltip = "DX station in sunset grayline window (enhanced propagation)";
            }
        }

        info += "  " + srText + "/" + ssText;
    }

    // Add grayline indicators
    if (homeInGrayline && dxInGrayline) {
        info += "  ⚡DOUBLE⚡";
        tooltip = "Both home and DX stations in grayline window (exceptional propagation!)";
    } else if (homeInGrayline) {
        info += "  [HOME GRAYLINE]";
        if (tooltip.isEmpty()) {
            tooltip = "Home station in grayline window (enhanced propagation)";
        } else {
            tooltip += " + Home station also in grayline";
        }
    }

    // Enable rich text and set content
    m_stationInfoLabel->setTextFormat(Qt::RichText);
    m_stationInfoLabel->setText(info);
    m_stationInfoLabel->setToolTip(tooltip);

    // Update grayline map if it's open
    if (m_graylineMapDialog && m_graylineMapDialog->isVisible() && !m_graylineMapDialog->isFrozen()) {
        QString myCallsign = settings.getMyCallsign();
        m_graylineMapDialog->updateStations(myCallsign, myLat, myLon,
                                           callsign, targetLat, targetLon);
    }
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
    unsigned long targetFreqKHz = 0;

    // Check if input contains a decimal point (e.g., "14.200" for 14.200 MHz)
    if (callsign.contains('.')) {
        bool isDouble = false;
        double freqMHz = callsign.toDouble(&isDouble);
        if (isDouble && freqMHz > 0) {
            // Decimal entry - treat as MHz and convert to kHz
            // e.g., "14.200" -> 14200 kHz
            targetFreqKHz = static_cast<unsigned long>(freqMHz * 1000.0);
            isNumeric = true;
            LOG_DEBUG("MainWindow", QString("Decimal frequency entry: %1 MHz -> %2 kHz")
                .arg(freqMHz).arg(targetFreqKHz));
        }
    } else {
        // No decimal - try to parse as integer (kHz)
        unsigned long freqValue = callsign.toULong(&isNumeric);

        LOG_DEBUG("MainWindow", QString("Numeric check - isNumeric: %1, freqValue: %2")
            .arg(isNumeric).arg(freqValue));

        if (isNumeric && freqValue > 0) {
            // Determine if this is an offset or absolute frequency
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
        }
    }

    if (isNumeric && targetFreqKHz > 0) {

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

    // Auto-send CW message when in CW mode (if enabled)
    bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
    bool autoSendEnabled = m_autoSendCWAction->isChecked();  // Check actual action state
    if (isCWMode && m_radioConnected && m_radio && autoSendEnabled) {
        if (m_operatingMode == OperatingMode::SP) {
            // S&P mode: Send your exchange after entering callsign
            QString messageTemplate = AppSettings::instance().getSPCWExchange();
            sendCWMessage(messageTemplate);
            LOG_DEBUG("MainWindow", QString("Auto-sent S&P exchange: %1").arg(messageTemplate));
        } else {
            // CQ mode: Send your exchange after entering their callsign
            QString messageTemplate = AppSettings::instance().getCQCWExchange();
            sendCWMessage(messageTemplate);
            LOG_DEBUG("MainWindow", QString("Auto-sent CQ exchange: %1").arg(messageTemplate));
        }
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
        SelectableMessageBox::warning(this, "Error", "Cannot edit QSO: Invalid QSO ID");
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

            // Rebuild multiplier window from all QSOs (sections may have changed)
            if (m_multiplierWindow) {
                m_multiplierWindow->clear();
                for (int r = 0; r < m_qsoTableModel->count(); ++r) {
                    QSO q = m_qsoTableModel->getQSO(r);
                    if (!q.arrlSection.isEmpty()) {
                        m_multiplierWindow->setMultiplierWorked(q.arrlSection, q.band);
                    }
                }
            }

            LOG_INFO("MainWindow", QString("Updated QSO #%1 (%2)")
                .arg(editedQSO.id)
                .arg(editedQSO.callsign));
        } else {
            DialogHelper::warning(this, "Error",
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

    // Check if contest uses mode group breakdown
    bool usesModeGroupBreakdown = m_activeContest && m_activeContest->usesModeGroupBreakdown();

    // Initialize per-band counters
    QMap<BandType, int> qsosPerBand;
    QMap<BandType, int> pointsPerBand;
    QMap<BandType, int> multsPerBand;  // Count of mult QSOs per band
    QMap<BandType, QSet<int>> zonesPerBand;

    // Mode group counters (for mixed-mode contests)
    QMap<ModeGroup, QMap<BandType, int>> qsosPerBandPerModeGroup;
    QMap<ModeGroup, int> totalQSOsPerModeGroup;

    int totalQSOs = 0;
    int totalQSOPoints = 0;
    int totalMultQSOs = 0;  // Total QSOs that provided mults

    // Track unique multiplier values for scoring calculation
    QMap<MultiplierType, QSet<QString>> uniqueMultValues;

    // Get multiplier definitions from active contest
    QList<MultiplierDefinition> multDefs;
    if (m_activeContest) {
        multDefs = m_activeContest->getMultiplierTypes();
    }

    // Iterate through all QSOs in the table model
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);

        if (qso.band == BandType::None) {
            continue;  // Skip QSOs with no band
        }

        // Count QSOs per band
        qsosPerBand[qso.band]++;
        totalQSOs++;

        // Track mode group statistics if contest uses breakdown
        if (usesModeGroupBreakdown) {
            ModeGroup group = modeTypeToModeGroup(qso.mode);
            qsosPerBandPerModeGroup[group][qso.band]++;
            totalQSOsPerModeGroup[group]++;
        }

        // Sum points per band
        pointsPerBand[qso.band] += qso.qsoPoints;
        totalQSOPoints += qso.qsoPoints;

        // Count multiplier QSOs per band (simple - just check the flag!)
        if (qso.isMultiplier) {
            multsPerBand[qso.band]++;
            totalMultQSOs++;
        }

        // Track unique multiplier values for scoring
        // (Need to recalculate this for accurate scoring)
        if (m_activeContest) {
            for (const MultiplierDefinition& multDef : multDefs) {
                QString multValue = m_activeContest->getMultiplierValue(
                    qso, multDef.type, QStringList());
                if (!multValue.isEmpty()) {
                    uniqueMultValues[multDef.type].insert(multValue);
                }
            }
        }

        // Track unique zones per band (for "Zones" display column)
        if (qso.cqZone > 0) {
            zonesPerBand[qso.band].insert(qso.cqZone);
        }
    }

    // Calculate total multiplier counts per type FOR SCORING
    QMap<MultiplierType, int> multiplierCounts;
    for (auto it = uniqueMultValues.begin(); it != uniqueMultValues.end(); ++it) {
        multiplierCounts[it.key()] = it.value().size();
    }

    // Update band summary grid with calculated values
    QList<BandType> bands = {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };

    int totalMults = 0;
    int totalZones = 0;

    // WebServer calculates band data from QSOTableModel - no need to push

    if (usesModeGroupBreakdown) {
        // Update mode group QSO counts per band
        QList<ModeGroup> modeGroups = {ModeGroup::Phone, ModeGroup::CW, ModeGroup::Digital};
        for (ModeGroup group : modeGroups) {
            for (BandType band : bands) {
                int count = qsosPerBandPerModeGroup.value(group).value(band, 0);
                m_bandSummaryGrid->setModeGroupQSOCount(band, group, count);
            }

            // Update "All" column for each mode group
            int totalForGroup = totalQSOsPerModeGroup.value(group, 0);
            m_bandSummaryGrid->setAllModeGroupQSOs(group, totalForGroup);
        }
    } else {
        // Single-mode: Update regular QSO counts
        for (BandType band : bands) {
            int qsos = qsosPerBand.value(band, 0);
            m_bandSummaryGrid->setQSOCount(band, qsos);
        }
        m_bandSummaryGrid->setAllQSOs(totalQSOs);
    }

    // Update points, mults, and zones (same for both modes)
    for (BandType band : bands) {
        int points = pointsPerBand.value(band, 0);
        int mults = multsPerBand.value(band, 0);  // Simple count of marked mult QSOs
        int zones = zonesPerBand.value(band).size();

        m_bandSummaryGrid->setPointsCount(band, points);
        m_bandSummaryGrid->setMultCount(band, mults);
        m_bandSummaryGrid->setZoneCount(band, zones);

        // WebServer calculates band data from QSOTableModel - no need to push

        totalMults += mults;
        totalZones += zones;
    }

    // Calculate final contest score using contest's formula
    int finalScore = totalQSOPoints;  // Default if no contest active
    if (m_activeContest) {
        finalScore = m_activeContest->calculateTotalScore(totalQSOPoints, multiplierCounts);
    }

    // Update "All" column totals
    // Note: For mode breakdown contests, setAllQSOs() is called per mode group above
    if (!usesModeGroupBreakdown) {
        // Only set total QSOs for single-mode contests
        // (already set per mode group for mixed-mode contests)
        m_bandSummaryGrid->setAllQSOs(totalQSOs);
    }
    m_bandSummaryGrid->setAllMults(totalMults);
    m_bandSummaryGrid->setAllZones(totalZones);
    m_bandSummaryGrid->setAllPoints(totalQSOPoints);  // Sum of QSO points
    m_bandSummaryGrid->setFinalScore(finalScore);     // Contest score (points × mults)

    // Update status bar
    m_statusLabel->setText(QString("%1 QSOs, %2 Points").arg(totalQSOs).arg(finalScore));

    // WebServer calculates score from QSOTableModel - no need to push
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
    if (!m_hasActiveContest || !m_qsoTableModel || !m_integrityManager) {
        return true;  // Nothing to check
    }

    // Check if database is open before running integrity check
    Database& db = Database::instance();
    if (!db.isOpen()) {
        LOG_DEBUG("MainWindow", "Skipping integrity check - database is not open");
        return true;  // Not an error, just skip the check
    }

    // Delegate to DataIntegrityManager
    int memoryCount = m_qsoTableModel->count();
    bool result = m_integrityManager->quickIntegrityCheck(memoryCount);

    if (!result) {
        // Get actual counts for mismatch handler
        QSqlQuery query = db.execute(
            "SELECT COUNT(*) FROM qsos WHERE contest_id = ? AND deleted = 0",
            {m_currentContestDbId});
        int dbCount = 0;
        if (query.next()) {
            dbCount = query.value(0).toInt();
        }
        handleIntegrityMismatch(memoryCount, dbCount);
    }

    return result;
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

    QMessageBox::StandardButton reply = DialogHelper::warning(
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

// Rescore contest without showing dialogs (used by ADIF import)
RescoreStats MainWindow::rescoreContestSilent() {
    RescoreStats stats;

    if (!m_hasActiveContest || !m_activeContest || !m_qsoTableModel || !m_integrityManager) {
        return stats;  // Return empty stats if no contest
    }

    // Get station info for QSO point calculation
    StationInfo myStation;
    myStation.callsign = AppSettings::instance().getMyCallsign();
    myStation.continent = AppSettings::instance().getMyContinent();
    myStation.cqZone = AppSettings::instance().getMyCQZone();

    CountryData myCountryData = m_countryFile.lookup(myStation.callsign);
    if (myCountryData.isValid()) {
        myStation.country = myCountryData.name;
    }

    // Get all QSOs from table model
    QList<QSO> qsos;
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        qsos.append(m_qsoTableModel->getQSO(row));
    }

    // Delegate to DataIntegrityManager for rescoring
    stats = m_integrityManager->rescoreContestSilent(qsos, m_activeContest, myStation);

    // Update table model with rescored QSOs
    for (int row = 0; row < qsos.size(); ++row) {
        m_qsoTableModel->updateQSO(row, qsos[row]);
    }

    // Refresh display
    updateScoreDisplay();

    // Rebuild multiplier window from all QSOs
    // Update based on contest's primary multiplier type
    if (m_multiplierWindow && m_activeContest) {
        m_multiplierWindow->clear();

        QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();
        if (!multDefs.isEmpty()) {
            MultiplierType primaryMultType = multDefs.first().type;

            for (int row = 0; row < m_qsoTableModel->count(); ++row) {
                QSO qso = m_qsoTableModel->getQSO(row);

                // Use contest's getMultiplierValue() method which has contest-specific
                // filtering logic (e.g., RTTY Roundup excludes US/Canada from countries)
                QString multValue = m_activeContest->getMultiplierValue(qso, primaryMultType, QStringList());

                if (!multValue.isEmpty()) {
                    m_multiplierWindow->setMultiplierWorked(multValue, qso.band);
                }
            }

            LOG_DEBUG("MainWindow", QString("Updated multiplier window with worked %1 multipliers")
                .arg(multDefs.first().displayName));
        }
    }

    return stats;
}

// Rescore entire contest (recalculate QSO points and multiplier flags)
void MainWindow::onRescoreContest() {
    if (!m_hasActiveContest || !m_activeContest || !m_qsoTableModel) {
        DialogHelper::information(this, "Rescore Contest",
            "No active contest to rescore.");
        return;
    }

    // Confirm with user
    QString dialogMessage = QString("This will recalculate QSO points, multiplier flags, and duplicate status for all %1 QSOs in the contest log.\n\n"
                "This is useful for:\n"
                "- Detecting and marking duplicates (set to 0 points)\n"
                "- Updating old logs to new scoring rules\n"
                "- Fixing multiplier flags on pre-v2.85.0 QSOs\n"
                "- Validating scoring calculations\n\n"
                "Continue?").arg(m_qsoTableModel->count());

    QMessageBox::StandardButton reply = DialogHelper::question(this,
        "Rescore Contest",
        dialogMessage,
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    m_statusLabel->setText("Rescoring contest...");
    QApplication::processEvents();

    // Rescore contest (recalculate points, mults, dupes)
    // NOTE: No longer need to sync SCP database - worked calls are queried
    // directly from contest database, so SCP is always in sync
    RescoreStats stats = rescoreContestSilent();

    // Show results
    QString resultsMessage = QString("Contest rescored successfully!\n\n"
                "QSOs updated: %1\n"
                "Multipliers marked: %2\n"
                "Duplicates found: %3\n\n"
                "Score display has been refreshed.")
            .arg(stats.qsosUpdated).arg(stats.multsMarked).arg(stats.dupesFound);

    DialogHelper::information(this, "Rescore Complete", resultsMessage);

    m_statusLabel->setText(QString("Rescore complete: %1 QSOs updated, %2 mults marked, %3 dupes found")
        .arg(stats.qsosUpdated).arg(stats.multsMarked).arg(stats.dupesFound));
}

void MainWindow::onEditContestSettings() {
    if (!m_hasActiveContest || !m_activeContest) {
        DialogHelper::information(this, "Edit Contest Settings",
            "No active contest. Create or resume a contest first.");
        return;
    }

    if (!m_contestService) {
        DialogHelper::critical(this, "Error", "Contest service not initialized");
        LOG_ERROR("MainWindow", "onEditContestSettings called but m_contestService is null");
        return;
    }

    // Get current exchange sent from the active contest
    QString currentExchange = m_activeContest->getExchangeSent();

    // Show input dialog to edit exchange
    bool ok;
    QString newExchange = QInputDialog::getText(
        this,
        "Edit Contest Settings",
        QString("Sent Exchange for %1:\n\nEnter the exchange you are sending (e.g., \"1H WCF\" for Winter Field Day)").arg(m_currentContest.contestName),
        QLineEdit::Normal,
        currentExchange,
        &ok
    );

    if (!ok || newExchange.trimmed().isEmpty()) {
        return;  // User cancelled or entered empty exchange
    }

    newExchange = newExchange.trimmed().toUpper();

    // Check if exchange actually changed
    if (newExchange == currentExchange) {
        DialogHelper::information(this, "Edit Contest Settings",
            "Exchange not changed.");
        return;
    }

    // Delegate to ContestService
    LOG_INFO("MainWindow", QString("Updating contest exchange from \"%1\" to \"%2\"")
        .arg(currentExchange).arg(newExchange));

    m_statusLabel->setText("Updating contest exchange...");
    QApplication::processEvents();

    ContestService::UpdateExchangeResult result = m_contestService->updateContestExchange(newExchange);

    if (!result.success) {
        DialogHelper::critical(this, "Error", result.errorMessage);
        m_statusLabel->setText("Failed to update contest exchange");
        return;
    }

    // Update status label
    m_statusLabel->setText(result.statusMessage);

    // Ask if user wants to rescore (to recalculate points and multipliers)
    if (result.qsosUpdated > 0) {
        QMessageBox::StandardButton reply = DialogHelper::question(
            this,
            "Rescore Contest?",
            QString("Exchange updated successfully for %1 QSOs!\n\n"
                    "Old exchange: %2\n"
                    "New exchange: %3\n\n"
                    "Would you like to rescore the contest to recalculate points and multipliers?")
                .arg(result.qsosUpdated)
                .arg(currentExchange.isEmpty() ? "(none)" : currentExchange)
                .arg(newExchange),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            onRescoreContest();
        }
    }
}

void MainWindow::onFullIntegrityCheck() {
    if (!m_hasActiveContest || !m_qsoTableModel) {
        DialogHelper::information(this, "Integrity Check",
            "No active contest to validate.");
        return;
    }

    m_statusLabel->setText("Running full integrity check...");
    QApplication::processEvents();  // Update UI

    QString report = fullIntegrityCheck(false);  // Show all checks (critical + informational)

    // Display report
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Log Integrity Check Report");
    dialog->resize(UIDefaults::ADIF_IMPORT_PREVIEW_WIDTH, UIDefaults::ADIF_IMPORT_PREVIEW_HEIGHT);

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

QString MainWindow::fullIntegrityCheck(bool criticalOnly) {
    QString report;
    report += "=== LOG INTEGRITY CHECK REPORT ===\n\n";
    report += QString("Contest: %1\n").arg(m_currentContest.contestName);
    report += QString("Database: %1\n").arg(m_currentContest.databasePath);
    report += QString("Check time: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    report += QString("Mode: %1\n\n").arg(criticalOnly ? "Critical issues only" : "All issues (critical + informational)");

    int memoryCount = m_qsoTableModel->count();
    Database& db = Database::instance();

    // Check if database is open before running integrity checks
    if (!db.isOpen()) {
        report += "✗ CRITICAL: Database is not open!\n\n";
        report += "Cannot perform integrity check on closed database.\n";
        report += "This may occur if the contest was closed or the database connection failed.\n\n";
        report += "=== END OF REPORT ===\n";
        return report;
    }

    // CRITICAL: Checkpoint WAL to ensure all recent writes are visible
    // Without this, the integrity check may report false positives for recently-logged QSOs
    // that are in the WAL but not yet visible to new queries
    QSqlQuery checkpointQuery = db.execute("PRAGMA wal_checkpoint(PASSIVE)", {});
    checkpointQuery.next();  // Execute the checkpoint

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

    // Check 5: Detect QSOs with Unknown/None band (CRITICAL)
    QSqlQuery unknownBandQuery = db.execute(
        "SELECT id, callsign, timestamp, band FROM qsos "
        "WHERE contest_id = ? AND deleted = 0 "
        "AND (band = 'Unknown' OR band = 'None' OR band = '')",
        {m_currentContestDbId});

    QStringList unknownBands;
    while (unknownBandQuery.next()) {
        int id = unknownBandQuery.value(0).toInt();
        QString callsign = unknownBandQuery.value(1).toString();
        QString timestamp = unknownBandQuery.value(2).toString();
        QString band = unknownBandQuery.value(3).toString();
        unknownBands.append(QString("ID=%1: %2 at %3 (band='%4')")
            .arg(id).arg(callsign).arg(timestamp).arg(band));
    }

    if (unknownBands.isEmpty()) {
        report += "✓ No QSOs with Unknown/None band\n\n";
    } else {
        report += QString("✗ CRITICAL: %1 QSOs with Unknown/None band:\n")
            .arg(unknownBands.size());
        for (const QString& item : unknownBands) {
            report += QString("  - %1\n").arg(item);
        }
        report += "  Recommendation: Manually edit these QSOs to set correct band\n\n";
    }

    // Check 6: Detect lowercase data in critical fields (INFORMATIONAL)
    // This is non-critical but indicates data entry inconsistency
    if (!criticalOnly) {
        QSqlQuery lowercaseQuery = db.execute(
            "SELECT id, callsign, rst_sent, rst_received, exchange_sent, exchange_received "
            "FROM qsos WHERE contest_id = ? AND deleted = 0",
            {m_currentContestDbId});

        QStringList lowercaseIssues;
        while (lowercaseQuery.next()) {
            int id = lowercaseQuery.value(0).toInt();
            QString callsign = lowercaseQuery.value(1).toString();
            QString rstSent = lowercaseQuery.value(2).toString();
            QString rstReceived = lowercaseQuery.value(3).toString();
            QString exchangeSent = lowercaseQuery.value(4).toString();
            QString exchangeReceived = lowercaseQuery.value(5).toString();

            QStringList fields;
            if (callsign != callsign.toUpper()) fields << "callsign";
            if (rstSent != rstSent.toUpper()) fields << "rst_sent";
            if (rstReceived != rstReceived.toUpper()) fields << "rst_received";
            if (exchangeSent != exchangeSent.toUpper()) fields << "exchange_sent";
            if (exchangeReceived != exchangeReceived.toUpper()) fields << "exchange_received";

            if (!fields.isEmpty()) {
                lowercaseIssues.append(QString("ID=%1: %2 (%3)")
                    .arg(id).arg(callsign).arg(fields.join(", ")));
            }
        }

        if (lowercaseIssues.isEmpty()) {
            report += "✓ All text fields are uppercase\n\n";
        } else {
            report += QString("ℹ INFO: %1 QSOs with lowercase data:\n")
                .arg(lowercaseIssues.size());
            // Limit to first 10 to avoid overwhelming output
            int displayed = qMin(10, lowercaseIssues.size());
            for (int i = 0; i < displayed; i++) {
                report += QString("  - %1\n").arg(lowercaseIssues[i]);
            }
            if (lowercaseIssues.size() > 10) {
                report += QString("  ... and %1 more\n").arg(lowercaseIssues.size() - 10);
            }
            report += "  Note: This is informational only. Uppercase validation added in v3.30.0.\n\n";
        }
    }

    // Check 7: Database schema version validation (CRITICAL)
    QSqlQuery versionQuery = db.execute("PRAGMA user_version", {});
    int dbSchemaVersion = 0;
    if (versionQuery.next()) {
        dbSchemaVersion = versionQuery.value(0).toInt();
    }

    const int EXPECTED_SCHEMA_VERSION = 8;  // From Database.h CURRENT_SCHEMA_VERSION
    bool schemaVersionMismatch = false;
    if (dbSchemaVersion == EXPECTED_SCHEMA_VERSION) {
        report += QString("✓ Database schema version matches (v%1)\n\n").arg(EXPECTED_SCHEMA_VERSION);
    } else {
        schemaVersionMismatch = true;
        report += QString("✗ CRITICAL: Schema version mismatch!\n");
        report += QString("  Database: v%1\n").arg(dbSchemaVersion);
        report += QString("  Expected: v%1\n").arg(EXPECTED_SCHEMA_VERSION);
        report += "  Recommendation: Restart TR4QT to trigger automatic migration\n\n";
    }

    // Check 8: Required columns existence check (CRITICAL)
    QSqlQuery columnsQuery = db.execute("PRAGMA table_info(qsos)", {});
    QSet<QString> existingColumns;
    while (columnsQuery.next()) {
        existingColumns.insert(columnsQuery.value(1).toString());
    }

    QStringList requiredColumns = {
        "id", "contest_id", "callsign", "timestamp", "frequency", "band", "mode",
        "rst_sent", "rst_received", "exchange_sent", "exchange_received",
        "serial_number", "serial_number_received", "precedence", "sweepstakes_check",
        "power", "operator_name", "itu_zone_exchange",
        "dxcc_entity", "cq_zone", "itu_zone", "continent",
        "qso_points", "is_dupe", "is_multiplier", "deleted"
    };

    QStringList missingColumns;
    for (const QString& col : requiredColumns) {
        if (!existingColumns.contains(col)) {
            missingColumns.append(col);
        }
    }

    if (missingColumns.isEmpty()) {
        report += QString("✓ All %1 required columns exist\n\n").arg(requiredColumns.size());
    } else {
        report += QString("✗ CRITICAL: %1 required columns missing from qsos table:\n")
            .arg(missingColumns.size());
        for (const QString& col : missingColumns) {
            report += QString("  - %1\n").arg(col);
        }
        report += "  Recommendation: Restart TR4QT to trigger automatic migration\n\n";
    }

    // Check 9: QSO load validation test (CRITICAL)
    int loadFailures = 0;
    int sampleSize = qMin(10, dbCount);  // Test first 10 QSOs

    if (sampleSize > 0) {
        QSqlQuery sampleQuery = db.execute(
            "SELECT id FROM qsos WHERE contest_id = ? AND deleted = 0 LIMIT ?",
            {m_currentContestDbId, sampleSize});

        while (sampleQuery.next()) {
            int qsoId = sampleQuery.value(0).toInt();
            QSO loadedQso = repo.findById(qsoId);

            // Verify QSO was actually loaded (not just default-constructed)
            if (loadedQso.id != qsoId || loadedQso.callsign.isEmpty()) {
                loadFailures++;
            }
        }

        if (loadFailures == 0) {
            report += QString("✓ QSO load test passed (sampled %1 QSOs)\n\n").arg(sampleSize);
        } else {
            report += QString("✗ CRITICAL: %1/%2 QSOs failed to load correctly\n")
                .arg(loadFailures).arg(sampleSize);
            report += "  This suggests a database schema issue or data corruption\n";
            report += "  Recommendation: Check logs for SQL errors, verify schema version\n\n";
        }
    } else {
        report += "✓ QSO load test skipped (no QSOs in database)\n\n";
    }

    // Summary
    report += "=== SUMMARY ===\n";
    bool criticalIssues = (memoryCount != dbCount) ||
                          !missingInDB.isEmpty() ||
                          !orphanedInDB.isEmpty() ||
                          (fieldMismatches > 0) ||
                          !unknownBands.isEmpty() ||
                          schemaVersionMismatch ||
                          !missingColumns.isEmpty() ||
                          (loadFailures > 0);

    if (!criticalIssues) {
        report += "✓ ALL CRITICAL CHECKS PASSED - Log integrity verified\n";
        if (!criticalOnly) {
            report += "  (Informational checks may have reported non-critical issues above)\n";
        }
    } else {
        report += "✗ CRITICAL ISSUES DETECTED - See details above\n";
        report += "\nRecommendation: Consider reloading contest from database\n";
    }

    LOG_INFO("MainWindow", QString("Full integrity check: %1")
        .arg(!criticalIssues ? "PASSED" : "FAILED"));

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
    QDateTime hourStart = QDateTime(now.date(), QTime(now.time().hour(), 0), QTimeZone::utc());

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

    // Update window menu checkmarks
    updateWindowMenuCheckmarks();
}

void MainWindow::updateRadioStatusGrid() {
    // Update band/mode (e.g., "15SSB") and frequency
    // Display frequency and mode even when band is unknown (e.g., outside amateur bands)
    if (m_currentState.frequencyA > 0) {
        // Show band+mode if band is known (e.g., "40LSB"), otherwise just mode (e.g., "LSB")
        if (m_currentState.bandA != BandType::None) {
            QString bandStr = bandToString(m_currentState.bandA).remove('M');  // Remove 'M' from "15M" -> "15"
            QString modeStr = modeToString(m_currentState.modeA);
            m_radioFreqBandLabel->setText(QString("%1%2").arg(bandStr).arg(modeStr));
        } else {
            // Unknown band (e.g., outside amateur bands) - just show mode
            QString modeStr = modeToString(m_currentState.modeA);
            m_radioFreqBandLabel->setText(modeStr);
        }

        // Update frequency (in MHz with 3 decimal places)
        double freqMHz = m_currentState.frequencyA / 1000000.0;
        m_radioFreqLabel->setText(QString("%1 MHz").arg(freqMHz, 0, 'f', 3));
    } else {
        m_radioFreqBandLabel->setText("--");
        m_radioFreqLabel->setText("0.000 MHz");
    }

    // Update WPM label (only enabled in CW mode AND when auto-send is enabled)
    bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
    bool autoSendEnabled = m_autoSendCWAction->isChecked();  // Check actual action state, not settings
    int wpm = m_currentState.cwSpeed;  // Display radio's actual CW speed, not app setting
    m_radioWpmLabel->setText(QString("%1 WPM").arg(wpm));
    m_radioWpmLabel->setEnabled(isCWMode && autoSendEnabled);  // Gray out when not in CW mode or auto-send disabled

    // Update date/time (current local time)
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("ddd dd-MMM-yyyy");
    QString timeStr = now.toString("hh:mm:ss");
    m_radioDateLabel->setText(dateStr);
    m_radioTimeLabel->setText(timeStr);
}

void MainWindow::updateWindowMenuCheckmarks() {
    // Update checkmarks in Window menu to reflect which windows are currently open
    if (m_bandMapAction) {
        m_bandMapAction->setChecked(m_bandMapWindow && m_bandMapWindow->isVisible());
    }
    if (m_dxClusterAction) {
        m_dxClusterAction->setChecked(m_dxClusterWindow && m_dxClusterWindow->isVisible());
    }
    if (m_radioControlAction) {
        m_radioControlAction->setChecked(m_radioControlWindow && m_radioControlWindow->isVisible());
    }
    if (m_multipliersAction) {
        m_multipliersAction->setChecked(m_multiplierWindow && m_multiplierWindow->isVisible());
    }
    if (m_statisticsAction) {
        m_statisticsAction->setChecked(m_statisticsWindow && m_statisticsWindow->isVisible());
    }
    if (m_sectionsMapAction) {
        m_sectionsMapAction->setChecked(m_sectionsMapViewer && m_sectionsMapViewer->isVisible());
    }
    if (m_statesMapAction) {
        m_statesMapAction->setChecked(m_statesMapViewer && m_statesMapViewer->isVisible());
    }
    if (m_graylineMapAction) {
        m_graylineMapAction->setChecked(m_graylineMapDialog && m_graylineMapDialog->isVisible());
    }
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
    m_stationInfoLabel->setFont(miscFont);

    // Apply SCP matches font size
    int scpFontSize = settings.getSCPFontSize();
    m_scpMatchesLabel->setStyleSheet(QString("QLabel { color: #0066cc; font-size: %1pt; }").arg(scpFontSize));
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
    QSqlQuery query = db.execute("SELECT contest_id, contest_name, start_time, contest_type FROM contests LIMIT 1", {});
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
    contestInfo.contestType = query.value(3).toString();  // Read contest_type directly from database!
    contestInfo.databasePath = lastContestPath;
    contestInfo.isExisting = true;

    // Determine mode from contest_id (for backward compatibility with CW/SSB specific contests)
    // This is only used for display and mode restrictions
    QString contestId = contestInfo.contestId;
    if (contestId.contains("_CW")) {
        contestInfo.mode = "CW";
    } else if (contestId.contains("_SSB")) {
        contestInfo.mode = "SSB";
    } else {
        contestInfo.mode = "Mixed";
    }

    LOG_DEBUG("MainWindow", QString("Loaded contest from database: type='%1', mode='%2', name='%3'")
        .arg(contestInfo.contestType, contestInfo.mode, contestInfo.contestName));

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
    // CRITICAL: Reset contest state FIRST to prevent corrupted state if activation fails
    // This ensures that if ANY step below fails and returns early, we're left in a clean state
    m_hasActiveContest = false;
    m_currentContestDbId = -1;

    // Clean up previous contest if any
    if (m_activeContest) {
        delete m_activeContest;
        m_activeContest = nullptr;

        // Clear contest from DX Cluster window
        if (m_dxClusterWindow) {
            m_dxClusterWindow->setActiveContest(nullptr, -1);
        }
    }

    // Delegate contest activation to ContestManager
    if (!m_contestManager) {
        DialogHelper::critical(this, "Configuration Error",
            "ContestManager not initialized. Cannot activate contest.");
        return;
    }

    ActivateContestResult result = m_contestManager->activateContest(contestInfo);

    // Handle activation errors
    if (!result.success) {
        DialogHelper::critical(this, "Contest Activation Error", result.errorMessage);
        return;
    }

    // Activation successful - update MainWindow state
    m_activeContest = result.contest;  // Transfer ownership
    m_currentContestDbId = result.contestDbId;
    m_nextSerialNumber = result.nextSerialNumber;
    m_currentContest = contestInfo;
    m_hasActiveContest = true;

    // Load existing QSOs into table model
    m_qsoTableModel->clear();
    m_bandSummaryGrid->clearAll();

    // Clear multiplier window when loading new contest
    if (m_multiplierWindow) {
        m_multiplierWindow->clear();
    }

    for (const QSO& qso : result.loadedQSOs) {
        m_qsoTableModel->addQSO(qso);
    }

    LOG_DEBUG("MainWindow", QString("Loaded %1 existing QSOs").arg(result.loadedQSOs.size()));

    // Update band summary grid with loaded QSOs
    updateScoreDisplay();

    // Scroll to bottom to show latest QSO
    if (!result.loadedQSOs.isEmpty()) {
        m_qsoTableView->scrollToBottom();
        int lastRow = m_qsoTableModel->rowCount() - 1;
        m_qsoTableView->selectRow(lastRow);
    }

    // Create QSOLogger instance with contest configuration
    if (m_qsoLogger) {
        delete m_qsoLogger;
    }
    QSOLogger::Config loggerConfig;
    loggerConfig.contest = m_activeContest;
    loggerConfig.countryFile = &m_countryFile;
    loggerConfig.myStation = result.myStation;  // Use myStation from ContestManager
    m_qsoLogger = new QSOLogger(loggerConfig);
    LOG_DEBUG("MainWindow", "QSOLogger created for contest");

    // Create DataIntegrityManager instance with contest configuration
    if (m_integrityManager) {
        delete m_integrityManager;
    }
    DataIntegrityManager::Config integrityConfig;
    integrityConfig.countryFile = &m_countryFile;
    integrityConfig.currentContestDbId = m_currentContestDbId;
    m_integrityManager = new DataIntegrityManager(integrityConfig);
    LOG_DEBUG("MainWindow", "DataIntegrityManager created for contest");

    // Create ContestService instance with contest configuration
    if (m_contestService) {
        delete m_contestService;
    }
    ContestService::Config contestServiceConfig;
    contestServiceConfig.activeContest = m_activeContest;
    contestServiceConfig.qsoTableModel = m_qsoTableModel;
    contestServiceConfig.currentContestDbId = m_currentContestDbId;
    m_contestService = new ContestService(contestServiceConfig);
    LOG_DEBUG("MainWindow", "ContestService created for contest");

    // Update ImportExportManager with new contest configuration (if it exists)
    // Note: During startup, reopenLastContest() runs before ImportExportManager is created
    if (m_importExportManager) {
        ImportExportManager::Config importExportConfig;
        importExportConfig.countryFile = &m_countryFile;
        importExportConfig.qsoTableModel = m_qsoTableModel;
        importExportConfig.activeContest = m_activeContest;
        importExportConfig.currentContestDbId = m_currentContestDbId;
        importExportConfig.currentContestName = m_currentContest.contestName;
        importExportConfig.hasActiveContest = m_hasActiveContest;
        m_importExportManager->updateConfig(importExportConfig);
        LOG_DEBUG("MainWindow", "ImportExportManager updated for contest");
    }

    // Update DX Cluster window with active contest (for dupe/mult checking)
    if (m_dxClusterWindow) {
        m_dxClusterWindow->setActiveContest(m_activeContest, m_currentContestDbId);
    }

    // Update web server with contest name (myCall is pulled from AppSettings)
    m_webServer->setContestName(contestInfo.contestName);

    // Save as last opened contest for auto-reopen on next startup
    AppSettings::instance().setLastContestPath(contestInfo.databasePath);

    // Update UI based on contest capabilities
    if (m_bandSummaryGrid) {
        m_bandSummaryGrid->setMultipliersEnabled(result.usesMultipliers);

        // Set visible bands based on contest restrictions (e.g., RTTY excludes 160m)
        m_bandSummaryGrid->setVisibleBands(result.allowedBands);

        // Configure grid for mode group breakdown and zone tracking
        m_bandSummaryGrid->configureForContest(
            result.usesModeGroupBreakdown,
            result.usesZones
        );

        // Update web server with contest settings
        m_webServer->setUsesZoneMultipliers(result.usesZones);
        m_webServer->setUsesModeGroupBreakdown(result.usesModeGroupBreakdown);
    }

    // Configure multiplier window to show contest-specific multipliers
    if (m_multiplierWindow && !result.multiplierTypes.isEmpty()) {
        // Set to the first (primary) multiplier type
        m_multiplierWindow->setMultiplierType(result.multiplierTypes.first().type);

        // For Country type, load the list from CountryFile instead of hardcoded
        if (result.multiplierTypes.first().type == MultiplierType::Country) {
            m_multiplierWindow->setCountryList(m_countryFile.getAllPrimaryPrefixes());
        }

        LOG_DEBUG("MainWindow", QString("Set multiplier window to type: %1")
            .arg(result.multiplierTypes.first().displayName));
    } else if (m_multiplierWindow) {
        // Default to sections if contest doesn't define multipliers
        m_multiplierWindow->setMultiplierType(MultiplierType::Section);
    }

    // Update window title to include contest name and type
    setWindowTitle(QString("%1 v%2 - %3 (%4)")
                      .arg(APP_NAME)
                      .arg(APP_VERSION)
                      .arg(contestInfo.contestName)
                      .arg(m_activeContest->getContestName()));

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

        // Notify Band Map of the default band (in case it was already restored from settings)
        emit currentBandChanged(m_currentState.bandA);
        emit currentFrequencyChanged(m_currentState.frequencyA);

        LOG_DEBUG("MainWindow", QString("Set default band/mode/freq: %1 %2 %3 Hz (radio not connected)")
            .arg(bandToString(m_currentState.bandA))
            .arg(modeToString(m_currentState.modeA))
            .arg(QString::number(m_currentState.frequencyA, 'f', 0)));
    }

    // Recalculate points for all QSOs (fixes old QSOs with 0 points)
    // Must be called after m_activeContest is created
    if (m_qsoTableModel->count() > 0) {
        recalculateAllPoints();
    }

    // Update multiplier window with all loaded QSOs (must be after contest is created)
    if (m_multiplierWindow && m_activeContest && m_qsoTableModel->count() > 0 &&
        !result.multiplierTypes.isEmpty()) {
        MultiplierType primaryMultType = result.multiplierTypes.first().type;

        for (int row = 0; row < m_qsoTableModel->count(); ++row) {
            QSO qso = m_qsoTableModel->getQSO(row);

            // Use contest's getMultiplierValue() method which has contest-specific
            // filtering logic (e.g., RTTY Roundup excludes US/Canada from countries)
            QString multValue = m_activeContest->getMultiplierValue(qso, primaryMultType, QStringList());

            if (!multValue.isEmpty()) {
                m_multiplierWindow->setMultiplierWorked(multValue, qso.band);
            }
        }

        LOG_DEBUG("MainWindow", QString("Loaded %1 worked multipliers into multiplier window")
            .arg(m_qsoTableModel->count()));
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

    // Restore saved column widths for this contest (after table columns are set)
    loadQSOTableColumnWidths();
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

// Band needs tracking methods

QSet<QString> MainWindow::getWorkedCallsigns() const {
    QSet<QString> workedCallsigns;

    // Collect all unique callsigns from the log
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);
        workedCallsigns.insert(qso.callsign.toUpper());  // Store in uppercase for matching
    }

    return workedCallsigns;
}

QList<BandType> MainWindow::getWorkedBandsForCallsign(const QString& callsign) const {
    QList<BandType> workedBands;

    if (callsign.isEmpty()) {
        return workedBands;
    }

    // Search through all QSOs in the table model for this callsign
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);
        if (qso.callsign.compare(callsign, Qt::CaseInsensitive) == 0) {
            if (!workedBands.contains(qso.band)) {
                workedBands.append(qso.band);
            }
        }
    }

    return workedBands;
}

QList<BandType> MainWindow::getWorkedBandsForMultiplier(const QString& multValue,
                                                        MultiplierType type) const {
    QList<BandType> workedBands;

    if (!m_activeContest || multValue.isEmpty()) {
        return workedBands;
    }

    // Search through all QSOs and find which bands have this multiplier
    for (int row = 0; row < m_qsoTableModel->count(); ++row) {
        QSO qso = m_qsoTableModel->getQSO(row);

        // Get the multiplier value for this QSO
        QString qsoMultValue = m_activeContest->getMultiplierValue(
            qso, type, QStringList());

        if (qsoMultValue.compare(multValue, Qt::CaseInsensitive) == 0) {
            if (!workedBands.contains(qso.band)) {
                workedBands.append(qso.band);
            }
        }
    }

    return workedBands;
}

QString MainWindow::getMultiplierValueForCallsign(const QString& callsign) const {
    if (!m_activeContest || callsign.isEmpty()) {
        return QString();
    }

    // Build a temporary QSO and populate DXCC fields using centralized function
    QSO tempQso;
    tempQso.callsign = callsign;
    m_countryFile.populateQSODXCCFields(tempQso);

    if (tempQso.dxccEntity.isEmpty()) {
        return QString();
    }

    // Get the contest's primary multiplier type
    QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();
    if (multDefs.isEmpty()) {
        return QString();
    }

    // For now, use the first multiplier type
    // Most contests have Country or Zone as primary multiplier
    MultiplierType primaryMultType = multDefs.first().type;

    // Get the multiplier value
    QString multValue = m_activeContest->getMultiplierValue(
        tempQso, primaryMultType, QStringList());

    return multValue;
}

// Window menu slot implementations

void MainWindow::onShowDXCluster() {
    if (!m_dxClusterWindow) {
        m_dxClusterWindow = new DXClusterWindow();
        m_dxClusterWindow->setWindowTitle("DX Cluster");
        m_dxClusterWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Pass country file for DXCC/zone lookup
        m_dxClusterWindow->setCountryFile(&m_countryFile);

        // Pass active contest if one is loaded
        if (m_hasActiveContest && m_activeContest) {
            m_dxClusterWindow->setActiveContest(m_activeContest, m_currentContestDbId);
        }

        // Connect spot signal to forward spots to band map
        connect(m_dxClusterWindow, &DXClusterWindow::spotReceived,
                this, &MainWindow::onDXSpotReceived);

        // Connect QSY signal to tune radio to clicked frequency
        connect(m_dxClusterWindow, &DXClusterWindow::qsyRequested,
                this, [this](double frequency) {
                    if (m_radioConnected) {
                        LOG_DEBUG("MainWindow", QString("DX Cluster click-to-QSY: %1 Hz").arg(QString::number(frequency, 'f', 0)));
                        m_radio->setFrequency(static_cast<freq_t>(frequency));
                    } else {
                        LOG_DEBUG("MainWindow", QString("DX Cluster click-to-QSY: Radio not connected, cannot QSY to %1 Hz").arg(QString::number(frequency, 'f', 0)));
                    }
                });

        // Update WindowManager config with new window
        if (m_windowManager) {
            WindowManager::Config config;
            config.dxClusterWindow = m_dxClusterWindow;
            config.bandMapWindow = m_bandMapWindow;
            config.radioControlWindow = m_radioControlWindow;
            config.multiplierWindow = m_multiplierWindow;
            config.statisticsWindow = m_statisticsWindow;
            config.sectionsMapViewer = m_sectionsMapViewer;
            config.statesMapViewer = m_statesMapViewer;
            config.graylineMapDialog = m_graylineMapDialog;
            m_windowManager->setWindows(config);
        }
    }

    // Delegate show/raise to WindowManager
    if (m_windowManager) {
        m_windowManager->showWindow(m_dxClusterWindow);
    } else {
        // Fallback if WindowManager not available
        m_dxClusterWindow->show();
        m_dxClusterWindow->raise();
        m_dxClusterWindow->activateWindow();
    }

    updateWindowMenuCheckmarks();
}

void MainWindow::onShowBandMap() {
    if (!m_bandMapWindow) {
        m_bandMapWindow = new BandMapWidget();
        m_bandMapWindow->setWindowTitle("Band Map");
        m_bandMapWindow->setWindowFlags(Qt::Window);
        m_bandMapWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Connect current frequency changes to Band Map (for band filtering)
        connect(this, &MainWindow::currentFrequencyChanged,
                m_bandMapWindow, &BandMapWidget::setCurrentFrequency);

        // Connect current band changes to Band Map (for filtering when radio not connected)
        connect(this, &MainWindow::currentBandChanged,
                m_bandMapWindow, &BandMapWidget::setCurrentBand);

        // Connect band map signals
        connect(m_bandMapWindow, &BandMapWidget::qsyRequested,
                this, [this](freq_t frequency) {
                    if (m_radioConnected) {
                        LOG_DEBUG("MainWindow", QString("Band Map QSY to %1 Hz").arg(QString::number(frequency, 'f', 0)));
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

    // Send current band to Band Map (works for new window or restored window)
    // This ensures the filter knows the current band even if window was restored from settings
    if (m_currentState.bandA != BandType::None) {
        LOG_DEBUG("MainWindow", QString("Sending current band to Band Map: %1").arg(bandToString(m_currentState.bandA)));
        m_bandMapWindow->setCurrentBand(m_currentState.bandA);
    }

    m_bandMapWindow->show();
    m_bandMapWindow->raise();
    m_bandMapWindow->activateWindow();
    updateWindowMenuCheckmarks();
}

void MainWindow::onShowRadioControl() {
    if (!m_radioControlWindow) {
        m_radioControlWindow = new RadioControlWidget();
        m_radioControlWindow->setWindowTitle("Radio Control");
        m_radioControlWindow->setWindowFlags(Qt::Window);
        m_radioControlWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Set radio controller reference for mode menu
        m_radioControlWindow->setRadioController(m_radio);

        // Connect mode change requests
        connect(m_radioControlWindow, &RadioControlWidget::modeChangeRequested,
                this, [this](ModeType mode) {
                    LOG_INFO("MainWindow", QString("Mode change requested from radio control: %1").arg(static_cast<int>(mode)));
                    m_radio->setMode(mode, VFO::VFO_A);
                });

        // Connect CW speed change requests
        connect(m_radioControlWindow, &RadioControlWidget::cwSpeedChangeRequested,
                this, [this](int wpm) {
                    LOG_INFO("MainWindow", QString("CW speed change requested from radio control: %1 WPM").arg(wpm));
                    m_radio->setCWSpeed(wpm);
                });

        // Connect RIT/XIT/SPLIT toggle requests
        connect(m_radioControlWindow, &RadioControlWidget::ritToggled,
                this, [this](bool enabled) {
                    LOG_INFO("MainWindow", QString("RIT toggle requested: %1").arg(enabled ? "ON" : "OFF"));
                    m_radio->enableRIT(enabled, VFO::VFO_A);
                });

        connect(m_radioControlWindow, &RadioControlWidget::xitToggled,
                this, [this](bool enabled) {
                    LOG_INFO("MainWindow", QString("XIT toggle requested: %1").arg(enabled ? "ON" : "OFF"));
                    m_radio->enableXIT(enabled, VFO::VFO_A);
                });

        connect(m_radioControlWindow, &RadioControlWidget::splitToggled,
                this, [this](bool enabled) {
                    LOG_INFO("MainWindow", QString("SPLIT toggle requested: %1").arg(enabled ? "ON" : "OFF"));
                    m_radio->setSplit(enabled, VFO::VFO_B);
                });

        // Update with current radio state
        if (m_radioConnected) {
            double freqKHz = m_currentState.frequencyA / 1000.0;
            LOG_DEBUG("MainWindow", QString("Initializing Radio Control Window with state - freq=%1 kHz, mode=%2, band=%3")
                      .arg(freqKHz, 0, 'f', 1)
                      .arg(static_cast<int>(m_currentState.modeA))
                      .arg(static_cast<int>(m_currentState.bandA)));
            m_radioControlWindow->updateRadioState(m_currentState);
        }
    }
    m_radioControlWindow->show();
    m_radioControlWindow->raise();
    m_radioControlWindow->activateWindow();
    updateWindowMenuCheckmarks();
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
    updateWindowMenuCheckmarks();
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
    updateWindowMenuCheckmarks();
}

void MainWindow::onShowSectionsMap() {
    // Create and show ARRL Sections map viewer
    // Uses native Qt graphics (QGraphicsView), works on all platforms
    if (!m_sectionsMapViewer) {
        m_sectionsMapViewer = new NativeMapViewer(NativeMapViewer::Sections, m_qsoTableModel, this);
        m_sectionsMapViewer->setWindowFlags(Qt::Window);
        m_sectionsMapViewer->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_sectionsMapViewer->show();
    m_sectionsMapViewer->raise();
    m_sectionsMapViewer->activateWindow();
    updateWindowMenuCheckmarks();
}

void MainWindow::onShowStatesMap() {
    // Create and show US States map viewer (WAS tracking)
    // Uses native Qt graphics (QGraphicsView), works on all platforms
    if (!m_statesMapViewer) {
        m_statesMapViewer = new NativeMapViewer(NativeMapViewer::States, m_qsoTableModel, this);
        m_statesMapViewer->setWindowFlags(Qt::Window);
        m_statesMapViewer->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_statesMapViewer->show();
    m_statesMapViewer->raise();
    m_statesMapViewer->activateWindow();
    updateWindowMenuCheckmarks();
}

void MainWindow::onShowGraylineMap() {
    // Create and show grayline propagation map
    if (!m_graylineMapDialog) {
        m_graylineMapDialog = new GraylineMapDialog(this);
        m_graylineMapDialog->setWindowFlags(Qt::Window);
        m_graylineMapDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_graylineMapDialog->show();
    m_graylineMapDialog->raise();
    m_graylineMapDialog->activateWindow();
    updateWindowMenuCheckmarks();
}

// Window menu placeholder implementations
void MainWindow::onSwapMultView() {
    LOG_DEBUG("MainWindow", "Swap Mult View (Alt+G) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Swap Mult View feature will be implemented in a future version.\n\n"
                           "This will toggle between different multiplier display modes.");
}

void MainWindow::onMissingMultsReport() {
    LOG_DEBUG("MainWindow", "Missing Mults Report (Ctrl+O) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Missing Mults Report will be implemented in a future version.\n\n"
                           "This will show a report of multipliers still needed.");
}

// Edit menu placeholder implementations
void MainWindow::onViewEditLog() {
    LOG_DEBUG("MainWindow", "View/Edit Log (Ctrl+L) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "View/Edit Log will be implemented in a future version.\n\n"
                           "This will show all logged QSOs in a table for viewing and editing.");
}

void MainWindow::onClearDupes() {
    LOG_DEBUG("MainWindow", "Clear Dupes (Ctrl+K) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Clear Dupes will be implemented in a future version.\n\n"
                           "This will remove duplicate QSOs from the log.");
}

void MainWindow::onNote() {
    LOG_DEBUG("MainWindow", "Note (Ctrl+N) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Note feature will be implemented in a future version.\n\n"
                           "This will allow adding notes to the log.");
}

void MainWindow::onRecallLast() {
    LOG_DEBUG("MainWindow", "Recall Last Entry (Ctrl+R) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Recall Last Entry will be implemented in a future version.\n\n"
                           "This will recall the last deleted log entry.");
}

// Tools menu placeholder implementations
void MainWindow::onWKMode() {
    LOG_DEBUG("MainWindow", "WK Mode (Alt+A) - Re-initialize WinKeyer - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "WinKeyer re-initialization will be implemented in a future version.\n\n"
                           "This will re-initialize the WinKeyer for CW keying.");
}

void MainWindow::onSendMorse() {
    if (!m_radioConnected) {
        DialogHelper::warning(this, "Radio Not Connected",
            "Radio must be connected to send morse code.\n\n"
            "Please connect to your radio first.");
        return;
    }

    SendMorseDialog dialog(m_radio, this);
    dialog.exec();
}

void MainWindow::onEditCWMessages() {
    CWMessageEditorDialog dialog(m_radio, m_activeContest, this);
    dialog.exec();
    LOG_DEBUG("MainWindow", "CW Messages Editor closed");
}

void MainWindow::handleFunctionKey(int fKey, bool ctrlPressed, bool altPressed) {
    if (!m_cwMessageManager) {
        LOG_ERROR("MainWindow", "CWMessageManager is null");
        return;
    }

    // Build input context
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    // Delegate to CWMessageManager
    CWMessageManager::Result result = m_cwMessageManager->sendFunctionKey(fKey, ctrlPressed, altPressed, input);

    // Update status and last CW message
    if (!result.statusMessage.isEmpty()) {
        m_statusLabel->setText(result.statusMessage);
    }
    if (result.success && !result.cwTextSent.isEmpty()) {
        m_lastCWMessage = result.cwTextSent;
    }
}

void MainWindow::sendCWMessage(const QString& messageTemplate) {
    if (!m_cwMessageManager) {
        LOG_ERROR("MainWindow", "CWMessageManager is null");
        return;
    }

    // Build input context
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    // Delegate to CWMessageManager
    CWMessageManager::Result result = m_cwMessageManager->sendCWMessage(messageTemplate, input);

    // Update status and last CW message
    if (!result.statusMessage.isEmpty()) {
        m_statusLabel->setText(result.statusMessage);
    }
    if (result.success && !result.cwTextSent.isEmpty()) {
        m_lastCWMessage = result.cwTextSent;
    }
}

void MainWindow::onShowFunctionKeysRef() {
    if (!m_functionKeysWindow) {
        m_functionKeysWindow = new FunctionKeysWindow(this);
        m_functionKeysWindow->setWindowFlags(Qt::Window);
        m_functionKeysWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    }

    m_functionKeysWindow->show();
    m_functionKeysWindow->raise();
    m_functionKeysWindow->activateWindow();
}

void MainWindow::onBackupLog() {
    if (!m_hasActiveContest) {
        DialogHelper::warning(this, "No Active Contest",
            "No contest is currently active. Please open or create a contest first.");
        return;
    }

    BackupRestoreDialog dialog(m_currentContest, this);
    dialog.exec();
}

void MainWindow::onToggleWebServer() {
    if (m_webServer->isRunning()) {
        // Stop the server
        m_webServer->stop();
        m_webServerAction->setText("Start Web Server");
        m_statusLabel->setText("Web server stopped");
        LOG_INFO("MainWindow", "Web server stopped by user");
    } else {
        // Start the server with settings
        AppSettings& settings = AppSettings::instance();
        quint16 port = settings.getWebServerPort();
        QString addressStr = settings.getWebServerAddress();
        QHostAddress address(addressStr);

        if (m_webServer->start(port, address)) {
            m_webServerAction->setText("Stop Web Server");
            QString url = m_webServer->url();
            m_statusLabel->setText(QString("Web server started: %1").arg(url));
            LOG_INFO("MainWindow", QString("Web server started: %1").arg(url));

            // Show info dialog with URL
            DialogHelper::information(this, "Web Server Started",
                QString("Web server is now running at:\n\n%1\n\n"
                        "Access the dashboard from any browser on this computer.\n\n"
                        "To access from other devices, change the binding address in Preferences.")
                    .arg(url));
        } else {
            // Manual start failed - show error dialog
            m_statusLabel->setText("Failed to start web server");
            LOG_ERROR("MainWindow", QString("Failed to start web server on %1:%2").arg(addressStr).arg(port));

            DialogHelper::warning(this, "Web Server Start Failed",
                QString("Failed to start web server on %1:%2\n\n"
                        "Port may already be in use by another instance of TR4QT or another application.\n\n"
                        "Please check if TR4QT is already running, or try a different port in Preferences.")
                    .arg(addressStr).arg(port));
        }
    }
}

void MainWindow::onResetWindowPositions() {
    // Confirm with user
    QMessageBox::StandardButton reply = DialogHelper::question(
        this,
        "Reset Window Positions",
        "This will reset all window positions to their defaults.\n\n"
        "All windows will be repositioned near the main window.\n\n"
        "Continue?"
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    LOG_INFO("MainWindow", "Resetting all window positions to defaults");

    // Clear all saved geometries from QSettings
    QSettings settings("TR4QT", "TR4QT");
    settings.remove("Windows");
    settings.remove("MapViewer");

    // Reposition all currently open windows
    // Use cascade offset so windows don't completely overlap
    int offsetX = 100;
    int offsetY = 100;
    const int cascadeStep = 30;

    if (m_statisticsWindow && m_statisticsWindow->isVisible()) {
        m_statisticsWindow->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned Statistics window to (%1, %2)").arg(offsetX).arg(offsetY));
        offsetX += cascadeStep;
        offsetY += cascadeStep;
    }

    if (m_sectionsMapViewer && m_sectionsMapViewer->isVisible()) {
        m_sectionsMapViewer->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned Sections Map window to (%1, %2)").arg(offsetX).arg(offsetY));
        offsetX += cascadeStep;
        offsetY += cascadeStep;
    }

    if (m_statesMapViewer && m_statesMapViewer->isVisible()) {
        m_statesMapViewer->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned States Map window to (%1, %2)").arg(offsetX).arg(offsetY));
        offsetX += cascadeStep;
        offsetY += cascadeStep;
    }

    if (m_dxClusterWindow && m_dxClusterWindow->isVisible()) {
        m_dxClusterWindow->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned DX Cluster window to (%1, %2)").arg(offsetX).arg(offsetY));
        offsetX += cascadeStep;
        offsetY += cascadeStep;
    }

    if (m_bandMapWindow && m_bandMapWindow->isVisible()) {
        m_bandMapWindow->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned Band Map window to (%1, %2)").arg(offsetX).arg(offsetY));
        offsetX += cascadeStep;
        offsetY += cascadeStep;
    }

    if (m_radioControlWindow && m_radioControlWindow->isVisible()) {
        m_radioControlWindow->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned Radio Control window to (%1, %2)").arg(offsetX).arg(offsetY));
        offsetX += cascadeStep;
        offsetY += cascadeStep;
    }

    if (m_multiplierWindow && m_multiplierWindow->isVisible()) {
        m_multiplierWindow->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned Multiplier window to (%1, %2)").arg(offsetX).arg(offsetY));
    }

    setStatusMessage("Window positions reset to defaults");
}

void MainWindow::onCTYUpdateAvailable(int currentVersion, int latestVersion, const QString& versionString) {
    Q_UNUSED(versionString);

    LOG_INFO("MainWindow", QString("CTY.DAT update available: CTY-%1 (current: CTY-%2)")
        .arg(latestVersion).arg(currentVersion));

    // Store latest version for saving after download
    m_latestCTYVersion = latestVersion;

    // Show clickable status bar message
    QString message = QString("CTY.DAT update available: CTY-%1. Click Tools → Download CTY.DAT or press Alt+O to update.")
        .arg(latestVersion);

    statusBar()->showMessage(message);

    // Keep the message visible indefinitely (until user downloads or manually clears)
    // Don't use timeout - we want this to stay visible
}

void MainWindow::onDownloadCTY(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download CTY.dat (Alt+O) - Starting download (headless=%1)").arg(headless));
    
    CTYDownloadResult result = m_downloadManager->downloadCTY(headless);
    
    if (result.success) {
        m_statusLabel->setText(result.statusMessage);
        LOG_INFO("MainWindow", result.statusMessage);
    } else {
        m_statusLabel->setText(QString("CTY download failed: %1").arg(result.errorMessage));
        LOG_ERROR("MainWindow", QString("CTY download failed: %1").arg(result.errorMessage));
    }
}

void MainWindow::onDownloadLOTW(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download LOTW (Alt+L) - Starting download (headless=%1)").arg(headless));
    
    LOTWDownloadResult result = m_downloadManager->downloadLOTW(headless);
    
    if (result.success) {
        m_statusLabel->setText(result.statusMessage);
        LOG_INFO("MainWindow", result.statusMessage);
    } else {
        m_statusLabel->setText(QString("LOTW download failed: %1").arg(result.errorMessage));
        LOG_ERROR("MainWindow", QString("LOTW download failed: %1").arg(result.errorMessage));
    }
}

void MainWindow::onDownloadSCP(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download SCP (Alt+S) - Starting download (headless=%1)").arg(headless));
    
    SCPDownloadResult result = m_downloadManager->downloadSCP(headless);
    
    if (result.success) {
        // Reload SCP matcher with new data
        delete m_scpMatcher;
        m_scpMatcher = new SCPMatcher();
        
        m_statusLabel->setText(result.statusMessage);
        LOG_INFO("MainWindow", result.statusMessage);
    } else {
        m_statusLabel->setText(QString("SCP download failed: %1").arg(result.errorMessage));
        LOG_ERROR("MainWindow", QString("SCP download failed: %1").arg(result.errorMessage));
    }
}

void MainWindow::onInitialize() {
    LOG_DEBUG("MainWindow", "Initialize (Alt+W) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Initialize will be implemented in a future version.\n\n"
                           "This will initialize/reset contest parameters.");
}

// Operating menu placeholder implementations
void MainWindow::onAutoCQ() {
    LOG_DEBUG("MainWindow", "Auto CQ (Alt+Q) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Auto CQ will be implemented in a future version.\n\n"
                           "This will enable automatic CQ sending.");
}

void MainWindow::onAutoCQResume() {
    LOG_DEBUG("MainWindow", "Auto CQ Resume (Alt+C) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Auto CQ Resume will be implemented in a future version.\n\n"
                           "This will resume automatic CQ after an interruption.");
}

void MainWindow::onKillCW() {
    LOG_DEBUG("MainWindow", "Kill CW (Alt+K) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Kill CW will be implemented in a future version.\n\n"
                           "This will immediately stop CW transmission.");
}

void MainWindow::onDupeCheck() {
    LOG_DEBUG("MainWindow", "Dupe Check (Alt+D) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Dupe Check will be implemented in a future version.\n\n"
                           "This will check if the entered callsign is a duplicate.");
}

void MainWindow::onSearchLog() {
    LOG_DEBUG("MainWindow", "Search Log (Alt+L) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Search Log will be implemented in a future version.\n\n"
                           "This will search the log for a specific callsign.");
}

void MainWindow::onDeleteLastQSO() {
    LOG_DEBUG("MainWindow", "Delete Last QSO (Alt+Y) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Delete Last QSO will be implemented in a future version.\n\n"
                           "This will delete the most recent QSO from the log.");
}

void MainWindow::onIncNumber() {
    LOG_DEBUG("MainWindow", "Inc Number (Alt+I) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Inc Number will be implemented in a future version.\n\n"
                           "This will increment the serial number.");
}

void MainWindow::onInitialExchange() {
    LOG_DEBUG("MainWindow", "Initial Exchange (Alt+Z) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Initial Exchange will be implemented in a future version.\n\n"
                           "This will set/reset the initial exchange information.");
}

// Removed: CW Speed menu item (was Alt+S, conflicted with Download SCP)
// Use PgUp/PgDn shortcuts or click WPM label in Radio Control window instead

void MainWindow::onToggleSidetone() {
    LOG_DEBUG("MainWindow", "Toggle Sidetone (Alt+=) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Toggle Sidetone will be implemented in a future version.\n\n"
                           "This will turn CW sidetone on/off.");
}

void MainWindow::onToggleAutosend() {
    LOG_DEBUG("MainWindow", "Toggle Autosend (Alt+-) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Toggle Autosend will be implemented in a future version.\n\n"
                           "This will enable/disable automatic sending.");
}

// Band menu placeholder implementations
void MainWindow::onToggleRigs() {
    LOG_DEBUG("MainWindow", "Toggle Rigs (Alt+R) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
                           "Toggle Rigs will be implemented in a future version.\n\n"
                           "This will switch between radios in SO2R mode.");
}

void MainWindow::onEditSO2R() {
    LOG_DEBUG("MainWindow", "Edit SO2R (Alt+E) - Not yet implemented");
    DialogHelper::information(this, "Not Implemented",
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
                double qsxKHz = spot.qsx / 1000.0;
                LOG_DEBUG("MainWindow", QString("Parsed QSX fragment: %1 kHz on %2 MHz band = %3 kHz")
                    .arg(qsxValue).arg(spotMHz / 1000000).arg(qsxKHz, 0, 'f', 1));
            } else {
                // Full frequency in MHz (e.g., 14.210)
                spot.qsx = static_cast<freq_t>(qsxValue * 1000000);
                double qsxKHz = spot.qsx / 1000.0;
                LOG_DEBUG("MainWindow", QString("Parsed QSX full frequency: %1 MHz = %2 kHz")
                    .arg(qsxValue).arg(qsxKHz, 0, 'f', 1));
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
                double txKHz = spot.frequency / 1000.0;
                double rxKHz = spot.qsx / 1000.0;
                LOG_DEBUG("MainWindow", QString("Parsed UP offset: TX=%1 kHz + %2 kHz = RX=%3 kHz")
                    .arg(txKHz, 0, 'f', 1).arg(offsetKHz).arg(rxKHz, 0, 'f', 1));
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
        logMsg += QString(" | TX: %1 Hz (%2 MHz)").arg(QString::number(spot.frequency, 'f', 0)).arg(spot.frequency / 1000000.0, 0, 'f', 3);
        if (spot.qsx > 0) {
            logMsg += QString(" | RX (QSX): %1 Hz (%2 MHz)").arg(QString::number(spot.qsx, 'f', 0)).arg(spot.qsx / 1000000.0, 0, 'f', 3);
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
        // Radio connected: Send band change to radio
        LOG_DEBUG("MainWindow", QString("Band clicked: %1 Sending setBand command")
            .arg(bandToString(band)));
        m_radio->setBand(band);
    } else {
        // Radio not connected: Manual band selection
        if (!m_bandSwitchingManager) {
            LOG_ERROR("MainWindow", "BandSwitchingManager is null");
            return;
        }

        // Delegate to BandSwitchingManager
        m_bandSwitchingManager->selectBand(band, m_currentState, false);

        // Update current state (manager doesn't modify state directly)
        m_currentState.bandA = band;
        m_currentState.frequencyA = m_bandSwitchingManager->getFrequencyForBand(band, m_currentState.modeA);

        // Update radio status display
        updateRadioStatusGrid();

        // Emit signals for other components
        emit currentFrequencyChanged(m_currentState.frequencyA);
        emit currentBandChanged(band);

        // Update status
        m_statusLabel->setText(QString("Band: %1 (manual)").arg(bandToString(band)));
    }
}

void MainWindow::onBandUp() {
    if (!m_bandSwitchingManager) {
        LOG_ERROR("MainWindow", "BandSwitchingManager is null");
        return;
    }

    BandType currentBand = m_currentState.bandA;
    BandType nextBand = m_bandSwitchingManager->getNextBand(currentBand, m_activeContest);

    if (nextBand != currentBand) {
        LOG_DEBUG("MainWindow", QString("Band up: %1 -> %2")
            .arg(bandToString(currentBand))
            .arg(bandToString(nextBand)));
        onBandClicked(nextBand);
    } else {
        LOG_DEBUG("MainWindow", "Already at highest band");
    }
}

void MainWindow::onBandDown() {
    if (!m_bandSwitchingManager) {
        LOG_ERROR("MainWindow", "BandSwitchingManager is null");
        return;
    }

    BandType currentBand = m_currentState.bandA;
    BandType prevBand = m_bandSwitchingManager->getPreviousBand(currentBand, m_activeContest);

    if (prevBand != currentBand) {
        LOG_DEBUG("MainWindow", QString("Band down: %1 -> %2")
            .arg(bandToString(currentBand))
            .arg(bandToString(prevBand)));
        onBandClicked(prevBand);
    } else {
        LOG_DEBUG("MainWindow", "Already at lowest band");
    }
}

freq_t MainWindow::getFrequencyForBand(BandType band, ModeType mode) const {
    Q_UNUSED(mode);  // Not used - we return band edge for manual selection

    // Return low band edge as visual reminder this is manually set, not from radio
    // Real radio would show frequency within CW/SSB segments
    freq_t freq = BandConstants::bandToFrequency(band);

    // If invalid band, default to 20m
    if (freq == 0) {
        return BandConstants::BAND_20M_EDGE;
    }

    return freq;
}

BandType MainWindow::getNextBand(BandType currentBand) const {
    // Get allowed bands from active contest (or default HF bands)
    QList<BandType> allowedBands;
    if (m_activeContest) {
        allowedBands = m_activeContest->getAllowedBands();
    } else {
        // Default: standard HF contest bands
        allowedBands = { BandType::Band160M, BandType::Band80M, BandType::Band40M,
                         BandType::Band20M, BandType::Band15M, BandType::Band10M };
    }

    // Sort bands from low to high frequency (should already be sorted, but ensure)
    // Standard order: 160M, 80M, 40M, 20M, 15M, 10M, 6M, 2M, 70CM

    int currentIndex = allowedBands.indexOf(currentBand);
    if (currentIndex == -1 || currentIndex >= allowedBands.size() - 1) {
        return currentBand;  // Already at highest or invalid band
    }

    return allowedBands[currentIndex + 1];
}

BandType MainWindow::getPreviousBand(BandType currentBand) const {
    // Get allowed bands from active contest (or default HF bands)
    QList<BandType> allowedBands;
    if (m_activeContest) {
        allowedBands = m_activeContest->getAllowedBands();
    } else {
        // Default: standard HF contest bands
        allowedBands = { BandType::Band160M, BandType::Band80M, BandType::Band40M,
                         BandType::Band20M, BandType::Band15M, BandType::Band10M };
    }

    // Sort bands from low to high frequency (should already be sorted, but ensure)
    // Standard order: 160M, 80M, 40M, 20M, 15M, 10M, 6M, 2M, 70CM

    int currentIndex = allowedBands.indexOf(currentBand);
    if (currentIndex <= 0) {
        return currentBand;  // Already at lowest or invalid band
    }

    return allowedBands[currentIndex - 1];
}

void MainWindow::saveQSOTableColumnWidths() {
    if (!m_qsoTableView || !m_activeContest) {
        return;
    }

    QList<int> widths;
    for (int i = 0; i < QSOTableModel::ColCount; ++i) {
        widths.append(m_qsoTableView->columnWidth(i));
    }

    // Save per-contest using contest ID
    QString contestId = m_activeContest->getContestId();
    AppSettings::instance().saveQSOTableColumnWidths(contestId, widths);
}

void MainWindow::loadQSOTableColumnWidths() {
    if (!m_qsoTableView || !m_activeContest) {
        return;
    }

    // Load per-contest using contest ID
    QString contestId = m_activeContest->getContestId();
    QList<int> widths = AppSettings::instance().loadQSOTableColumnWidths(contestId);

    if (widths.isEmpty() || widths.size() != QSOTableModel::ColCount) {
        return;  // No saved widths or wrong number of columns
    }

    // Temporarily disconnect the resize signal to avoid saving during restore
    QHeaderView* header = m_qsoTableView->horizontalHeader();
    disconnect(header, &QHeaderView::sectionResized, this, &MainWindow::onQSOTableColumnResized);

    // Restore column widths (except last column which stretches)
    for (int i = 0; i < QSOTableModel::ColCount - 1; ++i) {
        if (widths[i] > 0) {
            m_qsoTableView->setColumnWidth(i, widths[i]);
        }
    }

    // Reconnect the resize signal
    connect(header, &QHeaderView::sectionResized, this, &MainWindow::onQSOTableColumnResized);
}

void MainWindow::onQSOTableColumnResized(int logicalIndex, int oldSize, int newSize) {
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    Q_UNUSED(logicalIndex);

    // Save all column widths when any column is resized
    saveQSOTableColumnWidths();
}

void MainWindow::updateRadioStatusFlash() {
    if (!m_radioFreqBandLabel) {
        return;
    }

    ThemeManager& theme = ThemeManager::instance();

    if (m_radioConnected) {
        // Connected - use normal theme colors
        QString normalStyle = QString("QLabel { background-color: %1; padding: 5px; border: 1px solid %2; border-radius: 3px; }")
            .arg(theme.color(ColorRole::TextDisplayBackground).name())
            .arg(theme.color(ColorRole::BorderColor).name());
        m_radioFreqBandLabel->setStyleSheet(normalStyle);
    } else {
        // Disconnected - flash between red and normal
        if (m_radioFlashState) {
            // Red flash
            QString flashStyle = QString("QLabel { background-color: #ff0000; color: #ffffff; padding: 5px; border: 2px solid #aa0000; border-radius: 3px; font-weight: bold; }");
            m_radioFreqBandLabel->setStyleSheet(flashStyle);
        } else {
            // Normal (but still show disconnected state)
            QString normalStyle = QString("QLabel { background-color: %1; padding: 5px; border: 1px solid #aa0000; border-radius: 3px; }")
                .arg(theme.color(ColorRole::TextDisplayBackground).name());
            m_radioFreqBandLabel->setStyleSheet(normalStyle);
        }
    }
}

// Operating mode management
void MainWindow::setOperatingMode(OperatingMode mode) {
    m_operatingMode = mode;

    // Update visual indicator
    if (m_operatingModeLabel) {
        if (mode == OperatingMode::CQ) {
            m_operatingModeLabel->setText("CQ");
            m_operatingModeLabel->setStyleSheet("color: green; font-weight: bold; padding: 0 10px;");
        } else {
            m_operatingModeLabel->setText("S&P");
            m_operatingModeLabel->setStyleSheet("color: blue; font-weight: bold; padding: 0 10px;");
        }
    }

    // Update UDP broadcast manager
    if (m_udpBroadcastManager) {
        m_udpBroadcastManager->setOperatingMode(mode == OperatingMode::CQ);
    }

    LOG_DEBUG("MainWindow", QString("Operating mode changed to: %1").arg(mode == OperatingMode::CQ ? "CQ" : "S&P"));
}

void MainWindow::onCQMode() {
    setOperatingMode(OperatingMode::CQ);
}

void MainWindow::onSPMode() {
    setOperatingMode(OperatingMode::SP);
}

void MainWindow::checkAutoSP(freq_t newFrequency) {
    // Only check if AUTO S&P is enabled
    if (!AppSettings::instance().getAutoSPEnable()) {
        return;
    }

    // Ignore if already in S&P mode
    if (m_operatingMode == OperatingMode::SP) {
        return;
    }

    // Need at least two frequency samples to calculate rate
    if (m_lastFrequency == 0) {
        m_lastFrequency = newFrequency;
        m_lastFrequencyTime = QDateTime::currentDateTime();
        return;
    }

    // Calculate time difference in seconds
    QDateTime now = QDateTime::currentDateTime();
    qint64 msecs = m_lastFrequencyTime.msecsTo(now);

    // Ignore if time difference is too small (avoid division by zero and spurious triggers)
    if (msecs < 100) {  // Less than 100ms
        return;
    }

    double seconds = msecs / 1000.0;

    // Calculate Hz/sec
    qint64 freqDelta = qAbs(static_cast<qint64>(newFrequency) - static_cast<qint64>(m_lastFrequency));
    double hzPerSec = freqDelta / seconds;

    // Get sensitivity threshold from settings
    int threshold = AppSettings::instance().getAutoSPSensitivity();

    // Switch to S&P if movement exceeds threshold
    if (hzPerSec >= threshold) {
        LOG_DEBUG("MainWindow", QString("AUTO S&P triggered: %.1f Hz/sec (threshold: %2 Hz/sec)")
            .arg(hzPerSec).arg(threshold));
        setOperatingMode(OperatingMode::SP);
    }

    // Update tracking
    m_lastFrequency = newFrequency;
    m_lastFrequencyTime = now;
}

// Substitute template placeholders in sent exchange with actual values from settings
QString MainWindow::substituteSentExchangeTemplate(const QString& templateStr, int serialNumber, const QString& rst) const {
    QString result = templateStr;
    AppSettings& settings = AppSettings::instance();

    // Replace placeholders with actual values
    result.replace("{RST}", rst);
    result.replace("{SERIAL}", QString::number(serialNumber));
    result.replace("{ZONE}", QString::number(settings.getMyCQZone()));
    result.replace("{CQZONE}", QString::number(settings.getMyCQZone()));
    result.replace("{ITUZONE}", QString::number(settings.getMyITUZone()));
    result.replace("{STATE}", settings.getMyState());
    result.replace("{COUNTY}", settings.getMyCounty());
    result.replace("{SECTION}", settings.getMyARRLSection());
    result.replace("{GRID}", settings.getMyGridSquare());

    return result.trimmed();
}

} // namespace TR4QT
