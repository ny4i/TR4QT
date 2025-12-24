#include "PreferencesDialog.h"
#include "../../utils/AppSettings.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/RadioEnumerator.h"
#include "../../network/UdpBroadcaster.h"
#include "../../logging/Logger.h"
#include "../../logging/LogLevel.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QCompleter>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>

namespace TR4QT {

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    qDebug() << "*** PreferencesDialog constructor called ***";
    qDebug() << "*** Setting window title ***";
    setWindowTitle("Preferences");
    qDebug() << "*** Calling setupUI ***";
    setupUI();
    qDebug() << "*** setupUI completed, calling loadSettings ***";
    loadSettings();
    qDebug() << "*** loadSettings completed, resizing ***";
    resize(600, 500);
    qDebug() << "*** PreferencesDialog fully initialized with window title:" << windowTitle();
}

void PreferencesDialog::setupUI() {
    qDebug() << "*** setupUI: Creating main layout ***";
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    qDebug() << "*** setupUI: Creating tab widget ***";
    // Tab widget
    m_tabWidget = new QTabWidget(this);
    qDebug() << "*** setupUI: Creating Station tab ***";
    m_tabWidget->addTab(createStationTab(), "Station");
    qDebug() << "*** setupUI: Creating Radio tab ***";
    m_tabWidget->addTab(createRadioTab(), "Radio");
    qDebug() << "*** setupUI: Creating DX Cluster tab ***";
    m_tabWidget->addTab(createDXClusterTab(), "DX Cluster");
    qDebug() << "*** setupUI: Creating UDP Broadcast tab ***";
    m_tabWidget->addTab(createUDPBroadcastTab(), "UDP Broadcast");
    qDebug() << "*** setupUI: Creating Appearance tab ***";
    m_tabWidget->addTab(createAppearanceTab(), "Appearance");
    qDebug() << "*** setupUI: Creating Logging tab ***";
    m_tabWidget->addTab(createLoggingTab(), "Logging");
    qDebug() << "*** setupUI: Creating Contest tab ***";
    m_tabWidget->addTab(createContestTab(), "Contest");
    qDebug() << "*** setupUI: Creating Advanced tab ***";
    m_tabWidget->addTab(createAdvancedTab(), "Advanced");

    mainLayout->addWidget(m_tabWidget);

    // Button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);

    QPushButton* applyButton = buttonBox->addButton(QDialogButtonBox::Apply);
    connect(applyButton, &QPushButton::clicked, this, &PreferencesDialog::onApply);

    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

QWidget* PreferencesDialog::createStationTab() {
    QWidget* stationTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(stationTab);

    QGroupBox* stationGroup = new QGroupBox("Station Information", this);
    QFormLayout* formLayout = new QFormLayout(stationGroup);

    // Callsign
    m_callsignEdit = new QLineEdit(this);
    m_callsignEdit->setPlaceholderText("W1AW");
    m_callsignEdit->setToolTip("Your callsign (used in contests and logging)");
    formLayout->addRow("Callsign:", m_callsignEdit);

    // Grid square
    m_gridSquareEdit = new QLineEdit(this);
    m_gridSquareEdit->setPlaceholderText("FN31pr");
    m_gridSquareEdit->setToolTip("Maidenhead grid square locator");
    formLayout->addRow("Grid Square:", m_gridSquareEdit);

    // Continent
    m_continentCombo = new QComboBox(this);
    m_continentCombo->addItems({"NA", "SA", "EU", "AF", "AS", "OC"});
    m_continentCombo->setToolTip("Your continent (used for contest scoring)");
    formLayout->addRow("Continent:", m_continentCombo);

    // CQ Zone
    m_cqZoneSpin = new QSpinBox(this);
    m_cqZoneSpin->setRange(1, 40);
    m_cqZoneSpin->setValue(5);
    m_cqZoneSpin->setToolTip("Your CQ Zone (1-40)");
    formLayout->addRow("CQ Zone:", m_cqZoneSpin);

    // ITU Zone
    m_ituZoneSpin = new QSpinBox(this);
    m_ituZoneSpin->setRange(1, 90);
    m_ituZoneSpin->setValue(8);
    m_ituZoneSpin->setToolTip("Your ITU Zone (1-90)");
    formLayout->addRow("ITU Zone:", m_ituZoneSpin);

    // Default operator
    m_operatorEdit = new QLineEdit(this);
    m_operatorEdit->setPlaceholderText("Same as station callsign");
    m_operatorEdit->setToolTip("Default operator callsign (for multi-op contests)");
    formLayout->addRow("Default Operator:", m_operatorEdit);

    layout->addWidget(stationGroup);
    layout->addStretch();

    return stationTab;
}

QWidget* PreferencesDialog::createRadioTab() {
    QWidget* radioTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(radioTab);

    // Radio model selection
    QGroupBox* modelGroup = new QGroupBox("Radio Model", this);
    QFormLayout* modelLayout = new QFormLayout(modelGroup);

    m_radioModelCombo = new QComboBox(this);
    m_radioModelCombo->setEditable(true);  // Make searchable
    m_radioModelCombo->setInsertPolicy(QComboBox::NoInsert);  // Don't add typed text as new items

    // Block signals while populating to avoid triggering onRadioModelChanged before widgets are created
    m_radioModelCombo->blockSignals(true);

    // Populate radio models dynamically from hamlib
    m_radioModelCombo->addItem("Select a radio...", 0);

    QList<RadioModelInfo> radios = RadioEnumerator::getAvailableRadios();
    for (const RadioModelInfo& radio : radios) {
        // Display format: "Manufacturer Model (Status)"
        // Only show status if not Stable to reduce clutter
        QString displayText = radio.displayName();
        if (radio.status != "Stable") {
            displayText += QString(" (%1)").arg(radio.status);
        }
        m_radioModelCombo->addItem(displayText, radio.modelId);
    }

    m_radioModelCombo->addItem("Custom (enter model ID below)...", -1);

    // Add auto-completion for easy searching (300+ radios!)
    QCompleter* completer = new QCompleter(this);
    completer->setModel(m_radioModelCombo->model());
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);  // Match anywhere in string, not just start
    m_radioModelCombo->setCompleter(completer);

    m_customModelEdit = new QLineEdit(this);
    m_customModelEdit->setPlaceholderText("Enter hamlib model ID (e.g., 2046)");
    m_customModelEdit->setVisible(false);

    modelLayout->addRow("Model:", m_radioModelCombo);
    modelLayout->addRow("Custom Model ID:", m_customModelEdit);
    layout->addWidget(modelGroup);

    // Connection type
    QGroupBox* connectionGroup = new QGroupBox("Connection Type", this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    m_serialRadio = new QRadioButton("Serial Port", this);
    m_networkRadio = new QRadioButton("Network (TCP)", this);
    m_serialRadio->setChecked(true);

    connect(m_serialRadio, &QRadioButton::toggled,
            this, &PreferencesDialog::onConnectionTypeChanged);

    connectionLayout->addWidget(m_serialRadio);
    connectionLayout->addWidget(m_networkRadio);
    layout->addWidget(connectionGroup);

    // Serial port settings
    m_serialGroup = new QGroupBox("Serial Port Settings", this);
    QFormLayout* serialLayout = new QFormLayout(m_serialGroup);

    m_serialPortEdit = new QLineEdit(this);
    m_serialPortEdit->setPlaceholderText("/dev/ttyUSB0 or COM1");

    m_baudRateCombo = new QComboBox(this);
    m_baudRateCombo->addItems({"4800", "9600", "19200", "38400", "57600", "115200"});
    m_baudRateCombo->setCurrentText("38400");

    serialLayout->addRow("Port:", m_serialPortEdit);
    serialLayout->addRow("Baud Rate:", m_baudRateCombo);
    layout->addWidget(m_serialGroup);

    // Network settings
    m_networkGroup = new QGroupBox("Network Settings", this);
    QFormLayout* networkLayout = new QFormLayout(m_networkGroup);

    m_ipAddressEdit = new QLineEdit(this);
    m_ipAddressEdit->setPlaceholderText("192.168.1.100");

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(4532);  // Default rigctld port

    networkLayout->addRow("IP Address:", m_ipAddressEdit);
    networkLayout->addRow("Port:", m_portSpin);
    layout->addWidget(m_networkGroup);
    m_networkGroup->setVisible(false);

    // Advanced radio settings
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    QFormLayout* advancedLayout = new QFormLayout(advancedGroup);

    m_civAddressSpin = new QSpinBox(this);
    m_civAddressSpin->setRange(0, 255);
    m_civAddressSpin->setPrefix("0x");
    m_civAddressSpin->setDisplayIntegerBase(16);
    m_civAddressSpin->setValue(0);
    m_civAddressSpin->setToolTip("CI-V address for Icom radios (0 = default)");

    m_pollIntervalSpin = new QSpinBox(this);
    m_pollIntervalSpin->setRange(100, 5000);
    m_pollIntervalSpin->setValue(500);
    m_pollIntervalSpin->setSingleStep(100);
    m_pollIntervalSpin->setSuffix(" ms");
    m_pollIntervalSpin->setToolTip("How often to poll the radio for status updates");

    advancedLayout->addRow("CI-V Address:", m_civAddressSpin);
    advancedLayout->addRow("Poll Interval:", m_pollIntervalSpin);

    m_autoConnectCheck = new QCheckBox("Auto-connect on startup", this);
    m_autoConnectCheck->setChecked(true);
    m_autoConnectCheck->setToolTip("Automatically connect to radio when application starts");
    advancedLayout->addRow("", m_autoConnectCheck);

    layout->addWidget(advancedGroup);

    // Test connection button
    QHBoxLayout* testLayout = new QHBoxLayout();
    QPushButton* testButton = new QPushButton("Test Connection", this);
    connect(testButton, &QPushButton::clicked, this, &PreferencesDialog::onTestRadioConnection);
    testLayout->addWidget(testButton);
    testLayout->addStretch();
    layout->addLayout(testLayout);

    layout->addStretch();

    // Now that all widgets are created, connect signals and unblock
    connect(m_radioModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onRadioModelChanged);
    m_radioModelCombo->blockSignals(false);

    return radioTab;
}

QWidget* PreferencesDialog::createDXClusterTab() {
    QWidget* dxClusterTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(dxClusterTab);

    QGroupBox* clusterGroup = new QGroupBox("DX Cluster Settings", this);
    QFormLayout* formLayout = new QFormLayout(clusterGroup);

    // DX Cluster callsign
    m_dxClusterCallsignEdit = new QLineEdit(this);
    m_dxClusterCallsignEdit->setPlaceholderText("Leave blank to use station callsign");
    formLayout->addRow("DX Cluster Callsign:", m_dxClusterCallsignEdit);

    // Default server
    m_dxClusterServerEdit = new QLineEdit(this);
    m_dxClusterServerEdit->setPlaceholderText("server.example.com:7373");
    formLayout->addRow("Default Server:", m_dxClusterServerEdit);

    // Auto-connect
    m_dxClusterAutoConnectCheck = new QCheckBox("Auto-connect to DX Cluster on startup", this);
    formLayout->addRow("", m_dxClusterAutoConnectCheck);

    layout->addWidget(clusterGroup);

    // Help text
    QLabel* helpLabel = new QLabel(
        "The DX Cluster callsign is used for login authentication.\n"
        "If left blank, your station callsign will be used.\n\n"
        "Common DX Cluster servers:\n"
        "  • dxc.nc7j.com:7373 (NC7J)\n"
        "  • cluster.w6kk.org:7373 (W6KK)\n"
        "  • dxc.k9eq.net:7373 (K9EQ)",
        this
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    layout->addWidget(helpLabel);

    layout->addStretch();

    return dxClusterTab;
}

QWidget* PreferencesDialog::createUDPBroadcastTab() {
    QWidget* udpTab = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(udpTab);

    // Enable/Disable group
    QGroupBox* enableGroup = new QGroupBox("UDP Broadcast Settings", this);
    QVBoxLayout* enableLayout = new QVBoxLayout(enableGroup);

    m_udpBroadcastEnabledCheck = new QCheckBox("Enable UDP broadcast", this);
    m_udpBroadcastEnabledCheck->setToolTip("Send real-time updates via UDP to other applications");
    enableLayout->addWidget(m_udpBroadcastEnabledCheck);

    m_udpRadioInfoEnabledCheck = new QCheckBox("Send RadioInfo messages (frequency/mode changes)", this);
    m_udpRadioInfoEnabledCheck->setToolTip("Broadcast radio state changes (throttled)");
    enableLayout->addWidget(m_udpRadioInfoEnabledCheck);

    m_udpContactInfoEnabledCheck = new QCheckBox("Send ContactInfo messages (new QSOs)", this);
    m_udpContactInfoEnabledCheck->setToolTip("Broadcast new QSO entries immediately");
    enableLayout->addWidget(m_udpContactInfoEnabledCheck);

    QHBoxLayout* throttleLayout = new QHBoxLayout();
    throttleLayout->addWidget(new QLabel("Throttle interval:", this));
    m_udpThrottleIntervalSpin = new QSpinBox(this);
    m_udpThrottleIntervalSpin->setRange(100, 5000);
    m_udpThrottleIntervalSpin->setSingleStep(100);
    m_udpThrottleIntervalSpin->setValue(500);
    m_udpThrottleIntervalSpin->setSuffix(" ms");
    m_udpThrottleIntervalSpin->setToolTip("Minimum time between RadioInfo messages");
    throttleLayout->addWidget(m_udpThrottleIntervalSpin);
    throttleLayout->addStretch();
    enableLayout->addLayout(throttleLayout);

    mainLayout->addWidget(enableGroup);

    // Destinations group
    QGroupBox* destGroup = new QGroupBox("Broadcast Destinations", this);
    QVBoxLayout* destLayout = new QVBoxLayout(destGroup);

    m_udpDestinationsList = new QListWidget(this);
    m_udpDestinationsList->setToolTip("List of UDP destinations (host:port)");
    destLayout->addWidget(m_udpDestinationsList);

    // Add/Edit controls
    QHBoxLayout* addLayout = new QHBoxLayout();
    addLayout->addWidget(new QLabel("Host:", this));
    m_udpHostEdit = new QLineEdit(this);
    m_udpHostEdit->setPlaceholderText("127.0.0.1 or 239.255.0.1");
    m_udpHostEdit->setToolTip("IP address or multicast address");
    addLayout->addWidget(m_udpHostEdit);

    addLayout->addWidget(new QLabel("Port:", this));
    m_udpPortSpin = new QSpinBox(this);
    m_udpPortSpin->setRange(1, 65535);
    m_udpPortSpin->setValue(12060);  // N1MM+ default RadioInfo port
    m_udpPortSpin->setToolTip("UDP port number");
    addLayout->addWidget(m_udpPortSpin);

    destLayout->addLayout(addLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_udpAddButton = new QPushButton("Add", this);
    m_udpRemoveButton = new QPushButton("Remove", this);
    m_udpTestButton = new QPushButton("Test", this);
    buttonLayout->addWidget(m_udpAddButton);
    buttonLayout->addWidget(m_udpRemoveButton);
    buttonLayout->addWidget(m_udpTestButton);
    buttonLayout->addStretch();
    destLayout->addLayout(buttonLayout);

    mainLayout->addWidget(destGroup);

    // Help text
    QLabel* helpLabel = new QLabel(
        "UDP broadcast sends real-time updates to other applications (e.g., N1MM+, logging software).\n"
        "Uses N1MM+ compatible RadioInfo and ContactInfo XML formats.\n\n"
        "Multicast addresses: 224.0.0.0 - 239.255.255.255\n"
        "Default N1MM+ ports: 12060 (RadioInfo), 12061 (ContactInfo)",
        this
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    mainLayout->addWidget(helpLabel);

    mainLayout->addStretch();

    // Connect signals
    connect(m_udpAddButton, &QPushButton::clicked,
            this, &PreferencesDialog::onUdpAddDestination);
    connect(m_udpRemoveButton, &QPushButton::clicked,
            this, &PreferencesDialog::onUdpRemoveDestination);
    connect(m_udpTestButton, &QPushButton::clicked,
            this, &PreferencesDialog::onUdpTestDestination);

    return udpTab;
}

QWidget* PreferencesDialog::createAppearanceTab() {
    QWidget* appearanceTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(appearanceTab);

    QGroupBox* fontGroup = new QGroupBox("Font Sizes", this);
    QFormLayout* formLayout = new QFormLayout(fontGroup);

    // Entry field font size
    m_entryFontSizeSpin = new QSpinBox(this);
    m_entryFontSizeSpin->setRange(8, 24);
    m_entryFontSizeSpin->setValue(12);
    m_entryFontSizeSpin->setSuffix(" pt");
    m_entryFontSizeSpin->setToolTip("Font size for callsign and exchange entry fields");
    formLayout->addRow("Entry Fields:", m_entryFontSizeSpin);

    // QSO table font size
    m_tableFontSizeSpin = new QSpinBox(this);
    m_tableFontSizeSpin->setRange(6, 18);
    m_tableFontSizeSpin->setValue(9);
    m_tableFontSizeSpin->setSuffix(" pt");
    m_tableFontSizeSpin->setToolTip("Font size for QSO log table");
    formLayout->addRow("QSO Table:", m_tableFontSizeSpin);

    // Band summary grid font size
    m_gridFontSizeSpin = new QSpinBox(this);
    m_gridFontSizeSpin->setRange(6, 18);
    m_gridFontSizeSpin->setValue(11);
    m_gridFontSizeSpin->setSuffix(" pt");
    m_gridFontSizeSpin->setToolTip("Font size for band summary grid");
    formLayout->addRow("Band Summary:", m_gridFontSizeSpin);

    layout->addWidget(fontGroup);

    // Color Theme group
    QGroupBox* themeGroup = new QGroupBox("Color Theme", this);
    QVBoxLayout* themeLayout = new QVBoxLayout(themeGroup);

    // Theme selector
    QHBoxLayout* themeSelectLayout = new QHBoxLayout();
    QLabel* themeLabel = new QLabel("Theme Preset:", this);
    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem("TR4W Default", static_cast<int>(ThemeType::TR4WDefault));
    m_themeCombo->addItem("Dark Mode", static_cast<int>(ThemeType::DarkMode));
    m_themeCombo->addItem("High Contrast", static_cast<int>(ThemeType::HighContrast));
    m_themeCombo->addItem("Custom", static_cast<int>(ThemeType::Custom));
    m_themeCombo->setToolTip("Select a pre-defined color theme or create your own custom theme");
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onThemeChanged);

    themeSelectLayout->addWidget(themeLabel);
    themeSelectLayout->addWidget(m_themeCombo);
    themeSelectLayout->addStretch();

    themeLayout->addLayout(themeSelectLayout);

    // Customize colors button
    m_customizeColorsButton = new QPushButton("Customize Colors...", this);
    m_customizeColorsButton->setToolTip("Open color customization dialog to set individual element colors");
    connect(m_customizeColorsButton, &QPushButton::clicked,
            this, &PreferencesDialog::onCustomizeColors);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_customizeColorsButton);
    buttonLayout->addStretch();

    themeLayout->addLayout(buttonLayout);

    layout->addWidget(themeGroup);
    layout->addStretch();

    return appearanceTab;
}

QWidget* PreferencesDialog::createLoggingTab() {
    QWidget* loggingTab = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(loggingTab);

    // Log Level group
    QGroupBox* levelGroup = new QGroupBox("Log Level", this);
    QFormLayout* levelLayout = new QFormLayout(levelGroup);

    m_logLevelCombo = new QComboBox(this);
    m_logLevelCombo->addItem("Trace (Most Verbose)", static_cast<int>(LogLevel::Trace));
    m_logLevelCombo->addItem("Debug", static_cast<int>(LogLevel::Debug));
    m_logLevelCombo->addItem("Info (Recommended)", static_cast<int>(LogLevel::Info));
    m_logLevelCombo->addItem("Warning", static_cast<int>(LogLevel::Warn));
    m_logLevelCombo->addItem("Error", static_cast<int>(LogLevel::Error));
    m_logLevelCombo->addItem("Fatal Only", static_cast<int>(LogLevel::Fatal));
    m_logLevelCombo->addItem("Off (No Logging)", static_cast<int>(LogLevel::Off));
    m_logLevelCombo->setToolTip("Minimum log level - messages below this level will be filtered");
    levelLayout->addRow("Log Level:", m_logLevelCombo);

    mainLayout->addWidget(levelGroup);

    // Output Settings group
    QGroupBox* outputGroup = new QGroupBox("Output Settings", this);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);

    m_fileLoggingEnabledCheck = new QCheckBox("Enable file logging", this);
    m_fileLoggingEnabledCheck->setToolTip("Write log messages to file");
    outputLayout->addWidget(m_fileLoggingEnabledCheck);

    m_consoleLoggingEnabledCheck = new QCheckBox("Enable console logging", this);
    m_consoleLoggingEnabledCheck->setToolTip("Write log messages to stderr (visible in terminal)");
    outputLayout->addWidget(m_consoleLoggingEnabledCheck);

    mainLayout->addWidget(outputGroup);

    // File Settings group
    QGroupBox* fileGroup = new QGroupBox("File Settings", this);
    QFormLayout* fileLayout = new QFormLayout(fileGroup);

    // Log file path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_logFilePathEdit = new QLineEdit(this);
    m_logFilePathEdit->setPlaceholderText("~/.tr4qt/logs/tr4qt.log");
    m_logFilePathEdit->setToolTip("Path to log file");

    QPushButton* browseButton = new QPushButton("Browse...", this);
    connect(browseButton, &QPushButton::clicked, this, &PreferencesDialog::onBrowseLogFile);

    pathLayout->addWidget(m_logFilePathEdit);
    pathLayout->addWidget(browseButton);
    fileLayout->addRow("Log File Path:", pathLayout);

    // Max file size
    m_logMaxFileSizeSpin = new QSpinBox(this);
    m_logMaxFileSizeSpin->setRange(1, 100);
    m_logMaxFileSizeSpin->setValue(10);
    m_logMaxFileSizeSpin->setSuffix(" MB");
    m_logMaxFileSizeSpin->setToolTip("Maximum log file size before rotation");
    fileLayout->addRow("Max File Size:", m_logMaxFileSizeSpin);

    // Backup files
    m_logMaxBackupFilesSpin = new QSpinBox(this);
    m_logMaxBackupFilesSpin->setRange(0, 10);
    m_logMaxBackupFilesSpin->setValue(5);
    m_logMaxBackupFilesSpin->setToolTip("Number of backup log files to keep (tr4qt.log.1, .2, etc.)");
    fileLayout->addRow("Backup Files:", m_logMaxBackupFilesSpin);

    mainLayout->addWidget(fileGroup);

    // Log file actions
    QHBoxLayout* actionLayout = new QHBoxLayout();
    QPushButton* openLogButton = new QPushButton("Open Log File", this);
    QPushButton* clearLogButton = new QPushButton("Clear Log File", this);
    connect(openLogButton, &QPushButton::clicked, this, &PreferencesDialog::onOpenLogFile);
    connect(clearLogButton, &QPushButton::clicked, this, &PreferencesDialog::onClearLogFile);
    actionLayout->addWidget(openLogButton);
    actionLayout->addWidget(clearLogButton);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // Help text
    QLabel* helpLabel = new QLabel(
        "TR4QT uses TR4W-compatible logging format:\n"
        "  DD MMM YYYY HH:MM:SS.mmm elapsed [thread] level category - message\n\n"
        "Example:\n"
        "  24 Dec 2025 18:14:02.968 2650 [10252] info TR4QTMain - Program startup\n\n"
        "Log files automatically rotate when reaching max size.\n"
        "Changes to log level take effect immediately.",
        this
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    mainLayout->addWidget(helpLabel);

    mainLayout->addStretch();

    return loggingTab;
}

QWidget* PreferencesDialog::createContestTab() {
    QWidget* contestTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(contestTab);

    QGroupBox* contestGroup = new QGroupBox("Contest Settings", this);
    QFormLayout* formLayout = new QFormLayout(contestGroup);

    // Default contest
    m_defaultContestCombo = new QComboBox(this);
    m_defaultContestCombo->addItems({"CQ WW DX (CW)", "CQ WW DX (SSB)",
                                      "CQ WPX (CW)", "CQ WPX (SSB)",
                                      "Winter Field Day"});
    m_defaultContestCombo->setToolTip("Default contest type for new logs");
    formLayout->addRow("Default Contest:", m_defaultContestCombo);

    // Serial number start
    m_serialStartSpin = new QSpinBox(this);
    m_serialStartSpin->setRange(1, 9999);
    m_serialStartSpin->setValue(1);
    m_serialStartSpin->setToolTip("Starting serial number for contests that use serial numbers");
    formLayout->addRow("Serial Number Start:", m_serialStartSpin);

    layout->addWidget(contestGroup);
    layout->addStretch();

    return contestTab;
}

QWidget* PreferencesDialog::createAdvancedTab() {
    QWidget* advancedTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(advancedTab);

    QGroupBox* countryGroup = new QGroupBox("Country File", this);
    QFormLayout* formLayout = new QFormLayout(countryGroup);

    // Country file path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_countryFilePathEdit = new QLineEdit(this);
    m_countryFilePathEdit->setPlaceholderText("~/.tr4qt/cty.dat");
    m_countryFilePathEdit->setToolTip("Path to cty.dat country file");

    QPushButton* browseButton = new QPushButton("Browse...", this);
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(
            this, "Select Country File",
            m_countryFilePathEdit->text(),
            "Country Files (*.dat);;All Files (*)");
        if (!fileName.isEmpty()) {
            m_countryFilePathEdit->setText(fileName);
        }
    });

    pathLayout->addWidget(m_countryFilePathEdit);
    pathLayout->addWidget(browseButton);
    formLayout->addRow("Country File Path:", pathLayout);

    // Auto-update checkbox
    m_autoUpdateCountryFileCheck = new QCheckBox("Auto-update country file on startup", this);
    m_autoUpdateCountryFileCheck->setChecked(true);
    m_autoUpdateCountryFileCheck->setToolTip("Check for and download updated country file on startup");
    formLayout->addRow("", m_autoUpdateCountryFileCheck);

    layout->addWidget(countryGroup);
    layout->addStretch();

    return advancedTab;
}

