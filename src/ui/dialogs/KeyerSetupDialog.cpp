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

#include "KeyerSetupDialog.h"
#include "../../audio/SidetoneGenerator.h"
#include "../../keyers/KeyerController.h"
#include "../../keyers/IambicKeyer.h"
#include "../../keyers/KeyerConfig.h"
#include "../../radio/RadioController.h"
#include "../../utils/AppSettings.h"
#include "../../utils/DialogHelper.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPainter>

namespace TR4QT {

// Named constants for slider ranges
static constexpr int MIN_WPM = 5;
static constexpr int MAX_WPM = 50;
static constexpr int MIN_VOLUME = 0;
static constexpr int MAX_VOLUME = 100;
static constexpr int MIN_PITCH_HZ = 200;
static constexpr int MAX_PITCH_HZ = 1200;
static constexpr int PITCH_STEP_HZ = 50;
static constexpr int MIN_WEIGHTING = 10;
static constexpr int MAX_WEIGHTING = 90;
static constexpr int MAX_LEAD_IN = 250;
static constexpr int MAX_TAIL_TIME = 250;

// Colors for paddle indicators
static const QColor INDICATOR_ACTIVE_COLOR(0, 200, 0);   // Green
static const QColor INDICATOR_INACTIVE_COLOR(40, 40, 40); // Dark gray

KeyerSetupDialog::KeyerSetupDialog(KeyerController* keyerController,
                                   IambicKeyer* iambicKeyer,
                                   RadioController* radioController,
                                   QWidget* parent)
    : QDialog(parent)
    , m_keyerController(keyerController)
    , m_iambicKeyer(iambicKeyer)
    , m_radioController(radioController)
    , m_sidetone(new SidetoneGenerator(this))
{
    setWindowTitle("CW Keyer Setup");
    setMinimumWidth(380);

    setupUI();
    loadSettings();

    // Connect keyer controller signals
    connect(m_keyerController, &KeyerController::connectionStatusChanged,
            this, &KeyerSetupDialog::onConnectionStatusChanged);
    connect(m_keyerController, &KeyerController::paddleStateChanged,
            this, &KeyerSetupDialog::onPaddleStateChanged);
    connect(m_keyerController, &KeyerController::wpmChanged,
            this, [this](int wpm) {
                m_wpmSlider->blockSignals(true);
                m_wpmSlider->setValue(wpm);
                m_wpmSlider->blockSignals(false);
                m_wpmLabel->setText(QString("%1 WPM").arg(wpm));
            });

    // Connect iambic keyer signals to sidetone
    connect(m_iambicKeyer, &IambicKeyer::keyDown, m_sidetone, &SidetoneGenerator::keyDown);
    connect(m_iambicKeyer, &IambicKeyer::keyUp, m_sidetone, &SidetoneGenerator::keyUp);

    // Connect iambic keyer to radio (when not in sidetone-only mode)
    connect(m_iambicKeyer, &IambicKeyer::keyDown, this, [this]() {
        if (!m_sidetoneOnly && m_radioController) {
            m_radioController->sendKeyDown();
        }
    });
    connect(m_iambicKeyer, &IambicKeyer::keyUp, this, [this]() {
        if (!m_sidetoneOnly && m_radioController) {
            m_radioController->sendKeyUp();
        }
    });

    // Start sidetone audio engine
    m_sidetone->start();

    // Update UI to reflect current connection state
    updateConnectionUI(m_keyerController->isConnected());
    updateWinKeyerVisibility();

    LOG_INFO("KeyerSetupDialog", "Dialog opened");
}

KeyerSetupDialog::~KeyerSetupDialog() {
    saveSettings();

    // Disconnect iambic keyer from sidetone and radio before destruction
    disconnect(m_iambicKeyer, &IambicKeyer::keyDown, m_sidetone, &SidetoneGenerator::keyDown);
    disconnect(m_iambicKeyer, &IambicKeyer::keyUp, m_sidetone, &SidetoneGenerator::keyUp);

    m_sidetone->stop();
    LOG_INFO("KeyerSetupDialog", "Dialog closed");
}

void KeyerSetupDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- Key Status ---
    auto* keyStatusGroup = new QGroupBox("Key Status", this);
    auto* keyStatusLayout = new QHBoxLayout(keyStatusGroup);

