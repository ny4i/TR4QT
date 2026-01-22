#include "PreferencesDialog.h"
#include "../../utils/AppSettings.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/DXClusterListDownloader.h"
#include "../../utils/SCPDownloader.h"
#include "../../utils/SCPCallsignExtractor.h"
#include "../../data/SCPRepository.h"
#include "../../utils/RadioEnumerator.h"
#include "../../network/UdpBroadcaster.h"
#include "../../logging/Logger.h"
#include "../../logging/LogLevel.h"
#include "../../logging/LogMacros.h"
#include "../../utils/DialogHelper.h"
#include "../../core/Constants.h"
#include "../../core/Types.h"
#include "../../contests/ContestRegistry.h"
#include "../../radio/HamlibRadio.h"
#include "../../amplifiers/AmplifierFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QColorDialog>
#include <QTextEdit>
#include <QCompleter>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <QShowEvent>
#include <QHideEvent>
#include <QTreeWidget>
#include <QHeaderView>

namespace TR4QT {

// K4 default rigctld port (Elecraft K4 specific)
// K3/K4 radios typically use port 9200 for hamlib control
static constexpr int K4_DEFAULT_RIGCTLD_PORT = 9200;

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
    , m_k4Discovery(new K4Discovery(this))
    , m_icomDiscovery(new IcomDiscovery(this))
    , m_portRefreshTimer(new QTimer(this))
{
    LOG_DEBUG("PreferencesDialog", "*** PreferencesDialog constructor called ***");
    LOG_DEBUG("PreferencesDialog", "*** Setting window title ***");
    setWindowTitle("Preferences");
    LOG_DEBUG("PreferencesDialog", "*** Calling setupUI ***");
    setupUI();
    LOG_DEBUG("PreferencesDialog", "*** setupUI completed, calling loadSettings ***");
    loadSettings();
    LOG_DEBUG("PreferencesDialog", "*** loadSettings completed, resizing ***");
    resize(UIDefaults::PREFERENCES_WIDTH, UIDefaults::PREFERENCES_HEIGHT);  // Wider for sidebar layout
    LOG_DEBUG("PreferencesDialog", QString("*** PreferencesDialog fully initialized with window title: %1").arg(windowTitle()));

    // Connect K4 Discovery signals
    connect(m_k4Discovery, &K4Discovery::radioFound, this, &PreferencesDialog::onK4RadioFound);
    connect(m_k4Discovery, &K4Discovery::discoveryFinished, this, &PreferencesDialog::onK4DiscoveryFinished);

    // Connect Icom Discovery signals
    connect(m_icomDiscovery, &IcomDiscovery::radioFound, this, &PreferencesDialog::onIcomRadioFound);
    connect(m_icomDiscovery, &IcomDiscovery::discoveryFinished, this, &PreferencesDialog::onIcomDiscoveryFinished);

    // Setup serial port refresh timer (5 second interval)
    m_portRefreshTimer->setInterval(5000);
    connect(m_portRefreshTimer, &QTimer::timeout, this, &PreferencesDialog::refreshSerialPorts);
}

void PreferencesDialog::setupUI() {
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating main layout ***");
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create horizontal splitter for sidebar + content
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Left sidebar: Category list
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating category list ***");
    m_categoryList = new QListWidget(this);
    m_categoryList->setMaximumWidth(160);
    m_categoryList->setMinimumWidth(140);
    m_categoryList->addItem("Station");
    m_categoryList->addItem("Hardware");
    m_categoryList->addItem("DX Cluster");
    m_categoryList->addItem("SCP");
    m_categoryList->addItem("UDP Broadcast");
    m_categoryList->addItem("Network");
    m_categoryList->addItem("Appearance");
    m_categoryList->addItem("Logging");
    m_categoryList->addItem("Backup");
    m_categoryList->addItem("Contest");
    m_categoryList->addItem("CW Settings");
    m_categoryList->addItem("Web Server");
    m_categoryList->addItem("Advanced");

    // Right panel: Stacked widget with settings pages
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating settings stack ***");
    m_settingsStack = new QStackedWidget(this);
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Station page ***");
    m_settingsStack->addWidget(createStationTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Hardware page ***");
    m_settingsStack->addWidget(createHardwareTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating DX Cluster page ***");
    m_settingsStack->addWidget(createDXClusterTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating SCP page ***");
    m_settingsStack->addWidget(createSCPTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating UDP Broadcast page ***");
    m_settingsStack->addWidget(createUDPBroadcastTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Network page ***");
    m_settingsStack->addWidget(createNetworkTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Appearance page ***");
    m_settingsStack->addWidget(createAppearanceTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Logging page ***");
    m_settingsStack->addWidget(createLoggingTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Backup page ***");
    m_settingsStack->addWidget(createBackupTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Contest page ***");
    m_settingsStack->addWidget(createContestTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating CW Settings page ***");
    m_settingsStack->addWidget(createCWSettingsTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Web Server page ***");
    m_settingsStack->addWidget(createWebServerTab());
    LOG_DEBUG("PreferencesDialog", "*** setupUI: Creating Advanced page ***");
    m_settingsStack->addWidget(createAdvancedTab());

    // Connect list selection to stack page switching
    connect(m_categoryList, &QListWidget::currentRowChanged,
            m_settingsStack, &QStackedWidget::setCurrentIndex);

    // Add sidebar and content to horizontal layout
    contentLayout->addWidget(m_categoryList);
    contentLayout->addWidget(m_settingsStack, 1);  // Give stack more stretch

    mainLayout->addLayout(contentLayout);

    // Button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);

    QPushButton* applyButton = buttonBox->addButton(QDialogButtonBox::Apply);
    connect(applyButton, &QPushButton::clicked, this, &PreferencesDialog::onApply);

    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Select first category by default
    m_categoryList->setCurrentRow(0);
}

QWidget* PreferencesDialog::createStationTab() {
    QWidget* stationTab = new QWidget(this);
    stationTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* layout = new QVBoxLayout(stationTab);

    QGroupBox* stationGroup = new QGroupBox("Station Information", this);
    QFormLayout* formLayout = new QFormLayout(stationGroup);

    // Callsign
    m_callsignEdit = new QLineEdit(this);
    m_callsignEdit->setPlaceholderText("W1AW");
    m_callsignEdit->setToolTip("Your callsign (used in contests and logging)");
    formLayout->addRow("Callsign:", m_callsignEdit);

    // First Name (for contests like NAQP that include name in exchange)
    m_firstNameEdit = new QLineEdit(this);
    m_firstNameEdit->setPlaceholderText("Tom");
    m_firstNameEdit->setToolTip("Your first name (used in contests like NAQP that include name in exchange)");
    formLayout->addRow("First Name:", m_firstNameEdit);

    // Last Name
    m_lastNameEdit = new QLineEdit(this);
    m_lastNameEdit->setPlaceholderText("Smith");
    m_lastNameEdit->setToolTip("Your last name (for Cabrillo file headers)");
    formLayout->addRow("Last Name:", m_lastNameEdit);

    // License Class (US only)
    m_licenseClassCombo = new QComboBox(this);
    m_licenseClassCombo->addItems({"None", "Technician", "General", "Extra"});
    m_licenseClassCombo->setToolTip("US amateur radio license class (validates phone segment privileges on HF bands). Set to 'None' to disable warnings.");
    formLayout->addRow("License Class:", m_licenseClassCombo);

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

    // State/Province
    m_stateEdit = new QLineEdit(this);
    m_stateEdit->setPlaceholderText("CT");
    m_stateEdit->setToolTip("Your state/province (used for some contests like Sweepstakes)");
    formLayout->addRow("State/Province:", m_stateEdit);

    // ARRL Section
    m_arrlSectionEdit = new QLineEdit(this);
    m_arrlSectionEdit->setPlaceholderText("CT");
    m_arrlSectionEdit->setToolTip("Your ARRL section (used for contests like Sweepstakes)");
    formLayout->addRow("ARRL Section:", m_arrlSectionEdit);

    // County
    m_countyEdit = new QLineEdit(this);
    m_countyEdit->setPlaceholderText("GAD");
    m_countyEdit->setToolTip("Your county (used for contests like Florida QSO Party)");
    formLayout->addRow("County:", m_countyEdit);

    // Default operator
    m_operatorEdit = new QLineEdit(this);
    m_operatorEdit->setPlaceholderText("Same as station callsign");
    m_operatorEdit->setToolTip("Default operator callsign (for multi-op contests)");
    formLayout->addRow("Default Operator:", m_operatorEdit);

    layout->addWidget(stationGroup);
    layout->addStretch();

    return stationTab;
}

QWidget* PreferencesDialog::createHardwareTab() {
    QWidget* hardwareTab = new QWidget(this);
    hardwareTab->setAutoFillBackground(true);
    QVBoxLayout* layout = new QVBoxLayout(hardwareTab);

    // Create tab widget for hardware sub-categories
    QTabWidget* hardwareTabs = new QTabWidget(hardwareTab);

    // Add sub-tabs
    hardwareTabs->addTab(createRadioSettingsWidget(), "Radio");
    hardwareTabs->addTab(createAmplifierSettingsWidget(), "Amplifier");
    hardwareTabs->addTab(createRotatorSettingsWidget(), "Rotator");

    // Future hardware tabs (commented placeholders)
    // hardwareTabs->addTab(createWinKeyerSettingsWidget(), "WinKeyer");

    layout->addWidget(hardwareTabs);
    return hardwareTab;
}

QWidget* PreferencesDialog::createRadioSettingsWidget() {
    QWidget* radioWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(radioWidget);

    // Radio Profile Management Section
    QGroupBox* profileGroup = new QGroupBox("Radio Profiles", this);
    QVBoxLayout* profileVLayout = new QVBoxLayout(profileGroup);

    // Profile selector combo box
    m_profileSelectorCombo = new QComboBox(this);
    m_profileSelectorCombo->setMinimumWidth(300);
    connect(m_profileSelectorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onProfileSelected);

    // Profile management buttons
    QHBoxLayout* profileButtonLayout = new QHBoxLayout();
    m_newProfileButton = new QPushButton("New Profile", this);
    m_editProfileButton = new QPushButton("Edit Name/Notes", this);
    m_deleteProfileButton = new QPushButton("Delete", this);
    m_setActiveButton = new QPushButton("Set as Active", this);

    m_newProfileButton->setMaximumWidth(120);
    m_editProfileButton->setMaximumWidth(140);
    m_deleteProfileButton->setMaximumWidth(80);
    m_setActiveButton->setMaximumWidth(120);

    connect(m_newProfileButton, &QPushButton::clicked, this, &PreferencesDialog::onNewProfile);
    connect(m_editProfileButton, &QPushButton::clicked, this, &PreferencesDialog::onEditProfile);
    connect(m_deleteProfileButton, &QPushButton::clicked, this, &PreferencesDialog::onDeleteProfile);
    connect(m_setActiveButton, &QPushButton::clicked, this, &PreferencesDialog::onSetActiveProfile);

    profileButtonLayout->addWidget(m_newProfileButton);
    profileButtonLayout->addWidget(m_editProfileButton);
    profileButtonLayout->addWidget(m_deleteProfileButton);
    profileButtonLayout->addWidget(m_setActiveButton);
    profileButtonLayout->addStretch();

    // Active profile label
    m_activeProfileLabel = new QLabel("<i>Currently active: Default</i>", this);
    m_activeProfileLabel->setStyleSheet("color: #666;");

    // Add widgets to profile group
    QFormLayout* profileLayout = new QFormLayout();
    profileLayout->addRow("Select Profile:", m_profileSelectorCombo);
    profileVLayout->addLayout(profileLayout);
    profileVLayout->addLayout(profileButtonLayout);
    profileVLayout->addWidget(m_activeProfileLabel);

    layout->addWidget(profileGroup);

    // Horizontal separator line
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    // Radio model selection
    QGroupBox* modelGroup = new QGroupBox("Radio Model", this);
    QFormLayout* modelLayout = new QFormLayout(modelGroup);

    m_radioModelCombo = new QComboBox(this);
    m_radioModelCombo->setEditable(true);  // Make searchable
    m_radioModelCombo->setInsertPolicy(QComboBox::NoInsert);  // Don't add typed text as new items

    // Populate radio list will be called after checkboxes are created
    // (so filtering works correctly)

    m_customModelEdit = new QLineEdit(this);
    m_customModelEdit->setPlaceholderText("Enter hamlib model ID (e.g., 2046)");
    m_customModelEdit->setVisible(false);

    modelLayout->addRow("Model:", m_radioModelCombo);
    modelLayout->addRow("Custom Model ID:", m_customModelEdit);

    // Radio status filter checkboxes (in same group)
    m_showStableRadiosCheck = new QCheckBox("Stable", this);
    m_showBetaRadiosCheck = new QCheckBox("Beta", this);
    m_showAlphaRadiosCheck = new QCheckBox("Alpha", this);
    m_showUntestedRadiosCheck = new QCheckBox("Untested", this);

    // Explicitly set visibility and size policy for Windows compatibility
    m_showStableRadiosCheck->setVisible(true);
    m_showBetaRadiosCheck->setVisible(true);
    m_showAlphaRadiosCheck->setVisible(true);
    m_showUntestedRadiosCheck->setVisible(true);

    // Ensure minimum size for Windows rendering
    m_showStableRadiosCheck->setMinimumWidth(80);
    m_showBetaRadiosCheck->setMinimumWidth(60);
    m_showAlphaRadiosCheck->setMinimumWidth(70);
    m_showUntestedRadiosCheck->setMinimumWidth(90);

    // Default to only Stable checked
    m_showStableRadiosCheck->setChecked(true);
    m_showBetaRadiosCheck->setChecked(false);
    m_showAlphaRadiosCheck->setChecked(false);
    m_showUntestedRadiosCheck->setChecked(false);

    connect(m_showStableRadiosCheck, &QCheckBox::stateChanged,
            this, &PreferencesDialog::onRadioStatusFilterChanged);
    connect(m_showBetaRadiosCheck, &QCheckBox::stateChanged,
            this, &PreferencesDialog::onRadioStatusFilterChanged);
    connect(m_showAlphaRadiosCheck, &QCheckBox::stateChanged,
            this, &PreferencesDialog::onRadioStatusFilterChanged);
    connect(m_showUntestedRadiosCheck, &QCheckBox::stateChanged,
            this, &PreferencesDialog::onRadioStatusFilterChanged);

    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Show Status:", this));
    filterLayout->addWidget(m_showStableRadiosCheck);
    filterLayout->addWidget(m_showBetaRadiosCheck);
    filterLayout->addWidget(m_showAlphaRadiosCheck);
    filterLayout->addWidget(m_showUntestedRadiosCheck);
    filterLayout->addStretch();
    filterLayout->setAlignment(Qt::AlignLeft);

    modelLayout->addRow("", filterLayout);

    layout->addWidget(modelGroup);

    // Radio Interface Type (RadioFactory selection)
    QGroupBox* interfaceGroup = new QGroupBox("Radio Interface", this);
    QFormLayout* interfaceLayout = new QFormLayout(interfaceGroup);

    m_radioTypeCombo = new QComboBox(this);
    m_radioTypeCombo->addItem("Auto (Recommended)", -1);           // -1 = Auto
    m_radioTypeCombo->addItem("Hamlib (Universal)", 0);            // 0 = HAMLIB
    m_radioTypeCombo->addItem("K4 Direct (K4 only, 5-10x faster)", 1);  // 1 = K4_DIRECT
    m_radioTypeCombo->addItem("Icom Direct (Icom network radios)", 2);  // 2 = ICOM_DIRECT
    m_radioTypeCombo->setCurrentIndex(0);  // Default to Auto

    // Tooltip explaining the options
    m_radioTypeCombo->setToolTip(
        "Auto: Automatically selects the best interface for your radio (K4 Direct for K4, Icom Direct for supported Icom, Hamlib for others)\n"
        "Hamlib: Universal compatibility, works with all radios\n"
        "K4 Direct: Direct TCP control for Elecraft K4 (5-10x faster than Hamlib, requires K4/K4D/K4HD)\n"
        "Icom Direct: Native Icom network protocol (IC-905, IC-9700, IC-7610, IC-7600, IC-7300MK2, IC-705, IC-R8600, IC-7850, IC-7851, IC-7760)"
    );

    interfaceLayout->addRow("Type:", m_radioTypeCombo);

    // Connect to update default port when radio type changes
    connect(m_radioTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onRadioTypeChanged);

    // Add explanatory label
    QLabel* infoLabel = new QLabel(
        "<small><i>K4 Direct provides 5-10x faster radio control for Elecraft K4 radios.<br>"
        "For all other radios, Hamlib is used automatically.</i></small>",
        this
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666;");
    interfaceLayout->addRow("", infoLabel);

    layout->addWidget(interfaceGroup);

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

    // Serial port dropdown with detected ports
    QHBoxLayout* portLayout = new QHBoxLayout();
    m_serialPortCombo = new QComboBox(this);
    m_serialPortCombo->setToolTip("Select a detected serial port, or type a custom port name below");
    m_serialPortCombo->setMinimumWidth(200);
    portLayout->addWidget(m_serialPortCombo, 1);

    m_refreshPortsButton = new QPushButton("Refresh", this);
    m_refreshPortsButton->setToolTip("Rescan for serial ports");
    m_refreshPortsButton->setMaximumWidth(80);
    connect(m_refreshPortsButton, &QPushButton::clicked, this, &PreferencesDialog::refreshSerialPorts);
    portLayout->addWidget(m_refreshPortsButton);

    serialLayout->addRow("Port:", portLayout);

    // Manual entry fallback
    m_serialPortEdit = new QLineEdit(this);
    m_serialPortEdit->setPlaceholderText("Manual entry (e.g., COM10 or /dev/ttyUSB0)");
    m_serialPortEdit->setToolTip("Type a port name manually if not in dropdown");
    serialLayout->addRow("Or manual:", m_serialPortEdit);

    m_baudRateCombo = new QComboBox(this);
    m_baudRateCombo->addItems({"4800", "9600", "19200", "38400", "57600", "115200"});
    m_baudRateCombo->setCurrentText("38400");

    m_dataBitsCombo = new QComboBox(this);
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    m_dataBitsCombo->setToolTip("Number of data bits (default: 8)\nLeave at default unless radio requires specific setting");

    m_stopBitsCombo = new QComboBox(this);
    m_stopBitsCombo->addItems({"1", "2"});
    m_stopBitsCombo->setCurrentText("1");
    m_stopBitsCombo->setToolTip("Number of stop bits (default: 1)\nLeave at default unless radio requires specific setting");

    m_parityCombo = new QComboBox(this);
    m_parityCombo->addItems({"None", "Odd", "Even"});
    m_parityCombo->setCurrentIndex(0);  // None
    m_parityCombo->setToolTip("Parity checking (default: None)\nLeave at default unless radio requires specific setting");

    serialLayout->addRow("Baud Rate:", m_baudRateCombo);
    serialLayout->addRow("Data Bits:", m_dataBitsCombo);
    serialLayout->addRow("Stop Bits:", m_stopBitsCombo);
    serialLayout->addRow("Parity:", m_parityCombo);
    layout->addWidget(m_serialGroup);

    // Initial population of serial ports
    refreshSerialPorts();

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

    // Icom network credentials (only used for Icom Direct)
    m_icomUsernameEdit = new QLineEdit(this);
    m_icomUsernameEdit->setPlaceholderText("Username (optional, usually blank)");
    m_icomUsernameEdit->setToolTip("Icom network username (can be left blank for most radios)");

    m_icomPasswordEdit = new QLineEdit(this);
    m_icomPasswordEdit->setPlaceholderText("Password (optional, usually blank)");
    m_icomPasswordEdit->setEchoMode(QLineEdit::Password);
    m_icomPasswordEdit->setToolTip("Icom network password (can be left blank for most radios)");

    m_icomClientNameEdit = new QLineEdit(this);
    m_icomClientNameEdit->setText("TR4QT");
    m_icomClientNameEdit->setToolTip("Client identifier for Icom network protocol");

    networkLayout->addRow("Icom Username:", m_icomUsernameEdit);
    networkLayout->addRow("Icom Password:", m_icomPasswordEdit);
    networkLayout->addRow("Icom Client Name:", m_icomClientNameEdit);

    // Find Network Radios button (text changes based on selected radio type)
    m_findK4Button = new QPushButton("Find Radios on Network", this);
    m_findK4Button->setToolTip("Broadcast UDP discovery message to find radios on the network");
    connect(m_findK4Button, &QPushButton::clicked, this, &PreferencesDialog::onFindNetworkRadios);
    networkLayout->addRow("", m_findK4Button);

    layout->addWidget(m_networkGroup);
    m_networkGroup->setVisible(false);

    // Advanced radio settings
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    QFormLayout* advancedLayout = new QFormLayout(advancedGroup);

    // CI-V Address selection (for Icom radios)
    m_civAddressWidget = new CivAddressWidget(this);

    m_pollIntervalSpin = new QSpinBox(this);
    m_pollIntervalSpin->setRange(100, 5000);
    m_pollIntervalSpin->setValue(5000);
    m_pollIntervalSpin->setSingleStep(500);
    m_pollIntervalSpin->setSuffix(" ms");
    m_pollIntervalSpin->setToolTip("Polling fallback interval (CI-V Transceive provides instant updates)");

    advancedLayout->addRow("CI-V Address:", m_civAddressWidget);
    advancedLayout->addRow("Poll Interval:", m_pollIntervalSpin);

    m_autoConnectCheck = new QCheckBox("Auto-connect on startup", this);
    m_autoConnectCheck->setChecked(true);
    m_autoConnectCheck->setToolTip("Automatically connect to radio when application starts");
    advancedLayout->addRow("", m_autoConnectCheck);

    layout->addWidget(advancedGroup);

    // Test connection button and status label
    QHBoxLayout* testLayout = new QHBoxLayout();
    m_testConnectionButton = new QPushButton("Test Connection", this);
    connect(m_testConnectionButton, &QPushButton::clicked, this, &PreferencesDialog::onTestRadioConnection);
    testLayout->addWidget(m_testConnectionButton);

    m_connectionStatusLabel = new QLabel(this);
    m_connectionStatusLabel->setStyleSheet("color: #666; font-style: italic;");
    m_connectionStatusLabel->hide();  // Hidden by default
    testLayout->addWidget(m_connectionStatusLabel);

    testLayout->addStretch();
    layout->addLayout(testLayout);

    layout->addStretch();

    // Now that all widgets are created, populate radio list with filtering
    populateRadioList();

    // Connect signals and unblock
    connect(m_radioModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onRadioModelChanged);
    m_radioModelCombo->blockSignals(false);

    return radioWidget;
}

QWidget* PreferencesDialog::createAmplifierSettingsWidget() {
    QWidget* amplifierWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(amplifierWidget);

    // Amplifier Configuration
    QGroupBox* configGroup = new QGroupBox("Amplifier Configuration", this);
    QFormLayout* configLayout = new QFormLayout(configGroup);

    // Enable amplifier checkbox
    m_amplifierEnabledCheck = new QCheckBox("Enable amplifier control", this);
    m_amplifierEnabledCheck->setToolTip("Enable amplifier monitoring and control");
    configLayout->addRow("", m_amplifierEnabledCheck);

    // Model selection
    m_amplifierModelCombo = new QComboBox(this);
    m_amplifierModelCombo->setToolTip("Select amplifier model");
    populateAmplifierList();
    connect(m_amplifierModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onAmplifierModelChanged);
    configLayout->addRow("Model:", m_amplifierModelCombo);

    // Connection type
    m_amplifierConnectionTypeCombo = new QComboBox(this);
    m_amplifierConnectionTypeCombo->addItem("Direct (UDP)", "direct");
    m_amplifierConnectionTypeCombo->addItem("Hamlib (Serial)", "hamlib");
    m_amplifierConnectionTypeCombo->setToolTip("Direct: Native UDP control\nHamlib: Universal serial control");
    connect(m_amplifierConnectionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onAmplifierConnectionTypeChanged);
    configLayout->addRow("Connection:", m_amplifierConnectionTypeCombo);

    // Port configuration (IP:port or serial port)
    m_amplifierPortEdit = new QLineEdit(this);
    m_amplifierPortEdit->setPlaceholderText("192.168.1.100:1500");
    m_amplifierPortEdit->setToolTip("IP address and port (e.g., 192.168.1.100:1500)");
    configLayout->addRow("Network:", m_amplifierPortEdit);

    // Serial settings container (hidden for network mode)
    m_amplifierSerialSettingsWidget = new QWidget(this);
    QFormLayout* serialLayout = new QFormLayout(m_amplifierSerialSettingsWidget);
    serialLayout->setContentsMargins(0, 0, 0, 0);

    // Baud rate
    m_amplifierBaudRateCombo = new QComboBox(this);
    m_amplifierBaudRateCombo->addItems({"4800", "9600", "19200", "38400", "57600", "115200"});
    m_amplifierBaudRateCombo->setCurrentText("38400");
    m_amplifierBaudRateCombo->setToolTip("Serial port baud rate");
    serialLayout->addRow("Baud Rate:", m_amplifierBaudRateCombo);

    configLayout->addRow("", m_amplifierSerialSettingsWidget);
    m_amplifierSerialSettingsWidget->hide();  // Hidden by default (network mode)

    // Auto-connect
    m_amplifierAutoConnectCheck = new QCheckBox("Auto-connect on startup", this);
    m_amplifierAutoConnectCheck->setToolTip("Automatically connect to amplifier when TR4QT starts");
    configLayout->addRow("", m_amplifierAutoConnectCheck);

    // Test connection button
    m_testAmplifierConnectionButton = new QPushButton("Test Connection", this);
    connect(m_testAmplifierConnectionButton, &QPushButton::clicked,
            this, &PreferencesDialog::onTestAmplifierConnection);
    configLayout->addRow("", m_testAmplifierConnectionButton);

    layout->addWidget(configGroup);

    // Add informational label
    QLabel* infoLabel = new QLabel(
        "Amplifier control provides monitoring and control for compatible amplifiers. "
        "Direct mode uses native UDP protocol (KPA1500). "
        "Hamlib mode provides universal serial support for many amplifiers.",
        this
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    layout->addWidget(infoLabel);

    layout->addStretch();
    return amplifierWidget;
}

QWidget* PreferencesDialog::createRotatorSettingsWidget() {
    QWidget* rotatorWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(rotatorWidget);

    // Rotator Configuration
    QGroupBox* configGroup = new QGroupBox("Rotator Configuration", this);
    QFormLayout* configLayout = new QFormLayout(configGroup);

    // Enable rotator checkbox
    m_rotatorEnabledCheck = new QCheckBox("Enable rotator control", this);
    m_rotatorEnabledCheck->setToolTip("Enable rotator monitoring and control");
    configLayout->addRow("", m_rotatorEnabledCheck);

    // Model selection
    m_rotatorModelCombo = new QComboBox(this);
    m_rotatorModelCombo->setToolTip("Select rotator model");
    populateRotatorList();
    connect(m_rotatorModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onRotatorModelChanged);
    configLayout->addRow("Model:", m_rotatorModelCombo);

    // Connection type
    m_rotatorConnectionTypeCombo = new QComboBox(this);
    m_rotatorConnectionTypeCombo->addItem("Direct (UDP)", "direct");
    m_rotatorConnectionTypeCombo->addItem("Hamlib", "hamlib");
    m_rotatorConnectionTypeCombo->setToolTip("Direct: Native UDP control (PSTRotator)\nHamlib: Universal serial control");
    connect(m_rotatorConnectionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onRotatorConnectionTypeChanged);
    configLayout->addRow("Connection:", m_rotatorConnectionTypeCombo);

    // Network settings container
    m_rotatorNetworkSettingsWidget = new QWidget(this);
    QFormLayout* networkLayout = new QFormLayout(m_rotatorNetworkSettingsWidget);
    networkLayout->setContentsMargins(0, 0, 0, 0);

    m_rotatorIpEdit = new QLineEdit(this);
    m_rotatorIpEdit->setPlaceholderText("192.168.1.100");
    m_rotatorIpEdit->setToolTip("IP address of rotator controller");
    networkLayout->addRow("IP Address:", m_rotatorIpEdit);

    m_rotatorPortSpin = new QSpinBox(this);
    m_rotatorPortSpin->setRange(1, 65535);
    m_rotatorPortSpin->setValue(12000);  // Default PSTRotator port
    m_rotatorPortSpin->setToolTip("UDP port for rotator communication");
    networkLayout->addRow("Port:", m_rotatorPortSpin);

    configLayout->addRow("", m_rotatorNetworkSettingsWidget);

    // Serial settings container
    m_rotatorSerialSettingsWidget = new QWidget(this);
    QFormLayout* serialLayout = new QFormLayout(m_rotatorSerialSettingsWidget);
    serialLayout->setContentsMargins(0, 0, 0, 0);

    m_rotatorSerialPortEdit = new QLineEdit(this);
    m_rotatorSerialPortEdit->setPlaceholderText("/dev/ttyUSB0 or COM3");
    m_rotatorSerialPortEdit->setToolTip("Serial port device");
    serialLayout->addRow("Serial Port:", m_rotatorSerialPortEdit);

    m_rotatorBaudRateCombo = new QComboBox(this);
    m_rotatorBaudRateCombo->addItems({"4800", "9600", "19200", "38400", "57600", "115200"});
    m_rotatorBaudRateCombo->setCurrentText("9600");
    m_rotatorBaudRateCombo->setToolTip("Serial port baud rate");
    serialLayout->addRow("Baud Rate:", m_rotatorBaudRateCombo);

    configLayout->addRow("", m_rotatorSerialSettingsWidget);
    m_rotatorSerialSettingsWidget->hide();  // Hidden by default

    // Auto-connect
    m_rotatorAutoConnectCheck = new QCheckBox("Auto-connect on startup", this);
    m_rotatorAutoConnectCheck->setToolTip("Automatically connect to rotator when TR4QT starts");
    configLayout->addRow("", m_rotatorAutoConnectCheck);

    // Test connection button
    m_testRotatorConnectionButton = new QPushButton("Test Connection", this);
    connect(m_testRotatorConnectionButton, &QPushButton::clicked,
            this, &PreferencesDialog::onTestRotatorConnection);
    configLayout->addRow("", m_testRotatorConnectionButton);

    layout->addWidget(configGroup);

    // Add informational label
    QLabel* infoLabel = new QLabel(
        "Rotator control provides azimuth monitoring and control for compatible antenna rotators. "
        "Direct mode uses native UDP protocol (PSTRotator). "
        "Hamlib mode provides universal support for EASYCOMM, ROTOREZ, GS-232, and other protocols.",
        this
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    layout->addWidget(infoLabel);

    layout->addStretch();
    return rotatorWidget;
}

QWidget* PreferencesDialog::createDXClusterTab() {
    QWidget* dxClusterTab = new QWidget(this);
    dxClusterTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* layout = new QVBoxLayout(dxClusterTab);

    QGroupBox* clusterGroup = new QGroupBox("DX Cluster Settings", this);
    QFormLayout* formLayout = new QFormLayout(clusterGroup);

    // DX Cluster callsign
    m_dxClusterCallsignEdit = new QLineEdit(this);
    m_dxClusterCallsignEdit->setPlaceholderText("Leave blank to use station callsign");
    formLayout->addRow("DX Cluster Callsign:", m_dxClusterCallsignEdit);

    // Default server (editable combo box with downloaded server list)
    m_dxClusterServerCombo = new QComboBox(this);
    m_dxClusterServerCombo->setEditable(true);
    m_dxClusterServerCombo->setInsertPolicy(QComboBox::NoInsert);
    m_dxClusterServerCombo->lineEdit()->setPlaceholderText("server.example.com:7373");

    // Populate with downloaded server list
    AppSettings& settings = AppSettings::instance();
    QStringList serverList = settings.getDXClusterList();
    if (!serverList.isEmpty()) {
        m_dxClusterServerCombo->addItems(serverList);
    }

    // Add validation when text changes
    connect(m_dxClusterServerCombo, &QComboBox::currentTextChanged,
            this, &PreferencesDialog::onDXClusterServerChanged);

    formLayout->addRow("Default Server:", m_dxClusterServerCombo);

    // Auto-connect
    m_dxClusterAutoConnectCheck = new QCheckBox("Auto-connect to DX Cluster on startup", this);
    formLayout->addRow("", m_dxClusterAutoConnectCheck);

    // Enable LOTW lookup
    m_enableLotwLookupCheck = new QCheckBox("Enable LOTW user lookup for DX spots", this);
    m_enableLotwLookupCheck->setToolTip("Perform database lookup to identify LOTW users for each DX spot.\n"
                                        "Disable this if spots are arriving very quickly to improve performance.");

    // Connect checkbox change to immediately update settings and refresh band map
    connect(m_enableLotwLookupCheck, &QCheckBox::toggled,
            this, [this](bool checked) {
                AppSettings& settings = AppSettings::instance();
                settings.setEnableLotwLookup(checked);
                LOG_DEBUG("PreferencesDialog", QString("LOTW lookup %1").arg(checked ? "enabled" : "disabled"));
                emit lotwSettingsChanged();
            });

    formLayout->addRow("", m_enableLotwLookupCheck);

    // LOTW minimum upload months
    m_lotwMinUploadMonthsSpin = new QSpinBox(this);
    m_lotwMinUploadMonthsSpin->setRange(1, 120);  // 1 month to 10 years
    m_lotwMinUploadMonthsSpin->setValue(24);      // Default: 24 months
    m_lotwMinUploadMonthsSpin->setSuffix(" months");
    m_lotwMinUploadMonthsSpin->setToolTip("Only consider users active if they uploaded to LOTW within this timeframe.\n"
                                          "Users who haven't uploaded recently won't be marked as LOTW users.\n"
                                          "Default: 24 months (2 years)");

    // Connect value change to immediately update settings and refresh band map
    connect(m_lotwMinUploadMonthsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
                AppSettings& settings = AppSettings::instance();
                settings.setLotwMinUploadMonths(value);
                LOG_DEBUG("PreferencesDialog", QString("LOTW minimum upload months changed to %1").arg(value));
                emit lotwSettingsChanged();
            });

    formLayout->addRow("LOTW Recent Activity:", m_lotwMinUploadMonthsSpin);

    // Download cluster list button
    m_downloadClusterListButton = new QPushButton("Download Server List", this);
    connect(m_downloadClusterListButton, &QPushButton::clicked,
            this, &PreferencesDialog::onDownloadClusterList);
    formLayout->addRow("", m_downloadClusterListButton);

    layout->addWidget(clusterGroup);

    // Band Map Settings
    QGroupBox* bandMapGroup = new QGroupBox("Band Map Settings", this);
    QFormLayout* bandMapLayout = new QFormLayout(bandMapGroup);

    m_spotExpirySpin = new QSpinBox(this);
    m_spotExpirySpin->setRange(60, 3600);  // 1 minute to 1 hour
    m_spotExpirySpin->setValue(600);       // Default: 10 minutes
    m_spotExpirySpin->setSuffix(" seconds");
    m_spotExpirySpin->setToolTip("How long spots remain in the band map before expiring\n"
                                   "Default: 600 seconds (10 minutes)");
    bandMapLayout->addRow("Spot Expiry Time:", m_spotExpirySpin);

    m_newSpotThresholdSpin = new QSpinBox(this);
    m_newSpotThresholdSpin->setRange(5, 300);  // 5 seconds to 5 minutes
    m_newSpotThresholdSpin->setValue(60);      // Default: 1 minute
    m_newSpotThresholdSpin->setSuffix(" seconds");
    m_newSpotThresholdSpin->setToolTip("How long spots are highlighted as 'new' after appearing\n"
                                        "Default: 60 seconds (1 minute)");
    bandMapLayout->addRow("New Spot Highlight:", m_newSpotThresholdSpin);

    m_agingSpotThresholdSpin = new QSpinBox(this);
    m_agingSpotThresholdSpin->setRange(30, 600);  // 30 seconds to 10 minutes
    m_agingSpotThresholdSpin->setValue(120);      // Default: 2 minutes
    m_agingSpotThresholdSpin->setSuffix(" seconds");
    m_agingSpotThresholdSpin->setToolTip("How long before expiry spots start to fade/dim\n"
                                          "This is measured from the end (e.g., last 120 seconds before expiry)\n"
                                          "Default: 120 seconds (2 minutes)");
    bandMapLayout->addRow("Aging Threshold:", m_agingSpotThresholdSpin);

    layout->addWidget(bandMapGroup);

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

QWidget* PreferencesDialog::createSCPTab() {
    QWidget* scpTab = new QWidget(this);
    scpTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* mainLayout = new QVBoxLayout(scpTab);

    // SCP Settings Group
    QGroupBox* scpGroup = new QGroupBox("Super Check Partial (SCP) Settings", this);
    QFormLayout* scpLayout = new QFormLayout(scpGroup);

    // Enable SCP
    m_scpEnabledCheck = new QCheckBox("Enable real-time callsign matching", this);
    scpLayout->addRow("", m_scpEnabledCheck);

    // Include local logs
    m_scpIncludeLocalLogsCheck = new QCheckBox("Include callsigns from local contest logs", this);
    m_scpIncludeLocalLogsCheck->setChecked(true);
    scpLayout->addRow("", m_scpIncludeLocalLogsCheck);

    mainLayout->addWidget(scpGroup);

    // Database Status Group
    QGroupBox* statusGroup = new QGroupBox("Database Status", this);
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);

    m_scpStatusLabel = new QLabel(this);
    m_scpStatusLabel->setWordWrap(true);
    m_scpStatusLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; padding: 10px; }");

    // Get current stats from database
    SCPRepository repo;
    int masterCount = repo.getCallsignCountBySource("master_scp");
    int localCount = repo.getCallsignCountBySource("local_log");

    m_scpStatusLabel->setText(QString(
        "Master Database: %1 callsigns\n"
        "Local Logs: %2 callsigns\n"
        "Total: %3 callsigns available for matching"
    ).arg(masterCount).arg(localCount).arg(masterCount + localCount));

    statusLayout->addWidget(m_scpStatusLabel);
    mainLayout->addWidget(statusGroup);

    // Actions Group
    QGroupBox* actionsGroup = new QGroupBox("Database Management", this);
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);

    // Download MASTER.SCP button
    m_downloadSCPButton = new QPushButton("Download MASTER.SCP from Internet", this);
    connect(m_downloadSCPButton, &QPushButton::clicked, this, [this]() {
        SCPDownloader* downloader = new SCPDownloader(this);

        QProgressDialog* progress = new QProgressDialog(
            "Downloading MASTER.SCP...", "Cancel", 0, 100, this);
        progress->setWindowTitle("Download MASTER.SCP");
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(0);

        connect(downloader, &SCPDownloader::downloadProgress,
                this, [progress](qint64 received, qint64 total) {
            if (total > 0) {
                progress->setValue((received * 100) / total);
            }
        });

        connect(downloader, &SCPDownloader::downloadFinished,
                this, [this, progress, downloader](bool success, int count, const QString& error) {
            progress->close();
            progress->deleteLater();

            if (success) {
                // Update status display
                SCPRepository repo;
                int masterCount = repo.getCallsignCountBySource("master_scp");
                int localCount = repo.getCallsignCountBySource("local_log");

                m_scpStatusLabel->setText(QString(
                    "Master Database: %1 callsigns\n"
                    "Local Logs: %2 callsigns\n"
                    "Total: %3 callsigns available for matching"
                ).arg(masterCount).arg(localCount).arg(masterCount + localCount));

                DialogHelper::information(this, "Download Complete",
                    QString("MASTER.SCP downloaded successfully!\n\n%1 callsigns imported.").arg(count));
            } else {
                DialogHelper::critical(this, "Download Failed",
                    QString("Failed to download MASTER.SCP.\n\n%1").arg(error));
            }

            downloader->deleteLater();
        });

        connect(progress, &QProgressDialog::canceled, downloader, &SCPDownloader::cancel);
        downloader->downloadLatest();
    });
    actionsLayout->addWidget(m_downloadSCPButton);

    // Update from local logs button
    m_updateLocalSCPButton = new QPushButton("Update from Local Contest Logs", this);
    connect(m_updateLocalSCPButton, &QPushButton::clicked, this, [this]() {
        SCPCallsignExtractor extractor;
        QStringList callsigns = extractor.extractFromAllContests();

        if (callsigns.isEmpty()) {
            DialogHelper::information(this, "No Callsigns Found",
                "No contest logs found in ~/.tr4qt/logs/\n\n"
                "Create some contest logs first, then update the SCP database.");
            return;
        }

        SCPRepository repo;
        repo.clearBySource("local_log");
        int count = repo.bulkInsert(callsigns, "local_log");

        // Update status display
        int masterCount = repo.getCallsignCountBySource("master_scp");
        int localCount = repo.getCallsignCountBySource("local_log");

        m_scpStatusLabel->setText(QString(
            "Master Database: %1 callsigns\n"
            "Local Logs: %2 callsigns\n"
            "Total: %3 callsigns available for matching"
        ).arg(masterCount).arg(localCount).arg(masterCount + localCount));

        DialogHelper::information(this, "Update Complete",
            QString("Extracted %1 unique callsigns from local contest logs.").arg(count));
    });
    actionsLayout->addWidget(m_updateLocalSCPButton);

    mainLayout->addWidget(actionsGroup);

    // Info Section
    QLabel* infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; padding: 10px; }");
    infoLabel->setText(
        "Super Check Partial (SCP) provides real-time callsign matching as you type.\n\n"
        "Top 5 matches are displayed to the right of the callsign field to help identify\n"
        "stations when partial callsigns are copied from weak signals.\n\n"
        "Download MASTER.SCP for worldwide coverage, and augment with callsigns from\n"
        "your local contest logs for better regional matching."
    );
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return scpTab;
}

QWidget* PreferencesDialog::createUDPBroadcastTab() {
    QWidget* udpTab = new QWidget(this);
    udpTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
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

QWidget* PreferencesDialog::createNetworkTab() {
    QWidget* networkTab = new QWidget(this);
    networkTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* mainLayout = new QVBoxLayout(networkTab);

    // Network Settings group
    QGroupBox* networkGroup = new QGroupBox("Network Settings", this);
    QFormLayout* formLayout = new QFormLayout(networkGroup);

    // Computer ID
    m_computerIDEdit = new QLineEdit(this);
    m_computerIDEdit->setMaxLength(1);
    m_computerIDEdit->setPlaceholderText("A");
    m_computerIDEdit->setToolTip("Computer ID for networked multi-station operation (A-Z)");
    formLayout->addRow("Computer ID:", m_computerIDEdit);

    mainLayout->addWidget(networkGroup);

    // Help text
    QLabel* helpLabel = new QLabel(
        "Network settings for multi-station operation.\n\n"
        "Computer ID identifies this station in a networked TR4QT setup.\n"
        "Each station in the network should have a unique ID (A, B, C, etc.).\n"
        "The ID appears in the \"Id\" column of the QSO table.",
        this
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    mainLayout->addWidget(helpLabel);

    mainLayout->addStretch();

    return networkTab;
}

QWidget* PreferencesDialog::createAppearanceTab() {
    QWidget* appearanceTab = new QWidget(this);
    appearanceTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
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

    // Misc display font size (This Hr, Rate, Op, CQ/SP counts, etc.)
    m_miscDisplayFontSizeSpin = new QSpinBox(this);
    m_miscDisplayFontSizeSpin->setRange(6, 18);
    m_miscDisplayFontSizeSpin->setValue(9);
    m_miscDisplayFontSizeSpin->setSuffix(" pt");
    m_miscDisplayFontSizeSpin->setToolTip("Font size for miscellaneous display items (This Hr, Rate, Op, etc.)");
    formLayout->addRow("Misc Display:", m_miscDisplayFontSizeSpin);

    // SCP matches font size
    m_scpFontSizeSpin = new QSpinBox(this);
    m_scpFontSizeSpin->setRange(6, 18);
    m_scpFontSizeSpin->setValue(9);
    m_scpFontSizeSpin->setSuffix(" pt");
    m_scpFontSizeSpin->setToolTip("Font size for SCP (Super Check Partial) callsign matches");
    formLayout->addRow("SCP Matches:", m_scpFontSizeSpin);

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

    // Band Map Display group
    QGroupBox* bandMapGroup = new QGroupBox("Band Map Display", this);
    QVBoxLayout* bandMapLayout = new QVBoxLayout(bandMapGroup);

    m_useMetricDistanceCheck = new QCheckBox("Use kilometers (uncheck for miles)", this);
    m_useMetricDistanceCheck->setToolTip("Display distances in kilometers or miles");
    bandMapLayout->addWidget(m_useMetricDistanceCheck);

    layout->addWidget(bandMapGroup);

    // Band Needs Display group
    QGroupBox* needsGroup = new QGroupBox("Band Needs Display", this);
    QFormLayout* needsLayout = new QFormLayout(needsGroup);

    // Worked band color
    QHBoxLayout* workedColorLayout = new QHBoxLayout();
    m_workedColorButton = new QPushButton(this);
    m_workedColorButton->setFixedSize(80, 25);
    m_workedColorButton->setToolTip("Click to change color for bands already worked");
    connect(m_workedColorButton, &QPushButton::clicked, this, [this]() {
        QColor current = QColor(AppSettings::instance().getNeedsDisplayWorkedColor());
        QColor color = QColorDialog::getColor(current, this, "Select Worked Band Color");
        if (color.isValid()) {
            m_workedColorButton->setStyleSheet(
                QString("background-color: %1;").arg(color.name()));
            AppSettings::instance().setNeedsDisplayWorkedColor(color.name());
        }
    });
    workedColorLayout->addWidget(m_workedColorButton);
    workedColorLayout->addStretch();
    needsLayout->addRow("Worked Band Color:", workedColorLayout);

    // Needed band color
    QHBoxLayout* neededColorLayout = new QHBoxLayout();
    m_neededColorButton = new QPushButton(this);
    m_neededColorButton->setFixedSize(80, 25);
    m_neededColorButton->setToolTip("Click to change color for bands still needed");
    connect(m_neededColorButton, &QPushButton::clicked, this, [this]() {
        QColor current = QColor(AppSettings::instance().getNeedsDisplayNeededColor());
        QColor color = QColorDialog::getColor(current, this, "Select Needed Band Color");
        if (color.isValid()) {
            m_neededColorButton->setStyleSheet(
                QString("background-color: %1;").arg(color.name()));
            AppSettings::instance().setNeedsDisplayNeededColor(color.name());
        }
    });
    neededColorLayout->addWidget(m_neededColorButton);
    neededColorLayout->addStretch();
    needsLayout->addRow("Needed Band Color:", neededColorLayout);

    // VHF bands enabled checkbox
    m_vhfBandsEnabledCheck = new QCheckBox("Enable VHF bands (6M, 2M)", this);
    m_vhfBandsEnabledCheck->setToolTip("Show 6M and 2M bands in needs display (for VHF contests)");
    needsLayout->addRow("VHF Bands:", m_vhfBandsEnabledCheck);

    layout->addWidget(needsGroup);

    // DX Cluster Colors group
    QGroupBox* clusterColorsGroup = new QGroupBox("DX Cluster Spot Colors", this);
    QFormLayout* clusterColorsLayout = new QFormLayout(clusterColorsGroup);

    // Dupe color (already worked)
    QHBoxLayout* dupeColorLayout = new QHBoxLayout();
    m_clusterDupeColorButton = new QPushButton(this);
    m_clusterDupeColorButton->setFixedSize(80, 25);
    m_clusterDupeColorButton->setToolTip("Color for spots already worked (dupes)");
    connect(m_clusterDupeColorButton, &QPushButton::clicked, this, [this]() {
        QColor current = QColor(AppSettings::instance().getClusterDupeColor());
        QColor color = QColorDialog::getColor(current, this, "Select Dupe Spot Color");
        if (color.isValid()) {
            m_clusterDupeColorButton->setStyleSheet(
                QString("background-color: %1;").arg(color.name()));
            AppSettings::instance().setClusterDupeColor(color.name());
        }
    });
    dupeColorLayout->addWidget(m_clusterDupeColorButton);
    dupeColorLayout->addStretch();
    clusterColorsLayout->addRow("Dupe (Worked):", dupeColorLayout);

    // Multiplier color (new mult)
    QHBoxLayout* multColorLayout = new QHBoxLayout();
    m_clusterMultColorButton = new QPushButton(this);
    m_clusterMultColorButton->setFixedSize(80, 25);
    m_clusterMultColorButton->setToolTip("Color for spots that are new multipliers");
    connect(m_clusterMultColorButton, &QPushButton::clicked, this, [this]() {
        QColor current = QColor(AppSettings::instance().getClusterMultiplierColor());
        QColor color = QColorDialog::getColor(current, this, "Select Multiplier Spot Color");
        if (color.isValid()) {
            m_clusterMultColorButton->setStyleSheet(
                QString("background-color: %1;").arg(color.name()));
            AppSettings::instance().setClusterMultiplierColor(color.name());
        }
    });
    multColorLayout->addWidget(m_clusterMultColorButton);
    multColorLayout->addStretch();
    clusterColorsLayout->addRow("New Multiplier:", multColorLayout);

    layout->addWidget(clusterColorsGroup);

#ifdef Q_OS_MAC
    // macOS Window Behavior group
    QGroupBox* windowBehaviorGroup = new QGroupBox("Window Behavior (macOS)", this);
    QVBoxLayout* windowBehaviorLayout = new QVBoxLayout(windowBehaviorGroup);

    m_showAllWindowsOnActivateCheck = new QCheckBox("Show all windows when activating app", this);
    m_showAllWindowsOnActivateCheck->setToolTip(
        "When enabled, clicking on the app or its Dock icon brings all TR4QT windows to the front.\n"
        "When disabled, only the clicked window is raised (standard macOS behavior).");
    windowBehaviorLayout->addWidget(m_showAllWindowsOnActivateCheck);

    layout->addWidget(windowBehaviorGroup);
#else
    m_showAllWindowsOnActivateCheck = nullptr;  // Not used on other platforms
#endif

    layout->addStretch();

    return appearanceTab;
}

QWidget* PreferencesDialog::createLoggingTab() {
    QWidget* loggingTab = new QWidget(this);
    loggingTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
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

    // Advanced Settings group
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);

    m_hamlibDebugEnabledCheck = new QCheckBox("Enable Hamlib debug logging", this);
    m_hamlibDebugEnabledCheck->setToolTip("Enable verbose Hamlib radio library logging");
    advancedLayout->addWidget(m_hamlibDebugEnabledCheck);

    QLabel* hamlibWarning = new QLabel(
        "⚠️ WARNING: Hamlib debug logging generates extremely verbose output.\n"
        "Only enable this when requested by TR4QT support team for troubleshooting radio issues.",
        this
    );
    hamlibWarning->setWordWrap(true);
    hamlibWarning->setStyleSheet("QLabel { color: #d68910; font-size: 9pt; padding-left: 20px; }");
    advancedLayout->addWidget(hamlibWarning);

    mainLayout->addWidget(advancedGroup);

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

QWidget* PreferencesDialog::createBackupTab() {
    QWidget* backupTab = new QWidget(this);
    backupTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* mainLayout = new QVBoxLayout(backupTab);

    // Auto-backup Settings Group
    QGroupBox* autoBackupGroup = new QGroupBox("Auto-Backup Settings", this);
    QFormLayout* autoBackupLayout = new QFormLayout(autoBackupGroup);

    // Enable auto-backup
    m_autoBackupEnabledCheck = new QCheckBox("Enable automatic backups", this);
    autoBackupLayout->addRow("", m_autoBackupEnabledCheck);

    // Backup interval
    m_autoBackupIntervalSpin = new QSpinBox(this);
    m_autoBackupIntervalSpin->setRange(1, 1000);
    m_autoBackupIntervalSpin->setValue(50);
    m_autoBackupIntervalSpin->setSuffix(" QSOs");
    autoBackupLayout->addRow("Backup every:", m_autoBackupIntervalSpin);

    // Max backups to keep
    m_maxBackupsSpin = new QSpinBox(this);
    m_maxBackupsSpin->setRange(1, 100);
    m_maxBackupsSpin->setValue(10);
    m_maxBackupsSpin->setSuffix(" backups");
    autoBackupLayout->addRow("Keep most recent:", m_maxBackupsSpin);

    mainLayout->addWidget(autoBackupGroup);

    // Backup Location Group
    QGroupBox* locationGroup = new QGroupBox("Backup Location", this);
    QFormLayout* locationLayout = new QFormLayout(locationGroup);

    // Backup directory
    QHBoxLayout* dirLayout = new QHBoxLayout();
    m_backupDirectoryEdit = new QLineEdit(this);
    m_backupDirectoryEdit->setPlaceholderText("~/.tr4qt/backups");
    m_browseBackupDirButton = new QPushButton("Browse...", this);
    connect(m_browseBackupDirButton, &QPushButton::clicked,
            this, &PreferencesDialog::onBrowseBackupDirectory);
    dirLayout->addWidget(m_backupDirectoryEdit, 1);
    dirLayout->addWidget(m_browseBackupDirButton);
    locationLayout->addRow("Backup directory:", dirLayout);

    mainLayout->addWidget(locationGroup);

    // Info Section
    m_backupInfoLabel = new QLabel(this);
    m_backupInfoLabel->setWordWrap(true);
    m_backupInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; padding: 10px; }");
    m_backupInfoLabel->setText(
        "Automatic backups create a snapshot of your contest database every N QSOs.\n"
        "Backups are rotated automatically (oldest are deleted when limit is reached).\n\n"
        "You can manually backup or restore via Tools → Backup/Restore."
    );
    mainLayout->addWidget(m_backupInfoLabel);

    mainLayout->addStretch();

    return backupTab;
}

QWidget* PreferencesDialog::createContestTab() {
    QWidget* contestTab = new QWidget(this);
    contestTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* layout = new QVBoxLayout(contestTab);

    QGroupBox* contestGroup = new QGroupBox("Contest Settings", this);
    QFormLayout* formLayout = new QFormLayout(contestGroup);

    // Default contest - populate dynamically from ContestRegistry
    m_defaultContestCombo = new QComboBox(this);

    // Get all available contests from the registry
    ContestRegistry& registry = ContestRegistry::instance();
    QList<ContestMetadata> contests = registry.availableContests();

    // Build display list with mode-specific entries
    for (const ContestMetadata& metadata : contests) {
        if (metadata.hasSeparateContests) {
            // Separate entries for each mode (e.g., "CQ WW DX (CW)", "CQ WW DX (SSB)")
            for (ModeType mode : metadata.supportedModes) {
                QString modeSuffix = (mode == ModeType::CW) ? " (CW)" :
                                    (mode == ModeType::USB || mode == ModeType::LSB) ? " (SSB)" :
                                    (mode == ModeType::RTTY) ? " (RTTY)" : "";
                m_defaultContestCombo->addItem(metadata.displayName + modeSuffix);
            }
        } else {
            // Single entry for mixed-mode contest
            m_defaultContestCombo->addItem(metadata.displayName);
        }
    }

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

QWidget* PreferencesDialog::createCWSettingsTab() {
    QWidget* cwTab = new QWidget(this);
    cwTab->setAutoFillBackground(true);
    QVBoxLayout* layout = new QVBoxLayout(cwTab);

    QGroupBox* cwGroup = new QGroupBox("CW / Morse Settings", this);
    QFormLayout* formLayout = new QFormLayout(cwGroup);

    // Morse Speed
    m_morseWpmSpin = new QSpinBox(this);
    m_morseWpmSpin->setRange(5, 60);
    m_morseWpmSpin->setValue(25);
    m_morseWpmSpin->setSuffix(" WPM");
    m_morseWpmSpin->setToolTip("Morse code speed in words per minute for CW sending");
    formLayout->addRow("Morse Speed:", m_morseWpmSpin);

    // WPM Increment
    m_morseWpmIncrementSpin = new QSpinBox(this);
    m_morseWpmIncrementSpin->setRange(1, 10);
    m_morseWpmIncrementSpin->setValue(3);
    m_morseWpmIncrementSpin->setSuffix(" WPM");
    m_morseWpmIncrementSpin->setToolTip("WPM change when pressing PgUp/PgDn keys");
    formLayout->addRow("WPM Increment:", m_morseWpmIncrementSpin);

    // Enable Cut Numbers
    m_cutNumbersEnabledCheck = new QCheckBox("Enable Cut Numbers", this);
    m_cutNumbersEnabledCheck->setToolTip("Replace digits with letters for faster CW sending\n(T=0, A=1, U=2, V=3, E=5, B=7, D=8, N=9)");
    formLayout->addRow("", m_cutNumbersEnabledCheck);

    // Serial Number Width
    m_serialNumberWidthSpin = new QSpinBox(this);
    m_serialNumberWidthSpin->setRange(0, 4);
    m_serialNumberWidthSpin->setValue(3);
    m_serialNumberWidthSpin->setSuffix(" digits");
    m_serialNumberWidthSpin->setToolTip("Number of digits for serial numbers\n(0 = no padding, 3 = \"002\", 4 = \"0002\")");
    formLayout->addRow("Serial Width:", m_serialNumberWidthSpin);

    layout->addWidget(cwGroup);
    layout->addStretch();

    return cwTab;
}

QWidget* PreferencesDialog::createWebServerTab() {
    QWidget* webServerTab = new QWidget(this);
    webServerTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* layout = new QVBoxLayout(webServerTab);

    QGroupBox* webServerGroup = new QGroupBox("Web Server Settings", this);
    QFormLayout* formLayout = new QFormLayout(webServerGroup);

    // Auto-start checkbox
    m_webServerAutoStartCheck = new QCheckBox(this);
    m_webServerAutoStartCheck->setToolTip("Automatically start web server when TR4QT launches");
    formLayout->addRow("Auto-start on Launch:", m_webServerAutoStartCheck);

    // Port spin box
    m_webServerPortSpin = new QSpinBox(this);
    m_webServerPortSpin->setRange(1024, 65535);
    m_webServerPortSpin->setValue(14140);
    m_webServerPortSpin->setToolTip("TCP port for web server (default: 14140)");
    formLayout->addRow("Port:", m_webServerPortSpin);

    // Address line edit
    m_webServerAddressEdit = new QLineEdit(this);
    m_webServerAddressEdit->setPlaceholderText("127.0.0.1");
    m_webServerAddressEdit->setToolTip("IP address to bind to (127.0.0.1 = localhost only, 0.0.0.0 = all interfaces)");
    formLayout->addRow("Bind Address:", m_webServerAddressEdit);

    // Help text
    QLabel* helpLabel = new QLabel(
        "The web server provides a dashboard for viewing contest status from a web browser.\n\n"
        "• Localhost only (127.0.0.1): Accessible only from this computer\n"
        "• All interfaces (0.0.0.0): Accessible from other devices on your network", this);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; margin-top: 10px; }");
    formLayout->addRow(helpLabel);

    layout->addWidget(webServerGroup);
    layout->addStretch();

    return webServerTab;
}

QWidget* PreferencesDialog::createAdvancedTab() {
    QWidget* advancedTab = new QWidget(this);
    advancedTab->setAutoFillBackground(true);  // Prevent transparent/blank rendering
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
    m_firstNameEdit->setText(settings.getMyFirstName());
    m_lastNameEdit->setText(settings.getMyLastName());
    m_gridSquareEdit->setText(settings.getMyGridSquare());
    m_continentCombo->setCurrentText(settings.getMyContinent());
    m_cqZoneSpin->setValue(settings.getMyCQZone());
    m_ituZoneSpin->setValue(settings.getMyITUZone());
    m_stateEdit->setText(settings.getMyState());
    m_arrlSectionEdit->setText(settings.getMyARRLSection());
    m_countyEdit->setText(settings.getMyCounty());
    m_licenseClassCombo->setCurrentText(settings.getLicenseClass());
    // Operator will be added to AppSettings

    // Radio status filter checkboxes - MUST be loaded BEFORE radio model selection
    // so that the radio list is properly filtered when we try to find the saved model
    m_showStableRadiosCheck->setChecked(settings.getShowStableRadios());
    m_showBetaRadiosCheck->setChecked(settings.getShowBetaRadios());
    m_showAlphaRadiosCheck->setChecked(settings.getShowAlphaRadios());
    m_showUntestedRadiosCheck->setChecked(settings.getShowUntestedRadios());

    // Repopulate radio list with correct filter settings BEFORE loading saved model
    populateRadioList();

    // Radio tab - Load profiles
    // Migration happens automatically in AppSettings constructor
    m_radioProfiles = settings.loadRadioProfiles();
    QString activeProfile = settings.getActiveRadioProfile();

    // Populate profile combo box
    m_profileSelectorCombo->clear();
    for (const auto& profile : m_radioProfiles) {
        m_profileSelectorCombo->addItem(profile.displayString(), profile.name);
    }

    // Select active profile
    int activeIndex = -1;
    for (int i = 0; i < m_radioProfiles.size(); ++i) {
        if (m_radioProfiles[i].name == activeProfile) {
            activeIndex = i;
            break;
        }
    }

    if (activeIndex >= 0) {
        m_profileSelectorCombo->setCurrentIndex(activeIndex);
        loadProfileIntoUI(activeProfile);
    } else if (!m_radioProfiles.isEmpty()) {
        // Active profile not found - default to first
        m_profileSelectorCombo->setCurrentIndex(0);
        loadProfileIntoUI(m_radioProfiles[0].name);
        settings.setActiveRadioProfile(m_radioProfiles[0].name);
    }

    // Update active profile label
    m_activeProfileLabel->setText(QString("<i>Currently active: %1</i>").arg(activeProfile));

    // Update button states
    m_setActiveButton->setEnabled(false);  // Active profile already set
    m_deleteProfileButton->setEnabled(m_radioProfiles.size() > 1);

    m_autoConnectCheck->setChecked(settings.getRadioAutoConnect());
    m_morseWpmSpin->setValue(settings.getMorseWPM());
    m_morseWpmIncrementSpin->setValue(settings.getMorseWPMIncrement());
    m_cutNumbersEnabledCheck->setChecked(settings.getCutNumbersEnabled());
    m_serialNumberWidthSpin->setValue(settings.getSerialNumberWidth());

    // Amplifier settings (enhanced)
    m_amplifierEnabledCheck->setChecked(settings.getAmplifierEnabled());

    int ampModelId = settings.getAmplifierModel();
    int ampIndex = m_amplifierModelCombo->findData(ampModelId);
    if (ampIndex >= 0) {
        m_amplifierModelCombo->setCurrentIndex(ampIndex);
    }

    QString ampConnectionType = settings.getAmplifierConnectionType();
    int ampConnIndex = m_amplifierConnectionTypeCombo->findData(ampConnectionType);
    if (ampConnIndex >= 0) {
        m_amplifierConnectionTypeCombo->setCurrentIndex(ampConnIndex);
    }

    m_amplifierPortEdit->setText(settings.getAmplifierPort());
    m_amplifierBaudRateCombo->setCurrentText(QString::number(settings.getAmplifierBaudRate()));
    m_amplifierAutoConnectCheck->setChecked(settings.getAmplifierAutoConnect());

    // Trigger connection type changed to show/hide appropriate widgets
    onAmplifierConnectionTypeChanged(ampConnIndex);

    // Rotator settings
    m_rotatorEnabledCheck->setChecked(settings.getRotatorEnabled());

    int rotModelId = settings.getRotatorModel();
    int rotIndex = m_rotatorModelCombo->findData(rotModelId);
    if (rotIndex >= 0) {
        m_rotatorModelCombo->setCurrentIndex(rotIndex);
    }

    QString rotConnectionType = settings.getRotatorConnectionType();
    int rotConnIndex = m_rotatorConnectionTypeCombo->findData(rotConnectionType);
    if (rotConnIndex >= 0) {
        m_rotatorConnectionTypeCombo->setCurrentIndex(rotConnIndex);
    }

    m_rotatorIpEdit->setText(settings.getRotatorIpAddress());
    m_rotatorPortSpin->setValue(settings.getRotatorPort());
    m_rotatorSerialPortEdit->setText(settings.getRotatorSerialPort());
    m_rotatorBaudRateCombo->setCurrentText(QString::number(settings.getRotatorBaudRate()));
    m_rotatorAutoConnectCheck->setChecked(settings.getRotatorAutoConnect());

    // Trigger connection type changed to show/hide appropriate widgets
    onRotatorConnectionTypeChanged(rotConnIndex);

    // Note: Radio status filter checkboxes loaded earlier (before radio model selection)

    onConnectionTypeChanged();

    // DX Cluster tab
    m_dxClusterCallsignEdit->setText(settings.getDXClusterCallsign());
    m_dxClusterServerCombo->setCurrentText(settings.getDXClusterServer());
    m_dxClusterAutoConnectCheck->setChecked(settings.getDXClusterAutoConnect());
    m_enableLotwLookupCheck->setChecked(settings.getEnableLotwLookup());
    m_lotwMinUploadMonthsSpin->setValue(settings.getLotwMinUploadMonths());

    // Band Map timeout settings
    m_spotExpirySpin->setValue(settings.getSpotExpirySeconds());
    m_newSpotThresholdSpin->setValue(settings.getNewSpotThresholdSeconds());
    m_agingSpotThresholdSpin->setValue(settings.getAgingSpotThresholdSeconds());

    // SCP tab
    m_scpEnabledCheck->setChecked(settings.getSCPEnabled());
    m_scpIncludeLocalLogsCheck->setChecked(settings.getSCPIncludeLocalLogs());

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

    // Network tab
    m_computerIDEdit->setText(settings.getComputerID());

    // Appearance tab
    m_entryFontSizeSpin->setValue(settings.getEntryFontSize());
    m_tableFontSizeSpin->setValue(settings.getTableFontSize());
    m_gridFontSizeSpin->setValue(settings.getGridFontSize());
    m_miscDisplayFontSizeSpin->setValue(settings.getMiscDisplayFontSize());
    m_scpFontSizeSpin->setValue(settings.getSCPFontSize());
    m_useMetricDistanceCheck->setChecked(settings.getUseMetricDistance());

    // Band Needs Display settings
    QString workedColor = settings.getNeedsDisplayWorkedColor();
    m_workedColorButton->setStyleSheet(QString("background-color: %1;").arg(workedColor));
    QString neededColor = settings.getNeedsDisplayNeededColor();
    m_neededColorButton->setStyleSheet(QString("background-color: %1;").arg(neededColor));
    m_vhfBandsEnabledCheck->setChecked(settings.getVHFBandsEnabled());

    // DX Cluster spot colors
    QString dupeColor = settings.getClusterDupeColor();
    m_clusterDupeColorButton->setStyleSheet(QString("background-color: %1;").arg(dupeColor));
    QString multColor = settings.getClusterMultiplierColor();
    m_clusterMultColorButton->setStyleSheet(QString("background-color: %1;").arg(multColor));

#ifdef Q_OS_MAC
    // macOS Window Behavior
    if (m_showAllWindowsOnActivateCheck) {
        m_showAllWindowsOnActivateCheck->setChecked(settings.getShowAllWindowsOnActivate());
    }
#endif

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
    m_hamlibDebugEnabledCheck->setChecked(settings.getHamlibDebugEnabled());
    m_logFilePathEdit->setText(settings.getLogFilePath());
    m_logMaxFileSizeSpin->setValue(settings.getLogMaxFileSize() / (1024 * 1024));  // Convert bytes to MB
    m_logMaxBackupFilesSpin->setValue(settings.getLogMaxBackupFiles());

    // Backup tab
    m_autoBackupEnabledCheck->setChecked(settings.getAutoBackupEnabled());
    m_autoBackupIntervalSpin->setValue(settings.getAutoBackupInterval());
    m_backupDirectoryEdit->setText(settings.getBackupDirectory());
    m_maxBackupsSpin->setValue(settings.getMaxBackups());

    // Contest tab - will need to add getters to AppSettings

    // Web Server tab
    m_webServerAutoStartCheck->setChecked(settings.getWebServerAutoStart());
    m_webServerPortSpin->setValue(settings.getWebServerPort());
    m_webServerAddressEdit->setText(settings.getWebServerAddress());

    // Advanced tab
    m_countryFilePathEdit->setText(settings.getCountryFilePath());
}

void PreferencesDialog::saveSettings() {
    AppSettings& settings = AppSettings::instance();

    // Station tab
    settings.setMyCallsign(m_callsignEdit->text());
    settings.setMyFirstName(m_firstNameEdit->text());
    settings.setMyLastName(m_lastNameEdit->text());
    settings.setMyGridSquare(m_gridSquareEdit->text());
    settings.setMyContinent(m_continentCombo->currentText());
    settings.setMyCQZone(m_cqZoneSpin->value());
    settings.setMyITUZone(m_ituZoneSpin->value());
    settings.setMyState(m_stateEdit->text());
    settings.setMyARRLSection(m_arrlSectionEdit->text());
    settings.setMyCounty(m_countyEdit->text());
    settings.setLicenseClass(m_licenseClassCombo->currentText());

    // Radio tab - Save profiles
    // Update currently selected profile with UI values
    int selectedIndex = m_profileSelectorCombo->currentIndex();
    if (selectedIndex >= 0 && selectedIndex < m_radioProfiles.size()) {
        m_radioProfiles[selectedIndex].config = buildRadioConfigFromUI();
    }

    // Save all profiles
    settings.saveRadioProfiles(m_radioProfiles);
    settings.setRadioAutoConnect(m_autoConnectCheck->isChecked());
    settings.setMorseWPM(m_morseWpmSpin->value());
    settings.setMorseWPMIncrement(m_morseWpmIncrementSpin->value());
    settings.setCutNumbersEnabled(m_cutNumbersEnabledCheck->isChecked());
    settings.setSerialNumberWidth(m_serialNumberWidthSpin->value());

    // Radio status filter checkboxes
    settings.setShowStableRadios(m_showStableRadiosCheck->isChecked());
    settings.setShowBetaRadios(m_showBetaRadiosCheck->isChecked());
    settings.setShowAlphaRadios(m_showAlphaRadiosCheck->isChecked());
    settings.setShowUntestedRadios(m_showUntestedRadiosCheck->isChecked());

    // Amplifier settings (enhanced)
    settings.setAmplifierEnabled(m_amplifierEnabledCheck->isChecked());
    settings.setAmplifierModel(m_amplifierModelCombo->currentData().toInt());
    settings.setAmplifierConnectionType(m_amplifierConnectionTypeCombo->currentData().toString());
    settings.setAmplifierPort(m_amplifierPortEdit->text());
    settings.setAmplifierBaudRate(m_amplifierBaudRateCombo->currentText().toInt());
    settings.setAmplifierAutoConnect(m_amplifierAutoConnectCheck->isChecked());

    // Rotator settings
    settings.setRotatorEnabled(m_rotatorEnabledCheck->isChecked());
    settings.setRotatorModel(m_rotatorModelCombo->currentData().toInt());
    settings.setRotatorConnectionType(m_rotatorConnectionTypeCombo->currentData().toString());
    settings.setRotatorIpAddress(m_rotatorIpEdit->text());
    settings.setRotatorPort(m_rotatorPortSpin->value());
    settings.setRotatorSerialPort(m_rotatorSerialPortEdit->text());
    settings.setRotatorBaudRate(m_rotatorBaudRateCombo->currentText().toInt());
    settings.setRotatorAutoConnect(m_rotatorAutoConnectCheck->isChecked());

    // DX Cluster tab
    settings.setDXClusterCallsign(m_dxClusterCallsignEdit->text());
    settings.setDXClusterServer(m_dxClusterServerCombo->currentText());
    settings.setDXClusterAutoConnect(m_dxClusterAutoConnectCheck->isChecked());
    settings.setEnableLotwLookup(m_enableLotwLookupCheck->isChecked());
    settings.setLotwMinUploadMonths(m_lotwMinUploadMonthsSpin->value());

    // Band Map timeout settings
    settings.setSpotExpirySeconds(m_spotExpirySpin->value());
    settings.setNewSpotThresholdSeconds(m_newSpotThresholdSpin->value());
    settings.setAgingSpotThresholdSeconds(m_agingSpotThresholdSpin->value());

    // SCP tab
    settings.setSCPEnabled(m_scpEnabledCheck->isChecked());
    settings.setSCPIncludeLocalLogs(m_scpIncludeLocalLogsCheck->isChecked());

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

    // Network tab
    settings.setComputerID(m_computerIDEdit->text());

    // Appearance tab
    settings.setEntryFontSize(m_entryFontSizeSpin->value());
    settings.setTableFontSize(m_tableFontSizeSpin->value());
    settings.setGridFontSize(m_gridFontSizeSpin->value());
    settings.setMiscDisplayFontSize(m_miscDisplayFontSizeSpin->value());
    settings.setSCPFontSize(m_scpFontSizeSpin->value());
    settings.setUseMetricDistance(m_useMetricDistanceCheck->isChecked());

    // Band Needs Display
    settings.setVHFBandsEnabled(m_vhfBandsEnabledCheck->isChecked());
    // Colors are saved immediately in button click handlers

#ifdef Q_OS_MAC
    // macOS Window Behavior
    if (m_showAllWindowsOnActivateCheck) {
        settings.setShowAllWindowsOnActivate(m_showAllWindowsOnActivateCheck->isChecked());
    }
#endif

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
    settings.setHamlibDebugEnabled(m_hamlibDebugEnabledCheck->isChecked());
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

    // Apply Hamlib debug setting immediately
    if (m_hamlibDebugEnabledCheck->isChecked()) {
        rig_set_debug(RIG_DEBUG_VERBOSE);
        LOG_INFO("PreferencesDialog", "Hamlib debug logging enabled");
    } else {
        rig_set_debug(RIG_DEBUG_NONE);
        LOG_INFO("PreferencesDialog", "Hamlib debug logging disabled");
    }

    // Backup tab
    settings.setAutoBackupEnabled(m_autoBackupEnabledCheck->isChecked());
    settings.setAutoBackupInterval(m_autoBackupIntervalSpin->value());
    settings.setBackupDirectory(m_backupDirectoryEdit->text());
    settings.setMaxBackups(m_maxBackupsSpin->value());

    // Contest tab - will add setters to AppSettings

    // Web Server tab
    settings.setWebServerAutoStart(m_webServerAutoStartCheck->isChecked());
    settings.setWebServerPort(static_cast<quint16>(m_webServerPortSpin->value()));
    settings.setWebServerAddress(m_webServerAddressEdit->text());

    // Advanced tab
    settings.setCountryFilePath(m_countryFilePathEdit->text());
}

void PreferencesDialog::accept() {
    saveSettings();
    QDialog::accept();
}

void PreferencesDialog::selectCategory(const QString& categoryName) {
    // Find the category in the list and select it
    for (int i = 0; i < m_categoryList->count(); ++i) {
        if (m_categoryList->item(i)->text() == categoryName) {
            m_categoryList->setCurrentRow(i);
            return;
        }
    }
}

void PreferencesDialog::setRadioConnected(bool connected) {
    if (connected) {
        m_testConnectionButton->setEnabled(false);
        m_testConnectionButton->setToolTip("Cannot test - radio is already connected");
        m_connectionStatusLabel->setText("(Radio is connected - disconnect to test)");
        m_connectionStatusLabel->show();
    } else {
        m_testConnectionButton->setEnabled(true);
        m_testConnectionButton->setToolTip("Test connection to the configured radio");
        m_connectionStatusLabel->hide();
    }
}

void PreferencesDialog::onApply() {
    saveSettings();
}

void PreferencesDialog::onConnectionTypeChanged() {
    bool isSerial = m_serialRadio->isChecked();
    m_serialGroup->setVisible(isSerial);
    m_networkGroup->setVisible(!isSerial);

    // Manage port refresh timer based on connection type
    if (isSerial && isVisible()) {
        m_portRefreshTimer->start();
    } else {
        m_portRefreshTimer->stop();
    }
}

void PreferencesDialog::onRadioModelChanged(int index) {
    int modelId = m_radioModelCombo->currentData().toInt();

    // Show custom model ID field if "Custom" is selected
    m_customModelEdit->setVisible(modelId == -1);

    // Auto-configure CI-V address for known Icom radios
    m_civAddressWidget->autoConfigureForRadio(modelId);
}

void PreferencesDialog::onRadioTypeChanged(int index) {
    int radioType = m_radioTypeCombo->currentData().toInt();

    // Set default port based on radio type
    // Only change port for fresh selections - not when loading saved settings
    // (signal blocker in loadSettings prevents this from running during load)
    if (radioType == 2) {  // ICOM_DIRECT
        m_portSpin->setValue(50001);  // Icom default network port
        m_findK4Button->setText("Find Icom Radios on Network");
        m_findK4Button->setToolTip("Broadcast UDP discovery to find Icom radios on the network");
    } else if (radioType == 1) {  // K4_DIRECT
        m_portSpin->setValue(9200);   // K4 default TCP port
        m_findK4Button->setText("Find K4 Radios on Network");
        m_findK4Button->setToolTip("Broadcast UDP discovery to find Elecraft K4 radios on the network");
    } else {
        m_portSpin->setValue(4532);   // rigctld default for Hamlib
        m_findK4Button->setText("Find Radios on Network");
        m_findK4Button->setToolTip("Broadcast UDP discovery to find radios on the network");
    }
}

void PreferencesDialog::onTestRadioConnection() {
    // Build RadioConfig from current dialog settings
    RadioConfig config;

    int modelId = m_radioModelCombo->currentData().toInt();
    if (modelId == -1) {
        config.hamlibModelId = m_customModelEdit->text().toInt();
    } else {
        config.hamlibModelId = modelId;
    }

    if (config.hamlibModelId == 0) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please select a radio model.");
        return;
    }

    if (m_serialRadio->isChecked()) {
        // Prefer dropdown selection, fall back to manual entry
        QString selectedPort = m_serialPortCombo->currentData().toString();
        if (selectedPort.isEmpty() || !m_serialPortCombo->isEnabled()) {
            selectedPort = m_serialPortEdit->text().trimmed();
        }
        config.port = selectedPort;
        config.baudRate = m_baudRateCombo->currentText().toInt();
        config.dataBits = m_dataBitsCombo->currentText().toInt();
        config.stopBits = m_stopBitsCombo->currentText().toInt();
        config.parity = m_parityCombo->currentIndex();
    } else {
        config.port = QString("%1:%2")
                          .arg(m_ipAddressEdit->text())
                          .arg(m_portSpin->value());
        config.baudRate = 0;
        config.dataBits = 8;  // Defaults for network (not used)
        config.stopBits = 1;
        config.parity = 0;
    }

    if (config.port.isEmpty()) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please specify a port or IP address.");
        return;
    }

    // Get CI-V address from widget
    config.civAddress = m_civAddressWidget->getCivAddress();

    config.pollInterval = m_pollIntervalSpin->value();

    // Create temporary HamlibRadio instance for testing
    HamlibRadio testRadio;

    // Try to connect
    LOG_DEBUG("PreferencesDialog", QString("Testing connection to model %1 on %2")
        .arg(config.hamlibModelId).arg(config.port));

    bool connected = testRadio.connect(config);

    if (!connected) {
        DialogHelper::critical(this, "Connection Failed",
                            QString("Failed to connect to radio.\n\n"
                                    "Model ID: %1\n"
                                    "Port: %2\n"
                                    "Baud: %3\n"
                                    "CI-V: %4\n\n"
                                    "Check your settings and ensure the radio is powered on and connected.")
                                .arg(config.hamlibModelId)
                                .arg(config.port)
                                .arg(config.baudRate)
                                .arg(config.civAddress == 0 ? "0 (auto)" : QString("0x%1").arg(config.civAddress, 2, 16, QChar('0'))));
        return;
    }

    // Connection successful - query radio info
    QString model = testRadio.getRadioModel();
    QString version = testRadio.getRadioVersion();
    freq_t freq = testRadio.getFrequency(VFO::VFO_A);
    ModeType mode = testRadio.getMode(VFO::VFO_A);
    bool supportsCW = testRadio.supportsCWSending();

    // Disconnect
    testRadio.disconnect();

    // Show success message
    DialogHelper::information(this, "Connection Successful",
                           QString("Successfully connected to radio!\n\n"
                                   "Model: %1\n"
                                   "Firmware: %2\n"
                                   "Frequency: %3 kHz\n"
                                   "Mode: %4\n"
                                   "CW Support: %5")
                               .arg(model)
                               .arg(version.isEmpty() ? "Unknown" : version)
                               .arg(freq / 1000.0, 0, 'f', 3)
                               .arg(modeToString(mode))
                               .arg(supportsCW ? "Yes" : "No"));
}

// Radio profile management slots
void PreferencesDialog::onProfileSelected(int index) {
    if (index < 0 || index >= m_radioProfiles.size()) {
        return;
    }

    // Load selected profile into UI fields
    const RadioProfile& profile = m_radioProfiles[index];
    loadProfileIntoUI(profile.name);

    // Update button states
    QString activeProfile = AppSettings::instance().getActiveRadioProfile();
    m_setActiveButton->setEnabled(profile.name != activeProfile);
    m_deleteProfileButton->setEnabled(m_radioProfiles.size() > 1);
}

void PreferencesDialog::onNewProfile() {
    // Prompt for profile name
    bool ok;
    QString name = QInputDialog::getText(this, "New Radio Profile",
                                        "Profile name:",
                                        QLineEdit::Normal,
                                        "", &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    name = name.trimmed();

    // Check for duplicate name
    for (const auto& profile : m_radioProfiles) {
        if (profile.name == name) {
            DialogHelper::warning(this, "Duplicate Name",
                               QString("A profile named '%1' already exists.\n"
                                       "Please choose a different name.").arg(name));
            return;
        }
    }

    // Create new profile with current UI settings
    RadioProfile newProfile;
    newProfile.name = name;
    newProfile.config = buildRadioConfigFromUI();
    newProfile.lastUsed = QDateTime::currentDateTime();
    newProfile.notes = "";

    // Add to list
    m_radioProfiles.append(newProfile);

    // Update combo box
    m_profileSelectorCombo->addItem(newProfile.displayString(), newProfile.name);

    // Select the new profile
    m_profileSelectorCombo->setCurrentIndex(m_radioProfiles.size() - 1);

    LOG_INFO("PreferencesDialog", QString("Created new radio profile: %1").arg(name));
}

void PreferencesDialog::onEditProfile() {
    int index = m_profileSelectorCombo->currentIndex();
    if (index < 0 || index >= m_radioProfiles.size()) {
        return;
    }

    RadioProfile& profile = m_radioProfiles[index];

    // Show dialog to edit name and notes
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Profile");
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QFormLayout* formLayout = new QFormLayout();
    QLineEdit* nameEdit = new QLineEdit(profile.name, &dialog);
    QTextEdit* notesEdit = new QTextEdit(profile.notes, &dialog);
    notesEdit->setMaximumHeight(100);

    formLayout->addRow("Name:", nameEdit);
    formLayout->addRow("Notes:", notesEdit);
    layout->addLayout(formLayout);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString newName = nameEdit->text().trimmed();
    if (newName.isEmpty()) {
        DialogHelper::warning(this, "Invalid Name", "Profile name cannot be empty.");
        return;
    }

    // Check for duplicate name (if changed)
    if (newName != profile.name) {
        for (const auto& p : m_radioProfiles) {
            if (p.name == newName && p.name != profile.name) {
                DialogHelper::warning(this, "Duplicate Name",
                                   QString("A profile named '%1' already exists.").arg(newName));
                return;
            }
        }
    }

    // Update profile
    QString oldName = profile.name;
    profile.name = newName;
    profile.notes = notesEdit->toPlainText();

    // Update combo box
    m_profileSelectorCombo->setItemText(index, profile.displayString());
    m_profileSelectorCombo->setItemData(index, profile.name);

    // If this was the active profile, update active profile name
    QString activeProfile = AppSettings::instance().getActiveRadioProfile();
    if (activeProfile == oldName) {
        AppSettings::instance().setActiveRadioProfile(newName);
        m_activeProfileLabel->setText(QString("<i>Currently active: %1</i>").arg(newName));
    }

    LOG_INFO("PreferencesDialog", QString("Edited profile: %1 -> %2").arg(oldName).arg(newName));
}

void PreferencesDialog::onDeleteProfile() {
    int index = m_profileSelectorCombo->currentIndex();
    if (index < 0 || index >= m_radioProfiles.size()) {
        return;
    }

    const RadioProfile& profile = m_radioProfiles[index];

    // Cannot delete if it's the only profile
    if (m_radioProfiles.size() == 1) {
        DialogHelper::warning(this, "Cannot Delete",
                           "Cannot delete the only remaining profile.\n"
                           "Create a new profile first.");
        return;
    }

    // Cannot delete the active profile
    QString activeProfile = AppSettings::instance().getActiveRadioProfile();
    if (profile.name == activeProfile) {
        DialogHelper::warning(this, "Cannot Delete Active Profile",
                           QString("Cannot delete the currently active profile '%1'.\n"
                                   "Set a different profile as active first.").arg(profile.name));
        return;
    }

    // Confirm deletion
    QMessageBox::StandardButton reply = DialogHelper::question(this, "Confirm Delete",
        QString("Delete profile '%1'?\n\n"
                "This action cannot be undone.").arg(profile.name));

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Remove from list
    QString deletedName = profile.name;
    m_radioProfiles.removeAt(index);
    m_profileSelectorCombo->removeItem(index);

    // Select first remaining profile
    if (!m_radioProfiles.isEmpty()) {
        m_profileSelectorCombo->setCurrentIndex(0);
    }

    LOG_INFO("PreferencesDialog", QString("Deleted profile: %1").arg(deletedName));
}

void PreferencesDialog::onSetActiveProfile() {
    int index = m_profileSelectorCombo->currentIndex();
    if (index < 0 || index >= m_radioProfiles.size()) {
        return;
    }

    RadioProfile& profile = m_radioProfiles[index];

    // IMPORTANT: Update the profile with current UI settings before setting as active
    // This ensures any changes made in the UI are saved to the profile
    profile.config = buildRadioConfigFromUI();

    // Save all profiles (including the updated one)
    AppSettings& settings = AppSettings::instance();
    settings.saveRadioProfiles(m_radioProfiles);

    // Update active profile in settings
    settings.setActiveRadioProfile(profile.name);

    // Update label and button
    m_activeProfileLabel->setText(QString("<i>Currently active: %1</i>").arg(profile.name));
    m_setActiveButton->setEnabled(false);

    LOG_INFO("PreferencesDialog", QString("Set active profile: %1").arg(profile.name));
}

// Amplifier slots
void PreferencesDialog::onAmplifierModelChanged(int index) {
    int modelId = m_amplifierModelCombo->currentData().toInt();

    // Auto-select connection type based on model
    const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
    if (modelId == AMP_MODEL_ELECRAFT_KPA1500) {
        // KPA1500: Prefer direct UDP
        m_amplifierConnectionTypeCombo->setCurrentIndex(0);  // Direct
        m_amplifierPortEdit->setPlaceholderText("192.168.1.100:1500");
        m_amplifierPortEdit->setToolTip("IP address and port for KPA1500 UDP control");
    } else {
        // Other models: Use Hamlib serial
        m_amplifierConnectionTypeCombo->setCurrentIndex(1);  // Hamlib
        m_amplifierPortEdit->setPlaceholderText("/dev/ttyUSB0 or COM3");
        m_amplifierPortEdit->setToolTip("Serial port device");
    }
}

void PreferencesDialog::onAmplifierConnectionTypeChanged(int index) {
    QString connectionType = m_amplifierConnectionTypeCombo->currentData().toString();

    if (connectionType == "direct") {
        // Direct mode: Show network settings, hide serial settings
        m_amplifierPortEdit->setPlaceholderText("192.168.1.100:1500");
        m_amplifierPortEdit->setToolTip("IP address and port (e.g., 192.168.1.100:1500)");
        m_amplifierSerialSettingsWidget->hide();
    } else {
        // Hamlib mode: Show serial settings
        m_amplifierPortEdit->setPlaceholderText("/dev/ttyUSB0 or COM3");
        m_amplifierPortEdit->setToolTip("Serial port device");
        m_amplifierSerialSettingsWidget->show();
    }
}

void PreferencesDialog::onTestAmplifierConnection() {
    // Build AmplifierConfig from current dialog settings
    int modelId = m_amplifierModelCombo->currentData().toInt();
    if (modelId == 0) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please select an amplifier model.");
        return;
    }

    QString port = m_amplifierPortEdit->text().trimmed();
    if (port.isEmpty()) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please specify an IP:port or serial port.");
        return;
    }

    QString connectionType = m_amplifierConnectionTypeCombo->currentData().toString();
    int baudRate = m_amplifierBaudRateCombo->currentText().toInt();

    // Create temporary amplifier for testing
    AmplifierConfig config;
    config.hamlibModelId = modelId;
    config.connectionType = connectionType;
    config.port = port;
    config.baudRate = baudRate;

    AmplifierFactory::AmplifierType type;
    const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
    if (connectionType == "direct" && modelId == AMP_MODEL_ELECRAFT_KPA1500) {
        type = AmplifierFactory::AmplifierType::KPA1500_DIRECT;
    } else {
        type = AmplifierFactory::AmplifierType::HAMLIB;
    }

    IAmplifierController* testAmplifier = AmplifierFactory::createAmplifier(type, config, this);

    if (!testAmplifier) {
        DialogHelper::critical(this, "Error", "Failed to create amplifier instance.");
        return;
    }

    // Try to connect
    bool connected = testAmplifier->connect(config);

    if (!connected) {
        DialogHelper::critical(this, "Connection Failed",
                            QString("Failed to connect to amplifier.\n\n"
                                    "Model ID: %1\n"
                                    "Port: %2\n\n"
                                    "Check your settings and ensure the amplifier is powered on and connected.")
                                .arg(modelId)
                                .arg(port));
        delete testAmplifier;
        return;
    }

    // Connection successful - query status
    AmplifierState state = testAmplifier->getState();

    // Disconnect
    testAmplifier->disconnect();
    delete testAmplifier;

    // Show success message
    DialogHelper::information(this, "Connection Successful",
                           QString("Successfully connected to amplifier!\n\n"
                                   "Forward Power: %1 W\n"
                                   "SWR: %2\n"
                                   "Status: %3")
                               .arg(state.forwardPowerWatts)
                               .arg(state.swr, 0, 'f', 1)
                               .arg(state.connected ? "Connected" : "Disconnected"));
}

// Rotator slots
void PreferencesDialog::onRotatorModelChanged(int index) {
    int modelId = m_rotatorModelCombo->currentData().toInt();

    // Auto-select connection type based on model
    const int ROT_MODEL_PSTROTATOR = 9999;
    if (modelId == ROT_MODEL_PSTROTATOR) {
        // PSTRotator: Use direct UDP
        m_rotatorConnectionTypeCombo->setCurrentIndex(0);  // Direct
        m_rotatorNetworkSettingsWidget->show();
        m_rotatorSerialSettingsWidget->hide();
    } else {
        // Other models: Use Hamlib serial
        m_rotatorConnectionTypeCombo->setCurrentIndex(1);  // Hamlib
        m_rotatorNetworkSettingsWidget->hide();
        m_rotatorSerialSettingsWidget->show();
    }
}

void PreferencesDialog::onRotatorConnectionTypeChanged(int index) {
    QString connectionType = m_rotatorConnectionTypeCombo->currentData().toString();

    if (connectionType == "direct") {
        // Direct mode: Show network settings, hide serial settings
        m_rotatorNetworkSettingsWidget->show();
        m_rotatorSerialSettingsWidget->hide();
    } else {
        // Hamlib mode: Show serial settings, hide network settings
        m_rotatorNetworkSettingsWidget->hide();
        m_rotatorSerialSettingsWidget->show();
    }
}

void PreferencesDialog::onTestRotatorConnection() {
    // Build RotatorConfig from current dialog settings
    int modelId = m_rotatorModelCombo->currentData().toInt();
    if (modelId == 0) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please select a rotator model.");
        return;
    }

    QString connectionType = m_rotatorConnectionTypeCombo->currentData().toString();

    // TODO: Implement rotator test connection
    // For now, just show a placeholder message
    DialogHelper::information(this, "Not Implemented",
                           "Rotator connection testing will be implemented in a future update.");
}

// Helper methods
void PreferencesDialog::loadProfileIntoUI(const QString& profileName) {
    // Find profile by name
    RadioProfile* profile = nullptr;
    for (auto& p : m_radioProfiles) {
        if (p.name == profileName) {
            profile = &p;
            break;
        }
    }

    if (!profile) {
        LOG_WARN("PreferencesDialog", QString("Profile not found: %1").arg(profileName));
        return;
    }

    const RadioConfig& config = profile->config;

    // Block signals to prevent triggering change handlers
    QSignalBlocker blockerModel(m_radioModelCombo);
    QSignalBlocker blockerType(m_radioTypeCombo);
    QSignalBlocker blockerSerial(m_serialRadio);
    QSignalBlocker blockerPort(m_serialPortCombo);
    QSignalBlocker blockerBaud(m_baudRateCombo);
    QSignalBlocker blockerIP(m_ipAddressEdit);
    QSignalBlocker blockerPortSpin(m_portSpin);

    // Set radio model
    int modelIndex = m_radioModelCombo->findData(config.hamlibModelId);
    if (modelIndex >= 0) {
        m_radioModelCombo->setCurrentIndex(modelIndex);
    }

    // Set radio interface type
    int typeIndex = m_radioTypeCombo->findData(config.radioType);
    if (typeIndex >= 0) {
        m_radioTypeCombo->setCurrentIndex(typeIndex);
    }

    // Set connection type and port settings
    if (config.port.contains(":")) {
        // Network connection
        m_networkRadio->setChecked(true);
        QStringList parts = config.port.split(":");
        if (parts.size() == 2) {
            m_ipAddressEdit->setText(parts[0]);
            m_portSpin->setValue(parts[1].toInt());
        }
    } else {
        // Serial connection
        m_serialRadio->setChecked(true);
        int portIndex = m_serialPortCombo->findData(config.port);
        if (portIndex >= 0) {
            m_serialPortCombo->setCurrentIndex(portIndex);
        } else {
            m_serialPortEdit->setText(config.port);
        }
        m_baudRateCombo->setCurrentText(QString::number(config.baudRate));
        m_dataBitsCombo->setCurrentText(QString::number(config.dataBits));
        m_stopBitsCombo->setCurrentText(QString::number(config.stopBits));
        m_parityCombo->setCurrentIndex(config.parity);
    }

    // Set other fields
    m_civAddressWidget->setCivAddress(config.civAddress);
    m_pollIntervalSpin->setValue(config.pollInterval);
    m_icomUsernameEdit->setText(config.icomUsername);
    m_icomPasswordEdit->setText(config.icomPassword);
    m_icomClientNameEdit->setText(config.icomClientName);

    // Trigger UI updates
    onConnectionTypeChanged();
}

RadioConfig PreferencesDialog::buildRadioConfigFromUI() const {
    RadioConfig config;

    // Radio model
    int modelId = m_radioModelCombo->currentData().toInt();
    if (modelId == -1) {
        config.hamlibModelId = m_customModelEdit->text().toInt();
    } else {
        config.hamlibModelId = modelId;
    }

    // Radio type
    config.radioType = m_radioTypeCombo->currentData().toInt();

    // Connection settings
    if (m_serialRadio->isChecked()) {
        // Serial
        QString selectedPort = m_serialPortCombo->currentData().toString();
        if (selectedPort.isEmpty() || !m_serialPortCombo->isEnabled()) {
            selectedPort = m_serialPortEdit->text().trimmed();
        }
        config.port = selectedPort;
        config.baudRate = m_baudRateCombo->currentText().toInt();
        config.dataBits = m_dataBitsCombo->currentText().toInt();
        config.stopBits = m_stopBitsCombo->currentText().toInt();
        config.parity = m_parityCombo->currentIndex();
    } else {
        // Network
        config.port = QString("%1:%2")
                          .arg(m_ipAddressEdit->text())
                          .arg(m_portSpin->value());
        config.baudRate = 0;
        config.dataBits = 8;
        config.stopBits = 1;
        config.parity = 0;
    }

    // Other fields
    config.civAddress = m_civAddressWidget->getCivAddress();
    config.pollInterval = m_pollIntervalSpin->value();
    config.icomUsername = m_icomUsernameEdit->text();
    config.icomPassword = m_icomPasswordEdit->text();
    config.icomClientName = m_icomClientNameEdit->text();

    return config;
}

void PreferencesDialog::onUdpAddDestination() {
    QString host = m_udpHostEdit->text().trimmed();
    quint16 port = m_udpPortSpin->value();

    if (host.isEmpty()) {
        DialogHelper::warning(this, "Add Destination",
                           "Please enter a host address.");
        return;
    }

    // Check for duplicates
    QString newItem = QString("%1:%2").arg(host).arg(port);
    for (int i = 0; i < m_udpDestinationsList->count(); ++i) {
        QString existingItem = m_udpDestinationsList->item(i)->text();
        if (existingItem.startsWith(newItem)) {
            DialogHelper::warning(this, "Add Destination",
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
        DialogHelper::warning(this, "Remove Destination",
                           "Please select a destination to remove.");
        return;
    }

    delete currentItem;
}

void PreferencesDialog::onUdpTestDestination() {
    QString host = m_udpHostEdit->text().trimmed();
    quint16 port = m_udpPortSpin->value();

    if (host.isEmpty()) {
        DialogHelper::warning(this, "Test Destination",
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
        DialogHelper::information(this, "Test Destination",
                               QString("Successfully sent test message to %1:%2\n\n"
                                      "Check the receiving application to verify.")
                                   .arg(host).arg(port));
    } else {
        DialogHelper::critical(this, "Test Destination",
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
        DialogHelper::warning(this, "Open Log File",
                           QString("Log file does not exist:\n%1\n\n"
                                  "Start the application to create the log file.")
                               .arg(logPath));
        return;
    }

    // Open with default application
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(logPath))) {
        DialogHelper::critical(this, "Open Log File",
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

    QMessageBox::StandardButton reply = DialogHelper::question(
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
            DialogHelper::information(this, "Clear Log File",
                                   "Log file cleared successfully.");
        } else {
            DialogHelper::critical(this, "Clear Log File",
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

void PreferencesDialog::onBrowseBackupDirectory() {
    QString currentPath = m_backupDirectoryEdit->text();
    if (currentPath.isEmpty()) {
        currentPath = AppSettings::instance().getBackupDirectory();
    }

    // Expand home directory if present
    if (currentPath.startsWith("~/")) {
        currentPath = QDir::homePath() + currentPath.mid(1);
    }

    QString dirName = QFileDialog::getExistingDirectory(
        this,
        "Select Backup Directory",
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dirName.isEmpty()) {
        // Convert back to ~/ notation if within home directory
        QString homePath = QDir::homePath();
        if (dirName.startsWith(homePath)) {
            dirName = "~" + dirName.mid(homePath.length());
        }
        m_backupDirectoryEdit->setText(dirName);
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

    // Add description label
    QLabel* descLabel = new QLabel(
        "Customize individual color elements. Click color button to change, "
        "Reset to restore theme default. Categories can be collapsed/expanded.",
        colorDialog);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #666; padding: 5px;");
    mainLayout->addWidget(descLabel);

    ThemeManager& theme = ThemeManager::instance();

    // Helper struct to hold color button data
    struct ColorButton {
        ColorRole role;
        QPushButton* button;
        QColor currentColor;
    };

    QList<ColorButton*> colorButtons;

    // Create tree widget with columns: Name | Color | Reset
    QTreeWidget* tree = new QTreeWidget(colorDialog);
    tree->setHeaderLabels({"Color Element", "Color", "Reset"});
    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 120);
    tree->setColumnWidth(2, 80);
    tree->setRootIsDecorated(true);
    tree->setAlternatingRowColors(true);

    // Helper lambda to add color items to a category
    auto addColorItem = [&](QTreeWidgetItem* parent, ColorRole role) {
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        item->setText(0, ThemeManager::colorRoleName(role));

        // Color preview button
        QPushButton* colorButton = new QPushButton(colorDialog);
        QColor currentColor = theme.color(role);

        colorButton->setFixedSize(100, 30);
        colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(currentColor.name()));
        colorButton->setToolTip("Click to change color");

        // Store button data
        ColorButton* btnData = new ColorButton{role, colorButton, currentColor};
        colorButtons.append(btnData);

        // Connect to color picker
        connect(colorButton, &QPushButton::clicked, [=, &theme]() mutable {
            QColor newColor = QColorDialog::getColor(btnData->currentColor, colorDialog,
                QString("Select Color for %1").arg(ThemeManager::colorRoleName(role)));
            if (newColor.isValid()) {
                btnData->currentColor = newColor;
                btnData->button->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(newColor.name()));
            }
        });

        tree->setItemWidget(item, 1, colorButton);

        // Reset button
        QPushButton* resetButton = new QPushButton("Reset", colorDialog);
        resetButton->setFixedSize(70, 25);
        resetButton->setToolTip("Reset to theme default");
        connect(resetButton, &QPushButton::clicked, [=, &theme]() mutable {
            // Get default color from current theme
            ThemeType originalTheme = theme.currentTheme();
            QColor defaultColor;

            // If Custom theme, reset to TR4W default
            if (originalTheme == ThemeType::Custom) {
                theme.setTheme(ThemeType::TR4WDefault);
                defaultColor = theme.color(role);
                theme.setTheme(originalTheme);
            } else {
                defaultColor = theme.color(role);
            }

            btnData->currentColor = defaultColor;
            btnData->button->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(defaultColor.name()));
        });

        tree->setItemWidget(item, 2, resetButton);
    };

    // Display Colors category
    QTreeWidgetItem* displayCategory = new QTreeWidgetItem(tree);
    displayCategory->setText(0, "Display Colors");
    displayCategory->setExpanded(true);
    QFont categoryFont = displayCategory->font(0);
    categoryFont.setBold(true);
    displayCategory->setFont(0, categoryFont);

    addColorItem(displayCategory, ColorRole::VfoBackground);
    addColorItem(displayCategory, ColorRole::VfoText);
    addColorItem(displayCategory, ColorRole::WindowBackground);
    addColorItem(displayCategory, ColorRole::TextDisplayBackground);

    // Status Colors category
    QTreeWidgetItem* statusCategory = new QTreeWidgetItem(tree);
    statusCategory->setText(0, "Status Colors");
    statusCategory->setExpanded(true);
    statusCategory->setFont(0, categoryFont);

    addColorItem(statusCategory, ColorRole::ConnectedStatus);
    addColorItem(statusCategory, ColorRole::DisconnectedStatus);
    addColorItem(statusCategory, ColorRole::FrozenIndicator);

    // Functional Colors category
    QTreeWidgetItem* functionalCategory = new QTreeWidgetItem(tree);
    functionalCategory->setText(0, "Functional Colors");
    functionalCategory->setExpanded(true);
    functionalCategory->setFont(0, categoryFont);

    addColorItem(functionalCategory, ColorRole::DupeText);
    addColorItem(functionalCategory, ColorRole::NewMultiplierBackground);
    addColorItem(functionalCategory, ColorRole::WorkedStationText);
    addColorItem(functionalCategory, ColorRole::MultiplierText);
    addColorItem(functionalCategory, ColorRole::LotwUserText);
    addColorItem(functionalCategory, ColorRole::NeededMultiplierBackground);
    addColorItem(functionalCategory, ColorRole::ConfirmedMultiplierBackground);

    // Spot Aging Colors category
    QTreeWidgetItem* spotAgingCategory = new QTreeWidgetItem(tree);
    spotAgingCategory->setText(0, "Spot Aging Colors");
    spotAgingCategory->setExpanded(false);  // Collapsed by default
    spotAgingCategory->setFont(0, categoryFont);

    addColorItem(spotAgingCategory, ColorRole::NewSpotText);
    addColorItem(spotAgingCategory, ColorRole::NewSpotBackground);
    addColorItem(spotAgingCategory, ColorRole::AgingSpotText);
    addColorItem(spotAgingCategory, ColorRole::AgingSpotBackground);

    // Map Colors category (new!)
    QTreeWidgetItem* mapCategory = new QTreeWidgetItem(tree);
    mapCategory->setText(0, "Map Colors");
    mapCategory->setExpanded(false);  // Collapsed by default
    mapCategory->setFont(0, categoryFont);

    addColorItem(mapCategory, ColorRole::MapBackground);
    addColorItem(mapCategory, ColorRole::MapNotWorked);
    addColorItem(mapCategory, ColorRole::MapFirstContact);
    addColorItem(mapCategory, ColorRole::MapSecondContact);
    addColorItem(mapCategory, ColorRole::MapFew);
    addColorItem(mapCategory, ColorRole::MapSome);
    addColorItem(mapCategory, ColorRole::MapMany);
    addColorItem(mapCategory, ColorRole::MapManyMore);
    addColorItem(mapCategory, ColorRole::MapHundreds);
    addColorItem(mapCategory, ColorRole::MapHundredsMore);
    addColorItem(mapCategory, ColorRole::MapThousands);

    // UI Colors category
    QTreeWidgetItem* uiCategory = new QTreeWidgetItem(tree);
    uiCategory->setText(0, "UI Colors");
    uiCategory->setExpanded(false);  // Collapsed by default
    uiCategory->setFont(0, categoryFont);

    addColorItem(uiCategory, ColorRole::PrimaryText);
    addColorItem(uiCategory, ColorRole::SecondaryText);
    addColorItem(uiCategory, ColorRole::HoverHighlight);
    addColorItem(uiCategory, ColorRole::BorderColor);

    mainLayout->addWidget(tree);

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

    colorDialog->resize(500, 600);
    colorDialog->exec();

    // Clean up
    qDeleteAll(colorButtons);
    colorDialog->deleteLater();
}

void PreferencesDialog::onDownloadClusterList() {
    // Create and configure downloader
    DXClusterListDownloader* downloader = new DXClusterListDownloader(this);

    // Create progress dialog
    QProgressDialog* progressDialog = new QProgressDialog(
        "Downloading DX cluster server list...",
        "Cancel",
        0, 100,
        this
    );
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);

    // Connect signals
    connect(downloader, &DXClusterListDownloader::downloadProgress,
            this, [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int percentage = (bytesReceived * 100) / bytesTotal;
            progressDialog->setValue(percentage);
        }
    });

    connect(downloader, &DXClusterListDownloader::downloadFinished,
            this, &PreferencesDialog::onClusterListDownloadFinished);

    connect(downloader, &DXClusterListDownloader::errorOccurred,
            this, [this, progressDialog](const QString& error) {
        progressDialog->cancel();
        DialogHelper::warning(this, "Download Error",
            QString("Failed to download cluster list:\n%1").arg(error));
    });

    connect(progressDialog, &QProgressDialog::canceled,
            downloader, &DXClusterListDownloader::cancel);

    // Clean up when done
    connect(downloader, &DXClusterListDownloader::downloadFinished,
            downloader, &QObject::deleteLater);
    connect(downloader, &DXClusterListDownloader::downloadFinished,
            progressDialog, &QObject::deleteLater);

    // Start download
    downloader->downloadList();
}

void PreferencesDialog::onClusterListDownloadFinished(bool success, const QList<DXClusterServer>& servers) {
    if (!success) {
        return;  // Error already handled in error signal
    }

    LOG_DEBUG("PreferencesDialog", QString("Downloaded %1 cluster servers").arg(servers.size()));

    if (servers.isEmpty()) {
        DialogHelper::warning(this, "Download Complete",
            "No cluster servers found in downloaded list.");
        return;
    }

    // Sort by country with user's country first
    QList<DXClusterServer> sortedServers = servers;
    AppSettings& settings = AppSettings::instance();
    DXClusterListDownloader::sortByCountry(sortedServers, settings.getMyCallsign());

    // Convert to QStringList for storage
    QStringList serverList;
    for (const DXClusterServer& server : sortedServers) {
        serverList.append(server.displayString());
    }

    // Save to settings
    settings.saveDXClusterList(serverList);

    DialogHelper::information(this, "Download Complete",
        QString("Successfully downloaded and saved %1 DX cluster servers.\n\n"
                "The server list is now available in the DX Cluster window.")
            .arg(servers.size()));

    // Update combo box with newly downloaded servers
    m_dxClusterServerCombo->clear();
    m_dxClusterServerCombo->addItems(serverList);
}

void PreferencesDialog::onDXClusterServerChanged(const QString& text) {
    if (text.isEmpty()) {
        // Empty is allowed, clear any red styling
        m_dxClusterServerCombo->lineEdit()->setStyleSheet("");
        return;
    }

    // Check if this is a display format from downloaded list: "W9ODD (134.48.91.82:23) - AR-Cluster"
    QRegularExpression displayFormat(R"(\([^:]+:\d+\))");
    if (displayFormat.match(text).hasMatch()) {
        // Valid display format from downloaded list
        m_dxClusterServerCombo->lineEdit()->setStyleSheet("");
        return;
    }

    // Validate plain formats: IPv4, IPv6, or hostname:port format
    // Format: hostname:port or ip:port or just hostname

    // Split by colon to separate host and port
    QStringList parts = text.split(':');
    if (parts.size() < 1 || parts.size() > 2) {
        // Invalid format (too many colons for non-IPv6)
        // But IPv6 has multiple colons, so check for that
        if (!text.contains('[')) {
            m_dxClusterServerCombo->lineEdit()->setStyleSheet("QLineEdit { color: red; }");
            return;
        }
    }

    QString host;
    QString port;

    // Check for IPv6 format: [::1]:7373
    if (text.contains('[')) {
        QRegularExpression ipv6Pattern(R"(\[([0-9a-fA-F:]+)\](?::(\d+))?)");
        QRegularExpressionMatch match = ipv6Pattern.match(text);
        if (match.hasMatch()) {
            host = match.captured(1);
            port = match.captured(2);

            // Validate IPv6 address
            QRegularExpression ipv6Regex(
                R"(^(([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|::1|::)$)"
            );

            if (!ipv6Regex.match(host).hasMatch()) {
                m_dxClusterServerCombo->lineEdit()->setStyleSheet("QLineEdit { color: red; }");
                return;
            }
        } else {
            m_dxClusterServerCombo->lineEdit()->setStyleSheet("QLineEdit { color: red; }");
            return;
        }
    } else {
        // IPv4 or hostname format: host:port
        if (parts.size() == 2) {
            host = parts[0];
            port = parts[1];
        } else if (parts.size() == 1) {
            host = parts[0];
            port = "";  // Port optional
        }

        // Validate IPv4 address
        QRegularExpression ipv4Regex(
            R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)"
        );

        // Validate hostname (RFC 1123)
        QRegularExpression hostnameRegex(
            R"(^(?:[a-zA-Z0-9](?:[a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)*[a-zA-Z0-9](?:[a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?$)"
        );

        bool isValidIPv4 = ipv4Regex.match(host).hasMatch();
        bool isValidHostname = hostnameRegex.match(host).hasMatch();

        if (!isValidIPv4 && !isValidHostname) {
            m_dxClusterServerCombo->lineEdit()->setStyleSheet("QLineEdit { color: red; }");
            return;
        }
    }

    // Validate port if present
    if (!port.isEmpty()) {
        bool ok;
        int portNum = port.toInt(&ok);
        if (!ok || portNum < 1 || portNum > 65535) {
            m_dxClusterServerCombo->lineEdit()->setStyleSheet("QLineEdit { color: red; }");
            return;
        }
    }

    // Valid format
    m_dxClusterServerCombo->lineEdit()->setStyleSheet("");
}

void PreferencesDialog::populateRadioList() {
    // Save current selection
    int currentModelId = m_radioModelCombo->currentData().toInt();

    // Block signals while repopulating
    m_radioModelCombo->blockSignals(true);
    m_radioModelCombo->clear();

    // Add default item
    m_radioModelCombo->addItem("Select a radio...", 0);

    // Get all radios from hamlib
    QList<RadioModelInfo> allRadios = RadioEnumerator::getAvailableRadios();

    // Filter based on status checkboxes
    QList<RadioModelInfo> filteredRadios;
    RadioModelInfo k4Radio;
    bool k4Found = false;

    for (const RadioModelInfo& radio : allRadios) {
        bool include = false;

        if (radio.status == "Stable" && m_showStableRadiosCheck->isChecked()) {
            include = true;
        } else if (radio.status == "Beta" && m_showBetaRadiosCheck->isChecked()) {
            include = true;
        } else if (radio.status == "Alpha" && m_showAlphaRadiosCheck->isChecked()) {
            include = true;
        } else if (radio.status == "Untested" && m_showUntestedRadiosCheck->isChecked()) {
            include = true;
        }

        if (include) {
            // Check if this is the Elecraft K4
            if (radio.manufacturer == "Elecraft" && radio.modelName == "K4") {
                k4Radio = radio;
                k4Found = true;
            } else {
                filteredRadios.append(radio);
            }
        }
    }

    // Add K4 first if found (user preference)
    int addedCount = 0;
    if (k4Found) {
        QString displayText = k4Radio.displayName();
        if (k4Radio.status != "Stable") {
            displayText += QString(" (%1)").arg(k4Radio.status);
        }
        m_radioModelCombo->addItem(displayText, k4Radio.modelId);
        addedCount++;
    }

    // Add all other radios
    for (const RadioModelInfo& radio : filteredRadios) {
        // Display format: "Manufacturer Model (Status)"
        // Only show status if not Stable to reduce clutter
        QString displayText = radio.displayName();
        if (radio.status != "Stable") {
            displayText += QString(" (%1)").arg(radio.status);
        }
        m_radioModelCombo->addItem(displayText, radio.modelId);
        addedCount++;
    }

    m_radioModelCombo->addItem("Custom (enter model ID below)...", -1);

    // Set up auto-completion
    QCompleter* completer = new QCompleter(this);
    completer->setModel(m_radioModelCombo->model());
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    m_radioModelCombo->setCompleter(completer);

    // Restore selection if possible
    int index = m_radioModelCombo->findData(currentModelId);
    if (index >= 0) {
        m_radioModelCombo->setCurrentIndex(index);
    }

    m_radioModelCombo->blockSignals(false);

    LOG_DEBUG("PreferencesDialog", QString("Radio list filtered: %1 radios shown (out of %2 total)").arg(addedCount).arg(allRadios.size()));
}

void PreferencesDialog::populateAmplifierList() {
    // Block signals while populating
    m_amplifierModelCombo->blockSignals(true);
    m_amplifierModelCombo->clear();

    // Add default item
    m_amplifierModelCombo->addItem("Select amplifier...", 0);

    // Add KPA1500 (Hamlib model ID 1201) - most common
    const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
    m_amplifierModelCombo->addItem("Elecraft KPA1500", AMP_MODEL_ELECRAFT_KPA1500);

    // Add KPA500 (Hamlib model ID 1202)
    const int AMP_MODEL_ELECRAFT_KPA500 = 1202;
    m_amplifierModelCombo->addItem("Elecraft KPA500", AMP_MODEL_ELECRAFT_KPA500);

    // Add common amplifiers from Hamlib (example models - can expand later)
    const int AMP_MODEL_DUMMY = 1;
    m_amplifierModelCombo->addItem("Dummy Amplifier (Test)", AMP_MODEL_DUMMY);

    m_amplifierModelCombo->addItem("Custom (enter model ID)...", -1);

    m_amplifierModelCombo->blockSignals(false);
}

void PreferencesDialog::populateRotatorList() {
    // Block signals while populating
    m_rotatorModelCombo->blockSignals(true);
    m_rotatorModelCombo->clear();

    // Add default item
    m_rotatorModelCombo->addItem("Select rotator...", 0);

    // Add PSTRotator (custom model - use 9999 as placeholder)
    const int ROT_MODEL_PSTROTATOR = 9999;
    m_rotatorModelCombo->addItem("PSTRotator", ROT_MODEL_PSTROTATOR);

    // Add common Hamlib rotators
    const int ROT_MODEL_DUMMY = 1;
    m_rotatorModelCombo->addItem("Dummy Rotator (Test)", ROT_MODEL_DUMMY);

    const int ROT_MODEL_EASYCOMM1 = 201;
    m_rotatorModelCombo->addItem("EASYCOMM I", ROT_MODEL_EASYCOMM1);

    const int ROT_MODEL_EASYCOMM2 = 202;
    m_rotatorModelCombo->addItem("EASYCOMM II", ROT_MODEL_EASYCOMM2);

    const int ROT_MODEL_ROTOREZ = 301;
    m_rotatorModelCombo->addItem("Hy-Gain DCU-1/DCU-1X (ROTOREZ)", ROT_MODEL_ROTOREZ);

    const int ROT_MODEL_GS232A = 603;
    m_rotatorModelCombo->addItem("Yaesu GS-232A", ROT_MODEL_GS232A);

    const int ROT_MODEL_GS232B = 604;
    m_rotatorModelCombo->addItem("Yaesu GS-232B", ROT_MODEL_GS232B);

    m_rotatorModelCombo->addItem("Custom (enter model ID)...", -1);

    m_rotatorModelCombo->blockSignals(false);
}

void PreferencesDialog::onRadioStatusFilterChanged() {
    // Repopulate radio list with new filter settings
    populateRadioList();
}

void PreferencesDialog::onFindK4Radios() {
    LOG_INFO("PreferencesDialog", "Starting K4 radio discovery...");

    // Clear previous results
    m_foundK4Radios.clear();

    // Disable button during discovery
    m_findK4Button->setEnabled(false);
    m_findK4Button->setText("Searching...");

    // Start discovery
    m_k4Discovery->startDiscovery();
}

void PreferencesDialog::onK4RadioFound(const K4RadioInfo& radio) {
    LOG_INFO("PreferencesDialog", QString("K4 radio found: %1 at %2")
        .arg(radio.serialNumber)
        .arg(radio.ipAddress));

    m_foundK4Radios.append(radio);
}

void PreferencesDialog::onK4DiscoveryFinished(int count) {
    LOG_INFO("PreferencesDialog", QString("K4 discovery finished - found %1 radio(s)").arg(count));

    // Re-enable button
    m_findK4Button->setEnabled(true);
    m_findK4Button->setText("Find K4 Radios on Network");

    // Display results
    if (count == 0) {
        DialogHelper::information(this, "K4 Discovery",
            "No K4 radios found on the network.\n\n"
            "Make sure:\n"
            "• Your K4 is powered on\n"
            "• Your K4 is connected to the same network\n"
            "• Your computer's firewall allows UDP port 9100");
    } else {
        QString message = QString("Found %1 K4 radio%2:\n\n")
            .arg(count)
            .arg(count == 1 ? "" : "s");

        for (const K4RadioInfo& radio : m_foundK4Radios) {
            message += QString("• Serial Number: %1\n")
                .arg(radio.serialNumber);
            message += QString("  IP Address: %1\n")
                .arg(radio.ipAddress);
            message += QString("  Hostname: %1\n\n")
                .arg(radio.hostname());
        }

        // If network connection is selected, offer to use discovered IP
        if (m_networkRadio->isChecked()) {
            if (count == 1) {
                // Single K4 found - ask if they want to use it
                message += QString("Use IP address %1?").arg(m_foundK4Radios.first().ipAddress);

                QMessageBox::StandardButton reply = DialogHelper::question(
                    this,
                    "K4 Discovery",
                    message,
                    QMessageBox::Yes | QMessageBox::No
                );

                if (reply == QMessageBox::Yes) {
                    m_ipAddressEdit->setText(m_foundK4Radios.first().ipAddress);
                    m_portSpin->setValue(K4_DEFAULT_RIGCTLD_PORT);  // K4 uses port 9200
                }
            } else {
                // Multiple K4s found - let user select which one
                QStringList radioOptions;
                for (const K4RadioInfo& radio : m_foundK4Radios) {
                    radioOptions << QString("K4 SN%1 - %2")
                        .arg(radio.serialNumber.rightJustified(5, '0'))
                        .arg(radio.ipAddress);
                }

                bool ok;
                QString selection = QInputDialog::getItem(
                    this,
                    "Select K4 Radio",
                    message + "\nSelect a K4 radio to use:",
                    radioOptions,
                    0,      // default to first item
                    false,  // not editable
                    &ok
                );

                if (ok && !selection.isEmpty()) {
                    // Find the selected radio and populate IP
                    int index = radioOptions.indexOf(selection);
                    if (index >= 0 && index < m_foundK4Radios.count()) {
                        m_ipAddressEdit->setText(m_foundK4Radios[index].ipAddress);
                        m_portSpin->setValue(K4_DEFAULT_RIGCTLD_PORT);  // K4 uses port 9200
                    }
                }
            }
        } else {
            DialogHelper::information(this, "K4 Discovery", message);
        }
    }
}

void PreferencesDialog::refreshSerialPorts() {
    // Remember current selection
    QString currentPort = m_serialPortCombo->currentData().toString();
    if (currentPort.isEmpty() && m_serialPortCombo->currentIndex() >= 0) {
        currentPort = m_serialPortCombo->currentText();
    }

    // Clear and repopulate
    m_serialPortCombo->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        m_serialPortCombo->addItem("(No serial ports detected)", "");
        m_serialPortCombo->setEnabled(false);
    } else {
        m_serialPortCombo->setEnabled(true);

        for (const QSerialPortInfo& port : ports) {
            QString displayName = port.portName();
            if (!port.description().isEmpty()) {
                displayName += QString(" (%1)").arg(port.description());
            }
            m_serialPortCombo->addItem(displayName, port.portName());
        }

        // Try to restore previous selection
        if (!currentPort.isEmpty()) {
            int index = m_serialPortCombo->findData(currentPort);
            if (index >= 0) {
                m_serialPortCombo->setCurrentIndex(index);
            }
        }
    }

    LOG_DEBUG("PreferencesDialog", QString("Refreshed serial ports: %1 found").arg(ports.count()));
}

void PreferencesDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    // Refresh ports when dialog becomes visible
    refreshSerialPorts();

    // Start auto-refresh timer if serial mode is selected
    if (m_serialRadio && m_serialRadio->isChecked()) {
        m_portRefreshTimer->start();
    }
}

void PreferencesDialog::hideEvent(QHideEvent* event) {
    // Stop auto-refresh timer when dialog is hidden
    m_portRefreshTimer->stop();

    QDialog::hideEvent(event);
}

void PreferencesDialog::onFindNetworkRadios() {
    // Dispatcher - calls K4 or Icom discovery based on selected radio type
    int radioType = m_radioTypeCombo->currentData().toInt();

    if (radioType == 2) {  // ICOM_DIRECT
        onFindIcomRadios();
    } else if (radioType == 1) {  // K4_DIRECT
        onFindK4Radios();
    } else {
        // Auto or Hamlib - show info message
        DialogHelper::information(this, "Network Discovery",
            "Network discovery is only available for:\n\n"
            "• K4 Direct (Elecraft K4 radios)\n"
            "• Icom Direct (Icom network radios)\n\n"
            "Please select one of these radio types to use discovery.");
    }
}

void PreferencesDialog::onFindIcomRadios() {
    LOG_INFO("PreferencesDialog", "Starting Icom radio discovery...");

    // Clear previous results
    m_foundIcomRadios.clear();

    // Disable button during discovery
    m_findK4Button->setEnabled(false);
    m_findK4Button->setText("Searching...");

    // Start discovery
    m_icomDiscovery->startDiscovery();
}

void PreferencesDialog::onIcomRadioFound(const IcomRadioDiscoveryInfo& radio) {
    LOG_INFO("PreferencesDialog", QString("Icom radio found: %1 (ID: 0x%2) on %3")
        .arg(radio.ipAddress)
        .arg(radio.radioId, 8, 16, QChar('0'))
        .arg(radio.networkInterface));

    m_foundIcomRadios.append(radio);
}

void PreferencesDialog::onIcomDiscoveryFinished(int count) {
    LOG_INFO("PreferencesDialog", QString("Icom discovery finished - found %1 radio(s)").arg(count));

    // Re-enable button
    m_findK4Button->setEnabled(true);
    m_findK4Button->setText("Find Icom Radios on Network");

    // Display results
    if (count == 0) {
        DialogHelper::information(this, "Icom Discovery",
            "No Icom radios found on the network.\n\n"
            "Make sure:\n"
            "• Your Icom radio is powered on\n"
            "• The radio is connected to the same network\n"
            "• Your computer's firewall allows UDP port 50001\n"
            "• Your radio model supports network operation\n"
            "  (IC-705, IC-7100, IC-7300MK2, IC-7610, IC-7700, IC-7760,\n"
            "   IC-7800, IC-7850, IC-7851, IC-905, IC-9700, IC-R8600)");
    } else {
        QString message = QString("Found %1 Icom radio%2:\n\n")
            .arg(count)
            .arg(count == 1 ? "" : "s");

        for (const IcomRadioDiscoveryInfo& radio : m_foundIcomRadios) {
            message += QString("• IP Address: %1\n")
                .arg(radio.ipAddress);
            message += QString("  Radio ID: 0x%1\n")
                .arg(radio.radioId, 8, 16, QChar('0'));
            message += QString("  Interface: %1\n\n")
                .arg(radio.networkInterface);
        }

        // If network connection is selected, offer to use discovered IP
        if (m_networkRadio->isChecked()) {
            if (count == 1) {
                // Single Icom found - ask if they want to use it
                message += QString("Use IP address %1?").arg(m_foundIcomRadios.first().ipAddress);

                QMessageBox::StandardButton reply = DialogHelper::question(
                    this,
                    "Icom Discovery",
                    message,
                    QMessageBox::Yes | QMessageBox::No
                );

                if (reply == QMessageBox::Yes) {
                    m_ipAddressEdit->setText(m_foundIcomRadios.first().ipAddress);
                    m_portSpin->setValue(50001);  // Icom uses port 50001
                }
            } else {
                // Multiple Icom radios found - let user select which one
                QStringList radioOptions;
                for (const IcomRadioDiscoveryInfo& radio : m_foundIcomRadios) {
                    radioOptions << QString("%1 (ID: 0x%2)")
                        .arg(radio.ipAddress)
                        .arg(radio.radioId, 8, 16, QChar('0'));
                }

                bool ok;
                QString selection = QInputDialog::getItem(
                    this,
                    "Select Icom Radio",
                    message + "\nSelect an Icom radio to use:",
                    radioOptions,
                    0,      // default to first item
                    false,  // not editable
                    &ok
                );

                if (ok && !selection.isEmpty()) {
                    // Find the selected radio and populate IP
                    int index = radioOptions.indexOf(selection);
                    if (index >= 0 && index < m_foundIcomRadios.count()) {
                        m_ipAddressEdit->setText(m_foundIcomRadios[index].ipAddress);
                        m_portSpin->setValue(50001);  // Icom uses port 50001
                    }
                }
            }
        } else {
            DialogHelper::information(this, "Icom Discovery", message);
        }
    }
}

} // namespace TR4QT
