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
#include <QInputDialog>
#include <QIntValidator>
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

    // ===== 1. Radio Name =====
    QGroupBox* nameGroup = new QGroupBox("Radio Name", this);
    QFormLayout* nameLayout = new QFormLayout(nameGroup);
    m_radioNameEdit = new QLineEdit(this);
    m_radioNameEdit->setPlaceholderText("e.g., K4D, IC-7760, Contest Radio");
    nameLayout->addRow("Name:", m_radioNameEdit);
    mainLayout->addWidget(nameGroup);

    // ===== 2. Connection Type (Primary Choice) =====
    QGroupBox* connectionGroup = new QGroupBox("Connection", this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    // Serial / Network radio buttons
    QHBoxLayout* connTypeLayout = new QHBoxLayout();
    m_serialRadio = new QRadioButton("Serial Port", this);
    m_networkRadio = new QRadioButton("Network (TCP)", this);
    m_serialRadio->setChecked(true);
    connTypeLayout->addWidget(m_serialRadio);
    connTypeLayout->addWidget(m_networkRadio);
    connTypeLayout->addStretch();
    connectionLayout->addLayout(connTypeLayout);

    // ----- Serial Port Settings -----
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

    // Baud rate: combo on left, custom entry on right (enabled when "Custom" selected)
    QWidget* baudRateWidget = new QWidget(this);
    QHBoxLayout* baudRateLayout = new QHBoxLayout(baudRateWidget);
    baudRateLayout->setContentsMargins(0, 0, 0, 0);

    m_baudRateCombo = new QComboBox(this);
    m_baudRateCombo->addItems({"4800", "9600", "19200", "38400", "57600", "115200", "230400", "3000000", "Custom"});
    m_baudRateCombo->setCurrentText("38400");
    baudRateLayout->addWidget(m_baudRateCombo);

    m_customBaudRateEdit = new QLineEdit(this);
    m_customBaudRateEdit->setPlaceholderText("Enter baud rate");
    m_customBaudRateEdit->setValidator(new QIntValidator(300, 5000000, this));
    m_customBaudRateEdit->setEnabled(false);  // Disabled until "Custom" selected
    m_customBaudRateEdit->setFixedWidth(120);
    baudRateLayout->addWidget(m_customBaudRateEdit);

    // Enable/disable custom field based on combo selection
    connect(m_baudRateCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        bool isCustom = (text == "Custom");
        m_customBaudRateEdit->setEnabled(isCustom);
        if (isCustom && m_customBaudRateEdit->text().isEmpty()) {
            m_customBaudRateEdit->setFocus();
        }
    });

    serialLayout->addRow("Baud Rate:", baudRateWidget);

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

    // ----- Network Settings -----
    m_networkGroup = new QGroupBox("Network Settings", this);
    QVBoxLayout* networkMainLayout = new QVBoxLayout(m_networkGroup);

    // Interface Type (only for Network)
    m_interfaceTypeWidget = new QWidget(this);
    QHBoxLayout* interfaceLayout = new QHBoxLayout(m_interfaceTypeWidget);
    interfaceLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* interfaceLabel = new QLabel("Interface:", this);
    m_hamlibRadio = new QRadioButton("Hamlib", this);
    m_k4DirectRadio = new QRadioButton("K4 Direct", this);
    m_icomDirectRadio = new QRadioButton("Icom Direct", this);
    m_hamlibRadio->setChecked(true);
    m_hamlibRadio->setToolTip("Universal compatibility, works with all radios");
    m_k4DirectRadio->setToolTip("Direct TCP control for Elecraft K4 (5-10x faster)");
    m_icomDirectRadio->setToolTip("Native Icom network protocol for supported radios");
    interfaceLayout->addWidget(interfaceLabel);
    interfaceLayout->addWidget(m_hamlibRadio);
    interfaceLayout->addWidget(m_k4DirectRadio);
    interfaceLayout->addWidget(m_icomDirectRadio);
    interfaceLayout->addStretch();
    networkMainLayout->addWidget(m_interfaceTypeWidget);

    // Network address fields
    QFormLayout* networkLayout = new QFormLayout();
    m_ipAddressEdit = new QLineEdit(this);
    m_ipAddressEdit->setPlaceholderText("192.168.1.100");
    networkLayout->addRow("Host/IP:", m_ipAddressEdit);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(4532);
    networkLayout->addRow("Port:", m_portSpin);

    // Discovery button
    m_findRadiosButton = new QPushButton("Find Radios on Network", this);
    connect(m_findRadiosButton, &QPushButton::clicked, this, &RadioEditDialog::onFindNetworkRadios);
    networkLayout->addRow("", m_findRadiosButton);

    networkMainLayout->addLayout(networkLayout);

    // Icom credentials (shown only for Icom Direct)
    m_icomCredentialsWidget = new QWidget(this);
    QFormLayout* icomCredsLayout = new QFormLayout(m_icomCredentialsWidget);
    icomCredsLayout->setContentsMargins(0, 0, 0, 0);
    m_icomUsernameEdit = new QLineEdit(this);
    m_icomUsernameEdit->setPlaceholderText("Username (optional)");
    icomCredsLayout->addRow("Icom Username:", m_icomUsernameEdit);

    m_icomPasswordEdit = new QLineEdit(this);
    m_icomPasswordEdit->setPlaceholderText("Password (optional)");
    m_icomPasswordEdit->setEchoMode(QLineEdit::Password);
    icomCredsLayout->addRow("Icom Password:", m_icomPasswordEdit);

    m_icomClientNameEdit = new QLineEdit(this);
    m_icomClientNameEdit->setText("TR4QT");
    icomCredsLayout->addRow("Client Name:", m_icomClientNameEdit);

    networkMainLayout->addWidget(m_icomCredentialsWidget);
    m_icomCredentialsWidget->setVisible(false);

    connectionLayout->addWidget(m_networkGroup);
    m_networkGroup->setVisible(false);

    mainLayout->addWidget(connectionGroup);

    // ===== 3. Radio Model Selection =====
    m_modelSelectionWidget = new QWidget(this);
    QVBoxLayout* modelContainerLayout = new QVBoxLayout(m_modelSelectionWidget);
    modelContainerLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* modelGroup = new QGroupBox("Radio Model", this);
    QFormLayout* modelLayout = new QFormLayout(modelGroup);

    m_radioModelCombo = new QComboBox(this);
    m_radioModelCombo->setEditable(true);
    m_radioModelCombo->setInsertPolicy(QComboBox::NoInsert);
    m_radioModelCombo->blockSignals(true);
    modelLayout->addRow("Model:", m_radioModelCombo);

    m_customModelEdit = new QLineEdit(this);
    m_customModelEdit->setPlaceholderText("Enter hamlib model ID (e.g., 2046)");
    m_customModelEdit->setVisible(false);
    modelLayout->addRow("Custom Model ID:", m_customModelEdit);

    // Radio status filter checkboxes (for Hamlib only)
    m_statusFilterWidget = new QWidget(this);
    QHBoxLayout* filterLayout = new QHBoxLayout(m_statusFilterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    m_showStableRadiosCheck = new QCheckBox("Stable", this);
    m_showBetaRadiosCheck = new QCheckBox("Beta", this);
    m_showAlphaRadiosCheck = new QCheckBox("Alpha", this);
    m_showUntestedRadiosCheck = new QCheckBox("Untested", this);
    m_showStableRadiosCheck->setChecked(true);
    filterLayout->addWidget(new QLabel("Show:", this));
    filterLayout->addWidget(m_showStableRadiosCheck);
    filterLayout->addWidget(m_showBetaRadiosCheck);
    filterLayout->addWidget(m_showAlphaRadiosCheck);
    filterLayout->addWidget(m_showUntestedRadiosCheck);
    filterLayout->addStretch();
    modelLayout->addRow("", m_statusFilterWidget);

    modelContainerLayout->addWidget(modelGroup);
    mainLayout->addWidget(m_modelSelectionWidget);

    // ===== 4. Advanced Settings =====
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    QFormLayout* advancedLayout = new QFormLayout(advancedGroup);

    // CI-V Address (wrapped for show/hide)
    m_civWidget = new QWidget(this);
    QHBoxLayout* civLayout = new QHBoxLayout(m_civWidget);
    civLayout->setContentsMargins(0, 0, 0, 0);
    m_civAddressWidget = new CivAddressWidget(this);
    civLayout->addWidget(m_civAddressWidget);
    civLayout->addStretch();
    advancedLayout->addRow("CI-V Address:", m_civWidget);

    m_pollIntervalSpin = new QSpinBox(this);
    m_pollIntervalSpin->setRange(100, 5000);
    m_pollIntervalSpin->setValue(5000);
    m_pollIntervalSpin->setSingleStep(500);
    m_pollIntervalSpin->setSuffix(" ms");
    advancedLayout->addRow("Poll Interval:", m_pollIntervalSpin);

    mainLayout->addWidget(advancedGroup);

    // ===== 5. Test Connection =====
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

    // ===== 6. Dialog Buttons =====
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
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

    // ===== Connect Signals =====
    connect(m_serialRadio, &QRadioButton::toggled,
            this, &RadioEditDialog::onConnectionTypeChanged);
    connect(m_hamlibRadio, &QRadioButton::toggled,
            this, &RadioEditDialog::onNetworkInterfaceChanged);
    connect(m_k4DirectRadio, &QRadioButton::toggled,
            this, &RadioEditDialog::onNetworkInterfaceChanged);
    connect(m_icomDirectRadio, &QRadioButton::toggled,
            this, &RadioEditDialog::onNetworkInterfaceChanged);

    connect(m_showStableRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);
    connect(m_showBetaRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);
    connect(m_showAlphaRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);
    connect(m_showUntestedRadiosCheck, &QCheckBox::stateChanged,
            this, &RadioEditDialog::onRadioStatusFilterChanged);

    connect(m_radioModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RadioEditDialog::onRadioModelChanged);
    m_radioModelCombo->blockSignals(false);

    // ===== Setup Discovery Objects =====
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

    // ===== Initialize =====
    refreshSerialPorts();
    populateRadioList();
    updateVisibility();
}

