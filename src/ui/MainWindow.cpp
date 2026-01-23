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
#include "windows/AmplifierControlWindow.h"
#include "NativeMapViewer.h"
#include "../network/UdpBroadcastManager.h"
#include "../network/WebServer.h"
#include "../controllers/ImportExportManager.h"
#include "../controllers/CWMessageManager.h"
#include "../controllers/BandSwitchingManager.h"
#include "../amplifiers/AmplifierFactory.h"
#include "../rotator/RotatorFactory.h"
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
#include "../radio/K4Radio.h"
#include "../utils/PathManager.h"
#include "../data/Database.h"
#include "../data/QSORepository.h"
#include "../data/ContestRepository.h"
#include "../data/LOTWUserRepository.h"
#include "../data/BackupManager.h"
#include "../data/ExchangeMemoryRepository.h"
#include "../data/SCPRepository.h"
#include "../commands/CommandDispatcher.h"
#include "../contests/RSTValidator.h"
#include "../cw/CWTemplateEngine.h"
#include <QFile>
#include <QFileInfo>
#include <cmath>
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
    , m_worldMapViewer(nullptr)
    , m_graylineMapDialog(nullptr)
    , m_amplifierControlWindow(nullptr)
    , m_qsosThisHour(0)
    , m_qsosSinceLastIntegrityCheck(0)
    , m_hasActiveContest(false)
    , m_activeContest(nullptr)
    , m_currentContestDbId(-1)
    , m_nextSerialNumber(1)
    , m_qsoLogger(nullptr)
    , m_stationInfoService(nullptr)
    , m_scoreCalculationService(nullptr)
    , m_integrityManager(nullptr)
    , m_loggingCoordinator(nullptr)
    , m_loggingService(nullptr)
    , m_persistenceService(nullptr)
    , m_exchangeMemoryService(nullptr)
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
    , m_amplifierService(nullptr)
    , m_rotatorService(nullptr)
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

    // Create StationInfoService (Phase 7 extraction) - needs CountryFile
    m_stationInfoService = new StationInfoService(&m_countryFile);
    LOG_DEBUG("MainWindow", "StationInfoService created");

    // Create ScoreCalculationService (Phase 11 extraction)
    m_scoreCalculationService = new ScoreCalculationService();
    LOG_DEBUG("MainWindow", "ScoreCalculationService created");

    // Create QSOQueryService (Phase 13 extraction)
    m_qsoQueryService = new QSOQueryService();
    LOG_DEBUG("MainWindow", "QSOQueryService created");

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
    connect(m_downloadManager, &DownloadManager::ctyDownloadCompleted,
            this, &MainWindow::onCTYDownloadCompleted);
    m_radioManager = new RadioManager(this);
    m_radio = m_radioManager->radioController();  // Get RadioController from RadioManager
    m_bandSwitchingManager = new BandSwitchingManager(this);
    m_cwMessageManager = new CWMessageManager({m_radio, nullptr});  // Contest set later
    m_windowManager = new WindowManager(this);
    m_settingsManager = new SettingsManager();
    LOG_DEBUG("MainWindow", "Controllers and UI managers created");

    // Initialize hardware control services (amplifier and rotator)
    initializeHardwareServices();

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
    connect(m_radioManager, &RadioManager::maxPowerChanged,
            m_radioControlWindow, &RadioControlWidget::setMaxPower);

    // Fast frequency update from transceive mode (bypasses slow radioStateUpdated)
    connect(m_radioManager, &RadioManager::frequencyChanged,
            this, &MainWindow::onFastFrequencyUpdate);

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

    // Clean up logging services
    delete m_loggingService;
    delete m_loggingCoordinator;
    delete m_persistenceService;
    delete m_exchangeMemoryService;
    delete m_qsoLogger;
    delete m_integrityManager;
    delete m_contestService;
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
    config.onShowWorldMap = [this]() { onShowWorldMap(); };
    config.onShowGraylineMap = [this]() { onShowGraylineMap(); };
    config.onShowAmplifierControl = [this]() { onShowAmplifierControl(); };
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
    m_worldMapAction = m_menuManager->worldMapAction();
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
    int bandWidth = fm.horizontalAdvance("160m SSB") + COLUMN_PADDING;                          // Longest band/mode combo
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

void MainWindow::initializeHardwareServices() {
    AppSettings& settings = AppSettings::instance();

    // Initialize amplifier service if enabled
    if (settings.getAmplifierEnabled()) {
        int modelId = settings.getAmplifierModel();
        QString connectionType = settings.getAmplifierConnectionType();
        QString port = settings.getAmplifierPort();
        int baudRate = settings.getAmplifierBaudRate();

        AmplifierConfig config;
        config.hamlibModelId = modelId;
        config.connectionType = connectionType;
        config.port = port;
        config.baudRate = baudRate;

        // Determine amplifier type
        AmplifierFactory::AmplifierType type;
        const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
        if (connectionType == "direct" && modelId == AMP_MODEL_ELECRAFT_KPA1500) {
            type = AmplifierFactory::AmplifierType::KPA1500_DIRECT;
        } else {
            type = AmplifierFactory::AmplifierType::HAMLIB;
        }

        // Create amplifier controller
        IAmplifierController* amplifierController = AmplifierFactory::createAmplifier(type, config, this);

        if (amplifierController) {
            // Create amplifier service
            m_amplifierService = new AmplifierService(amplifierController, this);

            // Auto-connect if enabled
            if (settings.getAmplifierAutoConnect()) {
                bool connected = m_amplifierService->connectToAmplifier(config);
                if (connected) {
                    LOG_INFO("MainWindow", "Amplifier auto-connected successfully");
                } else {
                    LOG_WARN("MainWindow", "Amplifier auto-connect failed");
                }
            }

            LOG_DEBUG("MainWindow", "Amplifier service initialized");
        } else {
            LOG_ERROR("MainWindow", "Failed to create amplifier controller");
        }
    }

    // Initialize rotator service if enabled
    if (settings.getRotatorEnabled()) {
        int modelId = settings.getRotatorModel();
        QString connectionType = settings.getRotatorConnectionType();

        RotatorConfig config;

        if (connectionType == "direct") {
            config.ipAddress = settings.getRotatorIpAddress();
            config.port = settings.getRotatorPort();
        } else {
            config.serialPort = settings.getRotatorSerialPort();
            config.baudRate = settings.getRotatorBaudRate();
        }

        // Determine rotator type
        RotatorFactory::RotatorType type;
        const int ROT_MODEL_PSTROTATOR = 9999;
        if (connectionType == "direct" && modelId == ROT_MODEL_PSTROTATOR) {
            type = RotatorFactory::RotatorType::PSTROTATOR;
            config.rotatorType = 0;  // PSTRotator
        } else {
            type = RotatorFactory::RotatorType::HAMLIB;
            config.rotatorType = modelId;  // Hamlib model ID
        }

        // Create rotator controller
        IRotatorController* rotatorController = RotatorFactory::createRotator(type, config, this);

        if (rotatorController) {
            // Create rotator service
            m_rotatorService = new RotatorService(rotatorController, this);

            // Auto-connect if enabled
            if (settings.getRotatorAutoConnect()) {
                bool connected = rotatorController->connect(config);
                if (connected) {
                    LOG_INFO("MainWindow", "Rotator auto-connected successfully");
                } else {
                    LOG_WARN("MainWindow", "Rotator auto-connect failed");
                }
            }

            LOG_DEBUG("MainWindow", "Rotator service initialized");
        } else {
            LOG_ERROR("MainWindow", "Failed to create rotator controller");
        }
    }
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

    // Defer child window restoration until event loop is running
    // Qt forum advice: restoreGeometry works better after show() and event loop start
    // Using QTimer::singleShot(0, ...) defers execution to after event loop starts
    QTimer::singleShot(0, this, [this, geometry]() {
        restoreChildWindows(geometry);
    });
}

