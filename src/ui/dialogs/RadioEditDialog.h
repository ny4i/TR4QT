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
    void onNetworkInterfaceChanged();
    void onRadioModelChanged(int index);
    void onRadioStatusFilterChanged();
    void onTestConnection();
    void onFindNetworkRadios();
    void refreshSerialPorts();
    void updateVisibility();

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
    int getCurrentInterfaceType() const;  // 0=Hamlib, 1=K4 Direct, 2=Icom Direct
    void showK4SelectionDialog();
    void showIcomSelectionDialog();
    QSet<QString> getConfiguredRadioIPs() const;

    // Radio name
    QLineEdit* m_radioNameEdit;

    // Connection type (primary choice)
    QRadioButton* m_serialRadio;
    QRadioButton* m_networkRadio;

    // Serial settings group
    QGroupBox* m_serialGroup;

    // Network settings group
    QGroupBox* m_networkGroup;

    // Network interface type (shown only when Network selected)
    QRadioButton* m_hamlibRadio;
    QRadioButton* m_k4DirectRadio;
    QRadioButton* m_icomDirectRadio;
    QWidget* m_interfaceTypeWidget;

    // Radio model section (context-dependent)
    QWidget* m_modelSelectionWidget;
    QComboBox* m_radioModelCombo;
    QLineEdit* m_customModelEdit;
    QCheckBox* m_showStableRadiosCheck;
    QCheckBox* m_showBetaRadiosCheck;
    QCheckBox* m_showAlphaRadiosCheck;
    QCheckBox* m_showUntestedRadiosCheck;
    QWidget* m_statusFilterWidget;

    // CI-V widget container (for show/hide)
    QWidget* m_civWidget;

    // Serial settings
    QComboBox* m_serialPortCombo;
    QLineEdit* m_serialPortEdit;
    QPushButton* m_refreshPortsButton;
    QTimer* m_portRefreshTimer;
    QComboBox* m_baudRateCombo;
    QLineEdit* m_customBaudRateEdit;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QComboBox* m_handshakeCombo;
    QComboBox* m_dtrStateCombo;
    QComboBox* m_rtsStateCombo;

    // Network settings
    QLineEdit* m_ipAddressEdit;
    QSpinBox* m_portSpin;

    // Icom credentials (shown only for Icom Direct)
    QWidget* m_icomCredentialsWidget;
    QLineEdit* m_icomUsernameEdit;
    QLineEdit* m_icomPasswordEdit;
    QLineEdit* m_icomClientNameEdit;

    // Discovery button
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
