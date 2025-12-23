#ifndef RADIOCONFIGDIALOG_H
#define RADIOCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include "../../radio/RadioInterface.h"

namespace TR4QT {

/**
 * Dialog for configuring radio connection
 * Allows user to specify:
 * - Radio model (hamlib model ID)
 * - Connection type (serial or network)
 * - Port/address
 * - Baud rate (serial only)
 * - CI-V address (Icom radios)
 * - Poll interval
 */
class RadioConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit RadioConfigDialog(QWidget* parent = nullptr);
    ~RadioConfigDialog() override = default;

    // Get the configured settings
    RadioConfig getConfig() const;

    // Set initial values
    void setConfig(const RadioConfig& config);

    // Get auto-connect setting
    bool getAutoConnect() const;

private slots:
    void onConnectionTypeChanged();
    void onRadioModelChanged(int index);
    void onTestConnection();

private:
    void setupUI();
    void populateRadioModels();
    void updateFieldsForRadio();

    // UI widgets
    QComboBox* m_radioModelCombo;
    QLineEdit* m_customModelEdit;

    QRadioButton* m_serialRadio;
    QRadioButton* m_networkRadio;

    QLineEdit* m_serialPortEdit;
    QComboBox* m_baudRateCombo;

    QLineEdit* m_ipAddressEdit;
    QSpinBox* m_portSpin;

    QSpinBox* m_civAddressSpin;
    QSpinBox* m_pollIntervalSpin;

    QGroupBox* m_serialGroup;
    QGroupBox* m_networkGroup;

    QCheckBox* m_autoConnectCheck;

    // Current config
    RadioConfig m_config;
};

} // namespace TR4QT

#endif // RADIOCONFIGDIALOG_H
