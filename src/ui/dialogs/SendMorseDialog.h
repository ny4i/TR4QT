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

#ifndef SENDMORSEDIALOG_H
#define SENDMORSEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QVector>
#include <QTimer>
#include "../../radio/RadioController.h"
#include "../../cw/CWSender.h"

namespace TR4QT {

/**
 * Dialog for sending morse code messages
 *
 * Features:
 * - Editable macro buttons (right-click to edit)
 * - Free text entry field
 * - Send button
 * - Current WPM display
 * - Cancel/abort sending
 * - Persistent macro settings
 */
class SendMorseDialog : public QDialog {
    Q_OBJECT

public:
    explicit SendMorseDialog(RadioController* radio, CWSender* externalSender = nullptr, QWidget* parent = nullptr);
    ~SendMorseDialog() override = default;

    // Default macro definitions (label, CW text)
    static const int MACRO_COUNT = 12;
    static const char* DEFAULT_MACRO_LABELS[MACRO_COUNT];
    static const char* DEFAULT_MACRO_TEXTS[MACRO_COUNT];

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSendClicked();
    void onMacroClicked();
    void onAbortClicked();
    void onMacroRightClicked(int macroIndex);
    void onTransmissionComplete();
    void onCWSenderStateChanged(CWSender::State state);
    void onCWSenderError(const QString& error);

private:
    void setupUI();
    void sendMorse(const QString& text);
    QPushButton* createMacroButton(int index, const QString& label, const QString& morseText);
    void loadMacroSettings();
    void saveMacroSetting(int index, const QString& label, const QString& cwText);
    void updateMacroButton(int index, const QString& label, const QString& cwText);
    int estimateTransmissionTimeMs(const QString& text, int wpm) const;

    RadioController* m_radio;
    CWSender* m_cwSender;  // CW sender abstraction
    QLineEdit* m_textEdit;
    QPushButton* m_sendButton;
    QPushButton* m_abortButton;
    QLabel* m_statusLabel;
    QLabel* m_wpmLabel;

    // Store macro buttons for easy access
    QVector<QPushButton*> m_macroButtons;

    // Track sending state (kept for backward compatibility, synced with CWSender state)
    bool m_isSending;
};

} // namespace TR4QT

#endif // SENDMORSEDIALOG_H
