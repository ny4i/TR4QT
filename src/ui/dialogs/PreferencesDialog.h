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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QStackedWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "../../radio/RadioInterface.h"
#include "../../cw/CWOutputProfile.h"
#include "../../utils/AppSettings.h"
#include "../../utils/DXClusterListDownloader.h"

namespace TR4QT {

/**
 * Preferences dialog with sidebar navigation interface
 * Consolidates all application settings in one place:
 * - Station information (callsign, zone, grid, etc.)
 * - Radio configuration (model, port, auto-connect)
 * - Appearance settings (font sizes, colors)
 * - Contest preferences
 * - Advanced settings
 *
 * Uses QListWidget sidebar + QStackedWidget for better scalability
 */
class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog() override = default;

    // Accept/Apply settings
    void accept() override;

    /**
     * Select a specific category/tab by name
     * @param categoryName The category to show (e.g., "Radio", "Station", "Appearance")
     */
    void selectCategory(const QString& categoryName);

    /**
     * Select a specific sub-tab within Hardware category by name
     * @param subTabName The sub-tab to show (e.g., "CW Input", "CW Output", "Radio")
     */
    void selectHardwareSubTab(const QString& subTabName);

    /**
     * Set the radio connection status (disables Test Connection when connected)
     * @param connected true if radio is currently connected
     */
    void setRadioConnected(bool connected);

signals:
    /**
     * Emitted when LOTW settings change and Band Map needs refresh
     */
    void lotwSettingsChanged();

    /**
     * Emitted when user clicks "Paddle Test..." button in CW Input tab.
     * MainWindow connects this to open KeyerSetupDialog with live CWService objects.
     */
    void openKeyerSetupRequested();

private slots:
    void onApply();

    // ===== My Radios management slots =====
    void onAddRadio();
    void onEditRadio();
    void onRemoveRadio();
    void onRadioDoubleClicked(QListWidgetItem* item);

    // ===== CW Output Profile management slots =====
    void onAddCWOutput();
    void onEditCWOutput();
    void onRemoveCWOutput();
    void onCWOutputDoubleClicked(QListWidgetItem* item);

    // ===== Station Profile management slots =====
    void onStationProfileChanged(int index);
    void onNewStationProfile();
    void onRenameStationProfile();
    void onDeleteStationProfile();
    void onRadio1AssignChanged(int index);
    void onRadio2AssignChanged(int index);
    void onDefaultActiveChanged();
    void onSO2REnabledChanged(bool enabled);
    void onActivateProfile();

    // UDP Broadcast slots
    void onUdpAddDestination();
    void onUdpRemoveDestination();
    void onUdpTestDestination();

    // Logging slots
    void onOpenLogFile();
    void onClearLogFile();
    void onBrowseLogFile();

    // Theme slots
    void onThemeChanged(int index);
    void onCustomizeColors();

    // DX Cluster list slots
    void onDownloadClusterList();
    void onClusterListDownloadFinished(bool success, const QList<DXClusterServer>& servers);
    void onDXClusterServerChanged(const QString& text);

    // Backup slots
    void onBrowseBackupDirectory();

    // Amplifier slots
    void onAmplifierModelChanged(int index);
    void onAmplifierConnectionTypeChanged(int index);
    void onTestAmplifierConnection();

    // Rotator slots
    void onRotatorModelChanged(int index);
    void onRotatorConnectionTypeChanged(int index);
    void onTestRotatorConnection();

    // Paddle input slots
    void onPaddleDeviceChanged(int index);
    void onRefreshPaddlePorts();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // Tab creation methods
    QWidget* createStationTab();
    QWidget* createHardwareTab();
    QWidget* createRadioSettingsWidget();
    QWidget* createAmplifierSettingsWidget();
    QWidget* createRotatorSettingsWidget();
    QWidget* createCWOutputSettingsWidget();
    QWidget* createCWInputSettingsWidget();
    QWidget* createDXClusterTab();
    QWidget* createSCPTab();
    QWidget* createUDPBroadcastTab();
    QWidget* createNetworkTab();
    QWidget* createAppearanceTab();
    QWidget* createLoggingTab();
    QWidget* createBackupTab();
    QWidget* createContestTab();
    QWidget* createCWSettingsTab();
    QWidget* createWebServerTab();
    QWidget* createExternalSoftwareTab();
    QWidget* createAdvancedTab();

    // Helper methods
    void refreshRadioList();           // Refresh My Radios list
    void refreshCWOutputList();        // Refresh CW Output Profiles list
    void refreshRadioAssignCombos();   // Refresh Radio 1/Radio 2 assignment dropdowns
    void refreshCWAssignCombos();      // Refresh CW 1/CW 2 assignment dropdowns
    void refreshStationProfileCombo(); // Refresh station profile dropdown
    void loadStationProfileIntoUI(const QString& profileName);  // Load profile settings into UI
    void saveCurrentStationProfile();  // Save current UI state to profile
    void populateAmplifierList();
    void populateRotatorList();