    m_ditIndicator = new QWidget(this);
    m_ditIndicator->setFixedSize(INDICATOR_SIZE, INDICATOR_SIZE);
    m_ditIndicator->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: %2px;")
                                  .arg(INDICATOR_INACTIVE_COLOR.name())
                                  .arg(INDICATOR_SIZE / 2));

    m_dahIndicator = new QWidget(this);
    m_dahIndicator->setFixedSize(INDICATOR_SIZE, INDICATOR_SIZE);
    m_dahIndicator->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: %2px;")
                                  .arg(INDICATOR_INACTIVE_COLOR.name())
                                  .arg(INDICATOR_SIZE / 2));

    auto* ditLabel = new QLabel("Left Paddle", this);
    auto* dahLabel = new QLabel("Right Paddle", this);

    keyStatusLayout->addStretch();
    keyStatusLayout->addWidget(m_ditIndicator);
    keyStatusLayout->addWidget(ditLabel);
    keyStatusLayout->addSpacing(30);
    keyStatusLayout->addWidget(m_dahIndicator);
    keyStatusLayout->addWidget(dahLabel);
    keyStatusLayout->addStretch();
    mainLayout->addWidget(keyStatusGroup);

    // --- Sidetone Only ---
    m_sidetoneOnlyCheckbox = new QCheckBox("Sidetone Only (practice mode)", this);
    m_sidetoneOnlyCheckbox->setChecked(true);
    connect(m_sidetoneOnlyCheckbox, &QCheckBox::toggled,
            this, &KeyerSetupDialog::onSidetoneOnlyToggled);
    mainLayout->addWidget(m_sidetoneOnlyCheckbox);

    // --- CW Settings ---
    auto* cwGroup = new QGroupBox("CW Settings", this);
    auto* cwLayout = new QFormLayout(cwGroup);

    // Speed slider
    auto* wpmLayout = new QHBoxLayout();
    m_wpmSlider = new QSlider(Qt::Horizontal, this);
    m_wpmSlider->setRange(MIN_WPM, MAX_WPM);
    m_wpmLabel = new QLabel("25 WPM", this);
    m_wpmLabel->setFixedWidth(60);
    wpmLayout->addWidget(m_wpmSlider);
    wpmLayout->addWidget(m_wpmLabel);
    connect(m_wpmSlider, &QSlider::valueChanged, this, &KeyerSetupDialog::onWpmSliderChanged);
    cwLayout->addRow("Speed:", wpmLayout);

    // Volume slider
    auto* volLayout = new QHBoxLayout();
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(MIN_VOLUME, MAX_VOLUME);
    m_volumeLabel = new QLabel("50%", this);
    m_volumeLabel->setFixedWidth(60);
    volLayout->addWidget(m_volumeSlider);
    volLayout->addWidget(m_volumeLabel);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &KeyerSetupDialog::onVolumeSliderChanged);
    cwLayout->addRow("Volume:", volLayout);

    // Pitch slider
    auto* pitchLayout = new QHBoxLayout();
    m_pitchSlider = new QSlider(Qt::Horizontal, this);
    m_pitchSlider->setRange(MIN_PITCH_HZ, MAX_PITCH_HZ);
    m_pitchSlider->setSingleStep(PITCH_STEP_HZ);
    m_pitchLabel = new QLabel("600 Hz", this);
    m_pitchLabel->setFixedWidth(60);
    pitchLayout->addWidget(m_pitchSlider);
    pitchLayout->addWidget(m_pitchLabel);
    connect(m_pitchSlider, &QSlider::valueChanged, this, &KeyerSetupDialog::onPitchSliderChanged);
    cwLayout->addRow("Pitch:", pitchLayout);

    mainLayout->addWidget(cwGroup);

    // --- Keyer Mode ---
    auto* modeGroup = new QGroupBox("Keyer Mode", this);
    auto* modeLayout = new QVBoxLayout(modeGroup);

    auto* keyerTypeLayout = new QHBoxLayout();
    m_iambicRadio = new QRadioButton("Iambic", this);
    m_straightKeyRadio = new QRadioButton("Straight Key", this);
    m_iambicRadio->setChecked(true);
    keyerTypeLayout->addWidget(m_iambicRadio);
    keyerTypeLayout->addWidget(m_straightKeyRadio);
    keyerTypeLayout->addStretch();
    modeLayout->addLayout(keyerTypeLayout);

    auto* iambicModeLayout = new QHBoxLayout();
    m_modeARadio = new QRadioButton("Mode A", this);
    m_modeBRadio = new QRadioButton("Mode B", this);
    m_modeBRadio->setChecked(true);
    iambicModeLayout->addWidget(m_modeARadio);
    iambicModeLayout->addWidget(m_modeBRadio);
    iambicModeLayout->addStretch();
    modeLayout->addLayout(iambicModeLayout);

    m_swapPaddlesCheckbox = new QCheckBox("Swap Paddles (Left \u2194 Right)", this);
    modeLayout->addWidget(m_swapPaddlesCheckbox);

    connect(m_iambicRadio, &QRadioButton::toggled, this, &KeyerSetupDialog::onKeyerModeChanged);
    connect(m_modeARadio, &QRadioButton::toggled, this, &KeyerSetupDialog::onKeyerModeChanged);
    connect(m_swapPaddlesCheckbox, &QCheckBox::toggled, this, &KeyerSetupDialog::onSwapPaddlesToggled);

    mainLayout->addWidget(modeGroup);

    // --- WinKeyer Settings ---
    m_winKeyerGroup = new QGroupBox("WinKeyer Settings", this);
    auto* wkLayout = new QFormLayout(m_winKeyerGroup);

    // Weighting slider
    auto* weightLayout = new QHBoxLayout();
    m_weightingSlider = new QSlider(Qt::Horizontal, this);
    m_weightingSlider->setRange(MIN_WEIGHTING, MAX_WEIGHTING);
    m_weightingLabel = new QLabel("50", this);
    m_weightingLabel->setFixedWidth(40);
    weightLayout->addWidget(m_weightingSlider);
    weightLayout->addWidget(m_weightingLabel);
    connect(m_weightingSlider, &QSlider::valueChanged, this, &KeyerSetupDialog::onWeightingChanged);
    wkLayout->addRow("Weighting:", weightLayout);

    // Lead-in spin
    m_leadInSpin = new QSpinBox(this);
    m_leadInSpin->setRange(0, MAX_LEAD_IN);
    m_leadInSpin->setSuffix(" x10ms");
    connect(m_leadInSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KeyerSetupDialog::onLeadInChanged);
    wkLayout->addRow("Lead-in:", m_leadInSpin);

    // Tail time spin
    m_tailTimeSpin = new QSpinBox(this);
    m_tailTimeSpin->setRange(0, MAX_TAIL_TIME);
    m_tailTimeSpin->setSuffix(" x10ms");
    connect(m_tailTimeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KeyerSetupDialog::onTailTimeChanged);
    wkLayout->addRow("Tail time:", m_tailTimeSpin);

    mainLayout->addWidget(m_winKeyerGroup);

    // --- Buttons ---
    auto* buttonLayout = new QHBoxLayout();
    m_connectBtn = new QPushButton("Connect", this);
    m_disconnectBtn = new QPushButton("Disconnect", this);
    auto* closeBtn = new QPushButton("Close", this);

    connect(m_connectBtn, &QPushButton::clicked, this, &KeyerSetupDialog::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &KeyerSetupDialog::onDisconnectClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_connectBtn);
    buttonLayout->addWidget(m_disconnectBtn);
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);
}