void PreferencesDialog::loadSettings() {
    AppSettings& settings = AppSettings::instance();

    // Station tab
    m_callsignEdit->setText(settings.getMyCallsign());
    m_gridSquareEdit->setText(settings.getMyGridSquare());
    m_continentCombo->setCurrentText(settings.getMyContinent());
    m_cqZoneSpin->setValue(settings.getMyCQZone());
    m_ituZoneSpin->setValue(settings.getMyITUZone());
    // Operator will be added to AppSettings

    // Radio tab
    if (settings.hasRadioConfig()) {
        RadioConfig config = settings.loadRadioConfig();

        // Set model
        int comboIndex = m_radioModelCombo->findData(config.hamlibModelId);
        if (comboIndex >= 0) {
            m_radioModelCombo->setCurrentIndex(comboIndex);
        } else {
            // Custom model
            m_radioModelCombo->setCurrentIndex(m_radioModelCombo->count() - 1);
            m_customModelEdit->setText(QString::number(config.hamlibModelId));
            m_customModelEdit->setVisible(true);
        }

        // Connection type
        if (config.port.contains(':')) {
            m_networkRadio->setChecked(true);
            QStringList parts = config.port.split(':');
            if (parts.size() == 2) {
                m_ipAddressEdit->setText(parts[0]);
                m_portSpin->setValue(parts[1].toInt());
            }
        } else {
            m_serialRadio->setChecked(true);
            m_serialPortEdit->setText(config.port);
            m_baudRateCombo->setCurrentText(QString::number(config.baudRate));
        }

        m_civAddressSpin->setValue(config.civAddress);
        m_pollIntervalSpin->setValue(config.pollInterval);
    }

    m_autoConnectCheck->setChecked(settings.getRadioAutoConnect());
    onConnectionTypeChanged();

    // DX Cluster tab
    m_dxClusterCallsignEdit->setText(settings.getDXClusterCallsign());
    m_dxClusterServerEdit->setText(settings.getDXClusterServer());
    m_dxClusterAutoConnectCheck->setChecked(settings.getDXClusterAutoConnect());

    // UDP Broadcast tab
    m_udpBroadcastEnabledCheck->setChecked(settings.getUDPBroadcastEnabled());
    m_udpRadioInfoEnabledCheck->setChecked(settings.getUDPRadioInfoEnabled());
    m_udpContactInfoEnabledCheck->setChecked(settings.getUDPContactInfoEnabled());
    m_udpThrottleIntervalSpin->setValue(settings.getUDPThrottleInterval());

    // Load destinations
    QList<UdpDestination> destinations = settings.getUDPDestinations();
    m_udpDestinationsList->clear();
    for (const auto& dest : destinations) {
        QString itemText = QString("%1:%2%3")
            .arg(dest.host)
            .arg(dest.port)
            .arg(dest.enabled ? "" : " (disabled)");
        m_udpDestinationsList->addItem(itemText);
    }

    // Appearance tab
    m_entryFontSizeSpin->setValue(settings.getEntryFontSize());
    m_tableFontSizeSpin->setValue(settings.getTableFontSize());
    m_gridFontSizeSpin->setValue(settings.getGridFontSize());

    // Load current theme
    ThemeManager& theme = ThemeManager::instance();
    ThemeType currentTheme = theme.currentTheme();
    for (int i = 0; i < m_themeCombo->count(); i++) {
        if (m_themeCombo->itemData(i).toInt() == static_cast<int>(currentTheme)) {
            m_themeCombo->setCurrentIndex(i);
            break;
        }
    }

    // Logging tab
    int logLevelIndex = m_logLevelCombo->findData(static_cast<int>(settings.getLogLevel()));
    if (logLevelIndex >= 0) {
        m_logLevelCombo->setCurrentIndex(logLevelIndex);
    }
    m_fileLoggingEnabledCheck->setChecked(settings.getFileLoggingEnabled());
    m_consoleLoggingEnabledCheck->setChecked(settings.getConsoleLoggingEnabled());
    m_logFilePathEdit->setText(settings.getLogFilePath());
    m_logMaxFileSizeSpin->setValue(settings.getLogMaxFileSize() / (1024 * 1024));  // Convert bytes to MB
    m_logMaxBackupFilesSpin->setValue(settings.getLogMaxBackupFiles());

    // Contest tab - will need to add getters to AppSettings

    // Advanced tab
    m_countryFilePathEdit->setText(settings.getCountryFilePath());
}

