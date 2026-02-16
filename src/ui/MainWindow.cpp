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
#include "dialogs/QSOSearchDialog.h"
#include "dialogs/SO2RConfigDialog.h"
#include "widgets/DXClusterWindow.h"
#include "widgets/BandMapWidget.h"
#include "widgets/RadioControlWidget.h"
#include "widgets/MultiplierWidget.h"
#include "statistics/StatisticsWindow.h"
#include "windows/AmplifierControlWindow.h"
#ifdef PANADAPTER_ENABLED
#include "windows/PanadapterWindow.h"
#endif
#include "../radio/RadioFactory.h"
#include "widgets/QSOSearchPanel.h"
#include "NativeMapViewer.h"
#include "../network/UdpBroadcastManager.h"
#include "../network/WebServer.h"
#include "../controllers/ImportExportManager.h"
#include "../controllers/CWMessageManager.h"
#include "../controllers/BandSwitchingManager.h"
#include "../keyers/KeyerController.h"
#include "../keyers/IambicKeyer.h"
#include "dialogs/KeyerSetupDialog.h"
#include "../amplifiers/AmplifierFactory.h"
#include "../rotator/RotatorFactory.h"
#include "../core/Constants.h"
#include "../core/BandConstants.h"
#include "../logging/LogMacros.h"
#include "../logging/Logger.h"
#include "../utils/ThemeManager.h"
#include "../utils/FontManager.h"
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
#include "../services/QSOSearchService.h"
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
#include <QSplitter>
#include <QShortcut>
#include <QLineEdit>
#include <QTextEdit>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QElapsedTimer>
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
    , m_radio2ControlWindow(nullptr)
    , m_multiplierWindow(nullptr)
    , m_statisticsWindow(nullptr)
    , m_functionKeysWindow(nullptr)
    , m_sectionsMapViewer(nullptr)
    , m_statesMapViewer(nullptr)
    , m_worldMapViewer(nullptr)
    , m_graylineMapDialog(nullptr)
    , m_amplifierControlWindow(nullptr)
    // m_panadapterWindow initialized in header (conditionally compiled)
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
    , m_windowActivationHelper(nullptr)
    , m_importExportManager(nullptr)
    , m_downloadManager(nullptr)
    , m_radioManager(nullptr)
    , m_bandSwitchingManager(nullptr)
    , m_cwMessageManager(nullptr)
    , m_amplifierService(nullptr)
    , m_rotatorService(nullptr)
    , m_qsoTableModel(new QSOTableModel(this))
    , m_scpMatcher(std::make_unique<SCPMatcher>())
    , m_countryFileDownloader(new CountryFileDownloader(this))
    , m_latestCTYVersion(0)
    , m_udpBroadcastManager(new UdpBroadcastManager(this))
    , m_webServer(new WebServer(m_qsoTableModel, m_radio, this))
    , m_inRaiseAllWindows(false)
    , m_initialExchangePopulated(false)
    , m_operatingMode(OperatingMode::CQ)
    , m_operatingModeLabel(nullptr)
    , m_lastFrequency(0)
    , m_searchPanel(nullptr)
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
    m_stationInfoService = std::make_unique<StationInfoService>(&m_countryFile);
    LOG_DEBUG("MainWindow", "StationInfoService created");

    // Create ScoreCalculationService (Phase 11 extraction)
    m_scoreCalculationService = std::make_unique<ScoreCalculationService>();
    LOG_DEBUG("MainWindow", "ScoreCalculationService created");

    // Create MaintenanceService (clear log, backup workflows)
    m_maintenanceService = std::make_unique<MaintenanceService>(this);
    LOG_DEBUG("MainWindow", "MaintenanceService created");

    // Create QSOQueryService (Phase 13 extraction)
    m_qsoQueryService = std::make_unique<QSOQueryService>();
    LOG_DEBUG("MainWindow", "QSOQueryService created");

    // Create ContestManager with country file
    ContestManager::Config contestManagerConfig;
    contestManagerConfig.countryFile = &m_countryFile;
    m_contestManager = std::make_unique<ContestManager>(contestManagerConfig);
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
    m_cwMessageManager = std::make_unique<CWMessageManager>(CWMessageManager::Config{m_radio, nullptr});  // Contest set later
    m_windowManager = new WindowManager(this);
    m_windowActivationHelper = new WindowActivationHelper(this, this);
    m_settingsManager = std::make_unique<SettingsManager>();
    LOG_DEBUG("MainWindow", "Controllers and UI managers created");

    // Initialize hardware control services (amplifier and rotator)
    initializeHardwareServices();

    // Initialize input handler service (keyboard handling for CW, mode switching)
    InputHandlerService::Config inputConfig;
    inputConfig.radio = m_radio;
    inputConfig.currentState = &m_currentState;
    inputConfig.radioConnected = &m_radioConnected;
    m_inputHandler = new InputHandlerService(inputConfig, this);

    // Connect InputHandlerService signals to MainWindow slots
    connect(m_inputHandler, &InputHandlerService::switchToSPMode,
            this, &MainWindow::onSPMode);
    connect(m_inputHandler, &InputHandlerService::switchToCQMode,
            this, &MainWindow::onCQMode);
    connect(m_inputHandler, &InputHandlerService::sendFunctionKey,
            this, &MainWindow::handleFunctionKey);
    connect(m_inputHandler, &InputHandlerService::clearCallsign,
            m_callsignEntry, &QLineEdit::clear);
    connect(m_inputHandler, &InputHandlerService::clearExchange,
            m_exchangeEntry, &QLineEdit::clear);
    connect(m_inputHandler, &InputHandlerService::focusCallsign,
            m_callsignEntry, qOverload<>(&QLineEdit::setFocus));
    connect(m_inputHandler, &InputHandlerService::statusMessage,
            this, &MainWindow::setStatusMessage);
    LOG_DEBUG("MainWindow", "InputHandlerService created and connected");

    loadSettings();
    loadUdpBroadcastSettings();

    // Initialize ham radio privileges validator with license class from settings
    QString licenseClassStr = AppSettings::instance().getLicenseClass();
    HamRadioPrivileges::LicenseClass licenseClass =
        HamRadioPrivileges::stringToLicenseClass(licenseClassStr);
    m_hamPrivileges = std::make_unique<HamRadioPrivileges>(licenseClass);

    // Initialize backup manager from settings
    loadBackupSettings();

    // Reopen last contest if available
    reopenLastContest();

    // Create ImportExportManager (needs contest context)
    ImportExportManager::Config importExportConfig;
    importExportConfig.countryFile = &m_countryFile;
    importExportConfig.qsoTableModel = m_qsoTableModel;
    importExportConfig.activeContest = m_activeContest.get();
    importExportConfig.currentContestDbId = m_currentContestDbId;
    importExportConfig.currentContestName = m_currentContest.contestName;
    importExportConfig.hasActiveContest = m_hasActiveContest;
    m_importExportManager = std::make_unique<ImportExportManager>(importExportConfig, this);
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

    // Connect WebServer command API signals
    connect(m_webServer, &WebServer::logQSORequested,
            this, &MainWindow::onLogQSOFromWeb);
    connect(m_webServer, &WebServer::commandRequested,
            this, &MainWindow::onCommandFromWeb);

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
    connect(m_radioManager, &RadioManager::radioModelIdentified,
            this, &MainWindow::onRadioModelIdentified);
    connect(m_radioManager, &RadioManager::statusMessage,
            this, &MainWindow::setStatusMessage);
    connect(m_radioManager, &RadioManager::flashStateChanged,
            this, [this](bool flashState) {
                m_radioFlashState = flashState;
                updateRadioStatusFlash();
            });
    connect(m_radioManager, &RadioManager::maxPowerChanged,
            this, [this](int maxPowerWatts) {
                if (m_radioControlWindow) {
                    m_radioControlWindow->setMaxPower(maxPowerWatts);
                }
            });

    // Fast frequency update from transceive mode (bypasses slow radioStateUpdated)
    connect(m_radioManager, &RadioManager::frequencyChanged,
            this, &MainWindow::onFastFrequencyUpdate);

    // SO2R indexed state updates - update the correct Radio Control window and UDP broadcast
    connect(m_radioManager, &RadioManager::radioStateUpdatedIndexed,
            this, [this](int radioIndex, const RadioState& state) {
                if (radioIndex == 0 && m_radioControlWindow) {
                    m_radioControlWindow->updateRadioState(state);
                } else if (radioIndex == 1 && m_radio2ControlWindow) {
                    m_radio2ControlWindow->updateRadioState(state);
                }
                // Send UDP broadcast for this radio's state change
                QString stationCall = AppSettings::instance().getMyCallsign();
                m_udpBroadcastManager->onRadioStateChangedIndexed(radioIndex, state, stationCall);
            });

    // SO2R signals - update UI when active radio changes or standby frequency changes
    connect(m_radioManager, &RadioManager::activeRadioChanged,
            this, [this](int radioIndex) {
                LOG_INFO("MainWindow", QString("Active radio changed to Radio %1").arg(radioIndex + 1));
                updateRadioStatusGrid();
                // Update Radio Control windows to show which is active
                if (m_radioControlWindow) {
                    m_radioControlWindow->setActive(radioIndex == 0);
                }
                if (m_radio2ControlWindow) {
                    m_radio2ControlWindow->setActive(radioIndex == 1);
                }
                // Update CW message manager for per-radio message repeat
                if (m_cwMessageManager) {
                    m_cwMessageManager->setActiveRadioIndex(radioIndex);
                }
                // Update UDP broadcast manager for correct focusRadioNr/activeRadioNr
                if (m_udpBroadcastManager) {
                    m_udpBroadcastManager->setActiveRadioIndex(radioIndex);
                }
                // Re-emit signals for external listeners (band map, etc.)
                RadioState state = m_radioManager->currentState();
                emit currentFrequencyChanged(state.frequencyA);
                emit currentBandChanged(state.bandA);
            });
    connect(m_radioManager, &RadioManager::standbyFrequencyChanged,
            this, [this](freq_t /* frequency */) {
                // Update the standby frequency display in the radio status grid
                updateRadioStatusGrid();
            });

    // NOTE: Radio reconnection and flash timers are now handled by RadioManager
    // These local timers and variables (m_radioReconnectTimer, m_radioFlashTimer, etc.)
    // can be removed in a future cleanup

    // Connect CTY.DAT update notification
    connect(m_countryFileDownloader, &CountryFileDownloader::updateAvailable,
            this, &MainWindow::onCTYUpdateAvailable);

    // Check for CTY.DAT updates after startup (async, non-blocking)
    QTimer::singleShot(UITiming::DEFERRED_ACTION_DELAY_MS, this, [this]() {
        LOG_DEBUG("MainWindow", "Checking for CTY.DAT updates...");
        m_countryFileDownloader->checkLatestVersion();
    });

    // Initialize radio status display (date/time, band/mode/freq defaults)
    updateRadioStatusGrid();

    // Try auto-connect if enabled and config exists
    AppSettings& settings = AppSettings::instance();

    if (settings.hasAnyRadioConfig()) {
        RadioConfig config = settings.getActiveRadioConfig();
        bool autoConnectEnabled = settings.getRadioAutoConnect();
        LOG_DEBUG("MainWindow", QString("Radio auto-connect check: modelId=%1, autoConnect=%2")
            .arg(config.hamlibModelId).arg(autoConnectEnabled));
        // Only auto-connect if a valid radio model is selected (not "Select radio...")
        if (config.hamlibModelId > 0 && autoConnectEnabled) {
            // Auto-connect enabled - connect now
            LOG_INFO("MainWindow", "Auto-connecting to radio...");
            setStatusMessage("Auto-connecting to radio...");
            QTimer::singleShot(500, this, &MainWindow::onRadioConnect);  // Slight delay to let UI initialize
        } else if (config.hamlibModelId > 0) {
            setStatusMessage("Found saved radio configuration. Use Radio → Connect to connect.");
        } else {
            setStatusMessage("No valid radio model selected. Use Radio → Configure.");
        }
    } else {
        LOG_DEBUG("MainWindow", "No radio config found");
        setStatusMessage("No radio configuration found. Use Radio → Configure.");
    }

    // Check if grid square is configured (needed for azimuth/distance calculations)
    // Delay check to let UI fully initialize
    QTimer::singleShot(1000, this, [this]() {
        AppSettings& settings = AppSettings::instance();
        QString gridSquare = settings.getMyGridSquare();
        LOG_INFO("MainWindow", QString("Grid square check at 1s delay: value='%1', isEmpty=%2")
            .arg(gridSquare).arg(gridSquare.isEmpty()));

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
    QTimer::singleShot(UITiming::QUICK_DELAY_MS, this, [this]() {
        if (!AppSettings::instance().getCWAutoSendEnabled()) {
            setStatusMessage("⚠ CW Auto-Send is OFF - Enable in Radio menu");
            m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
                .arg(ThemeManager::instance().colorName(ColorRole::WarningText)));
            LOG_INFO("MainWindow", "Auto-Send is disabled at startup");
        }
    });
}

