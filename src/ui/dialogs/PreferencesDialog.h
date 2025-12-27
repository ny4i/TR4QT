#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
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
#include "../../utils/DXClusterListDownloader.h"

namespace TR4QT {

/**
 * Preferences dialog with tabbed interface
 * Consolidates all application settings in one place:
 * - Station information (callsign, zone, grid, etc.)
 * - Radio configuration (model, port, auto-connect)
 * - Appearance settings (font sizes, colors)
 * - Contest preferences
 * - Advanced settings
 */
class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog() override = default;

    // Accept/Apply settings
    void accept() override;

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

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // Tab creation methods
    QWidget* createStationTab();
    QWidget* createRadioTab();
    QWidget* createDXClusterTab();
    QWidget* createUDPBroadcastTab();
    QWidget* createAppearanceTab();
    QWidget* createLoggingTab();
    QWidget* createBackupTab();
    QWidget* createContestTab();
    QWidget* createWebServerTab();
    QWidget* createAdvancedTab();

    // Helper methods
    void populateRadioList();

    // Station tab widgets
    QLineEdit* m_callsignEdit;
    QLineEdit* m_gridSquareEdit;
    QSpinBox* m_cqZoneSpin;
    QSpinBox* m_ituZoneSpin;
    QLineEdit* m_stateEdit;
    QLineEdit* m_arrlSectionEdit;
    QComboBox* m_continentCombo;
    QLineEdit* m_operatorEdit;

    // Radio tab widgets (from RadioConfigDialog)
    QComboBox* m_radioModelCombo;
    QLineEdit* m_customModelEdit;
    QCheckBox* m_showStableRadiosCheck;
    QCheckBox* m_showBetaRadiosCheck;
    QCheckBox* m_showAlphaRadiosCheck;
    QCheckBox* m_showUntestedRadiosCheck;
    QRadioButton* m_serialRadio;
    QRadioButton* m_networkRadio;
    QLineEdit* m_serialPortEdit;
    QComboBox* m_baudRateCombo;
    QLineEdit* m_ipAddressEdit;
    QSpinBox* m_portSpin;
    QSpinBox* m_civAddressSpin;
    QSpinBox* m_pollIntervalSpin;
    QCheckBox* m_autoConnectCheck;
    QGroupBox* m_serialGroup;
    QGroupBox* m_networkGroup;
    QSpinBox* m_morseWpmSpin;
    QSpinBox* m_morseWpmIncrementSpin;

    // DX Cluster tab widgets
    QLineEdit* m_dxClusterCallsignEdit;
    QComboBox* m_dxClusterServerCombo;
    QCheckBox* m_dxClusterAutoConnectCheck;
    QCheckBox* m_enableLotwLookupCheck;
    QSpinBox* m_lotwMinUploadMonthsSpin;
    QPushButton* m_downloadClusterListButton;

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

    // Appearance tab widgets
    QSpinBox* m_entryFontSizeSpin;
    QSpinBox* m_tableFontSizeSpin;
    QSpinBox* m_gridFontSizeSpin;
    QSpinBox* m_miscDisplayFontSizeSpin;
    QComboBox* m_themeCombo;

    // Band Needs Display widgets
    QPushButton* m_workedColorButton;
    QPushButton* m_neededColorButton;
    QCheckBox* m_vhfBandsEnabledCheck;
    QPushButton* m_customizeColorsButton;
    QCheckBox* m_useMetricDistanceCheck;

    // Logging tab widgets
    QComboBox* m_logLevelCombo;
    QCheckBox* m_fileLoggingEnabledCheck;
    QCheckBox* m_consoleLoggingEnabledCheck;
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

    QTabWidget* m_tabWidget;
};

} // namespace TR4QT

#endif // PREFERENCESDIALOG_H