void MainWindow::restoreChildWindows(const WindowGeometry& geometry) {
    LOG_DEBUG("MainWindow", "Restoring child windows (deferred to event loop)");

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

    if (geometry.worldMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring World Map window (was visible on exit)");
        onShowWorldMap();
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring World Map window (was hidden on exit)");
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

    if (geometry.amplifierControlVisible) {
        LOG_DEBUG("MainWindow", "Restoring Amplifier Control window (was visible on exit)");
        onShowAmplifierControl();
        if (m_amplifierControlWindow && !geometry.amplifierControlGeometry.isEmpty()) {
            m_amplifierControlWindow->restoreGeometry(geometry.amplifierControlGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Amplifier Control window (was hidden on exit)");
    }
}

void MainWindow::saveSettings() {
    if (!m_settingsManager) {
        return;
    }

    LOG_DEBUG("MainWindow", "saveSettings() called - checking window visibility");
    if (m_amplifierControlWindow) {
        LOG_DEBUG("MainWindow", QString("At start of saveSettings: amplifier window exists, isVisible=%1").arg(m_amplifierControlWindow->isVisible()));
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
    }
    // Use tracked visibility (Qt's isVisible() can return false during SIGTERM shutdown)
    geometry.radioControlVisible = m_radioControlWindowVisible;
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
    if (m_worldMapViewer) {
        geometry.worldMapVisible = m_worldMapViewer->isVisible();
    }
    if (m_graylineMapDialog) {
        geometry.graylineMapGeometry = m_graylineMapDialog->saveGeometry();
        geometry.graylineMapVisible = m_graylineMapDialog->isVisible();
    }
    if (m_amplifierControlWindow) {
        geometry.amplifierControlGeometry = m_amplifierControlWindow->saveGeometry();
    }
    // Use tracked visibility (Qt's isVisible() can return false during SIGTERM shutdown)
    geometry.amplifierControlVisible = m_amplifierControlWindowVisible;
    LOG_DEBUG("MainWindow", QString("Amplifier window tracked visibility: %1").arg(m_amplifierControlWindowVisible));

    // Debug logging for window visibility
    LOG_DEBUG("MainWindow", QString("Saving window visibility - DXCluster:%1 BandMap:%2 RadioCtrl:%3 Mult:%4 Stats:%5 Sections:%6 States:%7 Grayline:%8 AmpCtrl:%9")
        .arg(geometry.dxClusterVisible)
        .arg(geometry.bandMapVisible)
        .arg(geometry.radioControlVisible)
        .arg(geometry.multipliersVisible)
        .arg(geometry.statisticsVisible)
        .arg(geometry.sectionsMapVisible)
        .arg(geometry.statesMapVisible)
        .arg(geometry.graylineMapVisible)
        .arg(geometry.amplifierControlVisible));

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

    // Disconnect radio before closing and wait for completion
    // CRITICAL: Must allow disconnect packets to be sent before app exits
    // Without this, Icom network radios stay in "connected" state and refuse reconnection
    if (m_radioConnected) {
        m_radio->disconnectFromRadio();

        // Give disconnect time to complete (sends CI-V close + control disconnect packets)
        // RadioController destructor will ensure full cleanup, but we need event loop
        // to process the queued disconnect operation before QApplication::quit()
        QApplication::processEvents();  // Process queued disconnect
        QThread::msleep(100);           // Allow UDP packets to be sent
        LOG_DEBUG("MainWindow", "Radio disconnect completed before exit");
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
#ifdef Q_OS_MAC
    // macOS: Bring all windows to front when app is activated (if setting enabled)
    if (event->type() == QEvent::ApplicationActivate) {
        if (AppSettings::instance().getShowAllWindowsOnActivate()) {
            LOG_DEBUG("MainWindow", "ApplicationActivate received - raising all windows");
            raiseAllWindows();
        }
    }
#endif

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

    // Radio Control window show/hide: Toggle detailed rig info (S-meter, temperature)
    if (obj == m_radioControlWindow) {
        if (event->type() == QEvent::Show) {
            // Window shown - enable detailed rig info
            if (m_radio && m_radioConnected) {
                if (K4Radio* k4 = qobject_cast<K4Radio*>(m_radio)) {
                    k4->setDetailedRigInfoEnabled(true);
                }
            }
        } else if (event->type() == QEvent::Hide) {
            // Window hidden - disable detailed rig info to reduce polling
            if (m_radio && m_radioConnected) {
                if (K4Radio* k4 = qobject_cast<K4Radio*>(m_radio)) {
                    k4->setDetailedRigInfoEnabled(false);
                }
            }
        }
    }

    // Pass event to base class
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::raiseAllWindows() {
    // Raise ALL top-level windows belonging to this application
    // This is more robust than tracking individual windows, as windows
    // may be created lazily and WindowManager config may be stale
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget* widget : topLevelWidgets) {
        if (widget && widget->isVisible() && !widget->isMinimized()) {
            widget->raise();
        }
    }

    // Ensure main window is on top and ACTIVE (for macOS menu bar)
    // Without activateWindow(), a child window may be "active" and
    // macOS will show that window's (empty) menu bar instead of ours
    raise();
    activateWindow();
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
        if (!settings.hasRadioProfiles()) {
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
    LogExportService service;
    service.exportLogsForSupport(this, m_radio ? m_radio->isConnected() : false);
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

void MainWindow::onFastFrequencyUpdate(freq_t freq) {
    // TIMING: Measure MainWindow's frequency display update latency
    static QElapsedTimer uiTimer;
    static bool uiTimerStarted = false;
    if (!uiTimerStarted) {
        uiTimer.start();
        uiTimerStarted = true;
    }
    qint64 uiStart = uiTimer.nsecsElapsed();

    // Fast path: Update only frequency display for instant transceive updates
    // Skip all the heavy processing (UDP broadcast, privilege checks, AUTO S&P, etc.)
    m_currentState.frequencyA = freq;
    m_currentState.bandA = frequencyToBand(freq);

    // Update VFO display immediately (3 decimal places for compact radio grid)
    // Use floor() instead of rounding to show actual band position
    // Example: 28.318644 displays as 28.318 (not 28.319)
    double freqMHz = freq / 1000000.0;
    double truncated = std::floor(freqMHz * 1000.0) / 1000.0;  // Truncate to 3 decimals
    m_radioFreqLabel->setText(QString("%1 MHz").arg(truncated, 0, 'f', 3));

    // Update band/mode label
    if (m_currentState.bandA != BandType::None) {
        QString bandStr = bandToString(m_currentState.bandA);
        QString modeStr = modeToString(m_currentState.modeA);
        m_radioFreqBandLabel->setText(QString("%1 %2").arg(bandStr).arg(modeStr));
    }

    qint64 uiEnd = uiTimer.nsecsElapsed();
    LOG_DEBUG("MainWindow", QString("VFO display updated: %1 MHz [ui=%2μs]")
        .arg(freqMHz, 0, 'f', 3).arg((uiEnd - uiStart) / 1000));
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

    // Step 1: Check for commands (OPON, UDP)
    if (handleLogQSOCommand(callsign)) {
        return;  // Command was handled
    }

    // Step 2: Verify service is initialized
    if (!m_loggingService) {
        m_statusLabel->setText("Error: No active contest - open a contest first");
        QApplication::beep();
        return;
    }

    // Step 3: Build request and execute logging workflow
    QSOLoggingService::LogQSORequest request = buildLogQSORequest(callsign, exchange);
    QSOLoggingService::LogQSOResult result = m_loggingService->logQSO(request);

    // Step 4: Handle validation errors
    if (!result.success) {
        handleLogQSOValidationError(result);
        return;
    }

    // Step 5: Update UI after successful logging
    updateUIAfterQSOLogged(result.qso, result);
}

bool MainWindow::handleLogQSOCommand(const QString& callsign) {
    CommandDispatcher::CommandResult cmd = CommandDispatcher::parseCommand(callsign);

    if (!cmd.wasCommand) {
        return false;
    }

    if (cmd.type == CommandDispatcher::ChangeOperator) {
        OperatorDialog dialog(this);
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
        return true;
    }

    if (cmd.type == CommandDispatcher::RebroadcastLog) {
        onRebroadcastLog();
        onClearEntry();
        return true;
    }

    return false;
}

QSOLoggingService::LogQSORequest MainWindow::buildLogQSORequest(const QString& callsign, const QString& exchange) {
    QSOLoggingService::LogQSORequest request;

    // Basic QSO data
    request.callsign = callsign;
    request.exchange = exchange;
    request.radioState = m_currentState;
    request.operatorCallsign = AppSettings::instance().getCurrentOperator();
    request.serialNumber = m_nextSerialNumber;
    request.operatingMode = m_operatingMode;

    // Existing QSOs for duplicate/multiplier checking
    request.existingQSOs = m_qsoTableModel->getAllQSOs();

    // Exchange memory settings
    request.saveExchangeMemory = true;
    request.autoPopulated = m_initialExchangePopulated;

    // Context for post-logging actions
    request.stationCallsign = AppSettings::instance().getMyCallsign();
    request.adifContestId = m_activeContest ? m_activeContest->getADIFContestId() : "";
    request.wa7bnmContestId = m_activeContest ? m_activeContest->getWA7BNMContestId() : 0;
    request.contestId = m_activeContest ? m_activeContest->getContestId() : "";
    request.databasePath = m_currentContest.databasePath;
    request.totalQSOCount = m_qsoTableModel->count() + 1;
    request.qsosSinceLastCheck = m_qsosSinceLastIntegrityCheck + 1;
    request.contestDbId = m_currentContestDbId;
    request.memoryQSOCount = m_qsoTableModel->count() + 1;

    return request;
}

void MainWindow::handleLogQSOValidationError(const QSOLoggingService::LogQSOResult& result) {
    m_statusLabel->setText(result.errorMessage);
    m_statusLabel->setStyleSheet("QLabel { color: #ff0000; font-weight: bold; }");
    QApplication::beep();

    // Set focus to appropriate field
    if (result.errorMessage.contains("Callsign")) {
        m_callsignEntry->setFocus();
    } else if (result.errorMessage.contains("Exchange")) {
        m_exchangeEntry->setFocus();
    }
}

void MainWindow::updateUIAfterQSOLogged(const QSO& qso, const QSOLoggingService::LogQSOResult& result) {
    // Update serial number
    m_nextSerialNumber = result.updatedSerialNumber;

    // Log duplicate info if present
    if (result.isDuplicate) {
        LOG_INFO("MainWindow", QString("Duplicate QSO detected: %1 - %2").arg(qso.callsign, result.dupeInfo));
    }

    // Add to table model
    m_qsoTableModel->addQSO(qso);
    updateScoreDisplay();

    // Update multiplier window
    if (m_multiplierWindow && m_activeContest) {
        QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();
        if (!multDefs.isEmpty()) {
            MultiplierType primaryMultType = multDefs.first().type;
            QString multValue = m_activeContest->getMultiplierValue(qso, primaryMultType, QStringList());
            if (!multValue.isEmpty()) {
                m_multiplierWindow->setMultiplierWorked(multValue, qso.band);
            }
        }
    }

    // Scroll to show newly logged QSO
    m_qsoTableView->scrollToBottom();

    // Handle persistence result
    if (result.persistenceResult.status == QSOPersistenceService::SaveResult::SavedToDatabase) {
        LOG_DEBUG("MainWindow", QString("QSO saved to database with ID: %1").arg(qso.id));
        m_qsoTableModel->updateQSO(m_qsoTableModel->count() - 1, qso);
    }
    else if (result.persistenceResult.status == QSOPersistenceService::SaveResult::SavedToEmergencyFile) {
        DialogHelper::information(this, "QSO Saved to Emergency File",
            QString("Database save failed. QSO saved to emergency file:\n%1\n\n"
                    "You can import this file later using File → Import ADIF")
            .arg(result.persistenceResult.emergencyFilePath));
    }
    else if (result.persistenceResult.status == QSOPersistenceService::SaveResult::Failed) {
        DialogHelper::critical(this, "QSO Save Failed",
            "Could not save QSO to database or emergency file!\n\n"
            "The QSO is only in memory and will be lost if TR4QT crashes.");
    }
    else if (result.persistenceResult.status == QSOPersistenceService::SaveResult::NeedsUserDecision) {
        DialogHelper::warning(this, "QSO Save Issue",
            QString("QSO save needs attention:\n%1").arg(result.persistenceResult.errorMessage));
    }

    // Update last QSO time
    m_lastQSOTime = qso.timestamp;

    // Update status message
    QString statusMsg = QString("Logged: %1 on %2 %3")
        .arg(qso.callsign)
        .arg(bandToString(qso.band))
        .arg(modeToString(qso.mode));

    if (!result.postLoggingActions.isEmpty()) {
        statusMsg += " | " + result.postLoggingActions.join(", ");
    }

    m_statusLabel->setText(statusMsg);
    m_statusLabel->setStyleSheet("");

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

    // Clear entry fields and update displays
    onClearEntry();
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
            // Delegate SCP formatting to StationInfoService (Phase 7 extraction)
            QSet<QString> workedCallsigns = getWorkedCallsigns();

            // Create dupe checker callback that uses MainWindow's checkForDuplicate
            auto dupeChecker = [this](const QString& call, BandType band, ModeType mode, QString& dupeInfo) {
                return checkForDuplicate(call, band, mode, dupeInfo);
            };

            SCPDisplayResult scpResult = m_stationInfoService->formatSCPMatches(
                matches, workedCallsigns, m_currentState.bandA, m_currentState.modeA, dupeChecker);

            m_scpMatchesLabel->setTextFormat(Qt::RichText);
            m_scpMatchesLabel->setText(scpResult.htmlContent);
            m_scpMatchesLabel->show();
            LOG_DEBUG("MainWindow", QString("SCP: displaying %1 matches (%2 worked, %3 dupes)")
                .arg(scpResult.totalMatches).arg(scpResult.workedCount).arg(scpResult.dupeCount));
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
    // Delegate to StationInfoService (Phase 7 extraction)
    AppSettings& settings = AppSettings::instance();
    QString myGrid = settings.getMyGridSquare();
    bool useMetric = settings.getUseMetricDistance();

    StationInfoResult result = m_stationInfoService->calculateStationInfo(
        callsign, myGrid, useMetric);

    // Update UI with results
    if (!result.valid) {
        m_countryNameLabel->setText("");
        m_stationInfoLabel->setText("");
        return;
    }

    m_countryNameLabel->setText(result.countryName);
    m_stationInfoLabel->setTextFormat(Qt::RichText);
    m_stationInfoLabel->setText(result.displayInfo);
    m_stationInfoLabel->setToolTip(result.tooltip);

    // Update grayline map if it's open (UI-specific, stays in MainWindow)
    if (m_graylineMapDialog && m_graylineMapDialog->isVisible() && !m_graylineMapDialog->isFrozen()) {
        double myLat, myLon;
        if (GeographicUtils::gridToLatLon(myGrid, myLat, myLon)) {
            QString myCallsign = settings.getMyCallsign();
            m_graylineMapDialog->updateStations(myCallsign, myLat, myLon,
                                               callsign, result.targetLat, result.targetLon);
        }
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

    // Check for numeric frequency entry using FrequencyInputService
    FrequencyInputService freqService;
    FrequencyInputResult freqResult = freqService.parseFrequencyInput(callsign, m_currentState.bandA);

    if (!freqResult.errorMessage.isEmpty()) {
        m_statusLabel->setText("Error: " + freqResult.errorMessage);
        onClearEntry();
        return;
    }

    if (freqResult.isFrequency) {
        // Set radio frequency
        if (m_radio && m_radioConnected) {
            m_radio->setFrequency(freqResult.frequencyHz);
            m_statusLabel->setText(freqResult.statusMessage);
            LOG_INFO("MainWindow", QString("Frequency changed via numeric entry: %1").arg(freqResult.statusMessage));
        } else {
            m_statusLabel->setText("Error: Radio not connected");
        }

        // Clear entry and return (don't process as callsign)
        onClearEntry();
        return;
    }

    // Check for special commands (OPON, UDP, etc.) - delegate to onLogQSO
    // Single source of truth: CommandDispatcher knows all valid commands
    if (CommandDispatcher::parseCommand(callsign).wasCommand) {
        onLogQSO();
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
            rebuildMultiplierWindow();

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

    // Get thread-safe copy of all QSOs
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();

    // Delegate calculation to ScoreCalculationService (Phase 11 extraction)
    ScoreResult result = m_scoreCalculationService->calculateScore(qsos, m_activeContest);

    // Update band summary grid with calculated values
    QList<BandType> bands = ScoreCalculationService::getStandardBands();

    if (result.usesModeGroupBreakdown) {
        // Update mode group QSO counts per band
        QList<ModeGroup> modeGroups = {ModeGroup::Phone, ModeGroup::CW, ModeGroup::Digital};
        for (ModeGroup group : modeGroups) {
            for (BandType band : bands) {
                int count = result.modeGroupStats.value(group).qsosPerBand.value(band, 0);
                m_bandSummaryGrid->setModeGroupQSOCount(band, group, count);
            }
            m_bandSummaryGrid->setAllModeGroupQSOs(group, result.modeGroupStats.value(group).totalQSOs);
        }
    } else {
        // Single-mode: Update regular QSO counts
        for (BandType band : bands) {
            m_bandSummaryGrid->setQSOCount(band, result.bandStats.value(band).qsoCount);
        }
        m_bandSummaryGrid->setAllQSOs(result.totalQSOs);
    }

    // Update points, mults, and zones
    for (BandType band : bands) {
        const BandStatistics& stats = result.bandStats.value(band);
        m_bandSummaryGrid->setPointsCount(band, stats.points);
        m_bandSummaryGrid->setMultCount(band, stats.multipliers);
        m_bandSummaryGrid->setZoneCount(band, stats.zones);
    }

    // Update "All" column totals
    if (!result.usesModeGroupBreakdown) {
        m_bandSummaryGrid->setAllQSOs(result.totalQSOs);
    }
    m_bandSummaryGrid->setAllMults(result.totalMultipliers);
    m_bandSummaryGrid->setAllZones(result.totalZones);
    m_bandSummaryGrid->setAllPoints(result.totalQSOPoints);
    m_bandSummaryGrid->setFinalScore(result.finalScore);

    // Update status bar
    m_statusLabel->setText(QString("%1 QSOs, %2 Points").arg(result.totalQSOs).arg(result.finalScore));
}

void MainWindow::recalculateAllPoints() {
    if (!m_activeContest || !m_qsoTableModel || !m_integrityManager) {
        LOG_WARN("MainWindow", "Cannot recalculate points - no active contest, table model, or integrity manager");
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

    // Get all QSOs from model
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();

    // Delegate rescoring to DataIntegrityManager (no business logic loops in UI!)
    RescoreStats stats = m_integrityManager->rescoreContestSilent(qsos, m_activeContest, myStation);

    // Update table model with rescored QSOs
    for (int row = 0; row < qsos.size(); ++row) {
        m_qsoTableModel->updateQSO(row, qsos[row]);
    }

    LOG_INFO("MainWindow", QString("Recalculated points for %1 QSOs").arg(stats.qsosUpdated));

    // Update display
    updateScoreDisplay();

    // Show result to user
    m_statusLabel->setText(QString("Recalculated points for %1 QSOs").arg(stats.qsosUpdated));
}

void MainWindow::rebuildMultiplierWindow() {
    if (!m_multiplierWindow || !m_activeContest) {
        return;
    }

    m_multiplierWindow->clear();

    QList<MultiplierDefinition> multDefs = m_activeContest->getMultiplierTypes();
    if (multDefs.isEmpty()) {
        return;
    }

    MultiplierType primaryMultType = multDefs.first().type;
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();

    for (const QSO& qso : qsos) {
        QString multValue = m_activeContest->getMultiplierValue(qso, primaryMultType, QStringList());
        if (!multValue.isEmpty()) {
            m_multiplierWindow->setMultiplierWorked(multValue, qso.band);
        }
    }

    LOG_DEBUG("MainWindow", QString("Rebuilt multiplier window with %1 QSOs").arg(qsos.size()));
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

    // Delegate to DataIntegrityManager (no SQL in UI!)
    int memoryCount = m_qsoTableModel->count();
    DataIntegrityManager::QuickCheckResult result = m_integrityManager->quickIntegrityCheck(memoryCount);

    if (!result.passed) {
        // Use counts returned by manager (no need to re-query!)
        handleIntegrityMismatch(result.memoryCount, result.dbCount);
    }

    return result.passed;
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

    // Get thread-safe copy of all QSOs
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();

    // Delegate to DataIntegrityManager for rescoring
    stats = m_integrityManager->rescoreContestSilent(qsos, m_activeContest, myStation);

    // Update table model with rescored QSOs
    for (int row = 0; row < qsos.size(); ++row) {
        m_qsoTableModel->updateQSO(row, qsos[row]);
    }

    // Refresh display
    updateScoreDisplay();

    // Rebuild multiplier window from all QSOs
    rebuildMultiplierWindow();

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
    if (!m_hasActiveContest || !m_qsoTableModel || !m_integrityManager) {
        DialogHelper::information(this, "Integrity Check",
            "No active contest to validate.");
        return;
    }

    m_statusLabel->setText("Running full integrity check...");
    QApplication::processEvents();  // Update UI

    // Delegate to DataIntegrityManager (no SQL in UI!)
    QList<QSO> memoryQSOs = m_qsoTableModel->getAllQSOs();
    QString report = m_integrityManager->fullIntegrityCheck(memoryQSOs, false);  // Show all checks

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

    // THREAD SAFETY: Get copy of all QSOs BEFORE entering thread
    // This prevents race conditions if main thread modifies model during rebroadcast
    QList<QSO> qsosCopy = m_qsoTableModel->getAllQSOs();
    QString stationCall = AppSettings::instance().getMyCallsign();
    QString adifContestId = m_activeContest ? m_activeContest->getADIFContestId() : "";
    int wa7bnmContestId = m_activeContest ? m_activeContest->getWA7BNMContestId() : 0;

    // Run in separate thread to avoid blocking UI
    auto future = QtConcurrent::run([this, qsosCopy, stationCall, adifContestId, wa7bnmContestId, totalQSOs]() {
        int quarter = qMax(1, totalQSOs / 4);  // For progress updates
        int sent = 0;

        for (const QSO& qso : qsosCopy) {
            // Broadcast this QSO
            m_udpBroadcastManager->onQSOLogged(qso, stationCall, adifContestId, wa7bnmContestId);
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

    // Calculate QSOs this hour and rate - delegate to QSOQueryService (Phase 13)
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime hourStart = QDateTime(now.date(), QTime(now.time().hour(), 0), QTimeZone::utc());

    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();

    // Delegate calculations to service
    const int RATE_LOOKBACK_COUNT = 10;
    m_qsosThisHour = m_qsoQueryService->countQSOsInTimeWindow(qsos, hourStart, now);
    int rate = m_qsoQueryService->calculateRate(qsos, RATE_LOOKBACK_COUNT);

    // Update labels
    m_thisHrLabel->setText(QString("This Hr = %1").arg(m_qsosThisHour));
    m_rateLabel->setText(QString("Rate = %1").arg(rate));

    // Update radio status grid (date/time updates every second)
    updateRadioStatusGrid();

    // Update window menu checkmarks
    updateWindowMenuCheckmarks();
}

void MainWindow::updateRadioStatusGrid() {
    // Update band/mode (e.g., "20m SSB") and frequency
    // Display frequency and mode even when band is unknown (e.g., outside amateur bands)
    if (m_currentState.frequencyA > 0) {
        // Show band+mode if band is known (e.g., "20m SSB"), otherwise just mode (e.g., "SSB")
        if (m_currentState.bandA != BandType::None) {
            QString bandStr = bandToString(m_currentState.bandA);  // ADIF format: "20m", "70cm", etc.
            QString modeStr = modeToString(m_currentState.modeA);
            m_radioFreqBandLabel->setText(QString("%1 %2").arg(bandStr).arg(modeStr));
        } else {
            // Unknown band (e.g., outside amateur bands) - just show mode
            QString modeStr = modeToString(m_currentState.modeA);
            m_radioFreqBandLabel->setText(modeStr);
        }

        // Update frequency (in MHz with 3 decimal places for compact radio grid)
        // Use floor() instead of rounding to show actual band position
        double freqMHz = m_currentState.frequencyA / 1000000.0;
        double truncated = std::floor(freqMHz * 1000.0) / 1000.0;  // Truncate to 3 decimals
        m_radioFreqLabel->setText(QString("%1 MHz").arg(truncated, 0, 'f', 3));
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
    // Skip if menu manager not initialized
    if (!m_menuManager) {
        return;
    }

    // Update checkmarks in Window menu to reflect which windows are currently open
    // Check each action pointer for null before using (actions can be destroyed during shutdown)
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
    if (m_worldMapAction) {
        m_worldMapAction->setChecked(m_worldMapViewer && m_worldMapViewer->isVisible());
    }
    if (m_graylineMapAction) {
        m_graylineMapAction->setChecked(m_graylineMapDialog && m_graylineMapDialog->isVisible());
    }

    // Amplifier control action
    QAction* ampAction = m_menuManager->amplifierControlAction();
    if (ampAction) {
        ampAction->setChecked(m_amplifierControlWindow && m_amplifierControlWindow->isVisible());
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

    // Read contest info using ContestRepository (no SQL in UI!)
    ContestRepository repo;
    ContestRecord record = repo.findFirst();
    if (!record.isValid()) {
        LOG_WARN("MainWindow", QString("Last contest database has no contest record: %1").arg(repo.lastError()));
        db.close();
        return;
    }

    // Convert ContestRecord to ContestInfo
    ContestInfo contestInfo;
    contestInfo.contestId = record.contestId;
    contestInfo.contestName = record.contestName;
    contestInfo.startDate = record.startTime;
    contestInfo.contestType = record.contestType;
    contestInfo.databasePath = lastContestPath;
    contestInfo.isExisting = true;

    // Determine mode from contest_id (for backward compatibility with CW/SSB specific contests)
    // This is only used for display and mode restrictions
    if (record.contestId.contains("_CW")) {
        contestInfo.mode = "CW";
    } else if (record.contestId.contains("_SSB")) {
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
    // Step 1: Reset state and cleanup previous contest
    resetContestState();

    // Step 2: Delegate contest activation to ContestManager
    if (!m_contestManager) {
        DialogHelper::critical(this, "Configuration Error",
            "ContestManager not initialized. Cannot activate contest.");
        return;
    }

    ActivateContestResult result = m_contestManager->activateContest(contestInfo);

    if (!result.success) {
        DialogHelper::critical(this, "Contest Activation Error", result.errorMessage);
        return;
    }

    // Step 3: Update MainWindow state from activation result
    m_activeContest = result.contest;
    m_currentContestDbId = result.contestDbId;
    m_nextSerialNumber = result.nextSerialNumber;
    m_currentContest = contestInfo;

    // Update contest config from database (may differ from dialog for resumed contests)
    m_currentContest.category = result.category;
    m_currentContest.powerClass = result.powerClass;
    m_currentContest.assisted = result.assisted;
    m_currentContest.operatorName = result.operatorName;

    m_hasActiveContest = true;

    // Step 4: Load existing QSOs into table model
    m_qsoTableModel->clear();
    m_bandSummaryGrid->clearAll();
    if (m_multiplierWindow) {
        m_multiplierWindow->clear();
    }

    for (const QSO& qso : result.loadedQSOs) {
        m_qsoTableModel->addQSO(qso);
    }
    LOG_DEBUG("MainWindow", QString("Loaded %1 existing QSOs").arg(result.loadedQSOs.size()));

    updateScoreDisplay();

    if (!result.loadedQSOs.isEmpty()) {
        m_qsoTableView->scrollToBottom();
        m_qsoTableView->selectRow(m_qsoTableModel->rowCount() - 1);
    }

    // Step 5: Create services for this contest
    createContestServices(result);

    // Step 6: Configure UI components for this contest
    configureUIForContest(result);

    // Step 7: Update window title
    setWindowTitle(QString("%1 v%2 - %3 (%4)")
        .arg(APP_NAME)
        .arg(APP_VERSION)
        .arg(contestInfo.contestName)
        .arg(m_activeContest->getContestName()));

    // Step 8: Set default band/mode if radio not connected
    if (!m_radioConnected) {
        setDefaultBandModeForContest(contestInfo);
    }

    // Step 9: Final setup - recalculate points and rebuild multipliers
    if (m_qsoTableModel->count() > 0) {
        recalculateAllPoints();
    }
    rebuildMultiplierWindow();
    updateExchangeFieldsForContest();

    // Save as last opened contest
    AppSettings::instance().setLastContestPath(contestInfo.databasePath);
}

void MainWindow::resetContestState() {
    // Reset state flags first (prevents corrupted state if activation fails later)
    m_hasActiveContest = false;
    m_currentContestDbId = -1;

    // Clean up previous contest
    if (m_activeContest) {
        delete m_activeContest;
        m_activeContest = nullptr;

        if (m_dxClusterWindow) {
            m_dxClusterWindow->setActiveContest(nullptr, -1);
        }
    }
}

void MainWindow::createContestServices(const ActivateContestResult& result) {
    // Create QSOLogger
    if (m_qsoLogger) {
        delete m_qsoLogger;
    }
    QSOLogger::Config loggerConfig;
    loggerConfig.contest = m_activeContest;
    loggerConfig.countryFile = &m_countryFile;
    loggerConfig.myStation = result.myStation;
    loggerConfig.operatorName = result.operatorName;  // For {NAME} substitution
    m_qsoLogger = new QSOLogger(loggerConfig);
    LOG_DEBUG("MainWindow", "QSOLogger created for contest");

    // Create DataIntegrityManager
    if (m_integrityManager) {
        delete m_integrityManager;
    }
    DataIntegrityManager::Config integrityConfig;
    integrityConfig.countryFile = &m_countryFile;
    integrityConfig.currentContestDbId = m_currentContestDbId;
    m_integrityManager = new DataIntegrityManager(integrityConfig);
    LOG_DEBUG("MainWindow", "DataIntegrityManager created for contest");

    // Create ContestService
    if (m_contestService) {
        delete m_contestService;
    }
    ContestService::Config contestServiceConfig;
    contestServiceConfig.activeContest = m_activeContest;
    contestServiceConfig.qsoTableModel = m_qsoTableModel;
    contestServiceConfig.currentContestDbId = m_currentContestDbId;
    m_contestService = new ContestService(contestServiceConfig);
    LOG_DEBUG("MainWindow", "ContestService created for contest");

    // Create QSOLoggingCoordinator (orchestrates post-logging actions)
    if (m_loggingCoordinator) {
        delete m_loggingCoordinator;
    }
    m_loggingCoordinator = new QSOLoggingCoordinator(
        m_udpBroadcastManager,
        &BackupManager::instance(),
        m_integrityManager
    );
    LOG_DEBUG("MainWindow", "QSOLoggingCoordinator created for contest");

    // Create QSOPersistenceService
    if (m_persistenceService) {
        delete m_persistenceService;
    }
    QSOPersistenceService::Config persistenceConfig;
    persistenceConfig.appDataDir = PathManager::getAppDataDir();
    m_persistenceService = new QSOPersistenceService(persistenceConfig);
    LOG_DEBUG("MainWindow", "QSOPersistenceService created for contest");

    // Create ExchangeMemoryService
    if (m_exchangeMemoryService) {
        delete m_exchangeMemoryService;
    }
    m_exchangeMemoryService = new ExchangeMemoryService();
    LOG_DEBUG("MainWindow", "ExchangeMemoryService created for contest");

    // Create QSOLoggingService (orchestrates complete logging workflow)
    if (m_loggingService) {
        delete m_loggingService;
    }
    QSOLoggingService::Dependencies loggingDeps;
    loggingDeps.qsoLogger = m_qsoLogger;
    loggingDeps.persistenceService = m_persistenceService;
    loggingDeps.exchangeMemoryService = m_exchangeMemoryService;
    loggingDeps.coordinator = m_loggingCoordinator;
    m_loggingService = new QSOLoggingService(loggingDeps);
    LOG_DEBUG("MainWindow", "QSOLoggingService created for contest");

    // Update ImportExportManager (if it exists - may not during startup)
    if (m_importExportManager) {
        ImportExportManager::Config importExportConfig;
        importExportConfig.countryFile = &m_countryFile;
        importExportConfig.qsoTableModel = m_qsoTableModel;
        importExportConfig.activeContest = m_activeContest;
        importExportConfig.currentContestDbId = m_currentContestDbId;
        importExportConfig.currentContestName = m_currentContest.contestName;
        importExportConfig.hasActiveContest = m_hasActiveContest;
        // Contest configuration for Cabrillo export
        importExportConfig.category = m_currentContest.category;
        importExportConfig.powerClass = m_currentContest.powerClass;
        importExportConfig.assisted = m_currentContest.assisted;
        m_importExportManager->updateConfig(importExportConfig);
        LOG_DEBUG("MainWindow", "ImportExportManager updated for contest");
    }
}

void MainWindow::configureUIForContest(const ActivateContestResult& result) {
    // Update DX Cluster window
    if (m_dxClusterWindow) {
        m_dxClusterWindow->setActiveContest(m_activeContest, m_currentContestDbId);
    }

    // Update web server
    m_webServer->setContestName(m_currentContest.contestName);
    m_webServer->setUsesZoneMultipliers(result.usesZones);
    m_webServer->setUsesModeGroupBreakdown(result.usesModeGroupBreakdown);

    // Configure band summary grid
    if (m_bandSummaryGrid) {
        m_bandSummaryGrid->setMultipliersEnabled(result.usesMultipliers);
        m_bandSummaryGrid->setVisibleBands(result.allowedBands);
        m_bandSummaryGrid->configureForContest(result.usesModeGroupBreakdown, result.usesZones);
    }

    // Configure multiplier window
    if (m_multiplierWindow && !result.multiplierTypes.isEmpty()) {
        MultiplierType primaryType = result.multiplierTypes.first().type;
        m_multiplierWindow->setMultiplierType(primaryType);

        if (primaryType == MultiplierType::Country) {
            m_multiplierWindow->setCountryList(m_countryFile.getAllPrimaryPrefixes());
        }

        LOG_DEBUG("MainWindow", QString("Set multiplier window to type: %1")
            .arg(result.multiplierTypes.first().displayName));
    } else if (m_multiplierWindow) {
        m_multiplierWindow->setMultiplierType(MultiplierType::Section);
    }
}

void MainWindow::setDefaultBandModeForContest(const ContestInfo& contestInfo) {
    // Set mode based on contest type
    if (contestInfo.contestType.contains("CW")) {
        m_currentState.modeA = ModeType::CW;
    } else if (contestInfo.contestType.contains("SSB")) {
        m_currentState.modeA = ModeType::USB;
    } else {
        m_currentState.modeA = ModeType::CW;  // Default for mixed mode
    }

    // Set default band (20M)
    m_currentState.bandA = BandType::Band20M;
    m_currentState.frequencyA = getFrequencyForBand(m_currentState.bandA, m_currentState.modeA);

    // Update display and notify listeners
    updateRadioStatusGrid();
    emit currentBandChanged(m_currentState.bandA);
    emit currentFrequencyChanged(m_currentState.frequencyA);

    LOG_DEBUG("MainWindow", QString("Set default band/mode/freq: %1 %2 %3 Hz (radio not connected)")
        .arg(bandToString(m_currentState.bandA))
        .arg(modeToString(m_currentState.modeA))
        .arg(QString::number(m_currentState.frequencyA, 'f', 0)));
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

    // Delegate to QSORepository (no SQL in UI!)
    DuplicateCheckingRule rule = m_activeContest->getDuplicateCheckingRule();
    QSORepository repo;
    QSORepository::DuplicateCheckResult result = repo.checkDuplicate(callsign, band, mode, rule, m_currentContestDbId);

    dupeInfo = result.dupeInfo;
    return result.isDuplicate;
}

// Band needs tracking methods - delegated to QSOQueryService (Phase 13 extraction)

QSet<QString> MainWindow::getWorkedCallsigns() const {
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();
    return m_qsoQueryService->getWorkedCallsigns(qsos);
}

QList<BandType> MainWindow::getWorkedBandsForCallsign(const QString& callsign) const {
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();
    return m_qsoQueryService->getWorkedBandsForCallsign(qsos, callsign);
}

QList<BandType> MainWindow::getWorkedBandsForMultiplier(const QString& multValue,
                                                        MultiplierType type) const {
    QList<QSO> qsos = m_qsoTableModel->getAllQSOs();
    return m_qsoQueryService->getWorkedBandsForMultiplier(qsos, multValue, type, m_activeContest);
}

QString MainWindow::getMultiplierValueForCallsign(const QString& callsign) const {
    // Delegate to StationInfoService (Phase 7 extraction)
    return m_stationInfoService->getMultiplierValueForCallsign(callsign, m_activeContest);
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
            config.worldMapViewer = m_worldMapViewer;
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

        // Connect close/hide event to disable detailed rig info
        connect(m_radioControlWindow, &QWidget::destroyed, this, [this]() {
            // Window destroyed - disable detailed rig info polling
            if (m_radio && m_radioConnected) {
                if (K4Radio* k4 = qobject_cast<K4Radio*>(m_radio)) {
                    k4->setDetailedRigInfoEnabled(false);
                }
            }
            m_radioControlWindowVisible = false;  // Track closure
        });

        // Install event filter to catch show/hide events
        m_radioControlWindow->installEventFilter(this);

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
    m_radioControlWindowVisible = true;  // Track visibility for reliable shutdown save
    updateWindowMenuCheckmarks();

    // Enable detailed rig info when window is shown
    if (m_radio && m_radioConnected) {
        if (K4Radio* k4 = qobject_cast<K4Radio*>(m_radio)) {
            k4->setDetailedRigInfoEnabled(true);
        }
    }
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

void MainWindow::onShowWorldMap() {
    // Create and show World Map (DXCC entity tracking)
    // Uses native Qt graphics (QGraphicsView), works on all platforms
    if (!m_worldMapViewer) {
        m_worldMapViewer = new NativeMapViewer(NativeMapViewer::DXCC, m_qsoTableModel, this);
        m_worldMapViewer->setWindowFlags(Qt::Window);
        m_worldMapViewer->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_worldMapViewer->show();
    m_worldMapViewer->raise();
    m_worldMapViewer->activateWindow();
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

void MainWindow::onShowAmplifierControl() {
    // Create and show amplifier control window
    if (!m_amplifierControlWindow) {
        if (!m_amplifierService) {
            LOG_WARN("MainWindow", "Cannot show amplifier control: amplifier service not initialized");
            return;
        }
        m_amplifierControlWindow = new AmplifierControlWindow(m_amplifierService, this);
        m_amplifierControlWindow->setWindowFlags(Qt::Window);
        m_amplifierControlWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Restore geometry
        QByteArray geometry = AppSettings::instance().loadAmplifierControlGeometry();
        if (!geometry.isEmpty()) {
            m_amplifierControlWindow->restoreGeometry(geometry);
        } else {
            // First time opening - position offset from main window
            QPoint offset(50, 50);
            m_amplifierControlWindow->move(this->pos() + offset);
        }

        // Connect destroyed signal to clear pointer
        connect(m_amplifierControlWindow, &QWidget::destroyed, this, [this]() {
            // Geometry and visibility saved in MainWindow::saveSettings()
            // DON'T save visibility here - it causes race condition on shutdown
            m_amplifierControlWindow = nullptr;  // Clear pointer
            m_amplifierControlWindowVisible = false;  // Track closure
            // DON'T call updateWindowMenuCheckmarks() here - menu might be destroyed during shutdown
        });
    }

    m_amplifierControlWindow->show();
    m_amplifierControlWindow->raise();
    m_amplifierControlWindow->activateWindow();
    m_amplifierControlWindowVisible = true;  // Track visibility for reliable shutdown save
    // Visibility will be saved in MainWindow::saveSettings() on exit
    updateWindowMenuCheckmarks();
}

// Window menu placeholder implementations
void MainWindow::onSwapMultView() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::SwapMultView, this);
}

void MainWindow::onMissingMultsReport() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::MissingMultsReport, this);
}

// Edit menu placeholder implementations
void MainWindow::onViewEditLog() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::ViewEditLog, this);
}

void MainWindow::onClearDupes() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::ClearDupes, this);
}

void MainWindow::onNote() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::Note, this);
}

void MainWindow::onRecallLast() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::RecallLast, this);
}

// Tools menu placeholder implementations
void MainWindow::onWKMode() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::WKMode, this);
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
    // Must use explicit org/app names to match AppSettings plist file
    QSettings settings(APP_ORG, APP_NAME);
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

    if (m_worldMapViewer && m_worldMapViewer->isVisible()) {
        m_worldMapViewer->move(this->pos() + QPoint(offsetX, offsetY));
        LOG_DEBUG("MainWindow", QString("Repositioned World Map window to (%1, %2)").arg(offsetX).arg(offsetY));
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

void MainWindow::onCTYDownloadCompleted(bool success) {
    if (success) {
        // Clear the "update available" status bar message now that update is applied
        statusBar()->clearMessage();
        LOG_DEBUG("MainWindow", "CTY download completed - cleared status bar notification");
    }
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
    // Note: Status bar cleared via onCTYDownloadCompleted() signal from DownloadManager
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
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::Initialize, this);
}

// Operating menu placeholder implementations
void MainWindow::onAutoCQ() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::AutoCQ, this);
}

void MainWindow::onAutoCQResume() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::AutoCQResume, this);
}

void MainWindow::onKillCW() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::KillCW, this);
}

