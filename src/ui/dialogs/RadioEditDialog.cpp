#include "RadioEditDialog.h"
#include "../../utils/AppSettings.h"
#include "../../utils/DialogHelper.h"
#include "../../logging/LogMacros.h"
#include "../../radio/RadioFactory.h"
#include "../../utils/CredentialStore.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QApplication>

// Hamlib includes for radio list
extern "C" {
#include <hamlib/rig.h>
}

namespace TR4QT {

RadioEditDialog::RadioEditDialog(QWidget* parent)
    : QDialog(parent)
    , m_isEditMode(false)
{
    setWindowTitle("Add New Radio");
    setModal(true);
    setMinimumWidth(500);
    setupUI();
}

RadioEditDialog::RadioEditDialog(const RadioProfile& profile, QWidget* parent)
    : QDialog(parent)
    , m_originalProfile(profile)
    , m_isEditMode(true)
{
    setWindowTitle(QString("Edit Radio: %1").arg(profile.name));
    setModal(true);
    setMinimumWidth(500);
    setupUI();
    loadProfileIntoUI(profile);
}

void RadioEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Radio Name
    QGroupBox* nameGroup = new QGroupBox("Radio Name", this);
    QFormLayout* nameLayout = new QFormLayout(nameGroup);
    m_radioNameEdit = new QLineEdit(this);
    m_radioNameEdit->setPlaceholderText("e.g., K4D, IC-7760, Contest Radio");
    nameLayout->addRow("Name:", m_radioNameEdit);
    mainLayout->addWidget(nameGroup);

    // Radio Model
    QGroupBox* modelGroup = new QGroupBox("Radio Model", this);
    QFormLayout* modelLayout = new QFormLayout(modelGroup);

    m_radioModelCombo = new QComboBox(this);
    m_radioModelCombo->setEditable(true);
    m_radioModelCombo->setInsertPolicy(QComboBox::NoInsert);
    m_radioModelCombo->blockSignals(true);

    m_customModelEdit = new QLineEdit(this);
    m_customModelEdit->setPlaceholderText("Enter hamlib model ID (e.g., 2046)");
    m_customModelEdit->setVisible(false);

    modelLayout->addRow("Model:", m_radioModelCombo);
    modelLayout->addRow("Custom Model ID:", m_customModelEdit);

    // Radio status filter checkboxes
    m_showStableRadiosCheck = new QCheckBox("Stable", this);
    m_showBetaRadiosCheck = new QCheckBox("Beta", this);
    m_showAlphaRadiosCheck = new QCheckBox("Alpha", this);
    m_showUntestedRadiosCheck = new QCheckBox("Untested", this);

    m_showStableRadiosCheck->setChecked(true);

    connect(m_showStableRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);
    connect(m_showBetaRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);
    connect(m_showAlphaRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);
    connect(m_showUntestedRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);

    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Show:", this));
    filterLayout->addWidget(m_showStableRadiosCheck);
    filterLayout->addWidget(m_showBetaRadiosCheck);
    filterLayout->addWidget(m_showAlphaRadiosCheck);
    filterLayout->addWidget(m_showUntestedRadiosCheck);
    filterLayout->addStretch();
    modelLayout->addRow("", filterLayout);

    // Radio Interface Type
    m_radioTypeCombo = new QComboBox(this);
    m_radioTypeCombo->addItem("Auto (Recommended)", -1);
    m_radioTypeCombo->addItem("Hamlib (Universal)", 0);
    m_radioTypeCombo->addItem("K4 Direct (K4 only, 5-10x faster)", 1);
    m_radioTypeCombo->addItem("Icom Direct (Icom network radios)", 2);
    m_radioTypeCombo->setCurrentIndex(0);
    m_radioTypeCombo->setToolTip(
        "Auto: Automatically selects the best interface for your radio\n"
        "Hamlib: Universal compatibility, works with all radios\n"
        "K4 Direct: Direct TCP control for Elecraft K4 (5-10x faster)\n"
        "Icom Direct: Native Icom network protocol for supported radios"
    );
    connect(m_radioTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RadioEditDialog::onRadioTypeChanged);
    modelLayout->addRow("Interface:", m_radioTypeCombo);

    mainLayout->addWidget(modelGroup);

