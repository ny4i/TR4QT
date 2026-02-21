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

#include "RadioControlWidget.h"
#include "SMeterWidget.h"
#include "../../radio/RadioController.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/FontManager.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QFrame>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QInputDialog>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QDialog>
#include <QCloseEvent>
#include <QElapsedTimer>

namespace TR4QT {

RadioControlWidget::RadioControlWidget(const QString& settingsKey, QWidget* parent)
    : PersistentWindow<QWidget>(settingsKey, parent, "RadioControlWindow")
    , m_radioNumber(1)
{
    LOG_DEBUG("RadioControlWidget", QString("Constructor called for Radio %1").arg(m_radioNumber));
    setupUI();
    clearDisplay();  // Start with cleared display (radio not connected)

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &RadioControlWidget::applyTheme);
    applyTheme();  // Apply initial theme
}

RadioControlWidget::~RadioControlWidget()
{
    QElapsedTimer timer;
    timer.start();
    LOG_DEBUG("RadioControlWidget", QString("Destructor START for Radio %1").arg(m_radioNumber));

    // Destructor body - nothing special to clean up, but log timing
    // Qt handles child widget cleanup automatically

    LOG_DEBUG("RadioControlWidget", QString("Destructor END for Radio %1 (took %2ms)")
              .arg(m_radioNumber).arg(timer.elapsed()));
}

void RadioControlWidget::closeEvent(QCloseEvent* event)
{
    QElapsedTimer timer;
    timer.start();
    LOG_DEBUG("RadioControlWidget", QString("closeEvent START for Radio %1").arg(m_radioNumber));

    // Call base class
    PersistentWindow<QWidget>::closeEvent(event);

    LOG_DEBUG("RadioControlWidget", QString("closeEvent END for Radio %1 (took %2ms)")
              .arg(m_radioNumber).arg(timer.elapsed()));
}

void RadioControlWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Title
    m_titleLabel = new QLabel("Radio 1", this);
    QFont titleFont;
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    // VFO display area
    QWidget* vfoWidget = new QWidget(this);
    vfoWidget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* vfoLayout = new QVBoxLayout(vfoWidget);
    vfoLayout->setSpacing(2);
    vfoLayout->setContentsMargins(0, 0, 0, 0);

    // VFO A
    m_vfoAWidget = new QWidget(this);
    m_vfoAWidget->setAutoFillBackground(true);
    // Palette will be set in applyTheme()
    QHBoxLayout* vfoALayout = new QHBoxLayout(m_vfoAWidget);
    vfoALayout->setContentsMargins(5, 5, 5, 5);

    m_vfoALabel = new QLabel("VFO A", this);
    QFont labelFont;
    labelFont.setPointSize(9);
    labelFont.setBold(true);
    m_vfoALabel->setFont(labelFont);

    m_vfoAFreqLabel = new QLabel("7076.50", this);
    QFont freqFont = FontManager::instance().monospaceFont(16);
    freqFont.setBold(true);
    m_vfoAFreqLabel->setFont(freqFont);
    m_vfoAFreqLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    vfoALayout->addWidget(m_vfoALabel);
    vfoALayout->addWidget(m_vfoAFreqLabel, 1);

    // VFO B
    m_vfoBWidget = new QWidget(this);
    m_vfoBWidget->setAutoFillBackground(true);
    // Palette will be set in applyTheme()
    QHBoxLayout* vfoBLayout = new QHBoxLayout(m_vfoBWidget);
    vfoBLayout->setContentsMargins(5, 5, 5, 5);

    m_vfoBLabel = new QLabel("VFO B", this);
    m_vfoBLabel->setFont(labelFont);

    m_vfoBFreqLabel = new QLabel("7138.77", this);
    m_vfoBFreqLabel->setFont(freqFont);
    m_vfoBFreqLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    vfoBLayout->addWidget(m_vfoBLabel);
    vfoBLayout->addWidget(m_vfoBFreqLabel, 1);

    vfoLayout->addWidget(m_vfoAWidget);
    vfoLayout->addWidget(m_vfoBWidget);

    mainLayout->addWidget(vfoWidget);

    // Mode display
    m_modeLabel = new QLabel("CW", this);
    QFont modeFont;
    modeFont.setPointSize(12);
    modeFont.setBold(true);
    m_modeLabel->setFont(modeFont);
    m_modeLabel->setAlignment(Qt::AlignCenter);
    m_modeLabel->setStyleSheet("QLabel { background-color: lightgray; padding: 5px; border-radius: 3px; }");
    m_modeLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_modeLabel->setCursor(Qt::PointingHandCursor);  // Show clickable cursor
    m_modeLabel->installEventFilter(this);  // Handle left-clicks
    connect(m_modeLabel, &QLabel::customContextMenuRequested, this, &RadioControlWidget::onModeContextMenu);
    mainLayout->addWidget(m_modeLabel);

    // WPM display (for CW mode)
    m_wpmLabel = new QLabel("-- WPM", this);
    QFont wpmFont;
    wpmFont.setPointSize(10);
    wpmFont.setBold(true);
    m_wpmLabel->setFont(wpmFont);
    m_wpmLabel->setAlignment(Qt::AlignCenter);
    m_wpmLabel->setStyleSheet(QString("QLabel { background-color: %1; padding: 3px; border-radius: 3px; }")
        .arg(ThemeManager::instance().colorName(ColorRole::ButtonUncheckedBackground)));
    m_wpmLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_wpmLabel->setCursor(Qt::PointingHandCursor);  // Show clickable cursor
    m_wpmLabel->installEventFilter(this);  // Handle left-clicks
    connect(m_wpmLabel, &QLabel::customContextMenuRequested, this, &RadioControlWidget::onWpmContextMenu);
    m_wpmLabel->setEnabled(false);  // Grayed out by default
    mainLayout->addWidget(m_wpmLabel);

    // S-meter widget
    m_sMeterWidget = new SMeterWidget(this);
    mainLayout->addWidget(m_sMeterWidget);

    // Control buttons
    QWidget* buttonWidget = new QWidget(this);
    buttonWidget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(5);
    buttonLayout->setContentsMargins(5, 5, 5, 5);

    // Style for checkable buttons - make checked state very obvious
    ThemeManager& theme = ThemeManager::instance();
    QString buttonStyle = QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  padding: 5px;"
        "  border-radius: 3px;"
        "}"
        "QPushButton:checked {"
        "  background-color: %3;"
        "  color: white;"
        "  font-weight: bold;"
        "  border: 2px solid %4;"
        "}"
        "QPushButton:hover {"
        "  background-color: %5;"
        "}"
        "QPushButton:checked:hover {"
        "  background-color: %6;"
        "}")
        .arg(theme.colorName(ColorRole::ButtonUncheckedBackground))
        .arg(theme.colorName(ColorRole::ButtonUncheckedBorder))
        .arg(theme.colorName(ColorRole::ButtonCheckedBackground))
        .arg(theme.colorName(ColorRole::ButtonCheckedBorder))
        .arg(theme.colorName(ColorRole::ButtonHoverBackground))
        .arg(theme.colorName(ColorRole::ButtonCheckedHover));

    // RIT widget - clickable frame with two rows (label + offset)
    m_ritWidget = new QFrame(this);
    m_ritWidget->setFrameShape(QFrame::StyledPanel);
    m_ritWidget->setFrameShadow(QFrame::Raised);
    m_ritWidget->setMaximumWidth(70);
    m_ritWidget->setMinimumHeight(50);
    m_ritWidget->setCursor(Qt::PointingHandCursor);
    m_ritWidget->installEventFilter(this);

    QVBoxLayout* ritLayout = new QVBoxLayout(m_ritWidget);
    ritLayout->setSpacing(2);
    ritLayout->setContentsMargins(5, 5, 5, 5);

    m_ritTitleLabel = new QLabel("RIT", m_ritWidget);
    QFont ritFont;
    ritFont.setPointSize(9);
    ritFont.setBold(true);
    m_ritTitleLabel->setFont(ritFont);
    m_ritTitleLabel->setAlignment(Qt::AlignCenter);

    m_ritOffsetLabel = new QLabel("0 Hz", m_ritWidget);
    m_ritOffsetLabel->setFont(FontManager::instance().monospaceFont(8));
    m_ritOffsetLabel->setAlignment(Qt::AlignCenter);

    ritLayout->addWidget(m_ritTitleLabel);
    ritLayout->addWidget(m_ritOffsetLabel);

    // XIT widget - clickable frame with two rows (label + offset)
    m_xitWidget = new QFrame(this);
    m_xitWidget->setFrameShape(QFrame::StyledPanel);
    m_xitWidget->setFrameShadow(QFrame::Raised);
    m_xitWidget->setMaximumWidth(70);
    m_xitWidget->setMinimumHeight(50);
    m_xitWidget->setCursor(Qt::PointingHandCursor);
    m_xitWidget->installEventFilter(this);

    QVBoxLayout* xitLayout = new QVBoxLayout(m_xitWidget);
    xitLayout->setSpacing(2);
    xitLayout->setContentsMargins(5, 5, 5, 5);

    m_xitTitleLabel = new QLabel("XIT", m_xitWidget);
    m_xitTitleLabel->setFont(ritFont);
    m_xitTitleLabel->setAlignment(Qt::AlignCenter);

    m_xitOffsetLabel = new QLabel("0 Hz", m_xitWidget);
    m_xitOffsetLabel->setFont(FontManager::instance().monospaceFont(8));
    m_xitOffsetLabel->setAlignment(Qt::AlignCenter);

    xitLayout->addWidget(m_xitTitleLabel);
    xitLayout->addWidget(m_xitOffsetLabel);

    // SPLIT button (no offset display needed)
    m_splitButton = new QPushButton("SPLIT", this);
    m_splitButton->setCheckable(true);
    m_splitButton->setMaximumWidth(70);
    m_splitButton->setStyleSheet(buttonStyle);
    connect(m_splitButton, &QPushButton::clicked, this, &RadioControlWidget::onSplitClicked);

    buttonLayout->addWidget(m_ritWidget);
    buttonLayout->addWidget(m_xitWidget);
    buttonLayout->addWidget(m_splitButton);

    mainLayout->addWidget(buttonWidget);

    // Status message label (K4 ER command - shows amp status, errors, etc.)
    // Always visible to prevent layout shifts - just shows blank when no message
    // Uses same font as title label for visual consistency
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setFont(titleFont);  // Same as "Radio 1 TX" title
    m_statusLabel->setMinimumHeight(m_statusLabel->fontMetrics().height() + 4);  // Reserve space
    mainLayout->addWidget(m_statusLabel);

    // Timer to auto-clear status messages
    const int STATUS_CLEAR_TIMEOUT_MS = 5000;  // 5 seconds
    m_statusClearTimer = new QTimer(this);
    m_statusClearTimer->setSingleShot(true);
    connect(m_statusClearTimer, &QTimer::timeout, this, [this]() {
        m_statusLabel->clear();  // Just clear text, don't hide
    });

    // Set minimum size (no maximum to allow user resizing)
    // Height accommodates: title, VFO displays, mode, WPM, S-meter, buttons, and status
    // All dimensions derived from font metrics (no magic numbers)
    setMinimumSize(250, 320);
}

