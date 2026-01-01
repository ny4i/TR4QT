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
#include <QSerialPortInfo>
#include <QIcon>

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

    // Serial port dropdown + refresh button
    QWidget* portWidget = new QWidget(this);
    QVBoxLayout* portLayout = new QVBoxLayout(portWidget);
    portLayout->setContentsMargins(0, 0, 0, 0);
    portLayout->setSpacing(4);

    // Port selection row: dropdown + refresh button
    QWidget* portSelectWidget = new QWidget(this);
    QHBoxLayout* portSelectLayout = new QHBoxLayout(portSelectWidget);
    portSelectLayout->setContentsMargins(0, 0, 0, 0);
    portSelectLayout->setSpacing(4);

    m_serialPortCombo = new QComboBox(this);
    m_serialPortCombo->setEditable(false);
    m_serialPortCombo->setToolTip("Select a detected serial port from the list");
    portSelectLayout->addWidget(m_serialPortCombo, 1);

    m_refreshPortsButton = new QPushButton("Refresh", this);
    m_refreshPortsButton->setToolTip("Scan for available serial ports");
    m_refreshPortsButton->setMaximumWidth(80);
    connect(m_refreshPortsButton, &QPushButton::clicked, this, &RadioConfigDialog::onRefreshPorts);
    portSelectLayout->addWidget(m_refreshPortsButton);

    portLayout->addWidget(portSelectWidget);

    // Manual entry field (for ports not auto-detected)
    m_serialPortEdit = new QLineEdit(this);
#ifdef Q_OS_WIN
    m_serialPortEdit->setPlaceholderText("Or enter manually: COM1 or just 1");
    m_serialPortEdit->setToolTip("Manual entry: Enter COM port name (e.g., COM4) or just the port number (e.g., 4).\nPort numbers are automatically formatted as COMn.");
#else
    m_serialPortEdit->setPlaceholderText("Or enter manually: /dev/ttyUSB0");
    m_serialPortEdit->setToolTip("Manual entry: Enter the serial device path (e.g., /dev/ttyUSB0)");
