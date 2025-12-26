#ifndef SENDMORSEDIALOG_H
#define SENDMORSEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include "../../radio/RadioController.h"

namespace TR4QT {

/**
 * Dialog for sending morse code messages
 *
 * Features:
 * - Predefined macro buttons (CQ, TEST, QRZ?, TU, etc.)
 * - Free text entry field
 * - Send button
 * - Current WPM display
 * - Cancel/abort sending
 */
class SendMorseDialog : public QDialog {
    Q_OBJECT

public:
    explicit SendMorseDialog(RadioController* radio, QWidget* parent = nullptr);
    ~SendMorseDialog() override = default;

private slots:
    void onSendClicked();
    void onMacroClicked();
    void onAbortClicked();

private:
    void setupUI();
    void sendMorse(const QString& text);
    QPushButton* createMacroButton(const QString& label, const QString& morseText);

    RadioController* m_radio;
    QLineEdit* m_textEdit;
    QPushButton* m_sendButton;
    QPushButton* m_abortButton;
    QLabel* m_statusLabel;
    QLabel* m_wpmLabel;
};

} // namespace TR4QT

#endif // SENDMORSEDIALOG_H
