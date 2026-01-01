#include "RadioConfigDialog.h"
#include "../../utils/AppSettings.h"
#include "../../utils/DialogHelper.h"
#include "../../logging/LogMacros.h"
#include "../../radio/HamlibRadio.h"
#include "../../core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace TR4QT {

RadioConfigDialog::RadioConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_DEBUG("RadioConfigDialog", "*** RadioConfigDialog constructor called ***");
    setWindowTitle("Radio Configuration");
    setupUI();
    populateRadioModels();
    LOG_DEBUG("RadioConfigDialog", QString("*** RadioConfigDialog fully initialized with window title: %1").arg(windowTitle()));
}

void RadioConfigDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Radio model selection
    QGroupBox* modelGroup = new QGroupBox("Radio Model", this);
    QFormLayout* modelLayout = new QFormLayout(modelGroup);

    m_radioModelCombo = new QComboBox(this);
    m_radioModelCombo->setEditable(false);
    connect(m_radioModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RadioConfigDialog::onRadioModelChanged);

    m_customModelEdit = new QLineEdit(this);
    m_customModelEdit->setPlaceholderText("Enter hamlib model ID (e.g., 2046)");
    m_customModelEdit->setVisible(false);

    modelLayout->addRow("Model:", m_radioModelCombo);
    modelLayout->addRow("Custom Model ID:", m_customModelEdit);
    mainLayout->addWidget(modelGroup);

    // Connection type
    QGroupBox* connectionGroup = new QGroupBox("Connection Type", this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    m_serialRadio = new QRadioButton("Serial Port", this);
    m_networkRadio = new QRadioButton("Network (TCP)", this);
    m_serialRadio->setChecked(true);

    connect(m_serialRadio, &QRadioButton::toggled,
            this, &RadioConfigDialog::onConnectionTypeChanged);

    connectionLayout->addWidget(m_serialRadio);
    connectionLayout->addWidget(m_networkRadio);
    mainLayout->addWidget(connectionGroup);

    // Serial port settings
    m_serialGroup = new QGroupBox("Serial Port Settings", this);
    QFormLayout* serialLayout = new QFormLayout(m_serialGroup);

    m_serialPortEdit = new QLineEdit(this);
    m_serialPortEdit->setPlaceholderText("/dev/ttyUSB0 or COM1");

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

    serialLayout->addRow("Port:", m_serialPortEdit);
    serialLayout->addRow("Baud Rate:", m_baudRateCombo);
    serialLayout->addRow("Data Bits:", m_dataBitsCombo);
    serialLayout->addRow("Stop Bits:", m_stopBitsCombo);
    serialLayout->addRow("Parity:", m_parityCombo);
    mainLayout->addWidget(m_serialGroup);

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
    mainLayout->addWidget(m_networkGroup);
    m_networkGroup->setVisible(false);

    // Additional settings
    QGroupBox* advancedGroup = new QGroupBox("Advanced Settings", this);
    QFormLayout* advancedLayout = new QFormLayout(advancedGroup);

    m_civAddressEdit = new QLineEdit(this);
    m_civAddressEdit->setPlaceholderText("0 (auto) or hex address (e.g., 94, 0x94, 0x98)");
    m_civAddressEdit->setMaxLength(4);
    m_civAddressEdit->setToolTip("CI-V address for Icom radios\n"
                                   "Leave blank or enter 0 for auto-detection\n"
                                   "Or enter hex address: 94, 0x94, 0x98, etc.");

    m_pollIntervalSpin = new QSpinBox(this);
    m_pollIntervalSpin->setRange(100, 5000);
    m_pollIntervalSpin->setValue(500);
    m_pollIntervalSpin->setSingleStep(100);
    m_pollIntervalSpin->setSuffix(" ms");
    m_pollIntervalSpin->setToolTip("How often to poll the radio for status updates (100-5000ms)");

    advancedLayout->addRow("CI-V Address:", m_civAddressEdit);
    advancedLayout->addRow("Poll Interval:", m_pollIntervalSpin);

    m_autoConnectCheck = new QCheckBox("Auto-connect on startup", this);
    m_autoConnectCheck->setChecked(true);  // Default: enabled
    m_autoConnectCheck->setToolTip("Automatically connect to radio when application starts");
    advancedLayout->addRow("", m_autoConnectCheck);

    mainLayout->addWidget(advancedGroup);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);

    QPushButton* testButton = new QPushButton("Test Connection", this);
    connect(testButton, &QPushButton::clicked, this, &RadioConfigDialog::onTestConnection);
    buttonBox->addButton(testButton, QDialogButtonBox::ActionRole);

    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    resize(500, 600);
}

void RadioConfigDialog::populateRadioModels() {
    // Add common radio models
    m_radioModelCombo->addItem("Select a radio...", 0);
    m_radioModelCombo->addItem("Elecraft K3", 2029);
    m_radioModelCombo->addItem("Elecraft K4", 2047);
    m_radioModelCombo->addItem("Elecraft KX3", 2043);
    m_radioModelCombo->addItem("Icom IC-7300", 3073);
    m_radioModelCombo->addItem("Icom IC-7610", 3078);
    m_radioModelCombo->addItem("Icom IC-7760", 3092);
    m_radioModelCombo->addItem("Icom IC-9700", 3081);
    m_radioModelCombo->addItem("Kenwood TS-890S", 2044);
    m_radioModelCombo->addItem("Kenwood TS-990S", 2033);
    m_radioModelCombo->addItem("Yaesu FT-991", 1035);
    m_radioModelCombo->addItem("Yaesu FTDX10", 1045);
    m_radioModelCombo->addItem("Yaesu FTDX101D", 1043);
    m_radioModelCombo->addItem("Custom (enter model ID below)...", -1);
}