    // Station tab widgets
    QLineEdit* m_callsignEdit;
    QLineEdit* m_firstNameEdit;
    QLineEdit* m_lastNameEdit;
    QComboBox* m_licenseClassCombo;
    QLineEdit* m_gridSquareEdit;
    QSpinBox* m_cqZoneSpin;
    QSpinBox* m_ituZoneSpin;
    QLineEdit* m_stateEdit;
    QLineEdit* m_arrlSectionEdit;
    QLineEdit* m_countyEdit;
    QComboBox* m_continentCombo;
    QLineEdit* m_operatorEdit;

    // ===== My Radios Section =====
    // List of defined radios (name + connection summary)
    QListWidget* m_radioListWidget;
    QPushButton* m_addRadioButton;        // [+] Add new radio
    QPushButton* m_removeRadioButton;     // [-] Remove selected radio
    QPushButton* m_editRadioButton;       // [Edit...] Edit selected radio
    QList<RadioProfile> m_radioProfiles;  // Cache of radio profiles for current session

    // ===== Station Profiles Section =====
    // Station profiles assign radios to Radio 1 and Radio 2 slots
    // Uses traditional OK/Cancel pattern - changes stored locally until OK is clicked
    QList<StationProfile> m_stationProfiles;  // Local cache (saved on OK, discarded on Cancel)
    QString m_pendingActiveProfile;            // Profile to activate on OK (empty = no change)
    QString m_currentEditingProfile;           // Tracks which profile is currently shown in UI
    QComboBox* m_stationProfileCombo;     // Select station profile
    QPushButton* m_newStationProfileButton;
    QPushButton* m_renameStationProfileButton;
    QPushButton* m_deleteStationProfileButton;
    QComboBox* m_radio1AssignCombo;       // Radio 1 assignment dropdown
    QComboBox* m_radio2AssignCombo;       // Radio 2 assignment dropdown
    QRadioButton* m_radio1DefaultButton;  // Radio 1 is default active
    QRadioButton* m_radio2DefaultButton;  // Radio 2 is default active
    QCheckBox* m_so2rEnabledCheck;        // Enable SO2R mode

    // Active profile display/activation
    QLabel* m_activeProfileLabel;
    QPushButton* m_activateButton;

    // Checkbox for auto-connect on startup
    QCheckBox* m_autoConnectCheck;

    // CW General Settings (global, not per-profile)
    QSpinBox* m_morseWpmSpin;
    QSpinBox* m_morseWpmIncrementSpin;
    QCheckBox* m_cutNumbersEnabledCheck;
    QSpinBox* m_serialNumberWidthSpin;
    QRadioButton* m_iambicARadio;
    QRadioButton* m_iambicBRadio;

    // Paddle Input (global, not per-profile)
    QComboBox* m_paddleDeviceCombo;
    QComboBox* m_paddlePortCombo;
    QPushButton* m_paddleRefreshPortsButton;
    QCheckBox* m_paddleSwapCheck;
    QWidget* m_paddlePortWidget;            // Container for port row (hidden when None)

    // ===== CW Output Profiles Section =====
    QListWidget* m_cwOutputListWidget;
    QPushButton* m_addCWOutputButton;
    QPushButton* m_removeCWOutputButton;
    QPushButton* m_editCWOutputButton;
    QList<CWOutputProfile> m_cwOutputProfiles;  // Cache of CW output profiles

    // ===== CW Assignment in Station Profiles =====
    QComboBox* m_cw1AssignCombo;      // CW output assigned to Radio 1
    QComboBox* m_cw2AssignCombo;      // CW output assigned to Radio 2

    // Amplifier widgets (enhanced for Hamlib support)
    QCheckBox* m_amplifierEnabledCheck;
    QComboBox* m_amplifierModelCombo;
    QComboBox* m_amplifierConnectionTypeCombo;
    QLineEdit* m_amplifierPortEdit;          // IP:port or serial port
    QComboBox* m_amplifierBaudRateCombo;
    QCheckBox* m_amplifierAutoConnectCheck;
    QSpinBox* m_amplifierPollIntervalSpin;
    QPushButton* m_testAmplifierConnectionButton;
    QWidget* m_amplifierSerialSettingsWidget;  // Container for serial-specific settings

    // Legacy amplifier widgets (for backward compatibility with old UI)
    QLineEdit* m_amplifierIpEdit;
    QSpinBox* m_amplifierPortSpin;

    // Rotator widgets
    QCheckBox* m_rotatorEnabledCheck;
    QComboBox* m_rotatorModelCombo;
    QComboBox* m_rotatorConnectionTypeCombo;
    QLineEdit* m_rotatorIpEdit;
    QSpinBox* m_rotatorPortSpin;
    QLineEdit* m_rotatorSerialPortEdit;
    QComboBox* m_rotatorBaudRateCombo;
    QCheckBox* m_rotatorAutoConnectCheck;
    QPushButton* m_testRotatorConnectionButton;
    QWidget* m_rotatorNetworkSettingsWidget;   // Container for network-specific settings
    QWidget* m_rotatorSerialSettingsWidget;    // Container for serial-specific settings

