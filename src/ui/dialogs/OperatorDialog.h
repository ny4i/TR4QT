#ifndef OPERATORDIALOG_H
#define OPERATORDIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace TR4QT {

/**
 * Simple dialog for changing the current operator callsign
 * Invoked when user enters "OPON" in the callsign field
 */
class OperatorDialog : public QDialog {
    Q_OBJECT

public:
    explicit OperatorDialog(QWidget* parent = nullptr);
    ~OperatorDialog() override = default;

    /**
     * Get the entered operator callsign
     */
    QString getOperatorCallsign() const;

    /**
     * Set the initial operator callsign in the field
     */
    void setOperatorCallsign(const QString& callsign);

private:
    void setupUI();

    QLineEdit* m_operatorEdit;
};

} // namespace TR4QT

#endif // OPERATORDIALOG_H