void KeyerSetupDialog::loadSettings() {
    auto& settings = AppSettings::instance();

    m_wpmSlider->setValue(settings.getMorseWPM());
    m_wpmLabel->setText(QString("%1 WPM").arg(settings.getMorseWPM()));

    m_volumeSlider->setValue(settings.getSidetoneVolume());
    m_volumeLabel->setText(QString("%1%").arg(settings.getSidetoneVolume()));
    m_sidetone->setVolume(settings.getSidetoneVolume());

    m_pitchSlider->setValue(settings.getSidetonePitch());
    m_pitchLabel->setText(QString("%1 Hz").arg(settings.getSidetonePitch()));
    m_sidetone->setFrequency(settings.getSidetonePitch());

    // Keyer mode
    const int iambicMode = settings.getKeyerIambicMode();
    m_modeARadio->setChecked(iambicMode == 0);
    m_modeBRadio->setChecked(iambicMode == 1);

    m_swapPaddlesCheckbox->setChecked(settings.getKeyerPaddleSwap());

    // WinKeyer settings
    m_weightingSlider->setValue(settings.getWinKeyerWeighting());
    m_weightingLabel->setText(QString::number(settings.getWinKeyerWeighting()));
    m_leadInSpin->setValue(settings.getWinKeyerLeadIn());
    m_tailTimeSpin->setValue(settings.getWinKeyerTailTime());
}

void KeyerSetupDialog::saveSettings() {
    auto& settings = AppSettings::instance();

    settings.setMorseWPM(m_wpmSlider->value());
    settings.setSidetoneVolume(m_volumeSlider->value());
    settings.setSidetonePitch(m_pitchSlider->value());
    settings.setKeyerIambicMode(m_modeARadio->isChecked() ? 0 : 1);
    settings.setKeyerPaddleSwap(m_swapPaddlesCheckbox->isChecked());
    settings.setWinKeyerWeighting(m_weightingSlider->value());
    settings.setWinKeyerLeadIn(m_leadInSpin->value());
    settings.setWinKeyerTailTime(m_tailTimeSpin->value());
}

void KeyerSetupDialog::updateConnectionUI(bool connected) {
    m_connectBtn->setEnabled(!connected);
    m_disconnectBtn->setEnabled(connected);
}

