#include "MenuManager.h"
#include "../../logging/LogMacros.h"
#include "../../utils/AppSettings.h"
#include "../../utils/DialogHelper.h"
#include <QMenu>
#include <QKeySequence>
#include <QInputDialog>

namespace TR4QT {

MenuManager::MenuManager(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_connectAction(nullptr)
    , m_disconnectAction(nullptr)
    , m_autoSendCWAction(nullptr)
    , m_webServerAction(nullptr)
    , m_bandMapAction(nullptr)
    , m_dxClusterAction(nullptr)
    , m_radioControlAction(nullptr)
    , m_multipliersAction(nullptr)
    , m_statisticsAction(nullptr)
    , m_sectionsMapAction(nullptr)
    , m_statesMapAction(nullptr)
    , m_worldMapAction(nullptr)
    , m_graylineMapAction(nullptr)
{
}

QMenuBar* MenuManager::createMenuBar(const Config& config) {
    QMenuBar* menuBar = new QMenuBar(m_parent);

    createFileMenu(menuBar, config);
    createRadioMenu(menuBar, config);
    createEditMenu(menuBar, config);
    createToolsMenu(menuBar, config);
    createOperatingMenu(menuBar, config);
    createCommandsMenu(menuBar, config);
    createAutomationMenu(menuBar, config);
    createWindowMenu(menuBar, config);
    createBandMenu(menuBar, config);
    createHelpMenu(menuBar, config);

    return menuBar;
}

void MenuManager::createFileMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* fileMenu = menuBar->addMenu("&File");

    QAction* newContestAction = fileMenu->addAction("&New/Open Contest...");
    newContestAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(newContestAction, &QAction::triggered, this, config.onNewOpenContest);

    fileMenu->addSeparator();

    QAction* clearLogAction = fileMenu->addAction("&Clear Log...");
    connect(clearLogAction, &QAction::triggered, this, config.onClearLog);

    fileMenu->addSeparator();

    // Import submenu
    QMenu* importMenu = fileMenu->addMenu("&Import");
    QAction* importADIFAction = importMenu->addAction("Import &ADIF...");
    connect(importADIFAction, &QAction::triggered, this, config.onImportADIF);

    // Export submenu
    QMenu* exportMenu = fileMenu->addMenu("&Export");
    QAction* exportADIFAction = exportMenu->addAction("Export &ADIF...");
    connect(exportADIFAction, &QAction::triggered, this, config.onExportADIF);

    QAction* exportCabrilloAction = exportMenu->addAction("Export &Cabrillo...");
    exportCabrilloAction->setShortcut(QKeySequence("Ctrl+Alt+B"));
    connect(exportCabrilloAction, &QAction::triggered, this, config.onExportCabrillo);

    fileMenu->addSeparator();

    QAction* preferencesAction = fileMenu->addAction("&Preferences...");
    preferencesAction->setShortcut(QKeySequence::Preferences);
    preferencesAction->setMenuRole(QAction::PreferencesRole);  // Explicitly set macOS menu role
    connect(preferencesAction, &QAction::triggered, this, [config]() {
        LOG_DEBUG("MenuManager", "*** Preferences action triggered ***");
        config.onPreferences();
    });
    LOG_DEBUG("MenuManager", QString("*** Preferences menu created with shortcut: %1 menuRole: %2")
        .arg(preferencesAction->shortcut().toString())
        .arg(preferencesAction->menuRole()));

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, config.onExit);
}

