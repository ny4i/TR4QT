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
    explicit SendMorseDialog(RadioController* radio, QWidget* parent = nullptr);
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

private:
    void setupUI();
    void sendMorse(const QString& text);
    QPushButton* createMacroButton(int index, const QString& label, const QString& morseText);
    void loadMacroSettings();
    void saveMacroSetting(int index, const QString& label, const QString& cwText);
    void updateMacroButton(int index, const QString& label, const QString& cwText);
    int estimateTransmissionTimeMs(const QString& text, int wpm) const;

    RadioController* m_radio;
    QLineEdit* m_textEdit;
    QPushButton* m_sendButton;
    QPushButton* m_abortButton;
    QLabel* m_statusLabel;
    QLabel* m_wpmLabel;

    // Store macro buttons for easy access
    QVector<QPushButton*> m_macroButtons;

    // Track sending state
    bool m_isSending;
    QTimer* m_transmissionTimer;
};

} // namespace TR4QT

#endif // SENDMORSEDIALOG_H