void PreferencesDialog::saveSettings() {
    AppSettings& settings = AppSettings::instance();

    // Station tab
    settings.setMyCallsign(m_callsignEdit->text());
    settings.setMyGridSquare(m_gridSquareEdit->text());
    settings.setMyContinent(m_continentCombo->currentText());
    settings.setMyCQZone(m_cqZoneSpin->value());
    settings.setMyITUZone(m_ituZoneSpin->value());

    // Radio tab
    RadioConfig config;

    int modelId = m_radioModelCombo->currentData().toInt();
    if (modelId == -1) {
        config.hamlibModelId = m_customModelEdit->text().toInt();
    } else {
        config.hamlibModelId = modelId;
    }

    if (m_serialRadio->isChecked()) {
        config.port = m_serialPortEdit->text();
        config.baudRate = m_baudRateCombo->currentText().toInt();
    } else {
        config.port = QString("%1:%2")
                          .arg(m_ipAddressEdit->text())
                          .arg(m_portSpin->value());
        config.baudRate = 0;
    }

    config.civAddress = m_civAddressSpin->value();
    config.pollInterval = m_pollIntervalSpin->value();

    settings.saveRadioConfig(config);
    settings.setRadioAutoConnect(m_autoConnectCheck->isChecked());

    // DX Cluster tab
    settings.setDXClusterCallsign(m_dxClusterCallsignEdit->text());
    settings.setDXClusterServer(m_dxClusterServerEdit->text());
    settings.setDXClusterAutoConnect(m_dxClusterAutoConnectCheck->isChecked());

    // UDP Broadcast tab
    settings.setUDPBroadcastEnabled(m_udpBroadcastEnabledCheck->isChecked());
    settings.setUDPRadioInfoEnabled(m_udpRadioInfoEnabledCheck->isChecked());
    settings.setUDPContactInfoEnabled(m_udpContactInfoEnabledCheck->isChecked());
    settings.setUDPThrottleInterval(m_udpThrottleIntervalSpin->value());

    // Save destinations
    QList<UdpDestination> destinations;
    for (int i = 0; i < m_udpDestinationsList->count(); ++i) {
        QString itemText = m_udpDestinationsList->item(i)->text();
        // Parse "host:port" or "host:port (disabled)"
        bool enabled = !itemText.contains("(disabled)");
        QString hostPort = itemText.remove(" (disabled)");
        QStringList parts = hostPort.split(':');
        if (parts.size() == 2) {
            UdpDestination dest;
            dest.host = parts[0];
            dest.port = parts[1].toUInt();
            dest.enabled = enabled;
            destinations.append(dest);
        }
    }
    settings.setUDPDestinations(destinations);

    // Appearance tab
    settings.setEntryFontSize(m_entryFontSizeSpin->value());
    settings.setTableFontSize(m_tableFontSizeSpin->value());
    settings.setGridFontSize(m_gridFontSizeSpin->value());

    // Save theme
    ThemeManager& theme = ThemeManager::instance();
    int themeIndex = m_themeCombo->currentData().toInt();
    ThemeType selectedTheme = static_cast<ThemeType>(themeIndex);
    theme.setTheme(selectedTheme);
    theme.saveToSettings();

    // Logging tab
    LogLevel logLevel = static_cast<LogLevel>(m_logLevelCombo->currentData().toInt());
    settings.setLogLevel(logLevel);
    settings.setFileLoggingEnabled(m_fileLoggingEnabledCheck->isChecked());
    settings.setConsoleLoggingEnabled(m_consoleLoggingEnabledCheck->isChecked());
    settings.setLogFilePath(m_logFilePathEdit->text());
    settings.setLogMaxFileSize(static_cast<qint64>(m_logMaxFileSizeSpin->value()) * 1024 * 1024);  // Convert MB to bytes
    settings.setLogMaxBackupFiles(m_logMaxBackupFilesSpin->value());

    // Apply logging changes immediately to the Logger
    Logger& logger = Logger::instance();
    logger.setLogLevel(logLevel);
    logger.setFileLoggingEnabled(m_fileLoggingEnabledCheck->isChecked());
    logger.setConsoleLoggingEnabled(m_consoleLoggingEnabledCheck->isChecked());
    logger.setLogFilePath(m_logFilePathEdit->text());
    logger.setMaxFileSize(static_cast<qint64>(m_logMaxFileSizeSpin->value()) * 1024 * 1024);
    logger.setMaxBackupFiles(m_logMaxBackupFilesSpin->value());

    // Contest tab - will add setters to AppSettings

    // Advanced tab
    settings.setCountryFilePath(m_countryFilePathEdit->text());
}

