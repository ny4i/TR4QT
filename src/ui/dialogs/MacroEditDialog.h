#ifndef MACROEDITDIALOG_H
#define MACROEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace TR4QT {

/**
 * Dialog for editing a CW macro button
 * Allows user to set the button label and the CW text to send
 */
class MacroEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit MacroEditDialog(int macroIndex, const QString& currentLabel,
                            const QString& currentText, QWidget* parent = nullptr);

    QString getLabel() const;
    QString getCWText() const;

private:
    void setupUI(const QString& currentLabel, const QString& currentText);

    int m_macroIndex;
    QLineEdit* m_labelEdit;
    QLineEdit* m_cwTextEdit;
};

} // namespace TR4QT

#endif // MACROEDITDIALOG_H
