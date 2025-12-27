#include "RadioControlWidget.h"
#include "../../radio/RadioController.h"
#include "../../utils/ThemeManager.h"
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

namespace TR4QT {

RadioControlWidget::RadioControlWidget(QWidget* parent)
    : QWidget(parent)
    , m_radioNumber(1)
{
    setupUI();
    clearDisplay();  // Start with cleared display (radio not connected)

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &RadioControlWidget::applyTheme);
    applyTheme();  // Apply initial theme
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
    QFont freqFont("Monospace", 16);
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
    connect(m_modeLabel, &QLabel::customContextMenuRequested, this, &RadioControlWidget::onModeContextMenu);
    mainLayout->addWidget(m_modeLabel);

    // WPM display (for CW mode)
    m_wpmLabel = new QLabel("-- WPM", this);
    QFont wpmFont;
    wpmFont.setPointSize(10);
    wpmFont.setBold(true);
    m_wpmLabel->setFont(wpmFont);
    m_wpmLabel->setAlignment(Qt::AlignCenter);
    m_wpmLabel->setStyleSheet("QLabel { background-color: #E0E0E0; padding: 3px; border-radius: 3px; }");
    m_wpmLabel->setEnabled(false);  // Grayed out by default
    mainLayout->addWidget(m_wpmLabel);

    // S-Meter display - custom widget with traditional radio meter styling
    m_sMeter = new SMeterWidget(this);
    mainLayout->addWidget(m_sMeter);

    // Control buttons
    QWidget* buttonWidget = new QWidget(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(5);
    buttonLayout->setContentsMargins(5, 5, 5, 5);

    // Style for checkable buttons - make checked state very obvious
    QString buttonStyle =
        "QPushButton {"
        "  background-color: #E0E0E0;"
        "  border: 1px solid #808080;"
        "  padding: 5px;"
        "  border-radius: 3px;"
        "}"
        "QPushButton:checked {"
        "  background-color: #4CAF50;"  // Green when checked/active
        "  color: white;"
        "  font-weight: bold;"
        "  border: 2px solid #2E7D32;"
        "}"
        "QPushButton:hover {"
        "  background-color: #D0D0D0;"
        "}"
        "QPushButton:checked:hover {"
        "  background-color: #45A049;"
        "}";

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
    QFont offsetFont("Monospace", 8);
    m_ritOffsetLabel->setFont(offsetFont);
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
    m_xitOffsetLabel->setFont(offsetFont);
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

    // Set minimum size (no maximum to allow user resizing)
    // Height needs to accommodate: VFO displays, mode, S-meter (60px), and buttons (50px)
    setMinimumSize(250, 300);
}

void RadioControlWidget::updateRadioState(const RadioState& state) {
    m_currentState = state;

    // Trace logging for RIT/XIT values
    LOG_TRACE("RadioControlWidget", QString("Radio state update - RIT: %1 (offset: %2 Hz), XIT: %3 (offset: %4 Hz), SPLIT: %5")
        .arg(state.isRitEnabled ? "ON" : "OFF").arg(state.ritOffsetA)
        .arg(state.isXitEnabled ? "ON" : "OFF").arg(state.xitOffsetA)
        .arg(state.isSplitEnabled ? "ON" : "OFF"));

    // Update VFO A frequency (show full precision from hamlib - typically 1 Hz or 10 Hz)
    double freqMhz = state.frequencyA / 1000000.0;
    m_vfoAFreqLabel->setText(QString::number(freqMhz, 'f', 5));

    // Update VFO B frequency
    if (state.frequencyB > 0) {
        double freqBMhz = state.frequencyB / 1000000.0;
        m_vfoBFreqLabel->setText(QString::number(freqBMhz, 'f', 5));
    } else {
        m_vfoBFreqLabel->setText("---");
    }

    // Update mode
    QString modeStr;
    switch (state.modeA) {
    case ModeType::CW:
        modeStr = "CW";
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
    default:
        modeStr = "---";
        break;
    }
    m_modeLabel->setText(modeStr);

    // Update WPM label (only enabled in CW mode)
    bool isCWMode = (state.modeA == ModeType::CW || state.modeA == ModeType::CWR);
    int wpm = AppSettings::instance().getMorseWPM();
    m_wpmLabel->setText(QString("%1 WPM").arg(wpm));
    m_wpmLabel->setEnabled(isCWMode);  // Gray out when not in CW mode

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

    // Update S-meter display
    updateSMeter(state.signalStrength);
}

void RadioControlWidget::clearDisplay() {
    // Clear all frequency and mode displays when radio disconnects
    m_vfoAFreqLabel->setText("----.-----");
    m_vfoBFreqLabel->setText("----.-----");
    m_modeLabel->setText("---");

    // Clear S-meter (set to minimum value)
    m_sMeter->setValue(-127);

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
    m_titleLabel->setText(QString("Radio %1").arg(number));
}

void RadioControlWidget::setRadioController(RadioController* controller) {
    m_radioController = controller;
}

void RadioControlWidget::onModeContextMenu(const QPoint& pos) {
    // Don't show menu if radio not connected or no radio controller set
    if (!m_radioController || !m_radioController->isConnected()) {
        LOG_DEBUG("RadioControlWidget", "Mode context menu suppressed - radio not connected");
        return;
    }

    // Get supported modes from radio
    QList<ModeType> supportedModes = m_radioController->getSupportedModes();

    if (supportedModes.isEmpty()) {
        LOG_DEBUG("RadioControlWidget", "No supported modes found for radio");
        return;
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

bool RadioControlWidget::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == m_ritWidget) {
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
    if (m_currentState.isRitEnabled) {
        // RIT is ON - green background, white text
        m_ritWidget->setStyleSheet(
            "QFrame {"
            "  background-color: #4CAF50;"  // Green
            "  border: 2px solid #2E7D32;"
            "  border-radius: 3px;"
            "}"
        );
        m_ritTitleLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
        m_ritOffsetLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
    } else {
        // RIT is OFF - gray background, dark text
        m_ritWidget->setStyleSheet(
            "QFrame {"
            "  background-color: #E0E0E0;"
            "  border: 1px solid #808080;"
            "  border-radius: 3px;"
            "}"
        );
        m_ritTitleLabel->setStyleSheet("QLabel { color: black; }");
        m_ritOffsetLabel->setStyleSheet("QLabel { color: black; }");
    }
}

void RadioControlWidget::updateXitWidgetStyle() {
    if (m_currentState.isXitEnabled) {
        // XIT is ON - green background, white text
        m_xitWidget->setStyleSheet(
            "QFrame {"
            "  background-color: #4CAF50;"  // Green
            "  border: 2px solid #2E7D32;"
            "  border-radius: 3px;"
            "}"
        );
        m_xitTitleLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
        m_xitOffsetLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
    } else {
        // XIT is OFF - gray background, dark text
        m_xitWidget->setStyleSheet(
            "QFrame {"
            "  background-color: #E0E0E0;"
            "  border: 1px solid #808080;"
            "  border-radius: 3px;"
            "}"
        );
        m_xitTitleLabel->setStyleSheet("QLabel { color: black; }");
        m_xitOffsetLabel->setStyleSheet("QLabel { color: black; }");
    }
}

void RadioControlWidget::updateSMeter(int signalStrength) {
    // Update custom S-meter widget with signal strength in dBm
    m_sMeter->setValue(signalStrength);

    LOG_TRACE("RadioControlWidget", QString("S-meter update: %1 dBm = %2")
        .arg(signalStrength).arg(SMeterWidget::dbmToSMeter(signalStrength)));
}

} // namespace TR4QT
