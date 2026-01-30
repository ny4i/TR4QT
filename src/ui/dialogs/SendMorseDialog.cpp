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

#include "SendMorseDialog.h"
#include "MacroEditDialog.h"
#include "../../utils/AppSettings.h"
#include "../../logging/Logger.h"
#include "../../logging/LogMacros.h"
#include "../../cw/CWSenderFactory.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
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
    , m_cwSender(nullptr)
    , m_isSending(false)
{
    m_macroButtons.resize(MACRO_COUNT);

    // Create CW sender using factory
    m_cwSender = CWSenderFactory::create(CWSenderFactory::Backend::Hamlib, radio, this);
    if (m_cwSender) {
        connect(m_cwSender, &CWSender::stateChanged, this, &SendMorseDialog::onCWSenderStateChanged);
        connect(m_cwSender, &CWSender::transmissionComplete, this, &SendMorseDialog::onTransmissionComplete);
        connect(m_cwSender, &CWSender::transmissionStopped, this, &SendMorseDialog::onTransmissionComplete);
        connect(m_cwSender, &CWSender::error, this, &SendMorseDialog::onCWSenderError);
        connect(m_cwSender, &CWSender::wpmChanged, this, [this](int wpm) {
            m_wpmLabel->setText(QString("Speed: %1 WPM").arg(wpm));
        });
    }

    setupUI();
    loadMacroSettings();
    setWindowTitle("Send Morse Code");
    resize(UIDefaults::SEND_MORSE_WIDTH, UIDefaults::SEND_MORSE_HEIGHT);

    // Install event filter on self to catch key events
    installEventFilter(this);
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

    // PgUp: Increase CW speed
    if (event->key() == Qt::Key_PageUp) {
        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = AppSettings::instance().getMorseWPM();
        int newWpm = qMin(currentWpm + increment, 60);  // Max 60 WPM

        AppSettings::instance().setMorseWPM(newWpm);
        m_wpmLabel->setText(QString("Speed: %1 WPM").arg(newWpm));

        // Update CW sender (will also update radio)
        if (m_cwSender) {
            m_cwSender->setWpm(newWpm);
        } else if (m_radio && m_radio->isConnected()) {
            m_radio->setCWSpeed(newWpm);
        }

        m_statusLabel->setText(QString("Speed changed to %1 WPM").arg(newWpm));
        m_statusLabel->setStyleSheet("QLabel { color: green; }");
        event->accept();
        return;
    }

    // PgDn: Decrease CW speed
    if (event->key() == Qt::Key_PageDown) {
        int increment = AppSettings::instance().getMorseWPMIncrement();
        int currentWpm = AppSettings::instance().getMorseWPM();
        int newWpm = qMax(currentWpm - increment, 5);  // Min 5 WPM

        AppSettings::instance().setMorseWPM(newWpm);
        m_wpmLabel->setText(QString("Speed: %1 WPM").arg(newWpm));

        // Update CW sender (will also update radio)
        if (m_cwSender) {
            m_cwSender->setWpm(newWpm);
        } else if (m_radio && m_radio->isConnected()) {
            m_radio->setCWSpeed(newWpm);
        }

        m_statusLabel->setText(QString("Speed changed to %1 WPM").arg(newWpm));
        m_statusLabel->setStyleSheet("QLabel { color: green; }");
        event->accept();
        return;
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
    // Handle key events for PgUp/PgDn WPM control
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // PgUp: Increase CW speed
        if (keyEvent->key() == Qt::Key_PageUp) {
            int increment = AppSettings::instance().getMorseWPMIncrement();
            int currentWpm = AppSettings::instance().getMorseWPM();
            int newWpm = qMin(currentWpm + increment, 60);

            AppSettings::instance().setMorseWPM(newWpm);
            m_wpmLabel->setText(QString("Speed: %1 WPM").arg(newWpm));

            if (m_cwSender) {
                m_cwSender->setWpm(newWpm);
            } else if (m_radio && m_radio->isConnected()) {
                m_radio->setCWSpeed(newWpm);
            }

            m_statusLabel->setText(QString("Speed: %1 WPM").arg(newWpm));
            m_statusLabel->setStyleSheet("QLabel { color: green; }");
            return true;  // Event handled
        }

        // PgDn: Decrease CW speed
        if (keyEvent->key() == Qt::Key_PageDown) {
            int increment = AppSettings::instance().getMorseWPMIncrement();
            int currentWpm = AppSettings::instance().getMorseWPM();
            int newWpm = qMax(currentWpm - increment, 5);

            AppSettings::instance().setMorseWPM(newWpm);
            m_wpmLabel->setText(QString("Speed: %1 WPM").arg(newWpm));

            if (m_cwSender) {
                m_cwSender->setWpm(newWpm);
            } else if (m_radio && m_radio->isConnected()) {
                m_radio->setCWSpeed(newWpm);
            }

            m_statusLabel->setText(QString("Speed: %1 WPM").arg(newWpm));
            m_statusLabel->setStyleSheet("QLabel { color: green; }");
            return true;  // Event handled
        }
    }

    // Handle right-click on macro buttons
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
    // Use CWSender if available
    if (m_cwSender) {
        if (!m_cwSender->isAvailable()) {
            m_statusLabel->setText("Error: Radio not connected");
            m_statusLabel->setStyleSheet("QLabel { color: red; }");
            return;
        }

        // Set WPM from settings
        int wpm = AppSettings::instance().getMorseWPM();
        m_cwSender->setWpm(wpm);

        // UI updates will be handled by onCWSenderStateChanged
        m_cwSender->send(text);
        return;
    }

    // Fallback to direct radio control (should not normally reach here)
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

    int wpm = AppSettings::instance().getMorseWPM();
    m_radio->setCWSpeed(wpm);
    LOG_INFO("SendMorseDialog", QString("Sending morse: '%1' at %2 WPM").arg(text).arg(wpm));

    m_statusLabel->setText(QString("Sending: %1 (ESC to stop)").arg(text));
    m_statusLabel->setStyleSheet("QLabel { color: blue; }");
    m_sendButton->setEnabled(false);
    m_abortButton->setEnabled(true);
    m_isSending = true;

    for (QPushButton* btn : m_macroButtons) {
        if (btn) btn->setEnabled(false);
    }

    m_radio->sendCW(text);
}