// Helper to get current interface type from radio buttons
// Returns: 0 = Hamlib, 1 = K4 Direct, 2 = Icom Direct
int RadioEditDialog::getCurrentInterfaceType() const
{
    // For serial, always Hamlib
    if (m_serialRadio->isChecked()) {
        return 0;  // Hamlib
    }
    // For network, check interface radio buttons
    if (m_k4DirectRadio->isChecked()) {
        return 1;  // K4 Direct
    }
    if (m_icomDirectRadio->isChecked()) {
        return 2;  // Icom Direct
    }
    return 0;  // Default to Hamlib
}

void RadioEditDialog::populateRadioList()
{
    m_radioModelCombo->blockSignals(true);
    m_radioModelCombo->clear();

    int radioType = getCurrentInterfaceType();

    // For direct interfaces, show only radios with actual implementations
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
        m_radioModelCombo->blockSignals(false);
        return;
    }

    // For Hamlib, show the full Hamlib list with status filters
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

    // Determine connection type from port format
    bool isNetwork = config.port.contains(':');

    // Set connection type FIRST
    if (isNetwork) {
        m_networkRadio->setChecked(true);
    } else {
        m_serialRadio->setChecked(true);
    }

    // Set interface type (only relevant for network)
    if (isNetwork) {
        switch (config.radioType) {
            case 1:  // K4 Direct
                m_k4DirectRadio->setChecked(true);
                break;
            case 2:  // Icom Direct
                m_icomDirectRadio->setChecked(true);
                break;
            default:  // Hamlib or Auto
                m_hamlibRadio->setChecked(true);
                break;
        }
    } else {
        m_hamlibRadio->setChecked(true);  // Serial is always Hamlib
    }

    // Update visibility and populate model list based on selections
    updateVisibility();
    populateRadioList();

    // Now find and select radio model (in the correctly filtered list)
    int modelIndex = m_radioModelCombo->findData(config.hamlibModelId);
    if (modelIndex >= 0) {
        m_radioModelCombo->setCurrentIndex(modelIndex);
    } else if (config.hamlibModelId > 0) {
        // Model not in current list - check if it's a custom model for Hamlib
        int radioType = getCurrentInterfaceType();
        if (radioType == 0) {  // Hamlib
            m_radioModelCombo->setCurrentIndex(m_radioModelCombo->count() - 1);
            m_customModelEdit->setText(QString::number(config.hamlibModelId));
            m_customModelEdit->setVisible(true);
        }
    }

    // Connection settings
    if (isNetwork) {
        QStringList parts = config.port.split(':');
        if (parts.size() >= 2) {
            m_ipAddressEdit->setText(parts[0]);
            m_portSpin->setValue(parts[1].toInt());
        }
    } else {
        int portIndex = m_serialPortCombo->findText(config.port);
        if (portIndex >= 0) {
            m_serialPortCombo->setCurrentIndex(portIndex);
        } else {
            m_serialPortEdit->setText(config.port);
        }
        // Check if baud rate is a standard preset or custom
        QString baudRateStr = QString::number(config.baudRate);
        int index = m_baudRateCombo->findText(baudRateStr);
        if (index >= 0) {
            m_baudRateCombo->setCurrentIndex(index);
            m_customBaudRateEdit->clear();
            m_customBaudRateEdit->setEnabled(false);
        } else {
            // Non-standard baud rate - select "Custom" and fill in the value
            m_baudRateCombo->setCurrentText("Custom");
            m_customBaudRateEdit->setText(baudRateStr);
            m_customBaudRateEdit->setEnabled(true);
        }
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

    // Radio type (interface)
    config.radioType = getCurrentInterfaceType();

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
        // Use custom baud rate if "Custom" is selected, otherwise use combo value
        if (m_baudRateCombo->currentText() == "Custom") {
            config.baudRate = m_customBaudRateEdit->text().toInt();
        } else {
            config.baudRate = m_baudRateCombo->currentText().toInt();
        }
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

    // When switching to serial, reset interface to Hamlib
    if (isSerial) {
        m_hamlibRadio->setChecked(true);
    }

    updateVisibility();
    populateRadioList();
}