void PreferencesDialog::accept() {
    saveSettings();
    QDialog::accept();
}

void PreferencesDialog::onApply() {
    saveSettings();
}

void PreferencesDialog::onConnectionTypeChanged() {
    bool isSerial = m_serialRadio->isChecked();
    m_serialGroup->setVisible(isSerial);
    m_networkGroup->setVisible(!isSerial);
}

void PreferencesDialog::onRadioModelChanged(int index) {
    int modelId = m_radioModelCombo->currentData().toInt();

    // Show custom model ID field if "Custom" is selected
    m_customModelEdit->setVisible(modelId == -1);

    // Auto-configure CI-V for Icom radios
    if (modelId >= 3000 && modelId < 4000) {
        m_civAddressSpin->setEnabled(true);
        if (modelId == 3078) {  // IC-7610
            m_civAddressSpin->setValue(0x98);
        } else if (modelId == 3092) {  // IC-7760
            m_civAddressSpin->setValue(0x7C);
        } else if (modelId == 3073) {  // IC-7300
            m_civAddressSpin->setValue(0x94);
        }
    } else {
        m_civAddressSpin->setValue(0);
        m_civAddressSpin->setEnabled(false);
    }
}

void PreferencesDialog::onTestRadioConnection() {
    QMessageBox::information(this, "Test Connection",
                           "Radio connection testing will be implemented when integrated with MainWindow.\n\n"
                           "For now, save these settings and use Radio → Connect to test.");
}