void MainWindow::onDupeCheck() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::DupeCheck, this);
}

void MainWindow::onSearchLog() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::SearchLog, this);
}

void MainWindow::onDeleteLastQSO() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::DeleteLastQSO, this);
}

void MainWindow::onIncNumber() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::IncNumber, this);
}

void MainWindow::onInitialExchange() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::InitialExchange, this);
}

// Removed: CW Speed menu item (was Alt+S, conflicted with Download SCP)
// Use PgUp/PgDn shortcuts or click WPM label in Radio Control window instead

void MainWindow::onToggleSidetone() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::ToggleSidetone, this);
}

void MainWindow::onToggleAutosend() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::ToggleAutosend, this);
}

// Band menu placeholder implementations
void MainWindow::onToggleRigs() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::ToggleRigs, this);
}

void MainWindow::onEditSO2R() {
    PlaceholderActions::showNotImplemented(PlaceholderActions::Action::EditSO2R, this);
}

void MainWindow::onDXSpotReceived(const QString& callsign,
                                   double frequency,
                                   const QString& spotter,
                                   const QString& comment) {
    if (!m_bandMapWindow) {
        LOG_DEBUG("MainWindow", "Band map window not open - spot not added");
        return;
    }

    // Delegate spot processing to service
    SpotProcessingService spotService;
    Spot spot = spotService.processSpot(callsign, frequency, spotter, comment);
    m_bandMapWindow->addSpot(spot);
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
