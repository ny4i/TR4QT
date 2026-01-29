#ifndef RADIOEDITDIALOG_H
#define RADIOEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include "../../radio/RadioInterface.h"
#include "../../utils/K4Discovery.h"
#include "../../utils/IcomDiscovery.h"
#include "../widgets/CivAddressWidget.h"

namespace TR4QT {

/**
 * Dialog for editing a single radio's configuration.
 *
 * This dialog contains all the settings for configuring a radio:
 * - Radio name
 * - Radio model selection
 * - Interface type (Hamlib/K4 Direct/Icom Direct)
 * - Connection settings (serial or network)
 * - Advanced settings (CI-V address, poll interval)
 * - Test connection functionality
 *
 * Used by PreferencesDialog when adding or editing radios in "My Radios" list.
 */
class RadioEditDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * Create dialog for adding a new radio
     * @param parent Parent widget
     */
    explicit RadioEditDialog(QWidget* parent = nullptr);

    /**
     * Create dialog for editing an existing radio
     * @param profile The radio profile to edit
     * @param parent Parent widget
     */
    explicit RadioEditDialog(const RadioProfile& profile, QWidget* parent = nullptr);

    ~RadioEditDialog() override = default;

    /**
     * Get the configured radio profile
     * @return RadioProfile with all settings from the dialog
     */
    RadioProfile getRadioProfile() const;

    /**
     * Get the radio name
     * @return The name entered for this radio
     */
    QString getRadioName() const;

private slots:
    void onConnectionTypeChanged();
    void onRadioModelChanged(int index);
    void onRadioStatusFilterChanged();
    void onRadioTypeChanged(int index);
    void onTestConnection();
    void onFindNetworkRadios();
    void refreshSerialPorts();

    // K4 Discovery slots
    void onK4RadioFound(const K4RadioInfo& radio);
    void onK4DiscoveryFinished(int count);

    // Icom Discovery slots
    void onIcomRadioFound(const IcomRadioDiscoveryInfo& radio);
    void onIcomDiscoveryFinished(int count);

private:
    void setupUI();
    void populateRadioList();
    void loadProfileIntoUI(const RadioProfile& profile);
    RadioConfig buildRadioConfigFromUI() const;

    // Radio name
    QLineEdit* m_radioNameEdit;

    // Radio model section
    QComboBox* m_radioModelCombo;
    QLineEdit* m_customModelEdit;
    QCheckBox* m_showStableRadiosCheck;
    QCheckBox* m_showBetaRadiosCheck;
    QCheckBox* m_showAlphaRadiosCheck;
    QCheckBox* m_showUntestedRadiosCheck;
    QComboBox* m_radioTypeCombo;

    // Connection type
    QRadioButton* m_serialRadio;
    QRadioButton* m_networkRadio;
    QGroupBox* m_serialGroup;
    QGroupBox* m_networkGroup;

    // Serial settings
    QComboBox* m_serialPortCombo;
    QLineEdit* m_serialPortEdit;
    QPushButton* m_refreshPortsButton;
    QTimer* m_portRefreshTimer;
    QComboBox* m_baudRateCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;

    // Network settings
    QLineEdit* m_ipAddressEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_icomUsernameEdit;
    QLineEdit* m_icomPasswordEdit;
    QLineEdit* m_icomClientNameEdit;
    QPushButton* m_findRadiosButton;

    // Advanced settings
    CivAddressWidget* m_civAddressWidget;
    QSpinBox* m_pollIntervalSpin;

    // Test connection
    QPushButton* m_testConnectionButton;
    QLabel* m_connectionStatusLabel;

    // Discovery helpers
    K4Discovery* m_k4Discovery;
    IcomDiscovery* m_icomDiscovery;
    QList<K4RadioInfo> m_foundK4Radios;
    QList<IcomRadioDiscoveryInfo> m_foundIcomRadios;

    // Original profile (for edit mode)
    RadioProfile m_originalProfile;
    bool m_isEditMode{false};
};

} // namespace TR4QT

#endif // RADIOEDITDIALOG_H