void PreferencesDialog::onUdpAddDestination() {
    QString host = m_udpHostEdit->text().trimmed();
    quint16 port = m_udpPortSpin->value();

    if (host.isEmpty()) {
        QMessageBox::warning(this, "Add Destination",
                           "Please enter a host address.");
        return;
    }

    // Check for duplicates
    QString newItem = QString("%1:%2").arg(host).arg(port);
    for (int i = 0; i < m_udpDestinationsList->count(); ++i) {
        QString existingItem = m_udpDestinationsList->item(i)->text();
        if (existingItem.startsWith(newItem)) {
            QMessageBox::warning(this, "Add Destination",
                               "This destination already exists in the list.");
            return;
        }
    }

    // Add to list
    m_udpDestinationsList->addItem(newItem);

    // Clear input fields
    m_udpHostEdit->clear();
    m_udpPortSpin->setValue(12060);
}

void PreferencesDialog::onUdpRemoveDestination() {
    QListWidgetItem* currentItem = m_udpDestinationsList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Remove Destination",
                           "Please select a destination to remove.");
        return;
    }

    delete currentItem;
}

void PreferencesDialog::onUdpTestDestination() {
    QString host = m_udpHostEdit->text().trimmed();
    quint16 port = m_udpPortSpin->value();

    if (host.isEmpty()) {
        QMessageBox::warning(this, "Test Destination",
                           "Please enter a host address to test.");
        return;
    }

    // Create test broadcaster
    UdpBroadcaster testBroadcaster;
    UdpDestination testDest;
    testDest.host = host;
    testDest.port = port;
    testDest.enabled = true;
    testBroadcaster.addDestination(testDest);

    // Send test message
    QByteArray testData = "<?xml version=\"1.0\"?><TestMessage><app>TR4QT</app><message>UDP Test</message></TestMessage>";
    bool success = testBroadcaster.sendRawData(testData);

    if (success) {
        QMessageBox::information(this, "Test Destination",
                               QString("Successfully sent test message to %1:%2\n\n"
                                      "Check the receiving application to verify.")
                                   .arg(host).arg(port));
    } else {
        QMessageBox::critical(this, "Test Destination",
                            QString("Failed to send test message to %1:%2\n\n"
                                   "Error: %3")
                                .arg(host).arg(port).arg(testBroadcaster.lastError()));
    }
}