MainWindow::~MainWindow() {
    // Settings are already saved in closeEvent()
    // Don't save here as windows will be closed and visibility will be wrong

    // unique_ptr members (m_activeContest, m_loggingService, m_qsoLogger, etc.)
    // are automatically cleaned up by their destructors - no manual delete needed
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
    config.onShowRadio2Control = [this]() { onShowRadio2Control(); };
    config.onSendMorse = [this]() { onSendMorse(); };
    config.onEditCWMessages = [this]() { onEditCWMessages(); };
    config.onShowKeyerSetup = [this]() { onShowKeyerSetup(); };
    config.onShowFunctionKeysRef = [this]() { onShowFunctionKeysRef(); };
    config.onShowMultipliers = [this]() { onShowMultipliers(); };
    config.onShowStatistics = [this]() { onShowStatistics(); };
    config.onShowSectionsMap = [this]() { onShowSectionsMap(); };
    config.onShowStatesMap = [this]() { onShowStatesMap(); };
    config.onShowWorldMap = [this]() { onShowWorldMap(); };
    config.onShowGraylineMap = [this]() { onShowGraylineMap(); };
    config.onShowAmplifierControl = [this]() { onShowAmplifierControl(); };
    config.onShowPanadapter = [this]() { onShowPanadapter(); };
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
    m_radio2ControlAction = m_menuManager->radio2ControlAction();
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
    // Band click with SO2R support: left-click=active radio, Shift+click or right-click=non-active radio
    connect(m_bandSummaryGrid, &BandSummaryGrid::bandClickedWithTarget,
            this, [this](BandType band, bool forNonActiveRadio) {
                int targetRadioIndex = forNonActiveRadio
                    ? m_radioManager->getStandbyRadioIndex()
                    : m_radioManager->getActiveRadioIndex();

                RadioController* targetRadio = m_radioManager->getRadioController(targetRadioIndex);
                if (targetRadio && targetRadio->isConnected()) {
                    LOG_INFO("MainWindow", QString("Band click: %1 -> Radio %2 (nonActive=%3)")
                             .arg(bandToString(band)).arg(targetRadioIndex + 1).arg(forNonActiveRadio));
                    targetRadio->setBand(band);
                } else if (!forNonActiveRadio) {
                    // Fallback to manual band selection for active radio if not connected
                    onBandClicked(band);
                } else {
                    LOG_DEBUG("MainWindow", QString("Band click ignored: Radio %1 not connected")
                              .arg(targetRadioIndex + 1));
                }
            });
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
    QFont stationInfoFont = FontManager::instance().monospaceFont(miscFontSize);
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
    m_qsoTableView->setFont(FontManager::instance().monospaceFont(9));

    // Force visible bottom border on header sections (macOS native style swallows partial stylesheets)
    m_qsoTableView->horizontalHeader()->setStyleSheet(
        QString("QHeaderView::section { "
                "border: none; "
                "border-bottom: 2px solid %1; "
                "padding: 2px 4px; "
                "background: %2; "
                "}")
        .arg(ThemeManager::instance().colorName(ColorRole::BorderColor))
        .arg(ThemeManager::instance().colorName(ColorRole::WindowBackground)));

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

    // Search results panel (hidden by default)
    m_searchPanel = new QSOSearchPanel(this);
    connect(m_searchPanel, &QSOSearchPanel::closeRequested, this, [this]() {
        m_searchPanel->clear();
        m_callsignEntry->setFocus();
    });
    connect(m_searchPanel, &QSOSearchPanel::qsoSelected, this, [this](const QSO& qso) {
        if (qso.id < 0) return;

        EditQSODialog dialog(qso, m_activeContest.get(), this);
        if (dialog.exec() != QDialog::Accepted) return;

        QSO editedQSO = dialog.getEditedQSO();
        QSORepository repo;
        if (repo.updateQSO(editedQSO)) {
            // Update main table if QSO is in current contest
            QList<QSO> allQSOs = m_qsoTableModel->getAllQSOs();
            for (int row = 0; row < allQSOs.size(); ++row) {
                if (allQSOs[row].id == editedQSO.id) {
                    m_qsoTableModel->updateQSO(row, editedQSO);
                    break;
                }
            }
            rebuildMultiplierWindow();
            refreshSearchResults();
            LOG_INFO("MainWindow", QString("Updated QSO #%1 (%2) from search results")
                .arg(editedQSO.id).arg(editedQSO.callsign));
        } else {
            DialogHelper::warning(this, "Error",
                QString("Failed to update QSO: %1").arg(repo.lastError()));
        }
    });

    // Vertical splitter: QSO table on top, search panel on bottom
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_qsoTableView);
    splitter->addWidget(m_searchPanel);
    splitter->setStretchFactor(0, 1);  // QSO table gets all space when search hidden
    splitter->setStretchFactor(1, 0);
    splitter->setChildrenCollapsible(false);

    mainLayout->addWidget(splitter, 1);  // Stretch factor 1 = takes remaining space

    // Bottom: Entry and stats panel (with radio status on left)
    QWidget* bottomPanel = createBottomPanel();
    mainLayout->addWidget(bottomPanel);

    setCentralWidget(central);

    // Ctrl+F / Cmd+F: Search for callsign in field, or open search dialog if empty
    QShortcut* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        QString callsign = m_callsignEntry->text().trimmed().toUpper();
        if (!callsign.isEmpty()) {
            searchForCallsign(callsign);
        } else {
            executeSearch();
        }
    });

    // Calculate minimum window width based on bottom panel's sizeHint
    // This ensures all widgets fit without overlap
    int bottomPanelMinWidth = bottomPanel->minimumSizeHint().width();
    if (bottomPanelMinWidth < UIDefaults::BOTTOM_PANEL_MIN_WIDTH) {
        bottomPanelMinWidth = UIDefaults::BOTTOM_PANEL_MIN_WIDTH;
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
    // === Radio Status Widget (TR4W-style layout) ===
    // Layout: [Band/Mode Box] [Freq Stack] [WPM] [Date/Time Stack]
    QWidget* radioStatusWidget = new QWidget(this);
    radioStatusWidget->setAutoFillBackground(true);
    QHBoxLayout* radioLayout = new QHBoxLayout(radioStatusWidget);
    radioLayout->setSpacing(10);
    radioLayout->setContentsMargins(5, 2, 5, 2);

    QFont labelFont = FontManager::instance().monospaceFont(11);
    labelFont.setBold(true);
    QFont freqFont = FontManager::instance().monospaceFont(10);

    // --- Band/Mode box (e.g., "15M USB") with border ---
    m_radioFreqBandLabel = new QLabel("--", radioStatusWidget);
    m_radioFreqBandLabel->setFont(labelFont);
    m_radioFreqBandLabel->setAlignment(Qt::AlignCenter);
    m_radioFreqBandLabel->setMinimumWidth(UIDefaults::RADIO_FREQ_BAND_LABEL_WIDTH);
    m_radioFreqBandLabel->setStyleSheet("QLabel { border: 1px solid gray; padding: 2px 5px; }");

    // --- Frequency stack (SO2R: standby on top grayed, active below bright) ---
    QWidget* freqStackWidget = new QWidget(radioStatusWidget);
    QVBoxLayout* freqStackLayout = new QVBoxLayout(freqStackWidget);
    freqStackLayout->setSpacing(2);  // Small gap between standby and active frequencies
    freqStackLayout->setContentsMargins(0, 0, 0, 0);

    // Standby frequency (hidden when not in SO2R mode)
    m_standbyFreqLabel = new QLabel("", freqStackWidget);
    m_standbyFreqLabel->setFont(freqFont);
    m_standbyFreqLabel->setAlignment(Qt::AlignLeft);
    m_standbyFreqLabel->setMinimumWidth(UIDefaults::RADIO_FREQ_LABEL_WIDTH);
    m_standbyFreqLabel->setVisible(false);  // Hidden by default

    // Active frequency
    m_radioFreqLabel = new QLabel("0.000", freqStackWidget);
    m_radioFreqLabel->setFont(freqFont);
    m_radioFreqLabel->setAlignment(Qt::AlignLeft);
    m_radioFreqLabel->setMinimumWidth(UIDefaults::RADIO_FREQ_LABEL_WIDTH);

    freqStackLayout->addWidget(m_standbyFreqLabel);
    freqStackLayout->addWidget(m_radioFreqLabel);

    // --- WPM label ---
    m_radioWpmLabel = new QLabel("-- WPM", radioStatusWidget);
    m_radioWpmLabel->setFont(labelFont);
    m_radioWpmLabel->setAlignment(Qt::AlignCenter);
    m_radioWpmLabel->setMinimumWidth(UIDefaults::RADIO_WPM_LABEL_WIDTH);
    m_radioWpmLabel->setEnabled(false);

    // --- Date/Time stack ---
    QWidget* dateTimeWidget = new QWidget(radioStatusWidget);
    QVBoxLayout* dateTimeLayout = new QVBoxLayout(dateTimeWidget);
    dateTimeLayout->setSpacing(0);
    dateTimeLayout->setContentsMargins(0, 0, 0, 0);

    QFont dateTimeFont = FontManager::instance().monospaceFont(labelFont.pointSize());

    m_radioDateLabel = new QLabel("", dateTimeWidget);
    m_radioDateLabel->setFont(dateTimeFont);
    m_radioDateLabel->setAlignment(Qt::AlignCenter);

    m_radioTimeLabel = new QLabel("", dateTimeWidget);
    m_radioTimeLabel->setFont(dateTimeFont);
    m_radioTimeLabel->setAlignment(Qt::AlignCenter);

    dateTimeLayout->addWidget(m_radioDateLabel);
    dateTimeLayout->addWidget(m_radioTimeLabel);

    // Add to horizontal layout
    radioLayout->addWidget(m_radioFreqBandLabel);
    radioLayout->addWidget(freqStackWidget);
    radioLayout->addWidget(m_radioWpmLabel);
    radioLayout->addWidget(dateTimeWidget);

    // Set fixed width to prevent layout shifts
    const int RADIO_STATUS_MIN_WIDTH = 320;
    radioStatusWidget->setMinimumWidth(RADIO_STATUS_MIN_WIDTH);

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
    m_callsignEntry->setFont(FontManager::instance().monospaceFont(miscFontSize));

    QLabel* exchLabel = new QLabel("Exch:", this);
    m_exchangeEntry = new QLineEdit(this);
    m_exchangeEntry->setPlaceholderText("RST + Zone");
    m_exchangeEntry->setFixedWidth(ENTRY_FIELD_WIDTH);
    m_exchangeEntry->setFont(FontManager::instance().monospaceFont(miscFontSize));

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
    m_scpMatchesLabel->setStyleSheet(QString("QLabel { color: %1; font-size: %2pt; }")
        .arg(ThemeManager::instance().colorName(ColorRole::SCPMatchText))
        .arg(scpFontSize));
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
    QFont monoFont = FontManager::instance().monospaceFont(miscFontSize);

    // Time and rate
    QHBoxLayout* timeRow = new QHBoxLayout();
    timeRow->setSpacing(10);  // Explicit spacing between labels
    m_timeLabel = new QLabel("00:00:00", this);
    m_timeLabel->setFont(monoFont);
    m_timeLabel->setMinimumWidth(UIDefaults::TIME_LABEL_MIN_WIDTH);  // Fixed width to prevent layout shifts
    m_timeLabel->setAlignment(Qt::AlignLeft);
    m_thisHrLabel = new QLabel("This Hr = 0", this);
    m_thisHrLabel->setFont(monoFont);
    m_thisHrLabel->setMinimumWidth(UIDefaults::THIS_HR_LABEL_MIN_WIDTH);
    m_rateLabel = new QLabel("Rate = 0", this);
    m_rateLabel->setFont(monoFont);
    m_rateLabel->setMinimumWidth(UIDefaults::RATE_LABEL_MIN_WIDTH);
    timeRow->addWidget(m_timeLabel);
    timeRow->addSpacing(5);  // Extra space before "This Hr"
    timeRow->addWidget(m_thisHrLabel);
    timeRow->addSpacing(5);  // Extra space before "Rate"
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
    // Allow status label to shrink and elide text to prevent overlap with permanent widgets
    m_statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_statusLabel->setMinimumWidth(100);
    status->addWidget(m_statusLabel, 1);  // stretch factor 1 to take available space

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
    // Issue #69: AmplifierController runs the device in a worker thread to prevent UI freezing
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
        config.pollIntervalMs = settings.getAmplifierPollInterval();

        // Determine amplifier type
        int amplifierType;
        const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
        if (connectionType == "direct" && modelId == AMP_MODEL_ELECRAFT_KPA1500) {
            amplifierType = static_cast<int>(AmplifierFactory::AmplifierType::KPA1500_DIRECT);
        } else {
            amplifierType = static_cast<int>(AmplifierFactory::AmplifierType::HAMLIB);
        }

        // Create amplifier controller (manages device in worker thread)
        m_amplifierController = new AmplifierController(this);

        // Create amplifier service (business logic layer)
        m_amplifierService = new AmplifierService(m_amplifierController, this);

        // Auto-connect if enabled (async - result via connectionStatusChanged signal)
        if (settings.getAmplifierAutoConnect()) {
            m_amplifierService->connectToAmplifier(amplifierType, config);
            LOG_INFO("MainWindow", "Amplifier auto-connect initiated (async)");
        }

        LOG_DEBUG("MainWindow", "Amplifier controller and service initialized (worker thread)");
    }

    // Initialize rotator service if enabled
    // Issue #69: RotatorController runs the device in a worker thread to prevent UI freezing
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
        int rotatorType;
        const int ROT_MODEL_PSTROTATOR = 9999;
        if (connectionType == "direct" && modelId == ROT_MODEL_PSTROTATOR) {
            rotatorType = static_cast<int>(RotatorFactory::RotatorType::PSTROTATOR);
            config.rotatorType = 0;  // PSTRotator
        } else {
            rotatorType = static_cast<int>(RotatorFactory::RotatorType::HAMLIB);
            config.rotatorType = modelId;  // Hamlib model ID
        }

        // Create rotator controller (manages device in worker thread)
        m_rotatorController = new RotatorController(this);

        // Create rotator service (business logic layer)
        m_rotatorService = new RotatorService(m_rotatorController, this);

        // Auto-connect if enabled (async - result via connectionStatusChanged signal)
        if (settings.getRotatorAutoConnect()) {
            m_rotatorService->connectToRotator(rotatorType, config);
            LOG_INFO("MainWindow", "Rotator auto-connect initiated (async)");
        }

        LOG_DEBUG("MainWindow", "Rotator controller and service initialized (worker thread)");
    }

    // --- CW Keyer Controller + Iambic Keyer ---
    m_keyerController = new KeyerController(this);
    m_iambicKeyer = new IambicKeyer(this);
    m_iambicKeyer->setWpm(settings.getMorseWPM());
    m_iambicKeyer->setMode(settings.getKeyerIambicMode() == 0
                           ? IambicMode::IambicA : IambicMode::IambicB);

    // Feed paddle state from keyer controller to iambic keyer
    connect(m_keyerController, &KeyerController::paddleStateChanged,
            m_iambicKeyer, &IambicKeyer::updatePaddleState);

    LOG_DEBUG("MainWindow", "Keyer controller and iambic keyer initialized");
}