void MenuManager::createRadioMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* radioMenu = menuBar->addMenu("&Radio");

    QAction* configAction = radioMenu->addAction("&Configure...");
    configAction->setMenuRole(QAction::NoRole);  // Prevent macOS from moving this to app menu
    connect(configAction, &QAction::triggered, this, [config]() {
        LOG_DEBUG("MenuManager", "*** Radio Configure action triggered ***");
        config.onRadioConfigure();
    });
    LOG_DEBUG("MenuManager", QString("*** Radio Configure menu created with menuRole: %1")
        .arg(configAction->menuRole()));

    radioMenu->addSeparator();

    m_connectAction = radioMenu->addAction("C&onnect/Reconnect");
    m_connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_connectAction->setStatusTip("Connect or reconnect to radio");
    connect(m_connectAction, &QAction::triggered, this, config.onRadioConnect);

    m_disconnectAction = radioMenu->addAction("&Disconnect");
    m_disconnectAction->setEnabled(false);
    connect(m_disconnectAction, &QAction::triggered, this, config.onRadioDisconnect);

    radioMenu->addSeparator();

    m_autoSendCWAction = radioMenu->addAction("Auto-Send CW (&ESM)");
    m_autoSendCWAction->setCheckable(true);
    m_autoSendCWAction->setChecked(AppSettings::instance().getCWAutoSendEnabled());  // Load from settings
    m_autoSendCWAction->setStatusTip("Enter Send Mode: Automatically send CW messages when Enter is pressed in CW mode");
    connect(m_autoSendCWAction, &QAction::toggled, this, [config](bool checked) {
        AppSettings::instance().setCWAutoSendEnabled(checked);  // Persist setting
        LOG_DEBUG("MenuManager", QString("Auto Send CW %1").arg(checked ? "enabled" : "disabled"));
        config.onUpdateRadioStatusGrid();  // Update WPM display
        if (config.onAutoSendCWToggled) {
            config.onAutoSendCWToggled(checked);
        }
    });
}

void MenuManager::createEditMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* editMenu = menuBar->addMenu("&Edit");

    QAction* viewEditLogAction = editMenu->addAction("View/&Edit Log");
    viewEditLogAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(viewEditLogAction, &QAction::triggered, this, config.onViewEditLog);

    QAction* clearDupesAction = editMenu->addAction("Clear &Dupes");
    clearDupesAction->setShortcut(QKeySequence("Ctrl+K"));
    connect(clearDupesAction, &QAction::triggered, this, config.onClearDupes);

    QAction* noteAction = editMenu->addAction("&Note");
    noteAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(noteAction, &QAction::triggered, this, config.onNote);

    QAction* recallLastAction = editMenu->addAction("&Recall Last Entry");
    recallLastAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(recallLastAction, &QAction::triggered, this, config.onRecallLast);
}

void MenuManager::createToolsMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* toolsMenu = menuBar->addMenu("&Tools");

    QAction* wkModeAction = toolsMenu->addAction("WK Mode (Re-initialize WinKeyer)");
    wkModeAction->setShortcut(QKeySequence("Alt+A"));
    connect(wkModeAction, &QAction::triggered, this, config.onWKMode);

    QAction* backupLogAction = toolsMenu->addAction("Backup Log");
    backupLogAction->setShortcut(QKeySequence("Alt+F"));
    connect(backupLogAction, &QAction::triggered, this, config.onBackupLog);

    // Download submenu
    QMenu* downloadMenu = toolsMenu->addMenu("&Download");

    QAction* downloadCTYAction = downloadMenu->addAction("CTY.dat (Country File)");
    downloadCTYAction->setShortcut(QKeySequence("Alt+O"));
    connect(downloadCTYAction, &QAction::triggered, this, config.onDownloadCTY);

    QAction* downloadLOTWAction = downloadMenu->addAction("LOTW Users");
    downloadLOTWAction->setShortcut(QKeySequence("Alt+L"));
    connect(downloadLOTWAction, &QAction::triggered, this, config.onDownloadLOTW);

    QAction* downloadSCPAction = downloadMenu->addAction("MASTER.SCP (Callsign Database)");
    downloadSCPAction->setShortcut(QKeySequence("Alt+S"));
    connect(downloadSCPAction, &QAction::triggered, this, config.onDownloadSCP);

    QAction* initializeAction = toolsMenu->addAction("Initialize");
    initializeAction->setShortcut(QKeySequence("Alt+W"));
    connect(initializeAction, &QAction::triggered, this, config.onInitialize);

    toolsMenu->addSeparator();

    // Rescore contest (recalculate points and multipliers)
    QAction* rescoreAction = toolsMenu->addAction("Rescore Contest");
    connect(rescoreAction, &QAction::triggered, this, config.onRescoreContest);

    // Edit contest settings (exchange, etc.)
    QAction* editContestAction = toolsMenu->addAction("Edit Contest Settings...");
    connect(editContestAction, &QAction::triggered, this, config.onEditContestSettings);

    // Data integrity check
    QAction* integrityCheckAction = toolsMenu->addAction("Validate Log Integrity");
    integrityCheckAction->setShortcut(QKeySequence("Alt+I"));
    connect(integrityCheckAction, &QAction::triggered, this, config.onFullIntegrityCheck);

    toolsMenu->addSeparator();

    m_webServerAction = toolsMenu->addAction("Start Web Server");
    connect(m_webServerAction, &QAction::triggered, this, config.onToggleWebServer);

    toolsMenu->addSeparator();

    QAction* resetWindowsAction = toolsMenu->addAction("Reset Window Positions");
    connect(resetWindowsAction, &QAction::triggered, this, config.onResetWindowPositions);

    QAction* optionsAction = toolsMenu->addAction("Options");
    optionsAction->setShortcut(QKeySequence("Ctrl+J"));
    connect(optionsAction, &QAction::triggered, this, config.onPreferences);
}