void RadioControlWidget::updateRadioState(const RadioState& state) {
    m_currentState = state;

    // Debug logging for mode updates
    double freqKHz = state.frequencyA / 1000.0;
    LOG_TRACE("RadioControlWidget", QString("updateRadioState - freq=%1 kHz, mode=%2 (%3), band=%4")
        .arg(freqKHz, 0, 'f', 1)
        .arg(static_cast<int>(state.modeA))
        .arg(modeToString(state.modeA))
        .arg(static_cast<int>(state.bandA)));

    // Trace logging for RIT/XIT values
    LOG_TRACE("RadioControlWidget", QString("Radio state update - RIT: %1 (offset: %2 Hz), XIT: %3 (offset: %4 Hz), SPLIT: %5")
        .arg(state.isRitEnabled ? "ON" : "OFF").arg(QString::number(state.ritOffsetA))
        .arg(state.isXitEnabled ? "ON" : "OFF").arg(QString::number(state.xitOffsetA))
        .arg(state.isSplitEnabled ? "ON" : "OFF"));

    // Update VFO A frequency (show full precision: 6 decimal places = 1 Hz precision)
    double freqMhz = state.frequencyA / 1000000.0;
    m_vfoAFreqLabel->setText(QString::number(freqMhz, 'f', 6));

    // Update VFO B frequency (6 decimal places = 1 Hz precision, matching radio display)
    if (state.frequencyB > 0) {
        double freqBMhz = state.frequencyB / 1000000.0;
        m_vfoBFreqLabel->setText(QString::number(freqBMhz, 'f', 6));
    } else {
        m_vfoBFreqLabel->setText("---");
    }

    // Update mode - handle all ModeType values
    QString modeStr;
    switch (state.modeA) {
    case ModeType::None:
        modeStr = "---";
        break;
    case ModeType::CW:
        modeStr = "CW";
        break;
    case ModeType::CWR:
        modeStr = "CWR";
        break;
    case ModeType::USB:
        modeStr = "USB";
        break;
    case ModeType::LSB:
        modeStr = "LSB";
        break;
    case ModeType::FM:
        modeStr = "FM";
        break;
    case ModeType::AM:
        modeStr = "AM";
        break;
    case ModeType::RTTY:
        modeStr = "RTTY";
        break;
    case ModeType::RTTYR:
        modeStr = "RTTY-R";
        break;
    case ModeType::PSK:
        modeStr = "PSK";
        break;
    case ModeType::PSKR:
        modeStr = "PSK-R";
        break;
    case ModeType::FT8:
        modeStr = "FT8";
        break;
    case ModeType::FT4:
        modeStr = "FT4";
        break;
    case ModeType::DATA:
        modeStr = "DATA";
        break;
    case ModeType::DATAR:
        modeStr = "DATA-R";
        break;
    }
    m_modeLabel->setText(modeStr);

    // Update WPM label (only enabled in CW mode)
    bool isCWMode = (state.modeA == ModeType::CW || state.modeA == ModeType::CWR);
    int wpm = state.cwSpeed;  // Display radio's CW speed, not app setting
    m_wpmLabel->setText(QString("%1 WPM").arg(wpm));
    m_wpmLabel->setEnabled(isCWMode);  // Gray out when not in CW mode

    // Update S-meter (auto-switches between RX and TX modes)
    m_sMeterWidget->updateFromRadioState(state);

    // Enable widgets when radio is connected
    m_ritWidget->setEnabled(true);
    m_xitWidget->setEnabled(true);
    m_splitButton->setEnabled(true);

    // Update RIT widget style based on enable status
    updateRitWidgetStyle();

    // Display RIT offset (convert Hz to more readable format)
    if (state.ritOffsetA == 0) {
        m_ritOffsetLabel->setText("0 Hz");
    } else if (abs(state.ritOffsetA) >= 1000) {
        // Display in kHz for large offsets
        double offsetKHz = state.ritOffsetA / 1000.0;
        m_ritOffsetLabel->setText(QString("%1 kHz").arg(offsetKHz, 0, 'f', 1));
    } else {
        // Display in Hz for small offsets
        m_ritOffsetLabel->setText(QString("%1 Hz").arg(state.ritOffsetA));
    }

    // Update XIT widget style based on enable status
    updateXitWidgetStyle();

    // Display XIT offset
    if (state.xitOffsetA == 0) {
        m_xitOffsetLabel->setText("0 Hz");
    } else if (abs(state.xitOffsetA) >= 1000) {
        // Display in kHz for large offsets
        double offsetKHz = state.xitOffsetA / 1000.0;
        m_xitOffsetLabel->setText(QString("%1 kHz").arg(offsetKHz, 0, 'f', 1));
    } else {
        // Display in Hz for small offsets
        m_xitOffsetLabel->setText(QString("%1 Hz").arg(state.xitOffsetA));
    }

    // SPLIT is active when split mode is enabled
    m_splitButton->setChecked(state.isSplitEnabled);
}