void MainWindow::loadSettings() {
    if (!m_settingsManager) {
        return;
    }

    // Load window geometry (but defer restoration until showEvent)
    m_pendingGeometry = m_settingsManager->loadWindowGeometry();

    // Restore operator immediately (doesn't need deferral)
    if (!m_pendingGeometry.currentOperator.isEmpty()) {
        if (m_operatorLabel) {
            m_operatorLabel->setText(m_pendingGeometry.currentOperator);
        }
    }

    // Apply font settings
    applyFontSettings();

    // Connect to theme changes and apply initial theme
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::applyTheme);
    applyTheme();

    // Connect to StatusNotifier for centralized status messages (Issue #75)
    connect(&StatusNotifier::instance(), &StatusNotifier::statusChanged,
            this, &MainWindow::onStatusChanged);

    // Geometry restoration is now handled in showEvent()
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    // Only restore geometry once, on first show
    if (!m_geometryRestored) {
        m_geometryRestored = true;

        // Use a short delay to ensure the window is fully laid out
        // This fires after the event loop processes the show event
        const int GEOMETRY_RESTORE_DELAY_MS = 50;
        QTimer::singleShot(GEOMETRY_RESTORE_DELAY_MS, this, [this]() {
            // Restore main window geometry first
            if (!m_pendingGeometry.mainWindowGeometry.isNull()) {
                LOG_DEBUG("MainWindow", "Restoring main window geometry (from showEvent)");
                restoreGeometry(m_pendingGeometry.mainWindowGeometry);
            }
            if (!m_pendingGeometry.mainWindowState.isNull()) {
                restoreState(m_pendingGeometry.mainWindowState);
            }
            // Then restore child windows
            restoreChildWindows(m_pendingGeometry);
        });
    }
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

    if (geometry.radio2ControlVisible) {
        LOG_DEBUG("MainWindow", "Restoring Radio 2 Control window (was visible on exit)");
        onShowRadio2Control();
        if (m_radio2ControlWindow && !geometry.radio2ControlGeometry.isEmpty()) {
            m_radio2ControlWindow->restoreGeometry(geometry.radio2ControlGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Radio 2 Control window (was hidden on exit)");
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

#ifdef PANADAPTER_ENABLED
    // Panadapter window
    if (geometry.panadapterVisible) {
        LOG_DEBUG("MainWindow", "Restoring Panadapter window (was visible on exit)");
        onShowPanadapter();
        if (m_panadapterWindow && !geometry.panadapterGeometry.isEmpty()) {
            m_panadapterWindow->restoreGeometry(geometry.panadapterGeometry);
        }
    } else {
        LOG_DEBUG("MainWindow", "NOT restoring Panadapter window (was hidden on exit)");
    }
#endif
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
    if (m_radio2ControlWindow) {
        geometry.radio2ControlGeometry = m_radio2ControlWindow->saveGeometry();
    }
    // Use tracked visibility (Qt's isVisible() can return false during SIGTERM shutdown)
    geometry.radio2ControlVisible = m_radio2ControlWindowVisible;
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

#ifdef PANADAPTER_ENABLED
    // Panadapter window
    if (m_panadapterWindow) {
        geometry.panadapterGeometry = m_panadapterWindow->saveGeometry();
    }
    geometry.panadapterVisible = m_panadapterWindowVisible;
    LOG_DEBUG("MainWindow", QString("Panadapter window tracked visibility: %1").arg(m_panadapterWindowVisible));
#endif

    // Debug logging for window visibility
    LOG_DEBUG("MainWindow", QString("Saving window visibility - DXCluster:%1 BandMap:%2 RadioCtrl:%3 Radio2Ctrl:%4 Mult:%5 Stats:%6 Sections:%7 States:%8 Grayline:%9 AmpCtrl:%10")
        .arg(geometry.dxClusterVisible)
        .arg(geometry.bandMapVisible)
        .arg(geometry.radioControlVisible)
        .arg(geometry.radio2ControlVisible)
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

    // TIMING: Start shutdown timer right after user confirms
    QElapsedTimer shutdownTimer;
    shutdownTimer.start();
    LOG_INFO("MainWindow", "SHUTDOWN TIMING: User confirmed exit, starting shutdown sequence");

    // Save settings BEFORE closing windows (so visibility state is correct)
    saveSettings();
    LOG_INFO("MainWindow", QString("SHUTDOWN TIMING: Settings saved (%1ms elapsed)")
             .arg(shutdownTimer.elapsed()));

    // Save band map spots to database before closing
    if (m_bandMapWindow) {
        m_bandMapWindow->saveSpotsToDatabase();
    }

    // Close all child windows
    LOG_DEBUG("MainWindow", "Closing child windows...");
    QElapsedTimer closeTimer;
    closeTimer.start();

    if (m_dxClusterWindow) {
        LOG_DEBUG("MainWindow", "Closing DX Cluster window...");
        m_dxClusterWindow->close();
        LOG_DEBUG("MainWindow", QString("DX Cluster window closed (%1ms)").arg(closeTimer.restart()));
    }
    if (m_bandMapWindow) {
        LOG_DEBUG("MainWindow", "Closing Band Map window...");
        m_bandMapWindow->close();
        LOG_DEBUG("MainWindow", QString("Band Map window closed (%1ms)").arg(closeTimer.restart()));
    }
    if (m_radioControlWindow) {
        LOG_DEBUG("MainWindow", "Closing Radio 1 Control window...");
        m_radioControlWindow->close();
        LOG_DEBUG("MainWindow", QString("Radio 1 Control window closed (%1ms)").arg(closeTimer.restart()));
    }
    if (m_radio2ControlWindow) {
        LOG_DEBUG("MainWindow", "Closing Radio 2 Control window...");
        m_radio2ControlWindow->close();
        LOG_DEBUG("MainWindow", QString("Radio 2 Control window closed (%1ms)").arg(closeTimer.restart()));
    }
    if (m_multiplierWindow) {
        LOG_DEBUG("MainWindow", "Closing Multiplier window...");
        m_multiplierWindow->close();
        LOG_DEBUG("MainWindow", QString("Multiplier window closed (%1ms)").arg(closeTimer.restart()));
    }
    if (m_statisticsWindow) {
        LOG_DEBUG("MainWindow", "Closing Statistics window...");
        m_statisticsWindow->rateCalculator()->stopAutoUpdate();
        m_statisticsWindow->close();
        LOG_DEBUG("MainWindow", QString("Statistics window closed (%1ms)").arg(closeTimer.restart()));
    }
    if (m_amplifierControlWindow) {
        LOG_INFO("MainWindow", QString("SHUTDOWN TIMING: Closing Amplifier Control window... (%1ms elapsed)")
                 .arg(shutdownTimer.elapsed()));
        m_amplifierControlWindow->close();
        LOG_INFO("MainWindow", QString("SHUTDOWN TIMING: Amplifier Control window closed (%1ms, total %2ms)")
                 .arg(closeTimer.restart()).arg(shutdownTimer.elapsed()));
    }
#ifdef PANADAPTER_ENABLED
    if (m_panadapterWindow) {
        LOG_INFO("MainWindow", QString("SHUTDOWN TIMING: Closing Panadapter window... (%1ms elapsed)")
                 .arg(shutdownTimer.elapsed()));
        m_panadapterWindow->close();
        LOG_INFO("MainWindow", QString("SHUTDOWN TIMING: Panadapter window closed (%1ms, total %2ms)")
                 .arg(closeTimer.restart()).arg(shutdownTimer.elapsed()));
    }
#endif

    // Disconnect radios before closing and wait for completion
    // CRITICAL: Must allow disconnect packets to be sent before app exits
    // Without this, Icom network radios stay in "connected" state and refuse reconnection
    LOG_DEBUG("MainWindow", "Disconnecting radios...");
    closeTimer.restart();

    // Use RadioManager to disconnect all radios (SO2R support)
    if (m_radioManager) {
        m_radioManager->disconnectFromRadio();  // This disconnects all radios

        // Give disconnect time to complete (sends CI-V close + control disconnect packets)
        // RadioController destructor will ensure full cleanup, but we need event loop
        // to process the queued disconnect operation before QApplication::quit()
        QApplication::processEvents();  // Process queued disconnect
        QThread::msleep(100);           // Allow UDP packets to be sent
        LOG_DEBUG("MainWindow", QString("Radio disconnect completed (%1ms)").arg(closeTimer.elapsed()));
    }

    event->accept();

    LOG_INFO("MainWindow", QString("SHUTDOWN TIMING: Shutdown complete, total time: %1ms")
             .arg(shutdownTimer.elapsed()));

    // Ensure application quits
    QApplication::quit();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    // PgUp: Increase WPM by configurable increment
    if (event->key() == Qt::Key_PageUp) {
        // Only allow CW speed change in CW mode with radio connected
        bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
        if (!isCWMode || !m_radioConnected) {
            setStatusMessage("CW speed adjust requires CW mode and radio connection");
            event->accept();
            return;
        }

        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = m_currentState.cwSpeed;  // Read from radio's actual speed
        const int MAX_WPM = 100;  // K4 maximum
        int newWpm = qMin(currentWpm + increment, MAX_WPM);

        // Send to radio - display will update when radio responds via stateUpdated
        m_radio->setCWSpeed(newWpm);

        setStatusMessage(QString("CW Speed: %1 WPM").arg(newWpm));
        LOG_DEBUG("MainWindow", QString("WPM increased to %1 (PgUp)").arg(newWpm));
        event->accept();
        return;
    }

    // PgDown: Decrease WPM by configurable increment
    if (event->key() == Qt::Key_PageDown) {
        // Only allow CW speed change in CW mode with radio connected
        bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
        if (!isCWMode || !m_radioConnected) {
            setStatusMessage("CW speed adjust requires CW mode and radio connection");
            event->accept();
            return;
        }

        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = m_currentState.cwSpeed;  // Read from radio's actual speed
        const int MIN_WPM = 8;  // K4 minimum
        int newWpm = qMax(currentWpm - increment, MIN_WPM);

        // Send to radio - display will update when radio responds via stateUpdated
        m_radio->setCWSpeed(newWpm);

        setStatusMessage(QString("CW Speed: %1 WPM").arg(newWpm));
        LOG_DEBUG("MainWindow", QString("WPM decreased to %1 (PgDn)").arg(newWpm));
        event->accept();
        return;
    }

    // ESC: Abort CW transmission
    if (event->key() == Qt::Key_Escape) {
        if (m_radioConnected) {
            m_radio->stopCW();
            setStatusMessage("CW transmission aborted");
            LOG_DEBUG("MainWindow", "CW transmission aborted via ESC key");
        }
        event->accept();
        return;
    }

    // Let parent handle other keys
    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // Delegate window activation events to WindowActivationHelper (Issue #76)
    // Handles: ApplicationActivate (macOS), WindowActivate on tracked child windows
    // Note: Check for null - eventFilter can be called during construction before helper is created
    if (m_windowActivationHelper && m_windowActivationHelper->handleEvent(obj, event)) {
        return true;
    }

    // Delegate keyboard events to InputHandlerService
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Build context for input handler
        InputHandlerService::KeyContext context;
        context.focusWidget = QApplication::focusWidget();
        context.callsignEntry = m_callsignEntry;
        context.exchangeEntry = m_exchangeEntry;
        context.callsignEmpty = m_callsignEntry->text().isEmpty();
        context.inSPMode = (m_operatingMode == OperatingMode::SP);
        context.lastCWMessage = m_lastCWMessage;

        if (m_inputHandler->handleKeyPress(keyEvent, context)) {
            return true;
        }
    }

    // Clear status message when exchange field gets focus
    if (event->type() == QEvent::FocusIn && obj == m_exchangeEntry) {
        setStatusMessage("Ready");
        return false;
    }

    // Radio Control window show/hide: Toggle detailed rig info (S-meter, temperature)
    if (obj == m_radioControlWindow) {
        if (event->type() == QEvent::Show) {
            if (m_radio && m_radioConnected) {
                if (K4Radio* k4 = qobject_cast<K4Radio*>(m_radio)) {
                    k4->setDetailedRigInfoEnabled(true);
                }
            }
        } else if (event->type() == QEvent::Hide) {
            if (m_radio && m_radioConnected) {
                if (K4Radio* k4 = qobject_cast<K4Radio*>(m_radio)) {
                    k4->setDetailedRigInfoEnabled(false);
                }
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::raiseAllWindows(QWidget* activatedWindow) {
    // Delegate to WindowActivationHelper (Issue #76)
    m_windowActivationHelper->raiseAllWindows(activatedWindow);
}

void MainWindow::setStatusMessage(const QString& message) {
    // Log all status messages for debugging
    LOG_WARN("MainWindow", QString("Status: %1").arg(message));

    // Truncate long messages to prevent overlap with permanent status widgets
    const int MAX_STATUS_LENGTH = 80;  // Approximate character limit
    QString displayMessage = message;
    if (message.length() > MAX_STATUS_LENGTH) {
        displayMessage = message.left(MAX_STATUS_LENGTH - 3) + "...";
    }
    m_statusLabel->setText(displayMessage);
    m_statusLabel->setToolTip(message);  // Full message in tooltip
}

void MainWindow::onStatusChanged(const QString& message, TR4QT::StatusStyle style) {
    // Apply style based on event type
    switch (style) {
        case TR4QT::StatusStyle::Warning:
            m_statusLabel->setStyleSheet("color: orange; font-weight: bold;");
            break;
        case TR4QT::StatusStyle::Error:
            m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
                .arg(ThemeManager::instance().colorName(ColorRole::DupeText)));
            break;
        case TR4QT::StatusStyle::Success:
            m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
                .arg(ThemeManager::instance().colorName(ColorRole::ConnectedStatus)));
            break;
        case TR4QT::StatusStyle::Highlight:
            m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
                .arg(ThemeManager::instance().colorName(ColorRole::MultiplierText)));
            break;
        case TR4QT::StatusStyle::Normal:
        default:
            m_statusLabel->setStyleSheet("");
            break;
    }

    // Use existing setStatusMessage for the actual display
    setStatusMessage(message);
}

void MainWindow::onRadioConfigure() {
    LOG_DEBUG("MainWindow", "*** onRadioConfigure() called - opening Preferences with Radio tab ***");
    PreferencesDialog dialog(this);
    dialog.selectCategory("Radio");
    dialog.setRadioConnected(m_radioConnected);

    if (dialog.exec() == QDialog::Accepted) {
        setStatusMessage("Radio configuration saved");

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

    // Show in a dialog with expandable details
    DialogHelper::informationWithDetails(
        this,
        "Radio Performance Report",
        "Performance comparison between K4 Direct and Hamlib interfaces:",
        report,
        "QTextEdit { min-width: 600px; min-height: 400px; }"  // Make dialog larger
    );
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
            setStatusMessage(QString("Resumed contest: %1").arg(contestInfo.contestName));
        } else {
            setStatusMessage(QString("Created new contest: %1").arg(contestInfo.contestName));
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
    bool hadRadioConfig = settings.hasAnyRadioConfig();
    if (hadRadioConfig) {
        oldConfig = settings.getActiveRadioConfig();
    }
    bool oldAutoConnect = settings.getRadioAutoConnect();
    QString oldStationProfile = settings.getActiveStationProfile();

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
        setStatusMessage("Preferences saved");

        // Apply font size changes immediately
        applyFontSettings();

        // Reload UDP broadcast settings
        loadUdpBroadcastSettings();

        // Reload license class for phone privilege validation
        QString licenseClassStr = settings.getLicenseClass();
        HamRadioPrivileges::LicenseClass licenseClass =
            HamRadioPrivileges::stringToLicenseClass(licenseClassStr);
        m_hamPrivileges = std::make_unique<HamRadioPrivileges>(licenseClass);
        LOG_DEBUG("MainWindow", QString("License class updated to: %1").arg(licenseClassStr));

        // Check if station profile changed
        QString newStationProfile = settings.getActiveStationProfile();
        bool stationProfileChanged = (oldStationProfile != newStationProfile);

        // Check if radio settings actually changed
        bool radioSettingsChanged = false;
        if (settings.hasAnyRadioConfig()) {
            RadioConfig newConfig = settings.getActiveRadioConfig();
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

        // Station profile change requires reconnection
        if (stationProfileChanged) {
            radioSettingsChanged = true;
            LOG_INFO("MainWindow", QString("Station profile changed from '%1' to '%2'")
                     .arg(oldStationProfile, newStationProfile));
        }

        // Only ask to reconnect if radio settings actually changed
        if (radioSettingsChanged && m_radioConnected) {
            QString message = stationProfileChanged
                ? QString("Station profile changed to '%1'. Reconnect to use the new radios?").arg(newStationProfile)
                : "Radio settings have changed. Reconnect to apply new settings?";

            QMessageBox::StandardButton reply = DialogHelper::question(
                this, "Reconnect Radio?", message,
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                onRadioDisconnect();
                QTimer::singleShot(UITiming::RECONNECT_DELAY_MS, this, &MainWindow::onRadioConnect);
            }
        }

        // Update amplifier menu state and initialize service if newly enabled
        bool amplifierEnabled = settings.getAmplifierEnabled();
        m_menuManager->amplifierControlAction()->setEnabled(amplifierEnabled);

        // Initialize amplifier service if enabled but not yet created
        if (amplifierEnabled && !m_amplifierService) {
            LOG_INFO("MainWindow", "Amplifier enabled in preferences - initializing service");
            int modelId = settings.getAmplifierModel();
            QString connectionType = settings.getAmplifierConnectionType();
            QString port = settings.getAmplifierPort();
            int baudRate = settings.getAmplifierBaudRate();

            AmplifierConfig config;
            config.hamlibModelId = modelId;
            config.connectionType = connectionType;
            config.port = port;
            config.baudRate = baudRate;
            config.pollIntervalMs = settings.getAmplifierPollInterval();

            // Determine amplifier type
            int amplifierType;
            const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
            if (connectionType == "direct" && modelId == AMP_MODEL_ELECRAFT_KPA1500) {
                amplifierType = static_cast<int>(AmplifierFactory::AmplifierType::KPA1500_DIRECT);
            } else {
                amplifierType = static_cast<int>(AmplifierFactory::AmplifierType::HAMLIB);
            }

            // Create amplifier controller (manages device in worker thread)
            m_amplifierController = new AmplifierController(this);

            // Create amplifier service (business logic layer)
            m_amplifierService = new AmplifierService(m_amplifierController, this);

            LOG_DEBUG("MainWindow", "Amplifier controller and service initialized from preferences");
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
        setStatusMessage(result.statusMessage);
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
        setStatusMessage(result.statusMessage);
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
        setStatusMessage(result.statusMessage);
    }
}

void MainWindow::onClearLog() {
    // Build request for MaintenanceService
    ClearLogRequest request;
    request.contestDbId = m_currentContestDbId;
    request.contestType = m_activeContest ? m_activeContest->getContestId() : QString();
    request.databasePath = m_currentContest.databasePath;
    request.qsoCount = m_qsoTableModel->count();

    // Call service to handle the workflow (dialogs, backup, database clear)
    ClearLogResult result = m_maintenanceService->clearLogWithBackup(request);

    // Handle result based on status
    switch (result.status) {
        case ClearLogResult::Status::AlreadyEmpty:
            DialogHelper::information(this, "Clear Log", "Log is already empty.");
            break;

        case ClearLogResult::Status::UserCancelled:
        case ClearLogResult::Status::BackupFailedUserAborted:
            // User cancelled - no action needed
            break;

        case ClearLogResult::Status::ClearFailed:
            DialogHelper::critical(this, "Error",
                QString("Failed to clear log from database: %1").arg(result.errorMessage));
            break;

        case ClearLogResult::Status::Success:
        case ClearLogResult::Status::BackupFailed:
            // Success (backup may have failed but user chose to continue)
            // Update UI state
            m_qsoTableModel->clear();
            m_lastQSOTime = QDateTime();
            m_qsosThisHour = 0;
            updateScoreDisplay();
            updateTimeDisplay();

            // Clear statistics window
            if (m_statisticsWindow) {
                m_statisticsWindow->clearStats();
            }

            if (result.backupCreated) {
                setStatusMessage(QString("Backup created: %1 - Log cleared")
                    .arg(QFileInfo(result.backupPath).fileName()));
            } else {
                setStatusMessage("Log cleared");
            }
            break;
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
    // Fast path: Update cached state and delegate to single display update function
    // This ensures consistent formatting regardless of update source
    m_currentState.frequencyA = freq;
    m_currentState.bandA = frequencyToBand(freq);

    // Single source of truth for display updates
    updateRadioStatusGrid();
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
    static QString lastPrivilegeWarning;
    if (m_hamPrivileges && state.frequencyA > 0) {
        QString warning = m_hamPrivileges->validatePhoneMode(
            state.frequencyA,
            state.bandA,
            state.modeA
        );
        if (!warning.isEmpty()) {
            // Display privilege warning in status bar with orange color
            setStatusMessage(QString("⚠ %1").arg(warning));
            m_statusLabel->setStyleSheet("color: orange; font-weight: bold;");
            // Only log once per unique warning to avoid flooding
            if (warning != lastPrivilegeWarning) {
                LOG_WARN("MainWindow", QString("Phone privilege violation: %1").arg(warning));
                lastPrivilegeWarning = warning;
            }
        } else {
            lastPrivilegeWarning.clear();  // Reset when no violation
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

    // Note: Radio Control windows are updated via radioStateUpdatedIndexed signal
    // to ensure each window gets the correct radio's state (not the active radio's state)

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
    // Legacy signal - the model will be updated via onRadioModelIdentified with radio index
    Q_UNUSED(model);
}

void MainWindow::onRadioModelIdentified(int radioIndex, const QString& model) {
    LOG_DEBUG("MainWindow", QString("MainWindow::onRadioModelIdentified: Radio %1 = %2")
              .arg(radioIndex + 1).arg(model));

    if (radioIndex >= 0 && radioIndex < 2) {
        m_radioModels[radioIndex] = model;
    }

    // Update the radio status label with current connected radios
    if (m_radioConnected) {
        updateRadioStatusLabel();
    }

    // Update Radio Control window title if it exists
    if (radioIndex == 0 && m_radioControlWindow) {
        m_radioControlWindow->setWindowTitle(QString("Radio 1: %1").arg(model));
    } else if (radioIndex == 1 && m_radio2ControlWindow) {
        m_radio2ControlWindow->setWindowTitle(QString("Radio 2: %1").arg(model));
    }
}

void MainWindow::updateRadioStatusLabel() {
    // Build status string showing connected radios
    QStringList radioInfo;

    if (!m_radioModels[0].isEmpty()) {
        radioInfo << QString("R1: %1").arg(m_radioModels[0]);
    }
    if (!m_radioModels[1].isEmpty()) {
        radioInfo << QString("R2: %1").arg(m_radioModels[1]);
    }

    if (radioInfo.isEmpty()) {
        m_radioStatusLabel->setText("Radio: Connected");
    } else {
        m_radioStatusLabel->setText(radioInfo.join(", "));
    }
    m_radioStatusLabel->setStyleSheet("color: green; font-weight: bold;");
}

void MainWindow::updateConnectionStatus(bool connected) {
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);

    if (connected) {
        // DO NOT call m_radio->getRadioModel() here - that's a cross-thread blocking call!
        // Radio models will be updated via onRadioModelIdentified() signal
        // For now show generic connected, will be updated when model is identified
        if (m_radioModels[0].isEmpty() && m_radioModels[1].isEmpty()) {
            m_radioStatusLabel->setText("Radio: Connecting...");
        } else {
            updateRadioStatusLabel();
        }
        m_radioStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        // Clear stored models on disconnect
        m_radioModels[0].clear();
        m_radioModels[1].clear();
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
        setStatusMessage("Error: No active contest - open a contest first");
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
                setStatusMessage(QString("Operator changed to: %1").arg(newOperator));
                LOG_INFO("MainWindow", QString("Operator changed to: %1").arg(newOperator));
            } else {
                setStatusMessage("Operator change cancelled (empty callsign)");
            }
        } else {
            setStatusMessage("Operator change cancelled");
        }
        onClearEntry();
        return true;
    }

    if (cmd.type == CommandDispatcher::RebroadcastLog) {
        onRebroadcastLog();
        onClearEntry();
        return true;
    }

    if (cmd.type == CommandDispatcher::FindQSO) {
        onClearEntry();
        executeSearch();
        return true;
    }

    return false;
}

QSOLoggingService::LogQSORequest MainWindow::buildLogQSORequest(const QString& callsign, const QString& exchange) {
    QSOLoggingService::LogQSORequest request;

    // Basic QSO data
    request.callsign = callsign;
    request.exchange = exchange;
    request.radioState = m_radioManager->currentState();  // Use active radio's state
    request.operatorCallsign = AppSettings::instance().getCurrentOperator();
    request.serialNumber = m_nextSerialNumber;
    request.operatingMode = m_operatingMode;
    request.radioNumber = m_radioManager->getActiveRadioIndex() + 1;  // 1-based radio number for SO2R

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
    setStatusMessage(result.errorMessage);
    m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
        .arg(ThemeManager::instance().colorName(ColorRole::ValidationErrorBorder)));
    QApplication::beep();

    // Set focus to appropriate field based on structured error enum
    using ErrorField = QSOLoggingService::ErrorField;
    switch (result.errorField) {
        case ErrorField::Callsign:
            m_callsignEntry->setFocus();
            break;
        case ErrorField::Exchange:
            m_exchangeEntry->setFocus();
            break;
        case ErrorField::Frequency:
        case ErrorField::Mode:
        case ErrorField::Database:
        case ErrorField::None:
        default:
            // No specific field to focus, keep current focus
            break;
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

    setStatusMessage(statusMsg);
    m_statusLabel->setStyleSheet("");

    // Update statistics window with new QSO
    if (m_statisticsWindow) {
        QString operatorCall = qso.operatorCall.isEmpty() ?
            AppSettings::instance().getCurrentOperator() : qso.operatorCall;
        QString stationId = m_radioManager->isSO2REnabled() ?
            QString("RADIO%1").arg(m_radioManager->getActiveRadioIndex() + 1) : "RADIO1";
        m_statisticsWindow->addQso(qso.timestamp, qso.band, qso.mode, operatorCall, stationId);
    }

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
        setStatusMessage("Ready");  // Clear any status messages
        return;
    }

    // Get needs display data (worked bands + multiplier bands with AllBands handling)
    QList<QSO> qsos = m_qsoTableModel ? m_qsoTableModel->getAllQSOs() : QList<QSO>();
    bool vhfEnabled = AppSettings::instance().getVHFBandsEnabled();
    NeedsDisplayData needsData = m_qsoQueryService->getNeedsDisplayData(
        qsos, callsign, m_activeContest.get(), m_stationInfoService.get(), vhfEnabled);

    // Update the needs display widget
    m_needsDisplayWidget->updateForCallsign(
        callsign, m_activeContest.get(), needsData.workedBands, needsData.workedMultBands);

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
    auto& theme = ThemeManager::instance();

    if (isValid) {
        // Green border for valid exchange
        styleSheet = QString("QLineEdit { border: 2px solid %1; background-color: %2; }")
            .arg(theme.colorName(ColorRole::ValidationValidBorder))
            .arg(theme.colorName(ColorRole::ValidationValidBackground));
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
            styleSheet = QString("QLineEdit { border: 2px solid %1; background-color: %2; }")
                .arg(theme.colorName(ColorRole::ValidationWarningBorder))
                .arg(theme.colorName(ColorRole::ValidationWarningBackground));
            tooltip = "⚠ Incomplete - " + errorMsg;
        } else {
            // Red border for invalid exchange
            styleSheet = QString("QLineEdit { border: 2px solid %1; background-color: %2; }")
                .arg(theme.colorName(ColorRole::ValidationErrorBorder))
                .arg(theme.colorName(ColorRole::ValidationErrorBackground));
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
        setStatusMessage("Error: " + freqResult.errorMessage);
        onClearEntry();
        return;
    }

    if (freqResult.isFrequency) {
        // Set radio frequency
        if (m_radio && m_radioConnected) {
            m_radio->setFrequency(freqResult.frequencyHz);
            setStatusMessage(freqResult.statusMessage);
            LOG_INFO("MainWindow", QString("Frequency changed via numeric entry: %1").arg(freqResult.statusMessage));
        } else {
            setStatusMessage("Error: Radio not connected");
        }

        // Clear entry and return (don't process as callsign)
        onClearEntry();
        return;
    }

    // Check for "CALL /find" pattern: callsign followed by /find suffix
    // e.g. "W1AW /find" → search log for W1AW
    if (callsign.endsWith(" /FIND") || callsign.endsWith("/FIND")) {
        QString searchCall = callsign.left(callsign.lastIndexOf("/FIND")).trimmed();
        if (!searchCall.isEmpty()) {
            onClearEntry();
            searchForCallsign(searchCall);
            return;
        }
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
        setStatusMessage("⚠ " + dupeInfo);
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
            .arg(ThemeManager::instance().colorName(ColorRole::WarningText)));
    } else {
        // Clear warning for non-duplicates
        setStatusMessage("Ready");
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
    setStatusMessage("Ready");
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
    EditQSODialog dialog(qso, m_activeContest.get(), this);
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
    ScoreResult result = m_scoreCalculationService->calculateScore(qsos, m_activeContest.get());

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
    setStatusMessage(QString("%1 QSOs, %2 Points").arg(result.totalQSOs).arg(result.finalScore));

    // Update Statistics window with scoring totals
    // (Per-band-mode points come from QSO data via RateCalculator)
    if (m_statisticsWindow) {
        m_statisticsWindow->updateScoringData(
            result.totalMultipliers, result.totalQSOPoints, result.finalScore);
    }
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
    RescoreStats stats = m_integrityManager->rescoreContestSilent(qsos, m_activeContest.get(), myStation);

    // Update table model with rescored QSOs
    for (int row = 0; row < qsos.size(); ++row) {
        m_qsoTableModel->updateQSO(row, qsos[row]);
    }

    LOG_INFO("MainWindow", QString("Recalculated points for %1 QSOs").arg(stats.qsosUpdated));

    // Update display
    updateScoreDisplay();

    // Show result to user
    setStatusMessage(QString("Recalculated points for %1 QSOs").arg(stats.qsosUpdated));
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
    stats = m_integrityManager->rescoreContestSilent(qsos, m_activeContest.get(), myStation);

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

    setStatusMessage("Rescoring contest...");
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

    setStatusMessage(QString("Rescore complete: %1 QSOs updated, %2 mults marked, %3 dupes found")
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

    setStatusMessage("Updating contest exchange...");
    QApplication::processEvents();

    ContestService::UpdateExchangeResult result = m_contestService->updateContestExchange(newExchange);

    if (!result.success) {
        DialogHelper::critical(this, "Error", result.errorMessage);
        setStatusMessage("Failed to update contest exchange");
        return;
    }

    // Update status label
    setStatusMessage(result.statusMessage);

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

    setStatusMessage("Running full integrity check...");
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
    reportText->setFont(FontManager::instance().monospaceFont(10));
    layout->addWidget(reportText);

    QPushButton* closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);

    dialog->exec();
    delete dialog;

    setStatusMessage("Integrity check complete");
}

// UDP command: Rebroadcast entire log
void MainWindow::onRebroadcastLog() {
    if (!m_hasActiveContest || !m_qsoTableModel) {
        setStatusMessage("Error: No active contest to rebroadcast");
        return;
    }

    if (!m_udpBroadcastManager->isEnabled()) {
        setStatusMessage("Error: UDP broadcasting is disabled");
        return;
    }

    int totalQSOs = m_qsoTableModel->count();
    if (totalQSOs == 0) {
        setStatusMessage("No QSOs to rebroadcast");
        return;
    }

    LOG_INFO("MainWindow", QString("Starting UDP rebroadcast of %1 QSOs").arg(totalQSOs));
    setStatusMessage(QString("Starting UDP rebroadcast of %1 QSOs...").arg(totalQSOs));

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
                    setStatusMessage(QString("UDP rebroadcast: %1/%2 (25%)")
                        .arg(sent).arg(totalQSOs));
                }, Qt::QueuedConnection);
            } else if (sent == quarter * 2) {
                QMetaObject::invokeMethod(this, [this, sent, totalQSOs]() {
                    setStatusMessage(QString("UDP rebroadcast: %1/%2 (50%)")
                        .arg(sent).arg(totalQSOs));
                }, Qt::QueuedConnection);
            } else if (sent == quarter * 3) {
                QMetaObject::invokeMethod(this, [this, sent, totalQSOs]() {
                    setStatusMessage(QString("UDP rebroadcast: %1/%2 (75%)")
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
            setStatusMessage(QString("UDP rebroadcast complete: %1 QSOs sent").arg(totalQSOs));
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
    // Helper lambda to format frequency in kHz (e.g., "7011.00" or "3524.90")
    auto formatFreqKHz = [](freq_t freq) -> QString {
        if (freq == 0) return "0.00";
        double freqKHz = freq / 1000.0;
        return QString::number(freqKHz, 'f', 2);
    };

    // Check if SO2R mode is enabled
    bool so2rEnabled = m_radioManager->isSO2REnabled();
    bool hasTwoRadios = m_radioManager->getConnectedRadioCount() > 1;

    // Show standby frequency label when SO2R is enabled (even if only 1 radio connected)
    m_standbyFreqLabel->setVisible(so2rEnabled);

    // Get active radio state (m_currentState is already the active radio's state)
    RadioState activeState = m_radioManager->currentState();

    // Update band/mode label for active radio (e.g., "20m SSB" or "40CW")
    if (activeState.frequencyA > 0) {
        if (activeState.bandA != BandType::None) {
            QString bandStr = bandToString(activeState.bandA);  // ADIF format: "20m", "70cm", etc.
            QString modeStr = modeToString(activeState.modeA);
            m_radioFreqBandLabel->setText(QString("%1 %2").arg(bandStr).arg(modeStr));
        } else {
            QString modeStr = modeToString(activeState.modeA);
            m_radioFreqBandLabel->setText(modeStr);
        }
    } else {
        m_radioFreqBandLabel->setText("--");
    }

    // Update SO2R frequency displays
    if (so2rEnabled) {
        // Get radio states by fixed index (Radio 1 = index 0, Radio 2 = index 1)
        // Radio 1 always on top, Radio 2 always on bottom (regardless of active status)
        RadioState radio1State = m_radioManager->getRadioState(0);
        RadioState radio2State = m_radioManager->getRadioState(1);
        int activeIndex = m_radioManager->getActiveRadioIndex();

        // Radio 1 frequency (top label) - show in kHz like TR4W
        if (radio1State.frequencyA > 0) {
            m_standbyFreqLabel->setText(formatFreqKHz(radio1State.frequencyA));
        } else {
            m_standbyFreqLabel->setText("--");
        }
        m_standbyFreqLabel->setEnabled(activeIndex == 0);  // Bright if Radio 1 is active

        // Radio 2 frequency (bottom label) - show in kHz like TR4W
        if (hasTwoRadios && radio2State.frequencyA > 0) {
            m_radioFreqLabel->setText(formatFreqKHz(radio2State.frequencyA));
        } else {
            m_radioFreqLabel->setText("--");
        }
        m_radioFreqLabel->setEnabled(activeIndex == 1);  // Bright if Radio 2 is active
    } else {
        // Single radio mode - show frequency in MHz format
        if (activeState.frequencyA > 0) {
            double freqMHz = activeState.frequencyA / 1000000.0;
            double truncated = std::floor(freqMHz * 1000.0) / 1000.0;
            m_radioFreqLabel->setText(QString("%1 MHz").arg(truncated, 0, 'f', 3));
        } else {
            m_radioFreqLabel->setText("0.000 MHz");
        }
    }

    // Update WPM label (only enabled in CW mode AND when auto-send is enabled)
    bool isCWMode = (activeState.modeA == ModeType::CW || activeState.modeA == ModeType::CWR);
    bool autoSendEnabled = m_autoSendCWAction->isChecked();
    int wpm = activeState.cwSpeed;
    m_radioWpmLabel->setText(QString("%1 WPM").arg(wpm));
    m_radioWpmLabel->setEnabled(isCWMode && autoSendEnabled);

    // Update date/time (UTC - contest logging standard)
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString dateStr = now.toString("dd-MMM-yy");  // Compact format like TR4W (e.g., "29-Jan-26")
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
    if (m_radio2ControlAction) {
        m_radio2ControlAction->setChecked(m_radio2ControlWindow && m_radio2ControlWindow->isVisible());
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

#ifdef PANADAPTER_ENABLED
    // Panadapter action
    QAction* panadapterAction = m_menuManager->panadapterAction();
    if (panadapterAction) {
        panadapterAction->setChecked(m_panadapterWindow && m_panadapterWindow->isVisible());
    }
#endif
}

void MainWindow::applyFontSettings() {
    AppSettings& settings = AppSettings::instance();

    // Apply entry field font sizes
    int entryFontSize = settings.getEntryFontSize();
    QFont entryFont = FontManager::instance().monospaceFont(entryFontSize);
    m_callsignEntry->setFont(entryFont);
    m_exchangeEntry->setFont(entryFont);

    // Apply QSO table font size
    int tableFontSize = settings.getTableFontSize();
    QFont tableFont = FontManager::instance().monospaceFont(tableFontSize);
    m_qsoTableView->setFont(tableFont);

    // Keep search panel in sync with main table font
    if (m_searchPanel) {
        m_searchPanel->syncAppearance(m_qsoTableView);
    }

    // Apply band summary grid font size
    int gridFontSize = settings.getGridFontSize();
    if (m_bandSummaryGrid) {
        m_bandSummaryGrid->setFontSize(gridFontSize);
    }

    // Apply misc display font size (stats panel: This Hr, Rate, Op, etc.)
    int miscFontSize = settings.getMiscDisplayFontSize();
    QFont miscFont = FontManager::instance().monospaceFont(miscFontSize);
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
    m_scpMatchesLabel->setStyleSheet(QString("QLabel { color: %1; font-size: %2pt; }")
        .arg(ThemeManager::instance().colorName(ColorRole::SCPMatchText))
        .arg(scpFontSize));
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

    // Style both frequency labels consistently (standby will be grayed via setEnabled)
    m_radioFreqLabel->setStyleSheet(freqLabelStyle);
    m_standbyFreqLabel->setStyleSheet(freqLabelStyle);
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
    setStatusMessage(QString("Reopened: %1").arg(contestInfo.contestName));

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
    m_activeContest.reset(result.contest);  // Take ownership of contest created by ContestRegistry
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

    // Load history into Statistics window
    if (m_statisticsWindow) {
        m_statisticsWindow->clearStats();
        m_statisticsWindow->setContestStartTime(contestInfo.startDate);

        // Convert QSOs to rate calculator records
        QVector<RateCalculator::QsoRecord> records;
        records.reserve(result.loadedQSOs.size());
        for (const QSO& qso : result.loadedQSOs) {
            RateCalculator::QsoRecord record;
            record.timestamp = qso.timestamp;
            record.band = qso.band;
            record.mode = qso.mode;
            record.operatorCall = qso.operatorCall.isEmpty() ?
                AppSettings::instance().getCurrentOperator() : qso.operatorCall;
            record.stationId = "RADIO1";  // Historical QSOs don't have station ID
            records.append(record);
        }
        m_statisticsWindow->loadHistory(records);
        LOG_DEBUG("MainWindow", QString("Loaded %1 QSOs into Statistics window").arg(records.size()));
    }

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

    // Clean up previous contest (unique_ptr handles deletion automatically)
    if (m_activeContest) {
        m_activeContest.reset();  // Release and destroy the contest

        if (m_dxClusterWindow) {
            m_dxClusterWindow->setActiveContest(nullptr, -1);
        }
    }
}

void MainWindow::createContestServices(const ActivateContestResult& result) {
    // Create QSOLogger (unique_ptr handles cleanup of previous instance)
    QSOLogger::Config loggerConfig;
    loggerConfig.contest = m_activeContest.get();
    loggerConfig.countryFile = &m_countryFile;
    loggerConfig.myStation = result.myStation;
    loggerConfig.operatorName = result.operatorName;  // For {NAME} substitution
    m_qsoLogger = std::make_unique<QSOLogger>(loggerConfig);
    LOG_DEBUG("MainWindow", "QSOLogger created for contest");

    // Create DataIntegrityManager
    DataIntegrityManager::Config integrityConfig;
    integrityConfig.countryFile = &m_countryFile;
    integrityConfig.currentContestDbId = m_currentContestDbId;
    m_integrityManager = std::make_unique<DataIntegrityManager>(integrityConfig);
    LOG_DEBUG("MainWindow", "DataIntegrityManager created for contest");

    // Create ContestService
    ContestService::Config contestServiceConfig;
    contestServiceConfig.activeContest = m_activeContest.get();
    contestServiceConfig.qsoTableModel = m_qsoTableModel;
    contestServiceConfig.currentContestDbId = m_currentContestDbId;
    m_contestService = std::make_unique<ContestService>(contestServiceConfig);
    LOG_DEBUG("MainWindow", "ContestService created for contest");

    // Create QSOLoggingCoordinator (orchestrates post-logging actions)
    m_loggingCoordinator = std::make_unique<QSOLoggingCoordinator>(
        m_udpBroadcastManager,
        &BackupManager::instance(),
        m_integrityManager.get()
    );
    LOG_DEBUG("MainWindow", "QSOLoggingCoordinator created for contest");

    // Create QSOPersistenceService
    QSOPersistenceService::Config persistenceConfig;
    persistenceConfig.appDataDir = PathManager::getAppDataDir();
    m_persistenceService = std::make_unique<QSOPersistenceService>(persistenceConfig);
    LOG_DEBUG("MainWindow", "QSOPersistenceService created for contest");

    // Create ExchangeMemoryService
    m_exchangeMemoryService = std::make_unique<ExchangeMemoryService>();
    LOG_DEBUG("MainWindow", "ExchangeMemoryService created for contest");

    // Create QSOLoggingService (orchestrates complete logging workflow)
    QSOLoggingService::Dependencies loggingDeps;
    loggingDeps.qsoLogger = m_qsoLogger.get();
    loggingDeps.persistenceService = m_persistenceService.get();
    loggingDeps.exchangeMemoryService = m_exchangeMemoryService.get();
    loggingDeps.coordinator = m_loggingCoordinator.get();
    m_loggingService = std::make_unique<QSOLoggingService>(loggingDeps);
    LOG_DEBUG("MainWindow", "QSOLoggingService created for contest");

    // Update ImportExportManager (if it exists - may not during startup)
    if (m_importExportManager) {
        ImportExportManager::Config importExportConfig;
        importExportConfig.countryFile = &m_countryFile;
        importExportConfig.qsoTableModel = m_qsoTableModel;
        importExportConfig.activeContest = m_activeContest.get();
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
        m_dxClusterWindow->setActiveContest(m_activeContest.get(), m_currentContestDbId);
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
        m_activeContest.get(),
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
    return m_qsoQueryService->getWorkedBandsForMultiplier(qsos, multValue, type, m_activeContest.get());
}

QString MainWindow::getMultiplierValueForCallsign(const QString& callsign) const {
    // Delegate to StationInfoService (Phase 7 extraction)
    return m_stationInfoService->getMultiplierValueForCallsign(callsign, m_activeContest.get());
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
            m_dxClusterWindow->setActiveContest(m_activeContest.get(), m_currentContestDbId);
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

        // Track window for activation behavior (Issue #76)
        m_windowActivationHelper->trackWindow(m_dxClusterWindow);

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

        // Track window for activation behavior (Issue #76)
        m_windowActivationHelper->trackWindow(m_bandMapWindow);
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
        // Set window title with radio model name if known
        QString title = m_radioModels[0].isEmpty()
            ? "Radio 1 Control"
            : QString("Radio 1: %1").arg(m_radioModels[0]);
        m_radioControlWindow->setWindowTitle(title);
        m_radioControlWindow->setRadioNumber(1);
        m_radioControlWindow->setActive(m_radioManager->getActiveRadioIndex() == 0);  // Set initial active state
        m_radioControlWindow->setWindowFlags(Qt::Window);
        m_radioControlWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Set radio controller reference for mode menu
        m_radioControlWindow->setRadioController(m_radio);

        // Set max power from radio (signal may have fired before widget existed)
        if (m_radio && m_radio->isConnected()) {
            m_radioControlWindow->setMaxPower(m_radio->maxPowerWatts());
        }

        // Connect mode change requests (supports manual override when radio can't set mode)
        connect(m_radioControlWindow, &RadioControlWidget::modeChangeRequested,
                this, [this](ModeType mode) {
                    LOG_INFO("MainWindow", QString("Mode change requested from radio control: %1").arg(modeToString(mode)));

                    // Always update local state (enables manual override for radios that don't support mode setting)
                    m_currentState.modeA = mode;

                    // Update the RadioControlWidget display immediately
                    if (m_radioControlWindow) {
                        m_radioControlWindow->updateRadioState(m_currentState);
                    }

                    // Try to set on radio if connected (may fail silently for unsupported radios)
                    if (m_radio && m_radio->isConnected()) {
                        m_radio->setMode(mode, VFO::VFO_A);
                    }
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

        // Connect radio status messages (K4 ER command) to status display
        connect(m_radio, &RadioController::statusMessageReceived,
                m_radioControlWindow, &RadioControlWidget::showStatusMessage);

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

        // Track window for activation behavior (Issue #76)
        m_windowActivationHelper->trackWindow(m_radioControlWindow);
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

void MainWindow::onShowRadio2Control() {
    // Only available when SO2R is enabled
    if (!m_radioManager->isSO2REnabled()) {
        setStatusMessage("SO2R not enabled - configure in Preferences → Hardware → Radio");
        return;
    }

    if (!m_radio2ControlWindow) {
        m_radio2ControlWindow = new RadioControlWidget();
        // Set window title with radio model name if known
        QString title = m_radioModels[1].isEmpty()
            ? "Radio 2 Control"
            : QString("Radio 2: %1").arg(m_radioModels[1]);
        m_radio2ControlWindow->setWindowTitle(title);
        m_radio2ControlWindow->setRadioNumber(2);
        m_radio2ControlWindow->setActive(m_radioManager->getActiveRadioIndex() == 1);  // Set initial active state
        m_radio2ControlWindow->setWindowFlags(Qt::Window);
        m_radio2ControlWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Get Radio 2 controller from RadioManager
        RadioController* radio2 = m_radioManager->getRadioController(1);  // 0-indexed
        m_radio2ControlWindow->setRadioController(radio2);

        // Set max power from radio if connected
        if (radio2 && radio2->isConnected()) {
            m_radio2ControlWindow->setMaxPower(radio2->maxPowerWatts());
        }

        // Connect mode change requests (supports manual override when radio can't set mode)
        connect(m_radio2ControlWindow, &RadioControlWidget::modeChangeRequested,
                this, [this, radio2](ModeType mode) {
                    LOG_INFO("MainWindow", QString("Mode change requested from Radio 2 control: %1").arg(modeToString(mode)));

                    // Update the RadioControlWidget display immediately for manual override
                    if (m_radio2ControlWindow) {
                        RadioState state;
                        if (radio2) {
                            state = radio2->getCurrentState();
                        }
                        state.modeA = mode;
                        m_radio2ControlWindow->updateRadioState(state);
                    }

                    // Try to set on radio if connected (may fail silently for unsupported radios)
                    if (radio2 && radio2->isConnected()) {
                        radio2->setMode(mode, VFO::VFO_A);
                    }
                });

        // Connect CW speed change requests
        connect(m_radio2ControlWindow, &RadioControlWidget::cwSpeedChangeRequested,
                this, [this, radio2](int wpm) {
                    LOG_INFO("MainWindow", QString("CW speed change requested from Radio 2 control: %1 WPM").arg(wpm));
                    if (radio2) radio2->setCWSpeed(wpm);
                });

        // Connect RIT/XIT/SPLIT toggle requests
        connect(m_radio2ControlWindow, &RadioControlWidget::ritToggled,
                this, [this, radio2](bool enabled) {
                    LOG_INFO("MainWindow", QString("Radio 2 RIT toggle requested: %1").arg(enabled ? "ON" : "OFF"));
                    if (radio2) radio2->enableRIT(enabled, VFO::VFO_A);
                });

        connect(m_radio2ControlWindow, &RadioControlWidget::xitToggled,
                this, [this, radio2](bool enabled) {
                    LOG_INFO("MainWindow", QString("Radio 2 XIT toggle requested: %1").arg(enabled ? "ON" : "OFF"));
                    if (radio2) radio2->enableXIT(enabled, VFO::VFO_A);
                });

        connect(m_radio2ControlWindow, &RadioControlWidget::splitToggled,
                this, [this, radio2](bool enabled) {
                    LOG_INFO("MainWindow", QString("Radio 2 SPLIT toggle requested: %1").arg(enabled ? "ON" : "OFF"));
                    if (radio2) radio2->setSplit(enabled, VFO::VFO_B);
                });

        // Connect close/hide event
        connect(m_radio2ControlWindow, &QWidget::destroyed, this, [this]() {
            m_radio2ControlWindowVisible = false;
        });

        // Install event filter to catch show/hide events
        m_radio2ControlWindow->installEventFilter(this);

        // Update with current Radio 2 state
        RadioState radio2State = m_radioManager->getRadioState(1);  // 0-indexed
        if (m_radioManager->isRadioConnected(1)) {
            m_radio2ControlWindow->updateRadioState(radio2State);
        }
    }
    m_radio2ControlWindow->show();
    m_radio2ControlWindow->raise();
    m_radio2ControlWindow->activateWindow();
    m_radio2ControlWindowVisible = true;
    updateWindowMenuCheckmarks();
}

void MainWindow::onShowMultipliers() {
    if (!m_multiplierWindow) {
        m_multiplierWindow = new MultiplierWidget();
        m_multiplierWindow->setWindowTitle("Multipliers");
        m_multiplierWindow->setWindowFlags(Qt::Window);
        m_multiplierWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Track window for activation behavior (Issue #76)
        m_windowActivationHelper->trackWindow(m_multiplierWindow);
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

        // Track window for activation behavior (Issue #76)
        m_windowActivationHelper->trackWindow(m_statisticsWindow);
    }

    // Always reload contest data when showing the window to ensure fresh data
    loadStatisticsWindowData();

    m_statisticsWindow->show();
    m_statisticsWindow->raise();
    m_statisticsWindow->activateWindow();
    updateWindowMenuCheckmarks();
}

void MainWindow::loadStatisticsWindowData() {
    if (!m_statisticsWindow) return;

    m_statisticsWindow->clearStats();

    if (!m_hasActiveContest || !m_qsoTableModel) {
        return;  // No contest, leave stats empty
    }

    m_statisticsWindow->setContestStartTime(m_currentContest.startDate);

    // Load all QSOs from the table model
    QVector<RateCalculator::QsoRecord> records;
    int qsoCount = m_qsoTableModel->count();
    records.reserve(qsoCount);

    for (int i = 0; i < qsoCount; ++i) {
        QSO qso = m_qsoTableModel->getQSO(i);
        RateCalculator::QsoRecord record;
        record.timestamp = qso.timestamp;
        record.band = qso.band;
        record.mode = qso.mode;
        record.operatorCall = qso.operatorCall.isEmpty() ?
            AppSettings::instance().getCurrentOperator() : qso.operatorCall;
        record.stationId = "RADIO1";
        record.qsoPoints = qso.qsoPoints;  // Points already calculated when QSO was logged
        record.isDupe = qso.isDupe;
        records.append(record);
    }
    m_statisticsWindow->loadHistory(records);
    LOG_DEBUG("MainWindow", QString("Loaded %1 QSOs into Statistics window").arg(records.size()));

    // Also update scoring data - this populates Mults, Points, Score columns
    updateScoreDisplay();
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
            QPoint offset(UIPositioning::WINDOW_INITIAL_OFFSET, UIPositioning::WINDOW_INITIAL_OFFSET);
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

#ifdef PANADAPTER_ENABLED
void MainWindow::onShowPanadapter() {
    // Create and show panadapter window
    if (!m_panadapterWindow) {
        m_panadapterWindow = new PanadapterWindow(this);
        m_panadapterWindow->setWindowFlags(Qt::Window);
        m_panadapterWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        // Position offset from main window (first time)
        QPoint offset(UIPositioning::WINDOW_INITIAL_OFFSET, UIPositioning::WINDOW_INITIAL_OFFSET);
        m_panadapterWindow->move(this->pos() + offset);

        // Connect frequency click to tune radio
        connect(m_panadapterWindow, &PanadapterWindow::frequencyClicked, this,
                [this](qint64 freqHz, int vfo) {
            // Tune the radio to the clicked frequency
            if (m_radio && m_radio->isConnected()) {
                VFO targetVfo = (vfo == 0) ? VFO::VFO_A : VFO::VFO_B;
                m_radio->setFrequency(static_cast<freq_t>(freqHz), targetVfo);
            }
        });

        // Connect destroyed signal to clear pointer
        connect(m_panadapterWindow, &QWidget::destroyed, this, [this]() {
            m_panadapterWindow = nullptr;
            m_panadapterWindowVisible = false;
        });

        // Connect windowClosed signal to track when user manually closes window
        connect(m_panadapterWindow, &PanadapterWindow::windowClosed, this, [this]() {
            m_panadapterWindowVisible = false;
            updateWindowMenuCheckmarks();
        });

        // Auto-connect to K4 panadapter if available
        m_panadapterWindow->connectToK4();
    }

    m_panadapterWindow->show();
    m_panadapterWindow->raise();
    m_panadapterWindow->activateWindow();
    m_panadapterWindowVisible = true;
    updateWindowMenuCheckmarks();
}
#else
void MainWindow::onShowPanadapter() {
    DialogHelper::information(this, "Panadapter Not Available",
        "Panadapter requires Qt 6.6 or later.\n\n"
        "This build was compiled with an older Qt version.");
}
#endif

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
    CWMessageEditorDialog dialog(m_radio, m_activeContest.get(), this);
    dialog.exec();
    LOG_DEBUG("MainWindow", "CW Messages Editor closed");
}

void MainWindow::onShowKeyerSetup() {
    KeyerSetupDialog dialog(m_keyerController, m_iambicKeyer, m_radio, this);
    dialog.exec();
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
        setStatusMessage(result.statusMessage);
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
        setStatusMessage(result.statusMessage);
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
        setStatusMessage("Web server stopped");
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
            setStatusMessage(QString("Web server started: %1").arg(url));
            LOG_INFO("MainWindow", QString("Web server started: %1").arg(url));

            // Show info dialog with URL
            DialogHelper::information(this, "Web Server Started",
                QString("Web server is now running at:\n\n%1\n\n"
                        "Access the dashboard from any browser on this computer.\n\n"
                        "To access from other devices, change the binding address in Preferences.")
                    .arg(url));
        } else {
            // Manual start failed - show error dialog
            setStatusMessage("Failed to start web server");
            LOG_ERROR("MainWindow", QString("Failed to start web server on %1:%2").arg(addressStr).arg(port));

            DialogHelper::warning(this, "Web Server Start Failed",
                QString("Failed to start web server on %1:%2\n\n"
                        "Port may already be in use by another instance of TR4QT or another application.\n\n"
                        "Please check if TR4QT is already running, or try a different port in Preferences.")
                    .arg(addressStr).arg(port));
        }
    }
}

// === WebServer Command API handlers ===

void MainWindow::onLogQSOFromWeb(const LogQSOWebRequest& request, LogQSOWebResponse* response) {
    LOG_DEBUG("MainWindow", QString("Processing log QSO from web: %1").arg(request.callsign));

    // Check for active contest
    if (!m_loggingService || !m_hasActiveContest) {
        response->success = false;
        response->error = "No active contest - open a contest first";
        LOG_WARN("MainWindow", "Web log QSO rejected: no active contest");
        return;
    }

    // Build the logging request using the existing method
    // Start with callsign and exchange from web request
    QString callsign = request.callsign;
    QString exchange = request.exchange;

    // Build the logging service request
    QSOLoggingService::LogQSORequest loggingRequest;

    // Basic QSO data
    loggingRequest.callsign = callsign;
    loggingRequest.exchange = exchange;

    // Use web request values if provided, otherwise use current radio state
    RadioState radioState = m_radioManager->currentState();

    if (request.frequency > 0) {
        radioState.frequencyA = request.frequency;
    }
    if (request.band != BandType::None) {
        radioState.bandA = request.band;
    }
    if (request.mode != ModeType::None) {
        radioState.modeA = request.mode;
    }

    loggingRequest.radioState = radioState;
    loggingRequest.operatorCallsign = AppSettings::instance().getCurrentOperator();
    loggingRequest.serialNumber = m_nextSerialNumber;
    loggingRequest.operatingMode = m_operatingMode;
    loggingRequest.radioNumber = m_radioManager->getActiveRadioIndex() + 1;

    // Existing QSOs for duplicate/multiplier checking
    loggingRequest.existingQSOs = m_qsoTableModel->getAllQSOs();

    // Exchange memory settings - don't save from web API (external program manages this)
    loggingRequest.saveExchangeMemory = false;
    loggingRequest.autoPopulated = true;  // Treat as auto-populated (no exchange memory save)

    // Context for post-logging actions
    loggingRequest.stationCallsign = AppSettings::instance().getMyCallsign();
    loggingRequest.adifContestId = m_activeContest ? m_activeContest->getADIFContestId() : "";
    loggingRequest.wa7bnmContestId = m_activeContest ? m_activeContest->getWA7BNMContestId() : 0;
    loggingRequest.contestId = m_activeContest ? m_activeContest->getContestId() : "";
    loggingRequest.databasePath = m_currentContest.databasePath;
    loggingRequest.totalQSOCount = m_qsoTableModel->count() + 1;
    loggingRequest.qsosSinceLastCheck = m_qsosSinceLastIntegrityCheck + 1;
    loggingRequest.contestDbId = m_currentContestDbId;
    loggingRequest.memoryQSOCount = m_qsoTableModel->count() + 1;

    // Execute logging workflow
    QSOLoggingService::LogQSOResult result = m_loggingService->logQSO(loggingRequest);

    // Map result to web response
    if (!result.success) {
        response->success = false;
        response->error = result.errorMessage;

        // Map error field
        using ErrorField = QSOLoggingService::ErrorField;
        switch (result.errorField) {
            case ErrorField::Callsign:
                response->errorField = "callsign";
                break;
            case ErrorField::Exchange:
                response->errorField = "exchange";
                break;
            case ErrorField::Frequency:
                response->errorField = "frequency";
                break;
            case ErrorField::Mode:
                response->errorField = "mode";
                break;
            case ErrorField::Database:
                response->errorField = "database";
                break;
            default:
                break;
        }

        LOG_WARN("MainWindow", QString("Web log QSO failed: %1").arg(result.errorMessage));
        return;
    }

    // Success - populate response with QSO details
    response->success = true;
    response->qsoId = result.qso.id;
    response->callsign = result.qso.callsign;
    response->timestamp = result.qso.timestamp;
    response->frequency = result.qso.frequency;
    response->band = bandToString(result.qso.band);
    response->mode = modeToString(result.qso.mode);
    response->exchangeSent = result.qso.exchangeSent;
    response->exchangeReceived = result.qso.exchangeReceived;
    response->points = result.qso.qsoPoints;
    response->isMultiplier = result.isNewMultiplier;
    response->isDuplicate = result.isDuplicate;
    response->serialNumber = result.updatedSerialNumber;

    // Update UI after successful logging (same as onLogQSO)
    updateUIAfterQSOLogged(result.qso, result);

    LOG_INFO("MainWindow", QString("Web log QSO success: %1 on %2 %3")
             .arg(result.qso.callsign)
             .arg(bandToString(result.qso.band))
             .arg(modeToString(result.qso.mode)));
}

void MainWindow::onCommandFromWeb(const CommandWebRequest& request, CommandWebResponse* response) {
    LOG_DEBUG("MainWindow", QString("Processing command from web: %1").arg(request.command));

    response->command = request.command;

    // Helper lambda to resolve radio index from "radio" parameter
    // Returns: 0 or 1 for valid radio, -1 for error (error message set in response)
    auto resolveRadioIndex = [this, response](const QVariantMap& params) -> int {
        QVariant radioParam = params.value("radio");

        // Default to active radio if not specified
        if (!radioParam.isValid() || radioParam.isNull()) {
            return m_radioManager ? m_radioManager->getActiveRadioIndex() : 0;
        }

        QString radioStr = radioParam.toString().toLower();

        if (radioStr == "active" || radioStr.isEmpty()) {
            return m_radioManager ? m_radioManager->getActiveRadioIndex() : 0;
        }
        if (radioStr == "standby") {
            return m_radioManager ? m_radioManager->getStandbyRadioIndex() : 1;
        }
        if (radioStr == "1") {
            return 0;  // Radio 1 = index 0
        }
        if (radioStr == "2") {
            return 1;  // Radio 2 = index 1
        }

        // Invalid radio parameter
        response->success = false;
        response->error = QString("Invalid radio parameter: %1 (use 'active', 'standby', '1', or '2')").arg(radioParam.toString());
        return -1;
    };

    // Dispatch command
    if (request.command == "send-cw") {
        // Check for CW support
        if (!m_cwMessageManager) {
            response->success = false;
            response->error = "CW messaging not available";
            return;
        }

        // Resolve target radio
        int radioIndex = resolveRadioIndex(request.params);
        if (radioIndex < 0) return;  // Error already set

        // Get message or fkey parameter
        QString message = request.params.value("message").toString();
        int fkey = request.params.value("fkey").toInt();

        // Get the radio controller for the target radio
        RadioController* radioController = m_radioManager ? m_radioManager->getRadioController(radioIndex) : nullptr;
        if (!radioController || !m_radioManager->isRadioConnected(radioIndex)) {
            response->success = false;
            response->error = QString("Radio %1 not connected").arg(radioIndex + 1);
            return;
        }

        QString radioLabel = (radioIndex == m_radioManager->getActiveRadioIndex()) ? "active" : "standby";

        if (!message.isEmpty()) {
            // Send direct CW message to specific radio
            radioController->sendCW(message);
            response->success = true;
            response->message = QString("CW message sent to %1 radio: %2").arg(radioLabel).arg(message);
        } else if (fkey >= 1 && fkey <= 12) {
            // Send function key message (uses active radio's context for now)
            handleFunctionKey(fkey, false, false);
            response->success = true;
            response->message = QString("Function key F%1 sent").arg(fkey);
        } else {
            response->success = false;
            response->error = "Either 'message' or 'fkey' (1-12) parameter required";
        }
    }
    else if (request.command == "set-frequency") {
        // Get frequency parameter (in Hz)
        QVariant freqParam = request.params.value("frequency");
        if (!freqParam.isValid()) {
            response->success = false;
            response->error = "Missing 'frequency' parameter";
            return;
        }

        freq_t frequency = static_cast<freq_t>(freqParam.toLongLong());
        if (frequency < 100000 || frequency > 500000000) {  // 100 kHz to 500 MHz
            response->success = false;
            response->error = QString("Invalid frequency: %1 Hz (must be 100000-500000000)").arg(frequency);
            return;
        }

        // Resolve target radio
        int radioIndex = resolveRadioIndex(request.params);
        if (radioIndex < 0) return;  // Error already set

        // Get the radio controller for the target radio
        RadioController* radioController = m_radioManager ? m_radioManager->getRadioController(radioIndex) : nullptr;
        if (!radioController || !m_radioManager->isRadioConnected(radioIndex)) {
            response->success = false;
            response->error = QString("Radio %1 not connected").arg(radioIndex + 1);
            return;
        }

        radioController->setFrequency(frequency);
        QString radioLabel = (radioIndex == m_radioManager->getActiveRadioIndex()) ? "active" : "standby";
        double freqMHz = frequency / 1000000.0;
        response->success = true;
        response->message = QString("Frequency set to %1 MHz on %2 radio").arg(freqMHz, 0, 'f', 3).arg(radioLabel);
    }
    else if (request.command == "set-band") {
        QString bandStr = request.params.value("band").toString().toUpper();
        if (bandStr.isEmpty()) {
            response->success = false;
            response->error = "Missing 'band' parameter";
            return;
        }

        BandType band = stringToBand(bandStr);
        if (band == BandType::None) {
            response->success = false;
            response->error = QString("Invalid band: %1").arg(bandStr);
            return;
        }

        // Resolve target radio
        int radioIndex = resolveRadioIndex(request.params);
        if (radioIndex < 0) return;  // Error already set

        // Get the radio controller for the target radio
        RadioController* radioController = m_radioManager ? m_radioManager->getRadioController(radioIndex) : nullptr;
        if (!radioController || !m_radioManager->isRadioConnected(radioIndex)) {
            response->success = false;
            response->error = QString("Radio %1 not connected").arg(radioIndex + 1);
            return;
        }

        radioController->setBand(band);
        QString radioLabel = (radioIndex == m_radioManager->getActiveRadioIndex()) ? "active" : "standby";
        response->success = true;
        response->message = QString("Band changed to %1 on %2 radio").arg(bandStr).arg(radioLabel);
    }
    else if (request.command == "set-mode") {
        QString modeStr = request.params.value("mode").toString().toUpper();
        if (modeStr.isEmpty()) {
            response->success = false;
            response->error = "Missing 'mode' parameter";
            return;
        }

        ModeType mode = stringToMode(modeStr);
        if (mode == ModeType::None) {
            response->success = false;
            response->error = QString("Invalid mode: %1").arg(modeStr);
            return;
        }

        // Resolve target radio
        int radioIndex = resolveRadioIndex(request.params);
        if (radioIndex < 0) return;  // Error already set

        // Get the radio controller for the target radio
        RadioController* radioController = m_radioManager ? m_radioManager->getRadioController(radioIndex) : nullptr;
        if (!radioController || !m_radioManager->isRadioConnected(radioIndex)) {
            response->success = false;
            response->error = QString("Radio %1 not connected").arg(radioIndex + 1);
            return;
        }

        radioController->setMode(mode);
        QString radioLabel = (radioIndex == m_radioManager->getActiveRadioIndex()) ? "active" : "standby";
        response->success = true;
        response->message = QString("Mode changed to %1 on %2 radio").arg(modeStr).arg(radioLabel);
    }
    else if (request.command == "toggle-run-mode") {
        // Toggle between CQ and S&P modes
        if (m_operatingMode == OperatingMode::CQ) {
            onSPMode();
            response->success = true;
            response->message = "Switched to S&P mode";
        } else {
            onCQMode();
            response->success = true;
            response->message = "Switched to CQ mode";
        }
    }
    else if (request.command == "clear-entry") {
        // Clear callsign and exchange fields
        onClearEntry();
        response->success = true;
        response->message = "Entry fields cleared";
    }
    else {
        response->success = false;
        response->error = QString("Unknown command: %1").arg(request.command);
    }

    if (response->success) {
        LOG_INFO("MainWindow", QString("Web command success: %1 - %2")
                 .arg(request.command).arg(response->message));
    } else {
        LOG_WARN("MainWindow", QString("Web command failed: %1 - %2")
                 .arg(request.command).arg(response->error));
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
    int offsetX = UIPositioning::CASCADE_START_OFFSET;
    int offsetY = UIPositioning::CASCADE_START_OFFSET;
    const int cascadeStep = UIPositioning::CASCADE_STEP;

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

    // Show CTY update notification via the standard status label
    QString message = QString("CTY-%1 available (Alt+O)")
        .arg(latestVersion);
    onStatusChanged(message, TR4QT::StatusStyle::Warning);
}

void MainWindow::onCTYDownloadCompleted(bool success) {
    if (success) {
        // Clear the "update available" status message now that update is applied
        setStatusMessage("CTY.DAT updated successfully");
        LOG_DEBUG("MainWindow", "CTY download completed - cleared status bar notification");
    }
}

void MainWindow::onDownloadCTY(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download CTY.dat (Alt+O) - Starting download (headless=%1)").arg(headless));

    CTYDownloadResult result = m_downloadManager->downloadCTY(headless);

    if (result.success) {
        setStatusMessage(result.statusMessage);
        LOG_INFO("MainWindow", result.statusMessage);
    } else {
        setStatusMessage(QString("CTY download failed: %1").arg(result.errorMessage));
        LOG_ERROR("MainWindow", QString("CTY download failed: %1").arg(result.errorMessage));
    }
    // Note: Status bar cleared via onCTYDownloadCompleted() signal from DownloadManager
}

void MainWindow::onDownloadLOTW(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download LOTW (Alt+L) - Starting download (headless=%1)").arg(headless));
    
    LOTWDownloadResult result = m_downloadManager->downloadLOTW(headless);
    
    if (result.success) {
        setStatusMessage(result.statusMessage);
        LOG_INFO("MainWindow", result.statusMessage);
    } else {
        setStatusMessage(QString("LOTW download failed: %1").arg(result.errorMessage));
        LOG_ERROR("MainWindow", QString("LOTW download failed: %1").arg(result.errorMessage));
    }
}

void MainWindow::onDownloadSCP(bool headless) {
    LOG_DEBUG("MainWindow", QString("Download SCP (Alt+S) - Starting download (headless=%1)").arg(headless));
    
    SCPDownloadResult result = m_downloadManager->downloadSCP(headless);
    
    if (result.success) {
        // Reload SCP matcher with new data (unique_ptr handles cleanup)
        m_scpMatcher = std::make_unique<SCPMatcher>();

        setStatusMessage(result.statusMessage);
        LOG_INFO("MainWindow", result.statusMessage);
    } else {
        setStatusMessage(QString("SCP download failed: %1").arg(result.errorMessage));
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
    executeSearch();
}

void MainWindow::executeSearch() {
    int contestId = m_hasActiveContest ? m_currentContestDbId : -1;
    QSOSearchDialog dialog(contestId, this);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QSOSearchCriteria criteria = dialog.getCriteria();
    if (!criteria.hasAnyCriteria()) {
        setStatusMessage("Search cancelled: no criteria specified");
        return;
    }

    m_lastSearchCriteria = criteria;
    refreshSearchResults();
}

void MainWindow::searchForCallsign(const QString& callsign) {
    QSOSearchCriteria criteria;
    criteria.callsign = callsign;
    if (m_hasActiveContest) {
        criteria.contestId = m_currentContestDbId;
    }

    m_lastSearchCriteria = criteria;
    refreshSearchResults();
}

void MainWindow::refreshSearchResults() {
    if (!m_lastSearchCriteria.hasAnyCriteria()) {
        return;
    }

    QSOSearchService searchService;
    QList<QSO> results = searchService.search(m_lastSearchCriteria);

    if (m_activeContest) {
        m_searchPanel->setContest(m_activeContest.get());
    }

    m_searchPanel->syncAppearance(m_qsoTableView);
    m_searchPanel->setResults(results);
    setStatusMessage(
        QString("Search: %1 QSO%2 found")
            .arg(results.size())
            .arg(results.size() != 1 ? "s" : ""));

    m_callsignEntry->setFocus();
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

// Band menu - SO2R implementations
void MainWindow::onToggleRigs() {
    if (!m_radioManager->isSO2REnabled()) {
        setStatusMessage("SO2R not enabled - configure in Edit SO2R (Alt+E)");
        return;
    }

    m_radioManager->toggleActiveRadio();
}

void MainWindow::onEditSO2R() {
    SO2RConfigDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Update RadioManager with new SO2R state
        m_radioManager->setSO2REnabled(dialog.isSO2REnabled());

        // If SO2R was just enabled and we're already connected, need to reconnect
        // to pick up the second radio
        if (dialog.isSO2REnabled() && m_radioManager->isConnected()) {
            setStatusMessage("SO2R enabled - reconnect to activate second radio");
        }

        // Update UI to reflect SO2R state (show/hide standby frequency)
        updateRadioStatusGrid();

        LOG_INFO("MainWindow", QString("SO2R configuration updated: enabled=%1")
                 .arg(dialog.isSO2REnabled()));
    }
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
    // Get the active radio via RadioManager
    RadioController* activeRadio = m_radioManager->getRadioController(m_radioManager->getActiveRadioIndex());

    if (activeRadio && activeRadio->isConnected()) {
        // Radio connected: Send band change to active radio
        LOG_DEBUG("MainWindow", QString("Band clicked: %1 Sending setBand command to Radio %2")
            .arg(bandToString(band)).arg(m_radioManager->getActiveRadioIndex() + 1));
        activeRadio->setBand(band);
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
        setStatusMessage(QString("Band: %1 (manual)").arg(bandToString(band)));
    }
}

void MainWindow::onBandUp() {
    if (!m_bandSwitchingManager) {
        LOG_ERROR("MainWindow", "BandSwitchingManager is null");
        return;
    }

    BandType currentBand = m_currentState.bandA;
    BandType nextBand = m_bandSwitchingManager->getNextBand(currentBand, m_activeContest.get());

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
    BandType prevBand = m_bandSwitchingManager->getPreviousBand(currentBand, m_activeContest.get());

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
            QString flashStyle = QString("QLabel { background-color: %1; color: %2; padding: 5px; border: 2px solid %3; border-radius: 3px; font-weight: bold; }")
                .arg(theme.colorName(ColorRole::StatusFlashBackground))
                .arg(theme.colorName(ColorRole::StatusFlashText))
                .arg(theme.colorName(ColorRole::StatusFlashBorder));
            m_radioFreqBandLabel->setStyleSheet(flashStyle);
        } else {
            // Normal (but still show disconnected state)
            QString normalStyle = QString("QLabel { background-color: %1; padding: 5px; border: 1px solid %2; border-radius: 3px; }")
                .arg(theme.colorName(ColorRole::TextDisplayBackground))
                .arg(theme.colorName(ColorRole::StatusFlashBorder));
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