    // DX Cluster tab widgets
    QLineEdit* m_dxClusterCallsignEdit;
    QComboBox* m_dxClusterServerCombo;
    QCheckBox* m_dxClusterAutoConnectCheck;
    QCheckBox* m_enableLotwLookupCheck;
    QSpinBox* m_lotwMinUploadMonthsSpin;
    QPushButton* m_downloadClusterListButton;

    // Band Map timeout settings
    QSpinBox* m_spotExpirySpin;          // Spot expiry time (seconds)
    QSpinBox* m_newSpotThresholdSpin;    // New spot threshold (seconds)
    QSpinBox* m_agingSpotThresholdSpin;  // Aging spot threshold (seconds)

    // SCP tab widgets
    QCheckBox* m_scpEnabledCheck;
    QLabel* m_scpStatusLabel;
    QPushButton* m_downloadSCPButton;
    QPushButton* m_updateLocalSCPButton;
    QCheckBox* m_scpIncludeLocalLogsCheck;

    // UDP Broadcast tab widgets
    QCheckBox* m_udpBroadcastEnabledCheck;
    QCheckBox* m_udpRadioInfoEnabledCheck;
    QCheckBox* m_udpContactInfoEnabledCheck;
    QSpinBox* m_udpThrottleIntervalSpin;
    QListWidget* m_udpDestinationsList;
    QLineEdit* m_udpHostEdit;
    QSpinBox* m_udpPortSpin;
    QPushButton* m_udpAddButton;
    QPushButton* m_udpRemoveButton;
    QPushButton* m_udpTestButton;

    // Network tab widgets
    QLineEdit* m_computerIDEdit;

    // WSJT-X integration widgets (on External Software tab)
    QCheckBox* m_wsjtxEnabledCheck;
    QSpinBox* m_wsjtxPortSpin;
    QLineEdit* m_wsjtxMulticastEdit;
    QCheckBox* m_wsjtxAutoLogCheck;
    QCheckBox* m_wsjtxHighlightCheck;

    // Appearance tab widgets
    QSpinBox* m_entryFontSizeSpin;
    QSpinBox* m_tableFontSizeSpin;
    QSpinBox* m_gridFontSizeSpin;
    QSpinBox* m_miscDisplayFontSizeSpin;
    QSpinBox* m_scpFontSizeSpin;
    QComboBox* m_themeCombo;

    // Band Needs Display widgets
    QPushButton* m_workedColorButton;
    QPushButton* m_neededColorButton;
    QCheckBox* m_vhfBandsEnabledCheck;

    // DX Cluster color widgets
    QPushButton* m_clusterDupeColorButton;
    QPushButton* m_clusterMultColorButton;
    QPushButton* m_customizeColorsButton;
    QCheckBox* m_useMetricDistanceCheck;
    QCheckBox* m_showAllWindowsOnActivateCheck;  // macOS: bring all windows to front on activate

    // Logging tab widgets
    QComboBox* m_logLevelCombo;
    QCheckBox* m_fileLoggingEnabledCheck;
    QCheckBox* m_consoleLoggingEnabledCheck;
    QCheckBox* m_hamlibDebugEnabledCheck;
    QLineEdit* m_logFilePathEdit;
    QSpinBox* m_logMaxFileSizeSpin;
    QSpinBox* m_logMaxBackupFilesSpin;

    // Backup tab widgets
    QCheckBox* m_autoBackupEnabledCheck;
    QSpinBox* m_autoBackupIntervalSpin;
    QLineEdit* m_backupDirectoryEdit;
    QPushButton* m_browseBackupDirButton;
    QSpinBox* m_maxBackupsSpin;
    QLabel* m_backupInfoLabel;

    // Web Server tab widgets
    QCheckBox* m_webServerAutoStartCheck;
    QSpinBox* m_webServerPortSpin;
    QLineEdit* m_webServerAddressEdit;

    // Contest tab widgets
    QComboBox* m_defaultContestCombo;
    QSpinBox* m_serialStartSpin;

    // Advanced tab widgets
    QLineEdit* m_countryFilePathEdit;
    QCheckBox* m_autoUpdateCountryFileCheck;
    QCheckBox* m_dxlabDDECheck;
    QCheckBox* m_dxlabDDEQSYCheck;

    // Sidebar navigation widgets
    QListWidget* m_categoryList;
    QStackedWidget* m_settingsStack;

    // Hardware sub-tab widget (for selectHardwareSubTab)
    QTabWidget* m_hardwareTabs = nullptr;
};

} // namespace TR4QT

#endif // PREFERENCESDIALOG_H