void RadioEditDialog::onNetworkInterfaceChanged()
{
    int interfaceType = getCurrentInterfaceType();

    // Update default port based on interface type
    if (interfaceType == 1) {  // K4 Direct
        m_portSpin->setValue(9200);
    } else if (interfaceType == 2) {  // Icom Direct
        m_portSpin->setValue(50001);
    } else {  // Hamlib
        m_portSpin->setValue(4532);  // Default rigctld
    }

    updateVisibility();
    populateRadioList();
}

void RadioEditDialog::updateVisibility()
{
    bool isSerial = m_serialRadio->isChecked();
    int interfaceType = getCurrentInterfaceType();

    // Model selection: Hide for K4 Direct (auto-selected), show for others
    bool showModelSelection = (interfaceType != 1);  // Not K4 Direct
    m_modelSelectionWidget->setVisible(showModelSelection);

    // Status filters: Only show for Hamlib
    bool showFilters = (interfaceType == 0);  // Hamlib only
    m_statusFilterWidget->setVisible(showFilters);

    // CI-V: Only for Hamlib with Icom radios
    bool showCiv = false;
    if (interfaceType == 0) {  // Hamlib
        int modelId = m_radioModelCombo->currentData().toInt();
        // Check if it's an Icom radio (model IDs 3xxx are typically Icom)
        QString modelName = m_radioModelCombo->currentText().toLower();
        showCiv = modelName.contains("icom") || (modelId >= 3000 && modelId < 4000);
    }
    m_civWidget->setVisible(showCiv);

    // Icom credentials: Only for Icom Direct
    bool showIcomCreds = (interfaceType == 2);  // Icom Direct
    m_icomCredentialsWidget->setVisible(showIcomCreds);

    // Discovery button: K4 for K4 Direct, Icom for Icom Direct, both for Hamlib
    if (interfaceType == 1) {
        m_findRadiosButton->setText("Find K4 on Network");
        m_findRadiosButton->setVisible(true);
    } else if (interfaceType == 2) {
        m_findRadiosButton->setText("Find Icom Radios");
        m_findRadiosButton->setVisible(true);
    } else {
        m_findRadiosButton->setText("Find Radios on Network");
        m_findRadiosButton->setVisible(!isSerial);  // Only for network
    }
}

