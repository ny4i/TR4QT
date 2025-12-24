#include "PreferencesDialog.h"
#include "../../utils/AppSettings.h"
#include "../../utils/RadioEnumerator.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>

namespace TR4QT {

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setupUI();
    loadSettings();
    resize(600, 500);
}

void PreferencesDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createStationTab(), "Station");
    m_tabWidget->addTab(createRadioTab(), "Radio");
    m_tabWidget->addTab(createAppearanceTab(), "Appearance");
    m_tabWidget->addTab(createContestTab(), "Contest");
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
    m_radioModelCombo->setEditable(false);
    connect(m_radioModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onRadioModelChanged);

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

    return radioTab;
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
    layout->addStretch();

    return appearanceTab;
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

    // Appearance tab - will need to add getters to AppSettings
    // For now, use defaults

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

    // Appearance tab - will add setters to AppSettings

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

} // namespace TR4QT