    // Connection Type
    QGroupBox* connectionGroup = new QGroupBox("Connection", this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    m_serialRadio = new QRadioButton("Serial Port", this);
    m_networkRadio = new QRadioButton("Network (TCP)", this);
    m_serialRadio->setChecked(true);
    connect(m_serialRadio, &QRadioButton::toggled,
            this, &RadioEditDialog::onConnectionTypeChanged);

    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(m_serialRadio);
    typeLayout->addWidget(m_networkRadio);
    typeLayout->addStretch();
    connectionLayout->addLayout(typeLayout);

    // Serial Port Settings
    m_serialGroup = new QGroupBox("Serial Port Settings", this);
    QFormLayout* serialLayout = new QFormLayout(m_serialGroup);

    QHBoxLayout* portLayout = new QHBoxLayout();
    m_serialPortCombo = new QComboBox(this);
    m_serialPortCombo->setMinimumWidth(200);
    portLayout->addWidget(m_serialPortCombo, 1);

    m_refreshPortsButton = new QPushButton("Refresh", this);
    m_refreshPortsButton->setMaximumWidth(80);
    connect(m_refreshPortsButton, &QPushButton::clicked, this, &RadioEditDialog::refreshSerialPorts);
    portLayout->addWidget(m_refreshPortsButton);
    serialLayout->addRow("Port:", portLayout);

    m_serialPortEdit = new QLineEdit(this);
    m_serialPortEdit->setPlaceholderText("Manual entry (e.g., COM10 or /dev/ttyUSB0)");
    serialLayout->addRow("Or manual:", m_serialPortEdit);

    m_baudRateCombo = new QComboBox(this);
    m_baudRateCombo->addItems({"4800", "9600", "19200", "38400", "57600", "115200"});
    m_baudRateCombo->setCurrentText("38400");
    serialLayout->addRow("Baud Rate:", m_baudRateCombo);

    m_dataBitsCombo = new QComboBox(this);
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    serialLayout->addRow("Data Bits:", m_dataBitsCombo);

    m_stopBitsCombo = new QComboBox(this);
    m_stopBitsCombo->addItems({"1", "2"});
    m_stopBitsCombo->setCurrentText("1");
    serialLayout->addRow("Stop Bits:", m_stopBitsCombo);

    m_parityCombo = new QComboBox(this);
    m_parityCombo->addItems({"None", "Odd", "Even"});
    m_parityCombo->setCurrentIndex(0);
    serialLayout->addRow("Parity:", m_parityCombo);

    connectionLayout->addWidget(m_serialGroup);

    // Network Settings
    m_networkGroup = new QGroupBox("Network Settings", this);
    QFormLayout* networkLayout = new QFormLayout(m_networkGroup);

    m_ipAddressEdit = new QLineEdit(this);
    m_ipAddressEdit->setPlaceholderText("192.168.1.100");
    networkLayout->addRow("IP Address:", m_ipAddressEdit);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(4532);
    networkLayout->addRow("Port:", m_portSpin);

    m_icomUsernameEdit = new QLineEdit(this);
    m_icomUsernameEdit->setPlaceholderText("Username (optional)");
    networkLayout->addRow("Icom Username:", m_icomUsernameEdit);

    m_icomPasswordEdit = new QLineEdit(this);
    m_icomPasswordEdit->setPlaceholderText("Password (optional)");
    m_icomPasswordEdit->setEchoMode(QLineEdit::Password);
    networkLayout->addRow("Icom Password:", m_icomPasswordEdit);

    m_icomClientNameEdit = new QLineEdit(this);
    m_icomClientNameEdit->setText("TR4QT");
    networkLayout->addRow("Client Name:", m_icomClientNameEdit);

    m_findRadiosButton = new QPushButton("Find Radios on Network", this);
    connect(m_findRadiosButton, &QPushButton::clicked, this, &RadioEditDialog::onFindNetworkRadios);
    networkLayout->addRow("", m_findRadiosButton);

    connectionLayout->addWidget(m_networkGroup);
    m_networkGroup->setVisible(false);

    mainLayout->addWidget(connectionGroup);

    // Advanced Settings
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    QFormLayout* advancedLayout = new QFormLayout(advancedGroup);

    m_civAddressWidget = new CivAddressWidget(this);
    advancedLayout->addRow("CI-V Address:", m_civAddressWidget);

    m_pollIntervalSpin = new QSpinBox(this);
    m_pollIntervalSpin->setRange(100, 5000);
    m_pollIntervalSpin->setValue(5000);
    m_pollIntervalSpin->setSingleStep(500);
    m_pollIntervalSpin->setSuffix(" ms");
    advancedLayout->addRow("Poll Interval:", m_pollIntervalSpin);

    mainLayout->addWidget(advancedGroup);

    // Test Connection
    QHBoxLayout* testLayout = new QHBoxLayout();
    m_testConnectionButton = new QPushButton("Test Connection", this);
    connect(m_testConnectionButton, &QPushButton::clicked, this, &RadioEditDialog::onTestConnection);
    testLayout->addWidget(m_testConnectionButton);

    m_connectionStatusLabel = new QLabel(this);
    m_connectionStatusLabel->setStyleSheet("color: #666; font-style: italic;");
    testLayout->addWidget(m_connectionStatusLabel);
    testLayout->addStretch();
    mainLayout->addLayout(testLayout);

    mainLayout->addStretch();

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Validate name
        if (m_radioNameEdit->text().trimmed().isEmpty()) {
            DialogHelper::warning(this, "Missing Name",
                "Please enter a name for this radio.");
            m_radioNameEdit->setFocus();
            return;
        }
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Initialize
    populateRadioList();
    refreshSerialPorts();

    connect(m_radioModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RadioEditDialog::onRadioModelChanged);
    m_radioModelCombo->blockSignals(false);

    // Setup discovery objects
    m_k4Discovery = new K4Discovery(this);
    connect(m_k4Discovery, &K4Discovery::radioFound,
            this, &RadioEditDialog::onK4RadioFound);
    connect(m_k4Discovery, &K4Discovery::discoveryFinished,
            this, &RadioEditDialog::onK4DiscoveryFinished);

    m_icomDiscovery = new IcomDiscovery(this);
    connect(m_icomDiscovery, &IcomDiscovery::radioFound,
            this, &RadioEditDialog::onIcomRadioFound);
    connect(m_icomDiscovery, &IcomDiscovery::discoveryFinished,
            this, &RadioEditDialog::onIcomDiscoveryFinished);
}

void RadioEditDialog::populateRadioList()
{
    m_radioModelCombo->blockSignals(true);
    m_radioModelCombo->clear();

    int radioType = m_radioTypeCombo->currentData().toInt();

    // For direct interfaces, show only radios with actual implementations
    // Query RadioFactory instead of hardcoding - single source of truth
    RadioFactory::RadioType factoryType = RadioFactory::RadioType::HAMLIB;
    if (radioType == 1) {
        factoryType = RadioFactory::RadioType::K4_DIRECT;
    } else if (radioType == 2) {
        factoryType = RadioFactory::RadioType::ICOM_DIRECT;
    }

    if (radioType == 1 || radioType == 2) {  // K4 Direct or Icom Direct
        QList<SupportedRadio> implementedRadios = RadioFactory::getImplementedRadios(factoryType);
        for (const SupportedRadio& radio : implementedRadios) {
            m_radioModelCombo->addItem(radio.displayName, radio.hamlibModelId);
        }
        // Hide the status filter checkboxes - not relevant for direct interfaces
        m_showStableRadiosCheck->setVisible(false);
        m_showBetaRadiosCheck->setVisible(false);
        m_showAlphaRadiosCheck->setVisible(false);
        m_showUntestedRadiosCheck->setVisible(false);
        m_radioModelCombo->blockSignals(false);
        return;
    }

    // For Hamlib or Auto, show the full Hamlib list with status filters
    m_showStableRadiosCheck->setVisible(true);
    m_showBetaRadiosCheck->setVisible(true);
    m_showAlphaRadiosCheck->setVisible(true);
    m_showUntestedRadiosCheck->setVisible(true);

    bool showStable = m_showStableRadiosCheck->isChecked();
    bool showBeta = m_showBetaRadiosCheck->isChecked();
    bool showAlpha = m_showAlphaRadiosCheck->isChecked();
    bool showUntested = m_showUntestedRadiosCheck->isChecked();

    // Get list from Hamlib
    rig_load_all_backends();

    struct RadioEntry {
        int modelId;
        QString name;
        QString statusStr;
        rig_status_e status;
    };
    QList<RadioEntry> radios;

    // Enumerate all radios
    int status = rig_list_foreach(
        [](const rig_caps* caps, void* data) -> int {
            auto* list = static_cast<QList<RadioEntry>*>(data);
            RadioEntry entry;
            entry.modelId = caps->rig_model;
            entry.name = QString("%1 %2").arg(caps->mfg_name).arg(caps->model_name);
            entry.status = caps->status;

            switch (caps->status) {
                case RIG_STATUS_STABLE: entry.statusStr = "Stable"; break;
                case RIG_STATUS_BETA: entry.statusStr = "Beta"; break;
                case RIG_STATUS_ALPHA: entry.statusStr = "Alpha"; break;
                default: entry.statusStr = "Untested"; break;
            }
            list->append(entry);
            return 1;
        },
        &radios
    );

    if (status != RIG_OK) {
        LOG_ERROR("RadioEditDialog", "Failed to enumerate radios from Hamlib");
        return;
    }

    // Sort by name
    std::sort(radios.begin(), radios.end(), [](const RadioEntry& a, const RadioEntry& b) {
        return a.name.toLower() < b.name.toLower();
    });

    // Add filtered items
    for (const auto& radio : radios) {
        bool include = false;
        if (radio.status == RIG_STATUS_STABLE && showStable) include = true;
        if (radio.status == RIG_STATUS_BETA && showBeta) include = true;
        if (radio.status == RIG_STATUS_ALPHA && showAlpha) include = true;
        if (radio.status == RIG_STATUS_UNTESTED && showUntested) include = true;

        if (include) {
            QString displayName = QString("%1 [%2]").arg(radio.name).arg(radio.statusStr);
            m_radioModelCombo->addItem(displayName, radio.modelId);
        }
    }

    // Add custom option at end
    m_radioModelCombo->addItem("-- Custom Model ID --", -1);

    m_radioModelCombo->blockSignals(false);
}

void RadioEditDialog::loadProfileIntoUI(const RadioProfile& profile)
{
    m_radioNameEdit->setText(profile.name);

    const RadioConfig& config = profile.config;

    // Set radio type/interface FIRST so the model list is correctly populated
    // (Icom Direct shows only supported radios, not the full Hamlib list)
    int typeIndex = m_radioTypeCombo->findData(config.radioType);
    if (typeIndex >= 0) {
        m_radioTypeCombo->setCurrentIndex(typeIndex);
        // This triggers onRadioTypeChanged which calls populateRadioList()
    }

    // Now find and select radio model (in the correctly filtered list)
    int modelIndex = m_radioModelCombo->findData(config.hamlibModelId);
    if (modelIndex >= 0) {
        m_radioModelCombo->setCurrentIndex(modelIndex);
    } else if (config.hamlibModelId > 0) {
        // Model not in current list - check if it's a custom model for Hamlib
        // For direct interfaces, the model should always be in the list
        int radioType = m_radioTypeCombo->currentData().toInt();
        if (radioType == 0 || radioType == -1) {  // Hamlib or Auto
            m_radioModelCombo->setCurrentIndex(m_radioModelCombo->count() - 1);
            m_customModelEdit->setText(QString::number(config.hamlibModelId));
            m_customModelEdit->setVisible(true);
        }
    }

    // Connection type and settings
    bool isNetwork = config.port.contains(':');
    if (isNetwork) {
        m_networkRadio->setChecked(true);
        QStringList parts = config.port.split(':');
        if (parts.size() >= 2) {
            m_ipAddressEdit->setText(parts[0]);
            m_portSpin->setValue(parts[1].toInt());
        }
    } else {
        m_serialRadio->setChecked(true);
        int portIndex = m_serialPortCombo->findText(config.port);
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

    // Icom settings
    m_icomUsernameEdit->setText(config.icomUsername);
    m_icomClientNameEdit->setText(config.icomClientName);

    // Load password from secure storage
    QString password = CredentialStore::instance().getPassword(
        CredentialKeys::icomRadioProfile(profile.name), config.icomUsername);
    m_icomPasswordEdit->setText(password);

    // Advanced settings
    if (config.civAddress > 0) {
        m_civAddressWidget->setCivAddress(config.civAddress);
    }
    m_pollIntervalSpin->setValue(config.pollInterval);

    onConnectionTypeChanged();
}

RadioConfig RadioEditDialog::buildRadioConfigFromUI() const
{
    RadioConfig config;

    // Model
    int modelId = m_radioModelCombo->currentData().toInt();
    if (modelId == -1 && !m_customModelEdit->text().isEmpty()) {
        modelId = m_customModelEdit->text().toInt();
    }
    config.hamlibModelId = modelId;

    // Radio type
    config.radioType = m_radioTypeCombo->currentData().toInt();

    // Connection
    if (m_networkRadio->isChecked()) {
        config.port = QString("%1:%2").arg(m_ipAddressEdit->text()).arg(m_portSpin->value());
        config.connectionMethod = 2;  // Network
    } else {
        // Prefer combo selection, fall back to manual entry
        if (m_serialPortCombo->currentIndex() > 0) {
            config.port = m_serialPortCombo->currentText();
        } else if (!m_serialPortEdit->text().isEmpty()) {
            config.port = m_serialPortEdit->text();
        } else {
            config.port = m_serialPortCombo->currentText();
        }
        config.baudRate = m_baudRateCombo->currentText().toInt();
        config.dataBits = m_dataBitsCombo->currentText().toInt();
        config.stopBits = m_stopBitsCombo->currentText().toInt();
        config.parity = m_parityCombo->currentIndex();
        config.connectionMethod = 1;  // Serial
    }

    // Icom settings
    config.icomUsername = m_icomUsernameEdit->text();
    config.icomPassword = m_icomPasswordEdit->text();
    config.icomClientName = m_icomClientNameEdit->text();

    // Advanced
    config.civAddress = m_civAddressWidget->getCivAddress();
    config.pollInterval = m_pollIntervalSpin->value();

    return config;
}

RadioProfile RadioEditDialog::getRadioProfile() const
{
    RadioProfile profile;
    profile.name = m_radioNameEdit->text().trimmed();
    profile.config = buildRadioConfigFromUI();
    profile.lastUsed = QDateTime::currentDateTime();

    if (m_isEditMode) {
        profile.notes = m_originalProfile.notes;
    }

    return profile;
}

QString RadioEditDialog::getRadioName() const
{
    return m_radioNameEdit->text().trimmed();
}

void RadioEditDialog::onConnectionTypeChanged()
{
    bool isSerial = m_serialRadio->isChecked();
    m_serialGroup->setVisible(isSerial);
    m_networkGroup->setVisible(!isSerial);
}

void RadioEditDialog::onRadioModelChanged(int index)
{
    int modelId = m_radioModelCombo->itemData(index).toInt();
    m_customModelEdit->setVisible(modelId == -1);
}

void RadioEditDialog::onRadioStatusFilterChanged()
{
    int currentModelId = m_radioModelCombo->currentData().toInt();
    populateRadioList();

    // Try to restore selection
    int newIndex = m_radioModelCombo->findData(currentModelId);
    if (newIndex >= 0) {
        m_radioModelCombo->setCurrentIndex(newIndex);
    }
}

void RadioEditDialog::onRadioTypeChanged(int index)
{
    int radioType = m_radioTypeCombo->itemData(index).toInt();

    // Update default port based on radio type
    if (radioType == 1) {  // K4 Direct
        m_portSpin->setValue(9200);
        m_networkRadio->setChecked(true);  // K4 Direct is always network
        onConnectionTypeChanged();
    } else if (radioType == 2) {  // Icom Direct
        m_portSpin->setValue(50001);
        m_networkRadio->setChecked(true);  // Icom Direct is always network
        onConnectionTypeChanged();
    } else {
        m_portSpin->setValue(4532);  // Default rigctld
    }

    // Repopulate radio list based on interface type
    // This filters to show only radios supported by the selected interface
    int currentModelId = m_radioModelCombo->currentData().toInt();
    populateRadioList();

    // Try to restore selection
    int newIndex = m_radioModelCombo->findData(currentModelId);
    if (newIndex >= 0) {
        m_radioModelCombo->setCurrentIndex(newIndex);
    } else if (m_radioModelCombo->count() > 0) {
        // If old model not in new list, select first item
        m_radioModelCombo->setCurrentIndex(0);
    }
}

void RadioEditDialog::onTestConnection()
{
    m_connectionStatusLabel->setText("Testing connection...");
    m_connectionStatusLabel->setStyleSheet("color: #666; font-style: italic;");
    m_testConnectionButton->setEnabled(false);
    QApplication::processEvents();

    RadioConfig config = buildRadioConfigFromUI();

    // Determine the radio type for the factory
    RadioFactory::RadioType radioType;
    if (config.radioType == 1) {
        radioType = RadioFactory::RadioType::K4_DIRECT;
    } else if (config.radioType == 2) {
        radioType = RadioFactory::RadioType::ICOM_DIRECT;
    } else {
        radioType = RadioFactory::RadioType::HAMLIB;
    }

    // Create temporary radio interface for testing
    auto radio = RadioFactory::createRadio(radioType, config, this);
    if (!radio) {
        m_connectionStatusLabel->setText("Failed to create radio interface");
        m_connectionStatusLabel->setStyleSheet("color: red;");
        m_testConnectionButton->setEnabled(true);
        return;
    }

    // Try to connect
    bool success = radio->connect(config);

    if (success) {
        m_connectionStatusLabel->setText("Connection successful!");
        m_connectionStatusLabel->setStyleSheet("color: green;");
        radio->disconnect();
    } else {
        m_connectionStatusLabel->setText("Connection failed");
        m_connectionStatusLabel->setStyleSheet("color: red;");
    }

    delete radio;  // Clean up the temporary radio
    m_testConnectionButton->setEnabled(true);
}

void RadioEditDialog::onFindNetworkRadios()
{
    int radioType = m_radioTypeCombo->currentData().toInt();

    m_findRadiosButton->setEnabled(false);
    m_findRadiosButton->setText("Searching...");
    m_foundK4Radios.clear();
    m_foundIcomRadios.clear();

    if (radioType == 1) {  // K4 Direct
        m_k4Discovery->startDiscovery();
    } else if (radioType == 2) {  // Icom Direct
        m_icomDiscovery->startDiscovery();
    } else {
        // Auto mode - try both
        m_k4Discovery->startDiscovery();
        m_icomDiscovery->startDiscovery();
    }
}

void RadioEditDialog::refreshSerialPorts()
{
    m_serialPortCombo->clear();
    m_serialPortCombo->addItem("-- Select Port --");

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const auto& port : ports) {
        QString displayName = port.portName();
        if (!port.description().isEmpty()) {
            displayName += QString(" (%1)").arg(port.description());
        }
        m_serialPortCombo->addItem(displayName, port.portName());
    }
}

void RadioEditDialog::onK4RadioFound(const K4RadioInfo& radio)
{
    m_foundK4Radios.append(radio);
    m_ipAddressEdit->setText(radio.ipAddress);
    m_portSpin->setValue(9200);
    m_networkRadio->setChecked(true);
    onConnectionTypeChanged();

    LOG_INFO("RadioEditDialog", QString("K4 found: %1 at %2")
             .arg(radio.rigType).arg(radio.ipAddress));
}

void RadioEditDialog::onK4DiscoveryFinished(int count)
{
    m_findRadiosButton->setEnabled(true);
    m_findRadiosButton->setText("Find Radios on Network");

    if (count == 0 && m_foundIcomRadios.isEmpty()) {
        DialogHelper::information(this, "Discovery Complete",
            "No radios found on the network.\n\n"
            "Make sure your radio is:\n"
            "- Powered on\n"
            "- Connected to the same network\n"
            "- Has network control enabled");
    } else if (count > 0) {
        DialogHelper::information(this, "Radio Found",
            QString("Found %1 K4 radio(s) on the network.\n"
                    "IP address has been filled in.").arg(count));
    }
}

void RadioEditDialog::onIcomRadioFound(const IcomRadioDiscoveryInfo& radio)
{
    m_foundIcomRadios.append(radio);
    m_ipAddressEdit->setText(radio.ipAddress);
    m_portSpin->setValue(50001);
    m_networkRadio->setChecked(true);
    onConnectionTypeChanged();

    LOG_INFO("RadioEditDialog", QString("Icom found: ID %1 at %2")
             .arg(radio.radioId).arg(radio.ipAddress));
}

void RadioEditDialog::onIcomDiscoveryFinished(int count)
{
    m_findRadiosButton->setEnabled(true);
    m_findRadiosButton->setText("Find Radios on Network");

    if (count == 0 && m_foundK4Radios.isEmpty()) {
        DialogHelper::information(this, "Discovery Complete",
            "No radios found on the network.\n\n"
            "Make sure your radio is:\n"
            "- Powered on\n"
            "- Connected to the same network\n"
            "- Has network control enabled");
    } else if (count > 0) {
        DialogHelper::information(this, "Radio Found",
            QString("Found %1 Icom radio(s) on the network.\n"
                    "IP address has been filled in.").arg(count));
    }
}

} // namespace TR4QT