int SendMorseDialog::estimateTransmissionTimeMs(const QString& text, int wpm) const {
    // PARIS standard: 50 units per word
    // At 1 WPM, 1 word = 60 seconds, so 1 unit = 1.2 seconds
    // At N WPM, 1 unit = 1200/N ms
    //
    // Approximate character lengths in units:
    // Letter: average ~10 units (including inter-character space)
    // Space: 7 units (inter-word space)
    // Number: ~20 units
    //
    // Simple approximation: 10 units per character average

    double msPerUnit = 1200.0 / wpm;
    int charCount = text.length();
    int estimatedUnits = charCount * 10;  // ~10 units per char average

    // Add a buffer of 500ms for radio latency
    int totalMs = static_cast<int>(estimatedUnits * msPerUnit) + 500;

    // Minimum 1 second, maximum 60 seconds
    return qBound(1000, totalMs, 60000);
}

void SendMorseDialog::onTransmissionComplete() {
    m_statusLabel->setText("Sent successfully");
    m_statusLabel->setStyleSheet("QLabel { color: green; }");

    m_sendButton->setEnabled(true);
    m_abortButton->setEnabled(false);
    m_isSending = false;

    // Re-enable macro buttons
    for (QPushButton* btn : m_macroButtons) {
        if (btn) btn->setEnabled(true);
    }
}

void SendMorseDialog::onAbortClicked() {
    // Use CWSender if available
    if (m_cwSender) {
        m_cwSender->stop();
        return;
    }

    // Fallback to direct radio control
    if (m_radio) {
        m_radio->stopCW();
    }

    m_statusLabel->setText("CW transmission stopped");
    m_statusLabel->setStyleSheet("QLabel { color: orange; }");
    LOG_INFO("SendMorseDialog", "CW transmission stopped by user");

    m_sendButton->setEnabled(true);
    m_abortButton->setEnabled(false);
    m_isSending = false;

    // Re-enable macro buttons
    for (QPushButton* btn : m_macroButtons) {
        if (btn) btn->setEnabled(true);
    }
}

void SendMorseDialog::onCWSenderStateChanged(CWSender::State state) {
    switch (state) {
        case CWSender::State::Idle:
            m_sendButton->setEnabled(true);
            m_abortButton->setEnabled(false);
            m_isSending = false;
            for (QPushButton* btn : m_macroButtons) {
                if (btn) btn->setEnabled(true);
            }
            break;

        case CWSender::State::Sending:
            m_statusLabel->setText("Sending CW... (ESC to stop)");
            m_statusLabel->setStyleSheet("QLabel { color: blue; }");
            m_sendButton->setEnabled(false);
            m_abortButton->setEnabled(true);
            m_isSending = true;
            for (QPushButton* btn : m_macroButtons) {
                if (btn) btn->setEnabled(false);
            }
            break;

        case CWSender::State::Stopping:
            m_statusLabel->setText("Stopping CW...");
            m_statusLabel->setStyleSheet("QLabel { color: orange; }");
            break;

        case CWSender::State::Error:
            m_statusLabel->setStyleSheet("QLabel { color: red; }");
            m_sendButton->setEnabled(true);
            m_abortButton->setEnabled(false);
            m_isSending = false;
            for (QPushButton* btn : m_macroButtons) {
                if (btn) btn->setEnabled(true);
            }
            break;
    }
}

void SendMorseDialog::onCWSenderError(const QString& error) {
    m_statusLabel->setText(QString("Error: %1").arg(error));
    m_statusLabel->setStyleSheet("QLabel { color: red; }");
    LOG_ERROR("SendMorseDialog", QString("CW error: %1").arg(error));
}

} // namespace TR4QT
