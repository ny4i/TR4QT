#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QStackedWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "../../radio/RadioInterface.h"
#include "../../utils/DXClusterListDownloader.h"
#include "../../utils/K4Discovery.h"
#include "../../utils/IcomDiscovery.h"
#include "../widgets/CivAddressWidget.h"

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
     * Set the radio connection status (disables Test Connection when connected)
     * @param connected true if radio is currently connected
     */
    void setRadioConnected(bool connected);

signals:
    /**
     * Emitted when LOTW settings change and Band Map needs refresh
     */
    void lotwSettingsChanged();

private slots:
    void onApply();
    void onTestRadioConnection();
    void onConnectionTypeChanged();
    void onRadioModelChanged(int index);
    void onRadioStatusFilterChanged();
    void onRadioTypeChanged(int index);

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

    // K4 Discovery slots
    void onFindK4Radios();
    void onK4RadioFound(const K4RadioInfo& radio);
    void onK4DiscoveryFinished(int count);

    // Icom Discovery slots
    void onFindIcomRadios();
    void onIcomRadioFound(const IcomRadioDiscoveryInfo& radio);
    void onIcomDiscoveryFinished(int count);

    // Network discovery slot (dispatches to K4 or Icom based on radio type)
    void onFindNetworkRadios();

    // Serial port discovery slots
    void refreshSerialPorts();

    // Radio profile management slots
    void onProfileSelected(int index);
    void onNewProfile();
    void onEditProfile();
    void onDeleteProfile();
    void onSetActiveProfile();

    // Amplifier slots
    void onAmplifierModelChanged(int index);
    void onAmplifierConnectionTypeChanged(int index);
    void onTestAmplifierConnection();

    // Rotator slots
    void onRotatorModelChanged(int index);
    void onRotatorConnectionTypeChanged(int index);
    void onTestRotatorConnection();

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
    QWidget* createAdvancedTab();

    // Helper methods
    void populateRadioList();
    void populateAmplifierList();
    void populateRotatorList();
    void loadProfileIntoUI(const QString& profileName);
    RadioConfig buildRadioConfigFromUI() const;

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

    // Radio tab widgets (from RadioConfigDialog)
    QComboBox* m_radioTypeCombo;        // Radio interface type (Auto/Hamlib/K4 Direct)
    QComboBox* m_radioModelCombo;
    QLineEdit* m_customModelEdit;
    QCheckBox* m_showStableRadiosCheck;
    QCheckBox* m_showBetaRadiosCheck;
    QCheckBox* m_showAlphaRadiosCheck;
    QCheckBox* m_showUntestedRadiosCheck;
    QRadioButton* m_serialRadio;
    QRadioButton* m_networkRadio;
    QComboBox* m_serialPortCombo;       // Dropdown with detected serial ports
    QLineEdit* m_serialPortEdit;        // Manual entry fallback
    QPushButton* m_refreshPortsButton;  // Manual refresh button
    QTimer* m_portRefreshTimer;         // Auto-refresh timer (5 seconds)
    QComboBox* m_baudRateCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QLineEdit* m_ipAddressEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_icomUsernameEdit;
    QLineEdit* m_icomPasswordEdit;
    QLineEdit* m_icomClientNameEdit;
    CivAddressWidget* m_civAddressWidget;
    QSpinBox* m_pollIntervalSpin;
    QCheckBox* m_autoConnectCheck;
    QGroupBox* m_serialGroup;
    QGroupBox* m_networkGroup;
    QSpinBox* m_morseWpmSpin;
    QSpinBox* m_morseWpmIncrementSpin;
    QCheckBox* m_cutNumbersEnabledCheck;
    QSpinBox* m_serialNumberWidthSpin;
    QPushButton* m_testConnectionButton;
    QLabel* m_connectionStatusLabel;

    // Radio profile management widgets
    QComboBox* m_profileSelectorCombo;
    QPushButton* m_newProfileButton;
    QPushButton* m_editProfileButton;
    QPushButton* m_deleteProfileButton;
    QPushButton* m_setActiveButton;
    QLabel* m_activeProfileLabel;
    QList<RadioProfile> m_radioProfiles;  // Cache for current session

    // Amplifier widgets (enhanced for Hamlib support)
    QCheckBox* m_amplifierEnabledCheck;
    QComboBox* m_amplifierModelCombo;
    QComboBox* m_amplifierConnectionTypeCombo;
    QLineEdit* m_amplifierPortEdit;          // IP:port or serial port
    QComboBox* m_amplifierBaudRateCombo;
    QCheckBox* m_amplifierAutoConnectCheck;
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

    // K4 Discovery
    K4Discovery* m_k4Discovery;
    QPushButton* m_findK4Button;
    QList<K4RadioInfo> m_foundK4Radios;

    // Icom Discovery
    IcomDiscovery* m_icomDiscovery;
    QList<IcomRadioDiscoveryInfo> m_foundIcomRadios;

    // Sidebar navigation widgets
    QListWidget* m_categoryList;
    QStackedWidget* m_settingsStack;
};

} // namespace TR4QT

#endif // PREFERENCESDIALOG_H