void RadioControlWidget::clearDisplay() {
    // Clear all frequency and mode displays when radio disconnects
    m_vfoAFreqLabel->setText("----.-----");
    m_vfoBFreqLabel->setText("----.-----");
    m_modeLabel->setText("---");

    // Clear S-meter
    m_sMeterWidget->clear();

    // Clear current state first (so style updates use cleared state)
    m_currentState = RadioState();

    // Update widget styles to OFF state
    updateRitWidgetStyle();
    updateXitWidgetStyle();
    m_splitButton->setChecked(false);

    // Disable widgets when radio not connected
    m_ritWidget->setEnabled(false);
    m_xitWidget->setEnabled(false);
    m_splitButton->setEnabled(false);
}

void RadioControlWidget::setRadioNumber(int number) {
    m_radioNumber = number;
    updateTitleLabel();
}

void RadioControlWidget::setActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;
    updateTitleLabel();

    // Update visual indicator for active radio (TR4W style)
    // Note: Avoid setStyleSheet on parent widget as it can affect custom-painted children (SMeterWidget)
    if (active) {
        // Active radio: cyan title bar, normal frequency text
        m_titleLabel->setStyleSheet("QLabel { background-color: #00FFFF; color: black; padding: 5px; font-weight: bold; border: 2px solid #00AAAA; }");
        // VFO frequencies: bright/normal text
        m_vfoAFreqLabel->setStyleSheet("");
        m_vfoBFreqLabel->setStyleSheet("");
    } else {
        // Inactive radio: normal title, grayed out frequencies
        m_titleLabel->setStyleSheet("QLabel { background-color: #CCCCCC; color: #666666; padding: 5px; }");
        // VFO frequencies: grayed out text for inactive radio
        m_vfoAFreqLabel->setStyleSheet("QLabel { color: #888888; }");
        m_vfoBFreqLabel->setStyleSheet("QLabel { color: #888888; }");
    }
}