void RadioEditDialog::onRadioModelChanged(int index)
{
    int modelId = m_radioModelCombo->itemData(index).toInt();
    m_customModelEdit->setVisible(modelId == -1);

    // Update CI-V visibility based on model
    updateVisibility();
}

void RadioEditDialog::onRadioStatusFilterChanged()
{
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
    int interfaceType = getCurrentInterfaceType();

    m_findRadiosButton->setEnabled(false);
    m_findRadiosButton->setText("Searching...");
    m_foundK4Radios.clear();
    m_foundIcomRadios.clear();

    if (interfaceType == 1) {  // K4 Direct
        m_k4Discovery->startDiscovery();
    } else if (interfaceType == 2) {  // Icom Direct
        m_icomDiscovery->startDiscovery();
    } else {  // Hamlib - try both
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
    LOG_INFO("RadioEditDialog", QString("K4 found: %1 S/N %2 at %3")
             .arg(radio.rigType).arg(radio.serialNumber).arg(radio.ipAddress));
}

void RadioEditDialog::onK4DiscoveryFinished(int count)
{
    m_findRadiosButton->setEnabled(true);
    updateVisibility();  // Restore correct button text

    if (count == 0 && m_foundIcomRadios.isEmpty()) {
        DialogHelper::information(this, "Discovery Complete",
            "No radios found on the network.\n\n"
            "Make sure your radio is:\n"
            "- Powered on\n"
            "- Connected to the same network\n"
            "- Has network control enabled");
    } else if (count > 0) {
        showK4SelectionDialog();
    }
}

void RadioEditDialog::onIcomRadioFound(const IcomRadioDiscoveryInfo& radio)
{
    m_foundIcomRadios.append(radio);
    LOG_INFO("RadioEditDialog", QString("Icom found: ID %1 at %2")
             .arg(radio.radioId).arg(radio.ipAddress));
}

void RadioEditDialog::onIcomDiscoveryFinished(int count)
{
    m_findRadiosButton->setEnabled(true);
    updateVisibility();  // Restore correct button text

    if (count == 0 && m_foundK4Radios.isEmpty()) {
        DialogHelper::information(this, "Discovery Complete",
            "No radios found on the network.\n\n"
            "Make sure your radio is:\n"
            "- Powered on\n"
            "- Connected to the same network\n"
            "- Has network control enabled");
    } else if (count > 0) {
        showIcomSelectionDialog();
    }
}

QSet<QString> RadioEditDialog::getConfiguredRadioIPs() const
{
    QSet<QString> configuredIPs;
    QList<RadioProfile> profiles = AppSettings::instance().loadRadioProfiles();

    for (const RadioProfile& profile : profiles) {
        // Skip our own profile if editing
        if (m_isEditMode && profile.name == m_originalProfile.name) {
            continue;
        }

        // Extract IP from network port (format: "ip:port")
        if (profile.config.port.contains(':')) {
            QString ip = profile.config.port.split(':').first();
            if (!ip.isEmpty()) {
                configuredIPs.insert(ip);
            }
        }
    }

    return configuredIPs;
}

void RadioEditDialog::showK4SelectionDialog()
{
    if (m_foundK4Radios.isEmpty()) {
        return;
    }

    // Single radio - auto-select
    if (m_foundK4Radios.size() == 1) {
        const K4RadioInfo& radio = m_foundK4Radios.first();
        m_ipAddressEdit->setText(radio.ipAddress);
        m_portSpin->setValue(9200);
        m_networkRadio->setChecked(true);
        m_k4DirectRadio->setChecked(true);
        onConnectionTypeChanged();

        DialogHelper::information(this, "K4 Found",
            QString("Found K4 S/N %1 at %2\nIP address has been filled in.")
                .arg(radio.serialNumber).arg(radio.ipAddress));
        return;
    }

    // Multiple radios - show selection dialog
    QSet<QString> configuredIPs = getConfiguredRadioIPs();

    QStringList items;
    QList<int> disabledIndices;

    for (int i = 0; i < m_foundK4Radios.size(); ++i) {
        const K4RadioInfo& radio = m_foundK4Radios[i];
        QString item = QString("K4 S/N %1 at %2").arg(radio.serialNumber).arg(radio.ipAddress);

        if (configuredIPs.contains(radio.ipAddress)) {
            item += " (already configured)";
            disabledIndices.append(i);
        }
        items.append(item);
    }

    // Use QInputDialog with combo box
    bool ok;
    QString selected = QInputDialog::getItem(this, "Select K4 Radio",
        QString("Found %1 K4 radios on the network.\nSelect the radio to use:")
            .arg(m_foundK4Radios.size()),
        items, 0, false, &ok);

    if (ok && !selected.isEmpty()) {
        int index = items.indexOf(selected);
        if (index >= 0 && index < m_foundK4Radios.size()) {
            if (disabledIndices.contains(index)) {
                DialogHelper::warning(this, "Radio Already Configured",
                    "This radio is already configured in another profile.\n"
                    "You can still use it, but you may want to edit the existing profile instead.");
            }

            const K4RadioInfo& radio = m_foundK4Radios[index];
            m_ipAddressEdit->setText(radio.ipAddress);
            m_portSpin->setValue(9200);
            m_networkRadio->setChecked(true);
            m_k4DirectRadio->setChecked(true);
            onConnectionTypeChanged();
        }
    }
}

void RadioEditDialog::showIcomSelectionDialog()
{
    if (m_foundIcomRadios.isEmpty()) {
        return;
    }

    // Single radio - auto-select
    if (m_foundIcomRadios.size() == 1) {
        const IcomRadioDiscoveryInfo& radio = m_foundIcomRadios.first();
        m_ipAddressEdit->setText(radio.ipAddress);
        m_portSpin->setValue(50001);
        m_networkRadio->setChecked(true);
        m_icomDirectRadio->setChecked(true);
        onConnectionTypeChanged();

        DialogHelper::information(this, "Icom Radio Found",
            QString("Found Icom radio (ID: %1) at %2\nIP address has been filled in.")
                .arg(radio.radioId, 8, 16, QChar('0')).arg(radio.ipAddress));
        return;
    }

    // Multiple radios - show selection dialog
    QSet<QString> configuredIPs = getConfiguredRadioIPs();

    QStringList items;
    QList<int> disabledIndices;

    for (int i = 0; i < m_foundIcomRadios.size(); ++i) {
        const IcomRadioDiscoveryInfo& radio = m_foundIcomRadios[i];
        QString item = QString("Icom ID %1 at %2")
            .arg(radio.radioId, 8, 16, QChar('0')).arg(radio.ipAddress);

        if (configuredIPs.contains(radio.ipAddress)) {
            item += " (already configured)";
            disabledIndices.append(i);
        }
        items.append(item);
    }

    // Use QInputDialog with combo box
    bool ok;
    QString selected = QInputDialog::getItem(this, "Select Icom Radio",
        QString("Found %1 Icom radios on the network.\nSelect the radio to use:")
            .arg(m_foundIcomRadios.size()),
        items, 0, false, &ok);

    if (ok && !selected.isEmpty()) {
        int index = items.indexOf(selected);
        if (index >= 0 && index < m_foundIcomRadios.size()) {
            if (disabledIndices.contains(index)) {
                DialogHelper::warning(this, "Radio Already Configured",
                    "This radio is already configured in another profile.\n"
                    "You can still use it, but you may want to edit the existing profile instead.");
            }

            const IcomRadioDiscoveryInfo& radio = m_foundIcomRadios[index];
            m_ipAddressEdit->setText(radio.ipAddress);
            m_portSpin->setValue(50001);
            m_networkRadio->setChecked(true);
            m_icomDirectRadio->setChecked(true);
            onConnectionTypeChanged();
        }
    }
}

} // namespace TR4QT