void MenuManager::createOperatingMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* operatingMenu = menuBar->addMenu("&Operating");

    QAction* autoCQAction = operatingMenu->addAction("Auto CQ");
    autoCQAction->setShortcut(QKeySequence("Alt+Q"));
    connect(autoCQAction, &QAction::triggered, this, config.onAutoCQ);

    QAction* autoCQResumeAction = operatingMenu->addAction("Auto CQ Resume");
    autoCQResumeAction->setShortcut(QKeySequence("Alt+C"));
    connect(autoCQResumeAction, &QAction::triggered, this, config.onAutoCQResume);

    QAction* killCWAction = operatingMenu->addAction("Kill CW");
    killCWAction->setShortcut(QKeySequence("Alt+K"));
    connect(killCWAction, &QAction::triggered, this, config.onKillCW);

    operatingMenu->addSeparator();

    QAction* dupeCheckAction = operatingMenu->addAction("Dupe Check");
    dupeCheckAction->setShortcut(QKeySequence("Alt+D"));
    connect(dupeCheckAction, &QAction::triggered, this, config.onDupeCheck);

    QAction* searchLogAction = operatingMenu->addAction("Search Log");
    searchLogAction->setShortcut(QKeySequence("Alt+L"));
    connect(searchLogAction, &QAction::triggered, this, config.onSearchLog);

    QAction* deleteLastQSOAction = operatingMenu->addAction("Delete Last QSO");
    deleteLastQSOAction->setShortcut(QKeySequence("Alt+Y"));
    connect(deleteLastQSOAction, &QAction::triggered, this, config.onDeleteLastQSO);

    operatingMenu->addSeparator();

    QAction* incNumberAction = operatingMenu->addAction("Inc Number");
    incNumberAction->setShortcut(QKeySequence("Alt+I"));
    connect(incNumberAction, &QAction::triggered, this, config.onIncNumber);

    QAction* initialExchangeAction = operatingMenu->addAction("Initial Exchange");
    initialExchangeAction->setShortcut(QKeySequence("Alt+Z"));
    connect(initialExchangeAction, &QAction::triggered, this, config.onInitialExchange);

    // Removed: CW Speed menu item (Alt+S conflicted with Download SCP)
    // Use PgUp/PgDn shortcuts or click WPM label in Radio Control window

    operatingMenu->addSeparator();

    QAction* toggleSidetoneAction = operatingMenu->addAction("Toggle Sidetone");
    toggleSidetoneAction->setShortcut(QKeySequence("Alt+="));
    connect(toggleSidetoneAction, &QAction::triggered, this, config.onToggleSidetone);

    QAction* toggleAutosendAction = operatingMenu->addAction("Toggle Autosend");
    toggleAutosendAction->setShortcut(QKeySequence("Alt+-"));
    connect(toggleAutosendAction, &QAction::triggered, this, config.onToggleAutosend);
}