void PreferencesDialog::onOpenLogFile() {
    QString logPath = m_logFilePathEdit->text();
    if (logPath.isEmpty()) {
        logPath = AppSettings::instance().getLogFilePath();
    }

    // Expand home directory if present
    if (logPath.startsWith("~/")) {
        logPath = QDir::homePath() + logPath.mid(1);
    }

    if (!QFile::exists(logPath)) {
        QMessageBox::warning(this, "Open Log File",
                           QString("Log file does not exist:\n%1\n\n"
                                  "Start the application to create the log file.")
                               .arg(logPath));
        return;
    }

    // Open with default application
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(logPath))) {
        QMessageBox::critical(this, "Open Log File",
                            QString("Failed to open log file:\n%1").arg(logPath));
    }
}

void PreferencesDialog::onClearLogFile() {
    QString logPath = m_logFilePathEdit->text();
    if (logPath.isEmpty()) {
        logPath = AppSettings::instance().getLogFilePath();
    }

    // Expand home directory if present
    if (logPath.startsWith("~/")) {
        logPath = QDir::homePath() + logPath.mid(1);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear Log File",
        QString("Are you sure you want to clear the log file?\n\n%1\n\n"
               "This will permanently delete all log contents.")
            .arg(logPath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Close the log file, truncate it, and reopen
    Logger& logger = Logger::instance();
    logger.shutdown();

    QFile logFile(logPath);
    if (logFile.exists()) {
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            logFile.close();
            QMessageBox::information(this, "Clear Log File",
                                   "Log file cleared successfully.");
        } else {
            QMessageBox::critical(this, "Clear Log File",
                                QString("Failed to clear log file:\n%1\n\nError: %2")
                                    .arg(logPath).arg(logFile.errorString()));
        }
    }

    // Reinitialize the logger
    logger.initialize();
    logger.setLogLevel(AppSettings::instance().getLogLevel());
    logger.setFileLoggingEnabled(AppSettings::instance().getFileLoggingEnabled());
    logger.setConsoleLoggingEnabled(AppSettings::instance().getConsoleLoggingEnabled());
    logger.setLogFilePath(AppSettings::instance().getLogFilePath());
}

void PreferencesDialog::onBrowseLogFile() {
    QString currentPath = m_logFilePathEdit->text();
    if (currentPath.isEmpty()) {
        currentPath = AppSettings::instance().getLogFilePath();
    }

    // Expand home directory if present
    if (currentPath.startsWith("~/")) {
        currentPath = QDir::homePath() + currentPath.mid(1);
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Select Log File Location",
        currentPath,
        "Log Files (*.log);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        // Convert back to ~/ notation if within home directory
        QString homePath = QDir::homePath();
        if (fileName.startsWith(homePath)) {
            fileName = "~" + fileName.mid(homePath.length());
        }
        m_logFilePathEdit->setText(fileName);
    }
}

void PreferencesDialog::onThemeChanged(int index) {
    // When theme combo changes, just mark that settings have changed
    // Actual theme will be applied when user clicks Apply or OK
    Q_UNUSED(index);
}

void PreferencesDialog::onCustomizeColors() {
    // Create and show color customization dialog
    QDialog* colorDialog = new QDialog(this);
    colorDialog->setWindowTitle("Customize Colors");
    colorDialog->setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(colorDialog);

    // Create grid for color buttons
    QGroupBox* colorsGroup = new QGroupBox("Color Elements", colorDialog);
    QGridLayout* gridLayout = new QGridLayout(colorsGroup);

    ThemeManager& theme = ThemeManager::instance();

    // Helper struct to hold color button data
    struct ColorButton {
        ColorRole role;
        QPushButton* button;
        QColor currentColor;
    };

    QList<ColorButton*> colorButtons;

    // All 17 ColorRoles
    QList<ColorRole> roles = {
        // Display Colors
        ColorRole::VfoBackground,
        ColorRole::VfoText,
        ColorRole::WindowBackground,
        ColorRole::TextDisplayBackground,

        // Status Colors
        ColorRole::ConnectedStatus,
        ColorRole::DisconnectedStatus,
        ColorRole::FrozenIndicator,

        // Functional Colors
        ColorRole::DupeText,
        ColorRole::NewMultiplierBackground,
        ColorRole::WorkedStationText,
        ColorRole::MultiplierText,
        ColorRole::NeededMultiplierBackground,
        ColorRole::ConfirmedMultiplierBackground,

        // UI Colors
        ColorRole::PrimaryText,
        ColorRole::SecondaryText,
        ColorRole::HoverHighlight,
        ColorRole::BorderColor
    };

    // Create buttons in a 3-column grid
    int row = 0;
    int col = 0;

    for (ColorRole role : roles) {
        // Label with color role name
        QLabel* label = new QLabel(ThemeManager::colorRoleName(role) + ":", colorDialog);
        gridLayout->addWidget(label, row, col * 3);

        // Color preview button
        QPushButton* colorButton = new QPushButton(colorDialog);
        QColor currentColor = theme.color(role);

        colorButton->setFixedSize(100, 30);
        colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(currentColor.name()));
        colorButton->setToolTip("Click to change color");

        // Store button data for later
        ColorButton* btnData = new ColorButton{role, colorButton, currentColor};
        colorButtons.append(btnData);

        // Connect to color picker
        connect(colorButton, &QPushButton::clicked, [=, &theme]() mutable {
            QColor newColor = QColorDialog::getColor(btnData->currentColor, colorDialog, "Select Color");
            if (newColor.isValid()) {
                btnData->currentColor = newColor;
                btnData->button->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(newColor.name()));
            }
        });

        gridLayout->addWidget(colorButton, row, col * 3 + 1);

        // Reset button
        QPushButton* resetButton = new QPushButton("Reset", colorDialog);
        resetButton->setToolTip("Reset to theme default");
        connect(resetButton, &QPushButton::clicked, [=, &theme]() mutable {
            // Get default color from TR4W theme by temporarily switching
            ThemeType originalTheme = theme.currentTheme();
            theme.setTheme(ThemeType::TR4WDefault);
            QColor defaultColor = theme.color(role);
            theme.setTheme(originalTheme);

            btnData->currentColor = defaultColor;
            btnData->button->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(defaultColor.name()));
        });

        gridLayout->addWidget(resetButton, row, col * 3 + 2);

        // Move to next position
        col++;
        if (col >= 2) {  // 2 columns of 3 widgets each = 6 columns total
            col = 0;
            row++;
        }
    }

    mainLayout->addWidget(colorsGroup);

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, colorDialog);

    connect(buttonBox, &QDialogButtonBox::accepted, [=, &theme]() {
        // Apply all custom colors
        for (ColorButton* btnData : colorButtons) {
            theme.setCustomColor(btnData->role, btnData->currentColor);
        }
        theme.saveToSettings();

        // Update theme combo to show "Custom"
        for (int i = 0; i < m_themeCombo->count(); i++) {
            if (m_themeCombo->itemData(i).toInt() == static_cast<int>(ThemeType::Custom)) {
                m_themeCombo->setCurrentIndex(i);
                break;
            }
        }

        colorDialog->accept();
    });

    connect(buttonBox, &QDialogButtonBox::rejected, colorDialog, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    colorDialog->resize(700, 500);
    colorDialog->exec();

    // Clean up
    qDeleteAll(colorButtons);
    colorDialog->deleteLater();
}

} // namespace TR4QT