void RadioConfigDialog::onConnectionTypeChanged() {
    bool isSerial = m_serialRadio->isChecked();
    m_serialGroup->setVisible(isSerial);
    m_networkGroup->setVisible(!isSerial);
}

void RadioConfigDialog::onRadioModelChanged(int index) {
    int modelId = m_radioModelCombo->currentData().toInt();

    // Show custom model ID field if "Custom" is selected
    m_customModelEdit->setVisible(modelId == -1);

    // Auto-configure CI-V for Icom radios
    if (modelId >= 3000 && modelId < 4000) {
        // Icom radio - enable CI-V address
        m_civAddressEdit->setEnabled(true);
        // Common Icom CI-V addresses (can be customized)
        if (modelId == 3078) {  // IC-7610
            m_civAddressEdit->setText("98");
        } else if (modelId == 3092) {  // IC-7760
            m_civAddressEdit->setText("7C");
        } else if (modelId == 3073) {  // IC-7300
            m_civAddressEdit->setText("94");
        }
    } else {
        m_civAddressEdit->clear();  // Blank for non-Icom radios
        m_civAddressEdit->setEnabled(false);
    }
}

void RadioConfigDialog::onTestConnection() {
    RadioConfig config = getConfig();

    if (config.hamlibModelId == 0) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please select a radio model.");
        return;
    }

    if (config.port.isEmpty()) {
        DialogHelper::warning(this, "Invalid Configuration",
                           "Please specify a port or IP address.");
        return;
    }

    // Create temporary HamlibRadio instance for testing
    HamlibRadio testRadio;

    // Try to connect
    LOG_DEBUG("RadioConfigDialog", QString("Testing connection to model %1 on %2")
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

RadioConfig RadioConfigDialog::getConfig() const {
    RadioConfig config;

    // Get model ID
    int modelId = m_radioModelCombo->currentData().toInt();
    if (modelId == -1) {
        // Custom model ID
        config.hamlibModelId = m_customModelEdit->text().toInt();
    } else {
        config.hamlibModelId = modelId;
    }

    // Get connection info
    if (m_serialRadio->isChecked()) {
        config.port = m_serialPortEdit->text();
        config.baudRate = m_baudRateCombo->currentText().toInt();
        config.dataBits = m_dataBitsCombo->currentText().toInt();
        config.stopBits = m_stopBitsCombo->currentText().toInt();
        config.parity = m_parityCombo->currentIndex();  // 0=None, 1=Odd, 2=Even
    } else {
        // Network connection: format as "ip:port"
        config.port = QString("%1:%2")
                          .arg(m_ipAddressEdit->text())
                          .arg(m_portSpin->value());
        config.baudRate = 0;  // Not used for network
        config.dataBits = 8;  // Defaults for network (not used)
        config.stopBits = 1;
        config.parity = 0;
    }

    // Parse CI-V address from text (support blank, "0", hex with/without 0x prefix)
    QString civText = m_civAddressEdit->text().trimmed();
    if (civText.isEmpty() || civText == "0") {
        config.civAddress = 0;  // Default/auto
    } else {
        // Remove 0x prefix if present
        if (civText.startsWith("0x", Qt::CaseInsensitive)) {
            civText = civText.mid(2);
        }
        bool ok = false;
        config.civAddress = civText.toInt(&ok, 16);  // Parse as hex
        if (!ok) {
            config.civAddress = 0;  // Invalid hex, default to 0
        }
    }

    config.pollInterval = m_pollIntervalSpin->value();

    return config;
}

void RadioConfigDialog::setConfig(const RadioConfig& config) {
    m_config = config;

    // Set model
    int comboIndex = m_radioModelCombo->findData(config.hamlibModelId);
    if (comboIndex >= 0) {
        m_radioModelCombo->setCurrentIndex(comboIndex);
    } else {
        // Custom model
        m_radioModelCombo->setCurrentIndex(m_radioModelCombo->count() - 1);  // "Custom"
        m_customModelEdit->setText(QString::number(config.hamlibModelId));
        m_customModelEdit->setVisible(true);
    }

    // Determine if network or serial based on port format
    if (config.port.contains(':')) {
        // Network format "ip:port"
        m_networkRadio->setChecked(true);
        QStringList parts = config.port.split(':');
        if (parts.size() == 2) {
            m_ipAddressEdit->setText(parts[0]);
            m_portSpin->setValue(parts[1].toInt());
        }
    } else {
        // Serial port
        m_serialRadio->setChecked(true);
        m_serialPortEdit->setText(config.port);
        m_baudRateCombo->setCurrentText(QString::number(config.baudRate));
        m_dataBitsCombo->setCurrentText(QString::number(config.dataBits));
        m_stopBitsCombo->setCurrentText(QString::number(config.stopBits));
        m_parityCombo->setCurrentIndex(config.parity);
    }

    // Display CI-V address (0 = blank, otherwise hex without 0x prefix)
    if (config.civAddress == 0) {
        m_civAddressEdit->clear();
    } else {
        m_civAddressEdit->setText(QString::number(config.civAddress, 16).toUpper());
    }

    m_pollIntervalSpin->setValue(config.pollInterval);

    // Load auto-connect setting from AppSettings
    m_autoConnectCheck->setChecked(AppSettings::instance().getRadioAutoConnect());

    onConnectionTypeChanged();
}

bool RadioConfigDialog::getAutoConnect() const {
    return m_autoConnectCheck->isChecked();
}

} // namespace TR4QT