void RadioControlWidget::updateTitleLabel() {
    QString title = QString("Radio %1").arg(m_radioNumber);
    if (m_isActive) {
        title += " ★ TX";  // Star and TX indicator for active radio
    }
    m_titleLabel->setText(title);
}

void RadioControlWidget::setMaxPower(int maxPowerWatts) {
    if (m_sMeterWidget) {
        m_sMeterWidget->setMaxPower(maxPowerWatts);
    }
}

void RadioControlWidget::setRadioController(RadioController* controller) {
    m_radioController = controller;
}

void RadioControlWidget::showStatusMessage(int code, const QString& message) {
    // Display format: "message (code)"
    QString displayText = QString("%1 (%2)").arg(message).arg(code);
    m_statusLabel->setText(displayText);

    // Restart the auto-clear timer
    const int STATUS_CLEAR_TIMEOUT_MS = 5000;  // 5 seconds
    m_statusClearTimer->start(STATUS_CLEAR_TIMEOUT_MS);

    LOG_DEBUG("RadioControlWidget", QString("Status message displayed: %1").arg(displayText));
}

void RadioControlWidget::onModeContextMenu(const QPoint& pos) {
    // Get supported modes from radio (if connected)
    QList<ModeType> supportedModes;
    bool radioConnected = m_radioController && m_radioController->isConnected();

    if (radioConnected) {
        supportedModes = m_radioController->getSupportedModes();
    }

    // If no supported modes (radio not connected or doesn't report modes),
    // use fallback list of common contest modes for manual override
    if (supportedModes.isEmpty()) {
        LOG_DEBUG("RadioControlWidget", "Using fallback mode list for manual override");
        supportedModes = {
            ModeType::CW,
            ModeType::CWR,
            ModeType::USB,
            ModeType::LSB,
            ModeType::RTTY,
            ModeType::RTTYR,
            ModeType::DATA,
            ModeType::DATAR,
            ModeType::FM,
            ModeType::AM
        };
    }

    // Create context menu
    QMenu menu(this);
    menu.setTitle("Select Mode");

    // Add menu items for each supported mode
    for (const ModeType& mode : supportedModes) {
        QString modeName;
        switch (mode) {
            case ModeType::CW:     modeName = "CW"; break;
            case ModeType::CWR:    modeName = "CW-R"; break;
            case ModeType::USB:    modeName = "USB"; break;
            case ModeType::LSB:    modeName = "LSB"; break;
            case ModeType::FM:     modeName = "FM"; break;
            case ModeType::AM:     modeName = "AM"; break;
            case ModeType::RTTY:   modeName = "RTTY"; break;
            case ModeType::RTTYR:  modeName = "RTTY-R"; break;
            case ModeType::DATA:   modeName = "DATA"; break;
            case ModeType::DATAR:  modeName = "DATA-R"; break;
            case ModeType::PSK:    modeName = "PSK"; break;
            case ModeType::PSKR:   modeName = "PSK-R"; break;
            case ModeType::FT8:    modeName = "FT8"; break;
            case ModeType::FT4:    modeName = "FT4"; break;
            default: continue;  // Skip unknown modes
        }

        // Check if this is the current mode
        bool isCurrent = (mode == m_currentState.modeA);

        QAction* action = menu.addAction(modeName);
        action->setCheckable(true);
        action->setChecked(isCurrent);
        action->setData(static_cast<int>(mode));  // Store mode as action data

        // Connect action to emit signal
        connect(action, &QAction::triggered, this, [this, mode]() {
            LOG_INFO("RadioControlWidget", QString("Mode change requested: %1").arg(static_cast<int>(mode)));
            emit modeChangeRequested(mode);
        });
    }

    // Show menu at cursor position (relative to mode label)
    menu.exec(m_modeLabel->mapToGlobal(pos));
}

