#include "RadioControlWidget.h"
#include "../../utils/ThemeManager.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QFrame>

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
    m_modeLabel->setStyleSheet("QLabel { background-color: lightgray; padding: 5px; }");
    mainLayout->addWidget(m_modeLabel);

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

    m_ritButton = new QPushButton("RIT", this);
    m_ritButton->setCheckable(true);
    m_ritButton->setMaximumWidth(60);
    m_ritButton->setStyleSheet(buttonStyle);
    connect(m_ritButton, &QPushButton::clicked, this, &RadioControlWidget::onRitClicked);

    m_xitButton = new QPushButton("XIT", this);
    m_xitButton->setCheckable(true);
    m_xitButton->setMaximumWidth(60);
    m_xitButton->setStyleSheet(buttonStyle);
    connect(m_xitButton, &QPushButton::clicked, this, &RadioControlWidget::onXitClicked);

    m_splitButton = new QPushButton("SPLIT", this);
    m_splitButton->setCheckable(true);
    m_splitButton->setMaximumWidth(60);
    m_splitButton->setStyleSheet(buttonStyle);
    connect(m_splitButton, &QPushButton::clicked, this, &RadioControlWidget::onSplitClicked);

    buttonLayout->addWidget(m_ritButton);
    buttonLayout->addWidget(m_xitButton);
    buttonLayout->addWidget(m_splitButton);

    mainLayout->addWidget(buttonWidget);

    // Set minimum size
    setMinimumSize(250, 200);
    setMaximumSize(400, 250);
}

void RadioControlWidget::updateRadioState(const RadioState& state) {
    m_currentState = state;

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

    // Enable buttons when radio is connected
    m_ritButton->setEnabled(true);
    m_xitButton->setEnabled(true);
    m_splitButton->setEnabled(true);

    // Update button states from actual radio status
    // RIT is active when there's a non-zero offset
    m_ritButton->setChecked(state.ritOffsetA != 0);

    // XIT is active when there's a non-zero offset
    m_xitButton->setChecked(state.xitOffsetA != 0);

    // SPLIT is active when split mode is enabled
    m_splitButton->setChecked(state.isSplitEnabled);
}

void RadioControlWidget::clearDisplay() {
    // Clear all frequency and mode displays when radio disconnects
    m_vfoAFreqLabel->setText("----.-----");
    m_vfoBFreqLabel->setText("----.-----");
    m_modeLabel->setText("---");

    // Uncheck all control buttons
    m_ritButton->setChecked(false);
    m_xitButton->setChecked(false);
    m_splitButton->setChecked(false);

    // Disable buttons when radio not connected
    m_ritButton->setEnabled(false);
    m_xitButton->setEnabled(false);
    m_splitButton->setEnabled(false);

    // Clear current state
    m_currentState = RadioState();
}

void RadioControlWidget::setRadioNumber(int number) {
    m_radioNumber = number;
    m_titleLabel->setText(QString("Radio %1").arg(number));
}

void RadioControlWidget::onRitClicked() {
    emit ritToggled(m_ritButton->isChecked());
}

void RadioControlWidget::onXitClicked() {
    emit xitToggled(m_xitButton->isChecked());
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

} // namespace TR4QT
