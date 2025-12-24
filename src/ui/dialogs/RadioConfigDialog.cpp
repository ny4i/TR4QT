#include "RadioConfigDialog.h"
#include "../../utils/AppSettings.h"
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
    qDebug() << "*** RadioConfigDialog constructor called ***";
    setWindowTitle("Radio Configuration");
    setupUI();
    populateRadioModels();
    qDebug() << "*** RadioConfigDialog fully initialized with window title:" << windowTitle();
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

    serialLayout->addRow("Port:", m_serialPortEdit);
    serialLayout->addRow("Baud Rate:", m_baudRateCombo);
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
    m_pollIntervalSpin->setToolTip("How often to poll the radio for status updates (100-5000ms)");

    advancedLayout->addRow("CI-V Address:", m_civAddressSpin);
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
        m_civAddressSpin->setEnabled(true);
        // Common Icom CI-V addresses (can be customized)
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

void RadioConfigDialog::onTestConnection() {
    RadioConfig config = getConfig();

    if (config.hamlibModelId == 0) {
        QMessageBox::warning(this, "Invalid Configuration",
                           "Please select a radio model.");
        return;
    }

    if (config.port.isEmpty()) {
        QMessageBox::warning(this, "Invalid Configuration",
                           "Please specify a port or IP address.");
        return;
    }

    QMessageBox::information(this, "Test Connection",
                           QString("Would test connection to:\n"
                                   "Model ID: %1\n"
                                   "Port: %2\n"
                                   "Baud: %3\n"
                                   "CI-V: 0x%4\n\n"
                                   "Actual testing requires MainWindow integration.")
                               .arg(config.hamlibModelId)
                               .arg(config.port)
                               .arg(config.baudRate)
                               .arg(config.civAddress, 2, 16, QChar('0')));
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
    } else {
        // Network connection: format as "ip:port"
        config.port = QString("%1:%2")
                          .arg(m_ipAddressEdit->text())
                          .arg(m_portSpin->value());
        config.baudRate = 0;  // Not used for network
    }

    config.civAddress = m_civAddressSpin->value();
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
    }

    m_civAddressSpin->setValue(config.civAddress);
    m_pollIntervalSpin->setValue(config.pollInterval);

    // Load auto-connect setting from AppSettings
    m_autoConnectCheck->setChecked(AppSettings::instance().getRadioAutoConnect());

    onConnectionTypeChanged();
}

bool RadioConfigDialog::getAutoConnect() const {
    return m_autoConnectCheck->isChecked();
}

} // namespace TR4QT
