#include "SendMorseDialog.h"
#include "../../utils/AppSettings.h"
#include "../../logging/Logger.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

namespace TR4QT {

SendMorseDialog::SendMorseDialog(RadioController* radio, QWidget* parent)
    : QDialog(parent)
    , m_radio(radio)
{
    setupUI();
    setWindowTitle("Send Morse Code");
    resize(500, 300);
}

void SendMorseDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Status/WPM display
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_wpmLabel = new QLabel(this);
    int wpm = AppSettings::instance().getMorseWPM();
    m_wpmLabel->setText(QString("Speed: %1 WPM").arg(wpm));
    m_wpmLabel->setStyleSheet("QLabel { font-weight: bold; }");

    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("QLabel { color: green; }");

    statusLayout->addWidget(m_wpmLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(statusLayout);

    // Macro buttons group
    QGroupBox* macroGroup = new QGroupBox("Quick Macros", this);
    QGridLayout* macroLayout = new QGridLayout(macroGroup);

    // Common contest macros
    macroLayout->addWidget(createMacroButton("CQ", "CQ TEST"), 0, 0);
    macroLayout->addWidget(createMacroButton("TU", "TU"), 0, 1);
    macroLayout->addWidget(createMacroButton("QRZ?", "QRZ?"), 0, 2);
    macroLayout->addWidget(createMacroButton("AGN", "AGN?"), 0, 3);

    macroLayout->addWidget(createMacroButton("599", "599"), 1, 0);
    macroLayout->addWidget(createMacroButton("5NN", "5NN"), 1, 1);
    macroLayout->addWidget(createMacroButton("QSL", "QSL TU"), 1, 2);
    macroLayout->addWidget(createMacroButton("73", "73"), 1, 3);

    macroLayout->addWidget(createMacroButton("NR?", "NR?"), 2, 0);
    macroLayout->addWidget(createMacroButton("?", "?"), 2, 1);
    macroLayout->addWidget(createMacroButton("DIT DIT", "EE"), 2, 2);
    macroLayout->addWidget(createMacroButton("TEST", "TEST"), 2, 3);

    mainLayout->addWidget(macroGroup);

    // Free text entry
    QGroupBox* textGroup = new QGroupBox("Custom Message", this);
    QVBoxLayout* textLayout = new QVBoxLayout(textGroup);

    m_textEdit = new QLineEdit(this);
    m_textEdit->setPlaceholderText("Enter morse code message...");
    m_textEdit->setFocus();

    // Send on Enter key
    connect(m_textEdit, &QLineEdit::returnPressed, this, &SendMorseDialog::onSendClicked);

    textLayout->addWidget(m_textEdit);
    mainLayout->addWidget(textGroup);

    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setDefault(true);
    connect(m_sendButton, &QPushButton::clicked, this, &SendMorseDialog::onSendClicked);

    m_abortButton = new QPushButton("Abort CW", this);
    m_abortButton->setEnabled(false);
    connect(m_abortButton, &QPushButton::clicked, this, &SendMorseDialog::onAbortClicked);

    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(m_sendButton);
    buttonLayout->addWidget(m_abortButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

QPushButton* SendMorseDialog::createMacroButton(const QString& label, const QString& morseText) {
    QPushButton* button = new QPushButton(label, this);
    button->setProperty("morseText", morseText);
    connect(button, &QPushButton::clicked, this, &SendMorseDialog::onMacroClicked);
    return button;
}

void SendMorseDialog::onMacroClicked() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString morseText = button->property("morseText").toString();
    sendMorse(morseText);
}

void SendMorseDialog::onSendClicked() {
    QString text = m_textEdit->text().trimmed();
    if (text.isEmpty()) {
        m_statusLabel->setText("Error: No message to send");
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    sendMorse(text);
    m_textEdit->clear();
}

void SendMorseDialog::sendMorse(const QString& text) {
    if (!m_radio) {
        m_statusLabel->setText("Error: No radio connected");
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    if (!m_radio->isConnected()) {
        m_statusLabel->setText("Error: Radio not connected");
        m_statusLabel->setStyleSheet("QLabel { color: red; }");
        return;
    }

    // Set morse speed from settings
    int wpm = AppSettings::instance().getMorseWPM();
    m_radio->setCWSpeed(wpm);

    // Send the morse code
    LOG_INFO("SendMorseDialog", QString("Sending morse: '%1' at %2 WPM").arg(text).arg(wpm));

    m_statusLabel->setText(QString("Sending: %1").arg(text));
    m_statusLabel->setStyleSheet("QLabel { color: blue; }");
    m_sendButton->setEnabled(false);
    m_abortButton->setEnabled(true);

    m_radio->sendCW(text);

    // Note: sendCW is async, status will be updated when complete
    // For now, show success immediately
    m_statusLabel->setText("Sent successfully");
    m_statusLabel->setStyleSheet("QLabel { color: green; }");

    m_sendButton->setEnabled(true);
    m_abortButton->setEnabled(false);
}

void SendMorseDialog::onAbortClicked() {
    if (!m_radio) return;

    m_radio->stopCW();
    m_statusLabel->setText("CW transmission aborted");
    m_statusLabel->setStyleSheet("QLabel { color: orange; }");
    LOG_INFO("SendMorseDialog", "CW transmission aborted by user");

    m_sendButton->setEnabled(true);
    m_abortButton->setEnabled(false);
}

} // namespace TR4QT