void KeyerSetupDialog::updateWinKeyerVisibility() {
    bool isWinKeyer = m_keyerController->isConnected() &&
                      m_keyerController->connectedDeviceType() == KeyerDeviceType::WinKeyer;
    m_winKeyerGroup->setVisible(isWinKeyer);
}

void KeyerSetupDialog::onConnectClicked() {
    auto& settings = AppSettings::instance();

    KeyerConfig config;
    config.type = static_cast<KeyerDeviceType>(settings.getKeyerDeviceType());
    config.portName = settings.getKeyerPortName();
    config.defaultWpm = m_wpmSlider->value();
    config.paddleSwap = m_swapPaddlesCheckbox->isChecked();
    config.iambicMode = m_modeARadio->isChecked() ? IambicMode::IambicA : IambicMode::IambicB;
    config.ditNoteNumber = settings.getKeyerDitNote();
    config.dahNoteNumber = settings.getKeyerDahNote();
    config.weighting = m_weightingSlider->value();
    config.leadInTime = m_leadInSpin->value();
    config.tailTime = m_tailTimeSpin->value();

    if (config.portName.isEmpty() && config.type != KeyerDeviceType::HaliKeyMidi) {
        DialogHelper::warning(this, "No Port Configured",
                              "Please configure the keyer port in Preferences first.");
        return;
    }

    LOG_INFO("KeyerSetupDialog", QString("Connecting keyer: type=%1 port=%2")
             .arg(static_cast<int>(config.type)).arg(config.portName));
    m_keyerController->connectKeyer(config);
}

void KeyerSetupDialog::onDisconnectClicked() {
    m_keyerController->disconnectKeyer();
}

void KeyerSetupDialog::onConnectionStatusChanged(bool connected) {
    updateConnectionUI(connected);
    updateWinKeyerVisibility();

    if (connected) {
        LOG_INFO("KeyerSetupDialog", "Keyer connected");
    } else {
        // Reset paddle indicators on disconnect
        onPaddleStateChanged(false, false);
        LOG_INFO("KeyerSetupDialog", "Keyer disconnected");
    }
}

void KeyerSetupDialog::onPaddleStateChanged(bool dit, bool dah) {
    const QColor ditColor = dit ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR;
    const QColor dahColor = dah ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR;

    m_ditIndicator->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: %2px;")
                                  .arg(ditColor.name()).arg(INDICATOR_SIZE / 2));
    m_dahIndicator->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: %2px;")
                                  .arg(dahColor.name()).arg(INDICATOR_SIZE / 2));
}

void KeyerSetupDialog::onWpmSliderChanged(int value) {
    m_wpmLabel->setText(QString("%1 WPM").arg(value));
    m_iambicKeyer->setWpm(value);
    m_keyerController->setWpm(value);
}

void KeyerSetupDialog::onVolumeSliderChanged(int value) {
    m_volumeLabel->setText(QString("%1%").arg(value));
    m_sidetone->setVolume(value);
}

void KeyerSetupDialog::onPitchSliderChanged(int value) {
    m_pitchLabel->setText(QString("%1 Hz").arg(value));
    m_sidetone->setFrequency(value);
}

void KeyerSetupDialog::onSidetoneOnlyToggled(bool checked) {
    m_sidetoneOnly = checked;
    LOG_DEBUG("KeyerSetupDialog", QString("Sidetone-only mode: %1").arg(checked ? "ON" : "OFF"));
}

void KeyerSetupDialog::onKeyerModeChanged() {
    IambicMode mode = m_modeARadio->isChecked() ? IambicMode::IambicA : IambicMode::IambicB;
    m_iambicKeyer->setMode(mode);

    // Enable/disable iambic mode selection based on keyer type
    const bool isIambic = m_iambicRadio->isChecked();
    m_modeARadio->setEnabled(isIambic);
    m_modeBRadio->setEnabled(isIambic);
}

void KeyerSetupDialog::onSwapPaddlesToggled(bool checked) {
    Q_UNUSED(checked);
    // Paddle swap is read from settings by the keyer device on connect.
    // For live adjustment, would need to reconnect. Just save for next connect.
    LOG_DEBUG("KeyerSetupDialog", QString("Paddle swap: %1").arg(checked ? "ON" : "OFF"));
}

void KeyerSetupDialog::onWeightingChanged(int value) {
    m_weightingLabel->setText(QString::number(value));
    m_keyerController->setWeighting(value);
}

void KeyerSetupDialog::onLeadInChanged(int value) {
    m_keyerController->setLeadInTime(value);
}

void KeyerSetupDialog::onTailTimeChanged(int value) {
    m_keyerController->setTailTime(value);
}

} // namespace TR4QT