void MenuManager::createCommandsMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* commandsMenu = menuBar->addMenu("&Commands");

    QAction* cqModeAction = commandsMenu->addAction("CQ Mode");
    cqModeAction->setShortcut(QKeySequence("Shift+Tab"));
    connect(cqModeAction, &QAction::triggered, this, config.onCQMode);

    QAction* spModeAction = commandsMenu->addAction("Search && Pounce Mode");
    spModeAction->setShortcut(QKeySequence("Tab"));
    connect(spModeAction, &QAction::triggered, this, config.onSPMode);
}

void MenuManager::createAutomationMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* automationMenu = menuBar->addMenu("&Automation");

    QAction* autoSPEnableAction = automationMenu->addAction("AUTO S&&P ENABLE");
    autoSPEnableAction->setCheckable(true);
    autoSPEnableAction->setChecked(AppSettings::instance().getAutoSPEnable());
    connect(autoSPEnableAction, &QAction::toggled, this, [config](bool checked) {
        AppSettings::instance().setAutoSPEnable(checked);
        LOG_DEBUG("MenuManager", QString("AUTO S&P ENABLE %1").arg(checked ? "ON" : "OFF"));
        if (config.onAutoSPEnableToggled) {
            config.onAutoSPEnableToggled(checked);
        }
    });

    QAction* autoSPSensitivityAction = automationMenu->addAction("Auto S&&P Sensitivity...");
    connect(autoSPSensitivityAction, &QAction::triggered, this, [this, config]() {
        bool ok;
        int currentValue = AppSettings::instance().getAutoSPSensitivity();
        int newValue = QInputDialog::getInt(m_parent,
            "Auto S&P Sensitivity",
            "Controls how quickly you must move the VFO (in Hz/sec)\n"
            "in order for the program to jump automatically into S&P Mode\n"
            "if AUTO S&P ENABLE is TRUE.\n\n"
            "Sensitivity (Hz/sec):",
            currentValue,
            50,      // minimum
            10000,   // maximum
            50,      // step
            &ok);

        if (ok) {
            AppSettings::instance().setAutoSPSensitivity(newValue);
            LOG_DEBUG("MenuManager", QString("Auto S&P Sensitivity set to %1 Hz/sec").arg(newValue));

            DialogHelper::information(m_parent,
                "AUTO S&P Sensitivity Updated",
                QString("Auto S&P sensitivity set to %1 Hz/sec.\n\n"
                        "The program will switch to S&P mode when you move\n"
                        "the VFO faster than %1 Hz per second.")
                    .arg(newValue));
        }

        if (config.onAutoSPSensitivity) {
            config.onAutoSPSensitivity();
        }
    });
}

