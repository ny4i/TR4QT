#include "SendMorseDialog.h"
#include "MacroEditDialog.h"
#include "../../utils/AppSettings.h"
#include "../../logging/Logger.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QMouseEvent>
#include <QKeyEvent>

namespace TR4QT {

// Default macro labels and CW text
const char* SendMorseDialog::DEFAULT_MACRO_LABELS[MACRO_COUNT] = {
    "CQ", "TU", "QRZ?", "AGN",
    "599", "5NN", "QSL", "73",
    "NR?", "?", "DIT DIT", "TEST"
};

const char* SendMorseDialog::DEFAULT_MACRO_TEXTS[MACRO_COUNT] = {
    "CQ TEST", "TU", "QRZ?", "AGN?",
    "599", "5NN", "QSL TU", "73",
    "NR?", "?", "EE", "TEST"
};

SendMorseDialog::SendMorseDialog(RadioController* radio, QWidget* parent)
    : QDialog(parent)
    , m_radio(radio)
    , m_isSending(false)
{
    m_macroButtons.resize(MACRO_COUNT);
    setupUI();
    loadMacroSettings();
    setWindowTitle("Send Morse Code");
    resize(500, 300);
}

void SendMorseDialog::keyPressEvent(QKeyEvent* event) {
    // ESC key stops CW transmission if sending, otherwise closes dialog
    if (event->key() == Qt::Key_Escape) {
        if (m_isSending) {
            onAbortClicked();
            event->accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
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
    QGroupBox* macroGroup = new QGroupBox("Quick Macros (right-click to edit)", this);
    QGridLayout* macroLayout = new QGridLayout(macroGroup);

    // Create macro buttons in a 3x4 grid
    for (int i = 0; i < MACRO_COUNT; ++i) {
        int row = i / 4;
        int col = i % 4;
        QPushButton* button = createMacroButton(i, DEFAULT_MACRO_LABELS[i], DEFAULT_MACRO_TEXTS[i]);
        macroLayout->addWidget(button, row, col);
        m_macroButtons[i] = button;
    }

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

QPushButton* SendMorseDialog::createMacroButton(int index, const QString& label, const QString& morseText) {
    QPushButton* button = new QPushButton(label, this);
    button->setProperty("morseText", morseText);
    button->setProperty("macroIndex", index);

    // Install event filter for right-click handling
    button->installEventFilter(this);

    connect(button, &QPushButton::clicked, this, &SendMorseDialog::onMacroClicked);
    return button;
}

bool SendMorseDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            QPushButton* button = qobject_cast<QPushButton*>(obj);
            if (button && m_macroButtons.contains(button)) {
                int index = button->property("macroIndex").toInt();
                onMacroRightClicked(index);
                return true;  // Event handled
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void SendMorseDialog::onMacroRightClicked(int macroIndex) {
    if (macroIndex < 0 || macroIndex >= MACRO_COUNT) return;

    QPushButton* button = m_macroButtons[macroIndex];
    if (!button) return;

    QString currentLabel = button->text();
    QString currentText = button->property("morseText").toString();

    MacroEditDialog dialog(macroIndex, currentLabel, currentText, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString newLabel = dialog.getLabel();
        QString newText = dialog.getCWText();

        // Use defaults if empty
        if (newLabel.isEmpty()) {
            newLabel = DEFAULT_MACRO_LABELS[macroIndex];
        }
        if (newText.isEmpty()) {
            newText = DEFAULT_MACRO_TEXTS[macroIndex];
        }

        updateMacroButton(macroIndex, newLabel, newText);
        saveMacroSetting(macroIndex, newLabel, newText);

        LOG_INFO("SendMorseDialog", QString("Macro %1 updated: label='%2', text='%3'")
                 .arg(macroIndex + 1).arg(newLabel).arg(newText));
    }
}

void SendMorseDialog::updateMacroButton(int index, const QString& label, const QString& cwText) {
    if (index < 0 || index >= MACRO_COUNT) return;

    QPushButton* button = m_macroButtons[index];
    if (button) {
        button->setText(label);
        button->setProperty("morseText", cwText);
    }
}

void SendMorseDialog::loadMacroSettings() {
    AppSettings& settings = AppSettings::instance();

    for (int i = 0; i < MACRO_COUNT; ++i) {
        QString label = settings.getMacroLabel(i);
        QString text = settings.getMacroCWText(i);

        // Use defaults if not set
        if (label.isEmpty()) {
            label = DEFAULT_MACRO_LABELS[i];
        }
        if (text.isEmpty()) {
            text = DEFAULT_MACRO_TEXTS[i];
        }

        updateMacroButton(i, label, text);
    }
}

void SendMorseDialog::saveMacroSetting(int index, const QString& label, const QString& cwText) {
    AppSettings& settings = AppSettings::instance();
    settings.setMacroLabel(index, label);
    settings.setMacroCWText(index, cwText);
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

    m_statusLabel->setText(QString("Sending: %1 (ESC to stop)").arg(text));
    m_statusLabel->setStyleSheet("QLabel { color: blue; }");
    m_sendButton->setEnabled(false);
    m_abortButton->setEnabled(true);
    m_isSending = true;

    m_radio->sendCW(text);

    // Note: sendCW is async, status will be updated when complete
    // For now, show success immediately
    m_statusLabel->setText("Sent successfully");
    m_statusLabel->setStyleSheet("QLabel { color: green; }");

    m_sendButton->setEnabled(true);
    m_abortButton->setEnabled(false);
    m_isSending = false;
}

void SendMorseDialog::onAbortClicked() {
    if (!m_radio) return;

    m_radio->stopCW();
    m_statusLabel->setText("CW transmission stopped");
    m_statusLabel->setStyleSheet("QLabel { color: orange; }");
    LOG_INFO("SendMorseDialog", "CW transmission stopped by user");

    m_sendButton->setEnabled(true);
    m_abortButton->setEnabled(false);
    m_isSending = false;
}

} // namespace TR4QT
