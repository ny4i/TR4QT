#ifndef RADIOCONFIGDIALOG_H
#define RADIOCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
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

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onConnectionTypeChanged();
    void onRadioModelChanged(int index);
    void onTestConnection();
    void onRefreshPorts();
    void onPortAutoRefresh();
    void onCivAddressModeChanged();

private:
    void setupUI();
    void populateRadioModels();
    void populateSerialPorts();
    void updateFieldsForRadio();

    // UI widgets
    QComboBox* m_radioModelCombo;
    QLineEdit* m_customModelEdit;

    QRadioButton* m_serialRadio;
    QRadioButton* m_networkRadio;

    QComboBox* m_serialPortCombo;
    QLineEdit* m_serialPortEdit;
    QPushButton* m_refreshPortsButton;
    QTimer* m_portRefreshTimer;
    QComboBox* m_baudRateCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;

    QLineEdit* m_ipAddressEdit;
    QSpinBox* m_portSpin;

    QRadioButton* m_civDefaultRadio;
    QRadioButton* m_civCustomRadio;
    QLineEdit* m_civAddressEdit;  // Changed from QSpinBox for better UX (allow "0", blank, or hex)
    QSpinBox* m_pollIntervalSpin;

    QGroupBox* m_serialGroup;
    QGroupBox* m_networkGroup;

    QCheckBox* m_autoConnectCheck;

    // Current config
    RadioConfig m_config;
};

} // namespace TR4QT

#endif // RADIOCONFIGDIALOG_H