void MenuManager::createWindowMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* windowMenu = menuBar->addMenu("&Window");

    m_bandMapAction = windowMenu->addAction("&Band Map");
    m_bandMapAction->setCheckable(true);
    connect(m_bandMapAction, &QAction::triggered, this, config.onShowBandMap);

    m_dxClusterAction = windowMenu->addAction("DX &Cluster");
    m_dxClusterAction->setCheckable(true);
    connect(m_dxClusterAction, &QAction::triggered, this, config.onShowDXCluster);

    m_radioControlAction = windowMenu->addAction("&Radio Control");
    m_radioControlAction->setCheckable(true);
    connect(m_radioControlAction, &QAction::triggered, this, config.onShowRadioControl);

    QAction* sendMorseAction = windowMenu->addAction("Send &Morse Code");
    sendMorseAction->setShortcut(QKeySequence("Alt+K"));
    connect(sendMorseAction, &QAction::triggered, this, config.onSendMorse);

    QAction* editCWMessagesAction = windowMenu->addAction("Edit CW &Messages");
    editCWMessagesAction->setShortcut(QKeySequence("Alt+M"));
    connect(editCWMessagesAction, &QAction::triggered, this, config.onEditCWMessages);

    QAction* functionKeysAction = windowMenu->addAction("&Function Keys Reference");
    functionKeysAction->setShortcut(QKeySequence("Ctrl+F1"));
    connect(functionKeysAction, &QAction::triggered, this, config.onShowFunctionKeysRef);

    m_multipliersAction = windowMenu->addAction("&Multipliers");
    m_multipliersAction->setCheckable(true);
    connect(m_multipliersAction, &QAction::triggered, this, config.onShowMultipliers);

    m_statisticsAction = windowMenu->addAction("&Statistics");
    m_statisticsAction->setCheckable(true);
    connect(m_statisticsAction, &QAction::triggered, this, config.onShowStatistics);

    m_sectionsMapAction = windowMenu->addAction("S&ections Map");
    m_sectionsMapAction->setCheckable(true);
    connect(m_sectionsMapAction, &QAction::triggered, this, config.onShowSectionsMap);

    m_statesMapAction = windowMenu->addAction("St&ates Map (WAS)");
    m_statesMapAction->setCheckable(true);
    connect(m_statesMapAction, &QAction::triggered, this, config.onShowStatesMap);

    m_worldMapAction = windowMenu->addAction("&World Map (DXCC)");
    m_worldMapAction->setCheckable(true);
    connect(m_worldMapAction, &QAction::triggered, this, config.onShowWorldMap);

    m_graylineMapAction = windowMenu->addAction("&Grayline Map");
    m_graylineMapAction->setCheckable(true);
    connect(m_graylineMapAction, &QAction::triggered, this, config.onShowGraylineMap);

    m_amplifierControlAction = windowMenu->addAction("A&mplifier Control");
    m_amplifierControlAction->setCheckable(true);
    connect(m_amplifierControlAction, &QAction::triggered, this, config.onShowAmplifierControl);

    // Disable if amplifier control is not enabled in settings
    if (!AppSettings::instance().getAmplifierEnabled()) {
        m_amplifierControlAction->setEnabled(false);
        m_amplifierControlAction->setToolTip("Enable amplifier control in Preferences to use this feature");
    }

    windowMenu->addSeparator();

    QAction* swapMultViewAction = windowMenu->addAction("Swap Mult View");
    swapMultViewAction->setShortcut(QKeySequence("Alt+G"));
    connect(swapMultViewAction, &QAction::triggered, this, config.onSwapMultView);

    QAction* missingMultsAction = windowMenu->addAction("Missing Mults Report");
    missingMultsAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(missingMultsAction, &QAction::triggered, this, config.onMissingMultsReport);
}

void MenuManager::createBandMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* bandMenu = menuBar->addMenu("&Band");

    QAction* bandUpAction = bandMenu->addAction("Band Up");
    bandUpAction->setShortcut(QKeySequence("Alt+B"));
    connect(bandUpAction, &QAction::triggered, this, config.onBandUp);

    QAction* bandDownAction = bandMenu->addAction("Band Down");
    bandDownAction->setShortcut(QKeySequence("Alt+V"));
    connect(bandDownAction, &QAction::triggered, this, config.onBandDown);

    bandMenu->addSeparator();

    QAction* toggleRigsAction = bandMenu->addAction("Toggle Rigs (SO2R)");
    toggleRigsAction->setShortcut(QKeySequence("Alt+R"));
    connect(toggleRigsAction, &QAction::triggered, this, config.onToggleRigs);

    QAction* editSO2RAction = bandMenu->addAction("Edit SO2R");
    editSO2RAction->setShortcut(QKeySequence("Alt+E"));
    connect(editSO2RAction, &QAction::triggered, this, config.onEditSO2R);
}

void MenuManager::createHelpMenu(QMenuBar* menuBar, const Config& config) {
    QMenu* helpMenu = menuBar->addMenu("&Help");

    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, config.onAbout);

#ifdef ENABLE_PERFORMANCE_PROFILING
    QAction* perfReportAction = helpMenu->addAction("Show Performance Report...");
    connect(perfReportAction, &QAction::triggered, this, config.onShowPerformanceReport);
#endif

    QAction* emailLogsAction = helpMenu->addAction("Email Logs to Support...");
    connect(emailLogsAction, &QAction::triggered, this, config.onEmailLogsToSupport);
}

} // namespace TR4QT