void RadioControlWidget::onWpmContextMenu(const QPoint& pos) {
    // Don't show popup if radio not connected or no radio controller set
    if (!m_radioController || !m_radioController->isConnected()) {
        LOG_DEBUG("RadioControlWidget", "WPM popup suppressed - radio not connected");
        return;
    }

    // Don't show popup if not in CW mode
    bool isCWMode = (m_currentState.modeA == ModeType::CW || m_currentState.modeA == ModeType::CWR);
    if (!isCWMode) {
        LOG_DEBUG("RadioControlWidget", "WPM popup suppressed - not in CW mode");
        return;
    }

    // Get radio-specific WPM range from RadioController (polymorphic)
    int MIN_WPM = 5;    // Default fallback
    int MAX_WPM = 60;   // Default fallback
    if (m_radioController) {
        m_radioController->getCWSpeedRange(MIN_WPM, MAX_WPM);
        LOG_DEBUG("RadioControlWidget", QString("Radio CW speed range: %1-%2 WPM").arg(MIN_WPM).arg(MAX_WPM));
    }

    // Create compact popup dialog with slider
    QDialog popup(this);
    popup.setWindowTitle("CW Speed");
    popup.setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    QVBoxLayout* layout = new QVBoxLayout(&popup);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // Current speed display
    QLabel* speedLabel = new QLabel(QString("%1 WPM").arg(m_currentState.cwSpeed), &popup);
    QFont labelFont = speedLabel->font();
    labelFont.setPointSize(14);
    labelFont.setBold(true);
    speedLabel->setFont(labelFont);
    speedLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(speedLabel);

    // Horizontal slider
    QSlider* slider = new QSlider(Qt::Horizontal, &popup);
    slider->setMinimum(MIN_WPM);
    slider->setMaximum(MAX_WPM);
    slider->setValue(m_currentState.cwSpeed);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(10);
    slider->setMinimumWidth(250);
    layout->addWidget(slider);

    // Min/Max labels
    QHBoxLayout* rangeLayout = new QHBoxLayout();
    QLabel* minLabel = new QLabel(QString("%1").arg(MIN_WPM), &popup);
    QLabel* maxLabel = new QLabel(QString("%1").arg(MAX_WPM), &popup);
    QFont rangeFont = minLabel->font();
    rangeFont.setPointSize(9);
    minLabel->setFont(rangeFont);
    maxLabel->setFont(rangeFont);
    rangeLayout->addWidget(minLabel);
    rangeLayout->addStretch();
    rangeLayout->addWidget(maxLabel);
    layout->addLayout(rangeLayout);

    // Preset buttons
    QHBoxLayout* presetLayout = new QHBoxLayout();
    QList<int> presets = {15, 20, 25, 30, 35, 40};
    for (int preset : presets) {
        QPushButton* btn = new QPushButton(QString::number(preset), &popup);
        btn->setMaximumWidth(40);
        connect(btn, &QPushButton::clicked, [this, slider, preset, &popup]() {
            slider->setValue(preset);
            LOG_INFO("RadioControlWidget", QString("CW speed preset button clicked: %1 WPM").arg(preset));
            emit cwSpeedChangeRequested(preset);
            popup.close();
        });
        presetLayout->addWidget(btn);
    }
    layout->addLayout(presetLayout);

    // Update label as slider moves
    connect(slider, &QSlider::valueChanged, [speedLabel](int value) {
        speedLabel->setText(QString("%1 WPM").arg(value));
    });

    // Send to radio when slider released
    connect(slider, &QSlider::sliderReleased, [this, slider, &popup]() {
        int newSpeed = slider->value();
        LOG_INFO("RadioControlWidget", QString("CW speed change requested (slider): %1 WPM").arg(newSpeed));
        emit cwSpeedChangeRequested(newSpeed);
        popup.close();
    });

    // Position popup near WPM label
    QPoint globalPos = m_wpmLabel->mapToGlobal(QPoint(0, m_wpmLabel->height()));
    popup.move(globalPos);

    // Show popup (blocks until closed)
    popup.exec();
}