#endif
    portLayout->addWidget(m_serialPortEdit);

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

    serialLayout->addRow("Port:", portWidget);
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

    // CI-V Address selection (for Icom radios)
    QWidget* civWidget = new QWidget(this);
    QVBoxLayout* civLayout = new QVBoxLayout(civWidget);
    civLayout->setContentsMargins(0, 0, 0, 0);

    m_civDefaultRadio = new QRadioButton("Use Default CI-V Address", this);
    m_civDefaultRadio->setToolTip("Use radio's default CI-V address (0x00)\n"
                                   "Radio will respond to broadcast commands");
    m_civDefaultRadio->setChecked(true);

    QWidget* civCustomWidget = new QWidget(this);
    QHBoxLayout* civCustomLayout = new QHBoxLayout(civCustomWidget);
    civCustomLayout->setContentsMargins(0, 0, 0, 0);

    m_civCustomRadio = new QRadioButton("Custom CI-V Address:", this);
    m_civCustomRadio->setToolTip("Specify a custom CI-V address for this radio");

    m_civAddressEdit = new QLineEdit(this);
    m_civAddressEdit->setPlaceholderText("Hex address (e.g., 94, 0x94, 0x98)");
    m_civAddressEdit->setMaxLength(4);
    m_civAddressEdit->setEnabled(false);  // Disabled by default
    m_civAddressEdit->setToolTip("Enter hex CI-V address\n"
                                   "Examples: 94, 0x94, 0x98\n"
                                   "Common: IC-7300=94, IC-7610=98, IC-9700=A2");

    civCustomLayout->addWidget(m_civCustomRadio);
    civCustomLayout->addWidget(m_civAddressEdit);
    civCustomLayout->addStretch();

    civLayout->addWidget(m_civDefaultRadio);
    civLayout->addWidget(civCustomWidget);

    // Connect radio button signals
    connect(m_civDefaultRadio, &QRadioButton::toggled, this, &RadioConfigDialog::onCivAddressModeChanged);
    connect(m_civCustomRadio, &QRadioButton::toggled, this, &RadioConfigDialog::onCivAddressModeChanged);

    m_pollIntervalSpin = new QSpinBox(this);
    m_pollIntervalSpin->setRange(100, 5000);
    m_pollIntervalSpin->setValue(500);
    m_pollIntervalSpin->setSingleStep(100);
    m_pollIntervalSpin->setSuffix(" ms");
    m_pollIntervalSpin->setToolTip("How often to poll the radio for status updates (100-5000ms)");

    advancedLayout->addRow("CI-V Address:", civWidget);
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

    // Initialize port refresh timer (auto-refresh every 5 seconds when dialog visible)
    m_portRefreshTimer = new QTimer(this);
    m_portRefreshTimer->setInterval(5000);  // 5 seconds
    connect(m_portRefreshTimer, &QTimer::timeout, this, &RadioConfigDialog::onPortAutoRefresh);

    // Initial population of serial ports
    populateSerialPorts();
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
        // Icom radio - auto-select custom CI-V with known address
        QString civAddress;
        if (modelId == 3078) {  // IC-7610
            civAddress = "98";
        } else if (modelId == 3092) {  // IC-7760
            civAddress = "7C";
        } else if (modelId == 3073) {  // IC-7300
            civAddress = "94";
        } else if (modelId == 3093) {  // IC-9700
            civAddress = "A2";
        }

        if (!civAddress.isEmpty()) {
            // Known Icom radio - set custom address
            m_civCustomRadio->setChecked(true);
            m_civAddressEdit->setText(civAddress);
            m_civAddressEdit->setEnabled(true);
        } else {
            // Unknown Icom radio - use default
            m_civDefaultRadio->setChecked(true);
            m_civAddressEdit->clear();
            m_civAddressEdit->setEnabled(false);
        }
    } else {
        // Non-Icom radio - use default (disabled)
        m_civDefaultRadio->setChecked(true);
        m_civAddressEdit->clear();
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
        QString portText;

        // Prefer combobox selection over manual entry
        if (m_serialPortCombo->isEnabled() && m_serialPortCombo->currentIndex() >= 0) {
            // Get port name from combobox data (not display text)
            portText = m_serialPortCombo->currentData().toString();

            if (!portText.isEmpty() && portText != "(No serial ports detected)") {
                LOG_DEBUG("RadioConfigDialog", QString("Using selected port from dropdown: %1").arg(portText));
            } else {
                // Fall back to manual entry if combobox selection is invalid
                portText = m_serialPortEdit->text().trimmed();
            }
        } else {
            // Combobox disabled or no ports - use manual entry
            portText = m_serialPortEdit->text().trimmed();
        }

        // Windows COM port auto-formatting (for manual entry)
        // If user enters just a number (e.g., "4"), format as "COM4"
        if (portText == m_serialPortEdit->text().trimmed()) {
#ifdef Q_OS_WIN
            bool isNumericOnly = false;
            int portNum = portText.toInt(&isNumericOnly);
            if (isNumericOnly && portNum > 0 && portNum <= 256) {
                // User entered just a number - format as COMn
                portText = QString("COM%1").arg(portNum);
                LOG_DEBUG("RadioConfigDialog", QString("Auto-formatted manual entry '%1' to '%2'")
                    .arg(m_serialPortEdit->text()).arg(portText));
            }
#endif
        }

        config.port = portText;
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

    // Parse CI-V address based on radio button selection
    if (m_civDefaultRadio->isChecked()) {
        config.civAddress = 0;  // Default/auto
    } else {
        // Custom address - parse from text field
        QString civText = m_civAddressEdit->text().trimmed();
        if (civText.isEmpty()) {
            config.civAddress = 0;  // Empty = default
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

        // Try to select port in combobox first
        int portIndex = m_serialPortCombo->findData(config.port);
        if (portIndex >= 0) {
            m_serialPortCombo->setCurrentIndex(portIndex);
            m_serialPortEdit->clear();  // Clear manual entry when using dropdown
            LOG_DEBUG("RadioConfigDialog", QString("Selected port '%1' from dropdown").arg(config.port));
        } else {
            // Port not in dropdown - use manual entry
            m_serialPortEdit->setText(config.port);
            LOG_DEBUG("RadioConfigDialog", QString("Port '%1' not in dropdown, using manual entry").arg(config.port));
        }

        m_baudRateCombo->setCurrentText(QString::number(config.baudRate));
        m_dataBitsCombo->setCurrentText(QString::number(config.dataBits));
        m_stopBitsCombo->setCurrentText(QString::number(config.stopBits));
        m_parityCombo->setCurrentIndex(config.parity);
    }

    // Set CI-V address mode based on value
    if (config.civAddress == 0) {
        m_civDefaultRadio->setChecked(true);
        m_civAddressEdit->clear();
        m_civAddressEdit->setEnabled(false);
    } else {
        m_civCustomRadio->setChecked(true);
        m_civAddressEdit->setText(QString::number(config.civAddress, 16).toUpper());
        m_civAddressEdit->setEnabled(true);
    }

    m_pollIntervalSpin->setValue(config.pollInterval);

    // Load auto-connect setting from AppSettings
    m_autoConnectCheck->setChecked(AppSettings::instance().getRadioAutoConnect());

    onConnectionTypeChanged();
}

bool RadioConfigDialog::getAutoConnect() const {
    return m_autoConnectCheck->isChecked();
}

void RadioConfigDialog::populateSerialPorts() {
    QString currentPort = m_serialPortCombo->currentText();
    m_serialPortCombo->clear();

    // Scan for available serial ports
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        m_serialPortCombo->addItem("(No serial ports detected)");
        m_serialPortCombo->setEnabled(false);
        LOG_DEBUG("RadioConfigDialog", "No serial ports detected");
    } else {
        m_serialPortCombo->setEnabled(true);

        for (const QSerialPortInfo& port : ports) {
            QString displayText;

            // Build display text with port name and description
            if (!port.description().isEmpty() && port.description() != port.portName()) {
                displayText = QString("%1 (%2)").arg(port.portName(), port.description());
            } else {
                displayText = port.portName();
            }

            // Store port name as user data
            m_serialPortCombo->addItem(displayText, port.portName());

            LOG_DEBUG("RadioConfigDialog", QString("Found serial port: %1 [%2] - %3")
                .arg(port.portName())
                .arg(port.systemLocation())
                .arg(port.description()));
        }

        // Restore previous selection if it still exists
        int index = m_serialPortCombo->findData(currentPort);
        if (index >= 0) {
            m_serialPortCombo->setCurrentIndex(index);
        }

        LOG_DEBUG("RadioConfigDialog", QString("Populated %1 serial ports").arg(ports.size()));
    }
}

