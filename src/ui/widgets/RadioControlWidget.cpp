#include "RadioControlWidget.h"
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

    // Create cyan background for frequency displays (like TR4W)
    QPalette vfoPalette;
    vfoPalette.setColor(QPalette::Window, QColor(0, 255, 255));  // Cyan
    vfoPalette.setColor(QPalette::WindowText, Qt::black);

    // VFO A
    QWidget* vfoAWidget = new QWidget(this);
    vfoAWidget->setAutoFillBackground(true);
    vfoAWidget->setPalette(vfoPalette);
    QHBoxLayout* vfoALayout = new QHBoxLayout(vfoAWidget);
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
    QWidget* vfoBWidget = new QWidget(this);
    vfoBWidget->setAutoFillBackground(true);
    vfoBWidget->setPalette(vfoPalette);
    QHBoxLayout* vfoBLayout = new QHBoxLayout(vfoBWidget);
    vfoBLayout->setContentsMargins(5, 5, 5, 5);

    m_vfoBLabel = new QLabel("VFO B", this);
    m_vfoBLabel->setFont(labelFont);

    m_vfoBFreqLabel = new QLabel("7138.77", this);
    m_vfoBFreqLabel->setFont(freqFont);
    m_vfoBFreqLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    vfoBLayout->addWidget(m_vfoBLabel);
    vfoBLayout->addWidget(m_vfoBFreqLabel, 1);

    vfoLayout->addWidget(vfoAWidget);
    vfoLayout->addWidget(vfoBWidget);

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
    buttonWidget->setAutoFillBackground(true);
    buttonWidget->setPalette(vfoPalette);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(5);
    buttonLayout->setContentsMargins(5, 5, 5, 5);

    m_ritButton = new QPushButton("RIT", this);
    m_ritButton->setCheckable(true);
    m_ritButton->setMaximumWidth(60);
    connect(m_ritButton, &QPushButton::clicked, this, &RadioControlWidget::onRitClicked);

    m_xitButton = new QPushButton("XIT", this);
    m_xitButton->setCheckable(true);
    m_xitButton->setMaximumWidth(60);
    connect(m_xitButton, &QPushButton::clicked, this, &RadioControlWidget::onXitClicked);

    m_splitButton = new QPushButton("SPLIT", this);
    m_splitButton->setCheckable(true);
    m_splitButton->setMaximumWidth(60);
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

    // Update VFO A frequency
    double freqMhz = state.frequencyA / 1000000.0;
    m_vfoAFreqLabel->setText(QString::number(freqMhz, 'f', 2));

    // Update VFO B frequency
    if (state.frequencyB > 0) {
        double freqBMhz = state.frequencyB / 1000000.0;
        m_vfoBFreqLabel->setText(QString::number(freqBMhz, 'f', 2));
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

    // Update button states
    // TODO: Get actual RIT/XIT/SPLIT status from radio if supported
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

} // namespace TR4QT