bool RadioControlWidget::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (obj == m_modeLabel && mouseEvent->button() == Qt::LeftButton) {
            // Show mode menu on left-click (same as right-click)
            onModeContextMenu(mouseEvent->pos());
            return true;
        } else if (obj == m_wpmLabel && mouseEvent->button() == Qt::LeftButton) {
            // Show WPM menu on left-click (same as right-click)
            onWpmContextMenu(mouseEvent->pos());
            return true;
        } else if (obj == m_ritWidget) {
            onRitClicked();
            return true;
        } else if (obj == m_xitWidget) {
            onXitClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void RadioControlWidget::onRitClicked() {
    // Toggle RIT - emit the opposite of current state
    emit ritToggled(!m_currentState.isRitEnabled);
}

void RadioControlWidget::onXitClicked() {
    // Toggle XIT - emit the opposite of current state
    emit xitToggled(!m_currentState.isXitEnabled);
}

void RadioControlWidget::onSplitClicked() {
    emit splitToggled(m_splitButton->isChecked());
}

void RadioControlWidget::applyTheme() {
    ThemeManager& theme = ThemeManager::instance();

    // Apply VFO colors (background and text)
    QPalette vfoPalette;
    vfoPalette.setColor(QPalette::Window, theme.color(ColorRole::VfoBackground));
    vfoPalette.setColor(QPalette::WindowText, theme.color(ColorRole::VfoText));

    m_vfoAWidget->setPalette(vfoPalette);
    m_vfoBWidget->setPalette(vfoPalette);
}

void RadioControlWidget::updateRitWidgetStyle() {
    ThemeManager& theme = ThemeManager::instance();
    if (m_currentState.isRitEnabled) {
        // RIT is ON - green background, white text
        m_ritWidget->setStyleSheet(QString(
            "QFrame {"
            "  background-color: %1;"
            "  border: 2px solid %2;"
            "  border-radius: 3px;"
            "}")
            .arg(theme.colorName(ColorRole::ButtonCheckedBackground))
            .arg(theme.colorName(ColorRole::ButtonCheckedBorder))
        );
        m_ritTitleLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
        m_ritOffsetLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
    } else {
        // RIT is OFF - gray background, dark text
        m_ritWidget->setStyleSheet(QString(
            "QFrame {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 3px;"
            "}")
            .arg(theme.colorName(ColorRole::ButtonUncheckedBackground))
            .arg(theme.colorName(ColorRole::ButtonUncheckedBorder))
        );
        m_ritTitleLabel->setStyleSheet("QLabel { color: black; }");
        m_ritOffsetLabel->setStyleSheet("QLabel { color: black; }");
    }
}

void RadioControlWidget::updateXitWidgetStyle() {
    ThemeManager& theme = ThemeManager::instance();
    if (m_currentState.isXitEnabled) {
        // XIT is ON - green background, white text
        m_xitWidget->setStyleSheet(QString(
            "QFrame {"
            "  background-color: %1;"
            "  border: 2px solid %2;"
            "  border-radius: 3px;"
            "}")
            .arg(theme.colorName(ColorRole::ButtonCheckedBackground))
            .arg(theme.colorName(ColorRole::ButtonCheckedBorder))
        );
        m_xitTitleLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
        m_xitOffsetLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
    } else {
        // XIT is OFF - gray background, dark text
        m_xitWidget->setStyleSheet(QString(
            "QFrame {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 3px;"
            "}")
            .arg(theme.colorName(ColorRole::ButtonUncheckedBackground))
            .arg(theme.colorName(ColorRole::ButtonUncheckedBorder))
        );
        m_xitTitleLabel->setStyleSheet("QLabel { color: black; }");
        m_xitOffsetLabel->setStyleSheet("QLabel { color: black; }");
    }
}

} // namespace TR4QT