void RadioConfigDialog::onRefreshPorts() {
    LOG_DEBUG("RadioConfigDialog", "Manual port refresh requested");
    populateSerialPorts();
}

void RadioConfigDialog::onPortAutoRefresh() {
    // Only refresh if dialog is visible and serial connection is selected
    if (!isVisible() || !m_serialRadio->isChecked()) {
        return;
    }

    LOG_DEBUG("RadioConfigDialog", "Auto-refreshing serial ports");
    populateSerialPorts();
}

void RadioConfigDialog::onCivAddressModeChanged() {
    // Enable/disable CI-V address text field based on radio button selection
    bool useCustom = m_civCustomRadio->isChecked();
    m_civAddressEdit->setEnabled(useCustom);

    if (useCustom) {
        // Custom selected - focus the text field for immediate entry
        m_civAddressEdit->setFocus();
        m_civAddressEdit->selectAll();
    } else {
        // Default selected - clear the text field
        m_civAddressEdit->clear();
    }
}

void RadioConfigDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    // Start auto-refresh timer when dialog is shown
    LOG_DEBUG("RadioConfigDialog", "Dialog shown - starting port auto-refresh timer");
    m_portRefreshTimer->start();

    // Do an immediate refresh
    populateSerialPorts();
}

void RadioConfigDialog::hideEvent(QHideEvent* event) {
    QDialog::hideEvent(event);

    // Stop auto-refresh timer when dialog is hidden
    LOG_DEBUG("RadioConfigDialog", "Dialog hidden - stopping port auto-refresh timer");
    m_portRefreshTimer->stop();
}

} // namespace TR4QT
