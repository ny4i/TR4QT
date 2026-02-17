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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QTimer>
#include <QObject>
#include "../radio/RadioInterface.h"
#include "../logging/LogLevel.h"

namespace TR4QT {

// Forward declaration
struct UdpDestination;

/**
 * Station profile - groups radios into a named configuration
 *
 * A StationProfile defines which radios are assigned to Radio 1 and Radio 2 slots,
 * plus SO2R settings. Users can create multiple profiles (e.g., "Contest Station",
 * "Field Day", "Portable") and quickly switch between them.
 */
struct StationProfile {
    QString name;           ///< Profile name (e.g., "Contest Station", "Field Day")
    QString radio1Name;     ///< Name of radio assigned to Radio 1 slot (or empty)
    QString radio2Name;     ///< Name of radio assigned to Radio 2 slot (or empty)
    int defaultActive{0};   ///< 0 = Radio 1 is default active, 1 = Radio 2
    bool so2rEnabled{false};///< Enable SO2R (two-radio operation)
};

/**
 * Application settings wrapper using QSettings
 * Provides persistent storage for radio config, user preferences, etc.
 *
 * SAFETY FEATURES:
 * - Periodic auto-save every 60 seconds
 * - Settings backup before critical writes
 * - Verification that writes actually persisted
 * - Graceful shutdown handlers
 */
class AppSettings : public QObject {
    Q_OBJECT

public:
    // Singleton access
    static AppSettings& instance();

    /**
     * @brief Start periodic auto-save timer
     * Call this once from main() after QApplication is created
     */
    void startAutoSave();

    /**
     * @brief Stop periodic auto-save timer
     * Call this during application shutdown
     */
    void stopAutoSave();

    /**
     * @brief Force immediate sync of all settings to disk
     * Call this before any potentially dangerous operation
     */
    void forceSync();

    /**
     * @brief Create a backup of current settings file
     * @return true if backup was created successfully
     */
    bool createBackup();

    /**
     * @brief Restore settings from most recent backup
     * @return true if restore was successful
     */
    bool restoreFromBackup();

    /**
     * @brief Check if settings file exists and is readable
     * @return true if settings are accessible
     */
    bool verifySettingsIntegrity() const;

    // Radio configuration (legacy single-config system)
    void saveRadioConfig(const RadioConfig& config);
    RadioConfig loadRadioConfig() const;
    bool hasRadioConfig() const;

    // Radio profiles (multi-config system)
    void saveRadioProfiles(const QList<RadioProfile>& profiles);
    QList<RadioProfile> loadRadioProfiles() const;
    bool hasRadioProfiles() const;
    void setActiveRadioProfile(const QString& profileName);
    QString getActiveRadioProfile() const;

    // Unified radio config access (checks profiles first, then legacy)
    RadioConfig getActiveRadioConfig() const;
    bool hasAnyRadioConfig() const;

    // Radio auto-connect
    void setRadioAutoConnect(bool autoConnect);
    bool getRadioAutoConnect() const;

    // SO2R (Single Operator Two Radio) settings
    void setSO2REnabled(bool enabled);
    bool isSO2REnabled() const;

    void setSO2RRadioProfile(int slot, const QString& profileName);
    QString getSO2RRadioProfile(int slot) const;

    // Station profiles (groups of radios for quick switching)
    void saveStationProfiles(const QList<StationProfile>& profiles);
    QList<StationProfile> loadStationProfiles() const;
    void setActiveStationProfile(const QString& profileName);
    QString getActiveStationProfile() const;

    /**
     * Get a station profile by name
     * @param name Profile name
     * @return StationProfile if found, empty profile with empty name if not found
     */
    StationProfile getStationProfile(const QString& name) const;

    // Radio status filter (for preferences dropdown)
    void setShowStableRadios(bool show);
    bool getShowStableRadios() const;
    void setShowBetaRadios(bool show);
    bool getShowBetaRadios() const;
    void setShowAlphaRadios(bool show);
    bool getShowAlphaRadios() const;
    void setShowUntestedRadios(bool show);
    bool getShowUntestedRadios() const;

    // Amplifier settings (expanded for Hamlib support)
    void setAmplifierEnabled(bool enabled);
    bool getAmplifierEnabled() const;

    void setAmplifierModel(int hamlibModelId);
    int getAmplifierModel() const;  // Hamlib AMP_MODEL_* constant

    void setAmplifierConnectionType(const QString& type);  // "direct" or "hamlib"
    QString getAmplifierConnectionType() const;

    void setAmplifierPort(const QString& port);  // IP:port or serial port
    QString getAmplifierPort() const;

    void setAmplifierBaudRate(int baudRate);
    int getAmplifierBaudRate() const;

    void setAmplifierAutoConnect(bool autoConnect);
    bool getAmplifierAutoConnect() const;

    void setAmplifierPollInterval(int intervalMs);
    int getAmplifierPollInterval() const;  // Default: 250ms

    // Legacy settings (for backward compatibility with existing KPA1500 configs)
    void setAmplifierIpAddress(const QString& ipAddress);
    QString getAmplifierIpAddress() const;
    void setAmplifierPortNumber(int port);  // Renamed to avoid conflict with setAmplifierPort(QString)
    int getAmplifierPortNumber() const;

    // Rotator settings (new for Hamlib support)
    void setRotatorEnabled(bool enabled);
    bool getRotatorEnabled() const;

    void setRotatorModel(int hamlibModelId);  // Hamlib ROT_MODEL_* constant
    int getRotatorModel() const;

    void setRotatorConnectionType(const QString& type);  // "direct" or "hamlib"
    QString getRotatorConnectionType() const;

    void setRotatorIpAddress(const QString& ipAddress);
    QString getRotatorIpAddress() const;

    void setRotatorPort(int port);
    int getRotatorPort() const;

    void setRotatorSerialPort(const QString& serialPort);
    QString getRotatorSerialPort() const;

    void setRotatorBaudRate(int baudRate);
    int getRotatorBaudRate() const;

    void setRotatorAutoConnect(bool autoConnect);
    bool getRotatorAutoConnect() const;

    // Morse code settings
    void setMorseWPM(int wpm);
    int getMorseWPM() const;  // Default: 25 WPM
    void setMorseWPMIncrement(int increment);
    int getMorseWPMIncrement() const;  // Default: 3 WPM

    // CW Macro settings (12 macro buttons)
    void setMacroLabel(int index, const QString& label);
    QString getMacroLabel(int index) const;
    void setMacroCWText(int index, const QString& text);
    QString getMacroCWText(int index) const;

    // TR4W-style CW Messages (F1-F12 with modifiers)
    // CQ Mode messages (F1-F12)
    void setCQMessage(int fKey, const QString& templateStr);
    QString getCQMessage(int fKey) const;

    // S&P Mode messages (F1-F12)
    void setSPMessage(int fKey, const QString& templateStr);
    QString getSPMessage(int fKey) const;

    // Ctrl+F messages (F1-F12, per mode)
    void setCtrlFMessage(int fKey, bool cqMode, const QString& templateStr);
    QString getCtrlFMessage(int fKey, bool cqMode) const;

    // Alt+F messages (F1-F12, per mode)
    void setAltFMessage(int fKey, bool cqMode, const QString& templateStr);
    QString getAltFMessage(int fKey, bool cqMode) const;

    // CW Auto-send toggle
    void setCWAutoSendEnabled(bool enabled);
    bool getCWAutoSendEnabled() const;

    // TR4W-style Auto-Send CW Messages
    void setCQCWExchange(const QString& message);
    QString getCQCWExchange() const;

    void setSPCWExchange(const QString& message);
    QString getSPCWExchange() const;

    void setQSLCWMessage(const QString& message);
    QString getQSLCWMessage() const;

    void setQuickQSLCWMessage(const QString& message);
    QString getQuickQSLCWMessage() const;

    void setQSOBeforeCWMessage(const QString& message);
    QString getQSOBeforeCWMessage() const;

    void setRepeatSPCWExchange(const QString& message);
    QString getRepeatSPCWExchange() const;

    void setCQCWExchangeNameKnown(const QString& message);
    QString getCQCWExchangeNameKnown() const;

    void setCallOkNowCWMessage(const QString& message);
    QString getCallOkNowCWMessage() const;

    void setTailEndCWMessage(const QString& message);
    QString getTailEndCWMessage() const;

    // CW Cut Numbers (SHORT_0 through SHORT_9 messages)
    void setCutNumbersEnabled(bool enabled);
    bool getCutNumbersEnabled() const;

    void setShortMessage(int digit, const QString& message);  // digit 0-9
    QString getShortMessage(int digit) const;

    // CW Serial Number Formatting
    void setSerialNumberWidth(int width);  // 0-4 leading zeros
    int getSerialNumberWidth() const;      // Default: 3 (e.g., "002")

    // CW Keyer Hardware settings
    void setKeyerDeviceType(int type);         // KeyerDeviceType enum value
    int getKeyerDeviceType() const;            // Default: 0 (WinKeyer)
    void setKeyerPortName(const QString& port);
    QString getKeyerPortName() const;
    void setKeyerPaddleSwap(bool swap);
    bool getKeyerPaddleSwap() const;
    void setKeyerIambicMode(int mode);         // 0=IambicA, 1=IambicB
    int getKeyerIambicMode() const;            // Default: 1 (IambicB)
    void setKeyerDitNote(int note);
    int getKeyerDitNote() const;               // Default: 20
    void setKeyerDahNote(int note);
    int getKeyerDahNote() const;               // Default: 21
    void setKeyerEnabled(bool enabled);
    bool getKeyerEnabled() const;
    void setKeyerAutoConnect(bool autoConnect);
    bool getKeyerAutoConnect() const;

    // CW Keying Source: 0=Radio (KY command), 1=External Keyer, 2=DTR/RTS
    void setCWKeyingSource(int source);
    int getCWKeyingSource() const;             // Default: 0 (Radio)

    // DTR/RTS CW keying settings (always uses a separate dedicated serial port)
    void setDtrRtsPortName(const QString& port);
    QString getDtrRtsPortName() const;         // Default: "" (no port configured)
    void setDtrRtsPin(int pin);
    int getDtrRtsPin() const;                  // Default: 0 (DTR), 1=RTS

    // Sidetone settings
    void setSidetonePitch(int hz);
    int getSidetonePitch() const;              // Default: 600 Hz
    void setSidetoneVolume(int percent);
    int getSidetoneVolume() const;             // Default: 50%

    // WinKeyer extended settings
    void setWinKeyerWeighting(int weight);
    int getWinKeyerWeighting() const;          // Default: 50 (normal)
    void setWinKeyerLeadIn(int time);
    int getWinKeyerLeadIn() const;             // Default: 0 (x10ms)
    void setWinKeyerTailTime(int time);
    int getWinKeyerTailTime() const;           // Default: 0 (x10ms)

    // Station information
    void setMyCallsign(const QString& callsign);
    QString getMyCallsign() const;

    void setMyGridSquare(const QString& grid);
    QString getMyGridSquare() const;

    void setMyContinent(const QString& continent);
    QString getMyContinent() const;

    void setMyCQZone(int zone);
    int getMyCQZone() const;

    void setMyITUZone(int zone);
    int getMyITUZone() const;

    void setMyState(const QString& state);
    QString getMyState() const;

    void setMyARRLSection(const QString& section);
    QString getMyARRLSection() const;

    void setMyCounty(const QString& county);
    QString getMyCounty() const;

    void setMyFirstName(const QString& firstName);
    QString getMyFirstName() const;

    void setMyLastName(const QString& lastName);
    QString getMyLastName() const;

    void setComputerID(const QString& id);
    QString getComputerID() const;  // Default: "A"

    void setLicenseClass(const QString& licenseClass);
    QString getLicenseClass() const;  // Default: "General"

    // Current operator (for multi-op contests)
    void setCurrentOperator(const QString& callsign);
    QString getCurrentOperator() const;

    // Main window geometry
    void saveWindowGeometry(const QByteArray& geometry);
    QByteArray loadWindowGeometry() const;

    void saveWindowState(const QByteArray& state);
    QByteArray loadWindowState() const;

    // Child window geometry and visibility
    void saveDXClusterGeometry(const QByteArray& geometry);
    QByteArray loadDXClusterGeometry() const;
    void setDXClusterVisible(bool visible);
    bool getDXClusterVisible() const;

    void saveBandMapGeometry(const QByteArray& geometry);
    QByteArray loadBandMapGeometry() const;
    void setBandMapVisible(bool visible);
    bool getBandMapVisible() const;

    void saveRadioControlGeometry(const QByteArray& geometry);
    QByteArray loadRadioControlGeometry() const;
    void setRadioControlVisible(bool visible);
    bool getRadioControlVisible() const;

    void saveRadio2ControlGeometry(const QByteArray& geometry);
    QByteArray loadRadio2ControlGeometry() const;
    void setRadio2ControlVisible(bool visible);
    bool getRadio2ControlVisible() const;

    void saveMultipliersGeometry(const QByteArray& geometry);
    QByteArray loadMultipliersGeometry() const;
    void setMultipliersVisible(bool visible);
    bool getMultipliersVisible() const;

    void saveGraylineMapGeometry(const QByteArray& geometry);
    QByteArray loadGraylineMapGeometry() const;
    void setGraylineMapVisible(bool visible);
    bool getGraylineMapVisible() const;

    void saveAmplifierControlGeometry(const QByteArray& geometry);
    QByteArray loadAmplifierControlGeometry() const;
    void setAmplifierControlVisible(bool visible);
    bool getAmplifierControlVisible() const;

    // Statistics window settings
    void saveStatisticsWindowGeometry(const QByteArray& geometry);
    QByteArray loadStatisticsWindowGeometry() const;

    // DX Cluster settings
    void setDXClusterCallsign(const QString& callsign);
    QString getDXClusterCallsign() const;

    void setDXClusterServer(const QString& server);
    QString getDXClusterServer() const;

    void setDXClusterAutoConnect(bool autoConnect);
    bool getDXClusterAutoConnect() const;

    void saveDXClusterList(const QStringList& servers);
    QStringList getDXClusterList() const;

    // Backup settings
    void setAutoBackupEnabled(bool enabled);
    bool getAutoBackupEnabled() const;

    void setAutoBackupInterval(int qsoCount);
    int getAutoBackupInterval() const;

    void setBackupDirectory(const QString& path);
    QString getBackupDirectory() const;

    void setMaxBackups(int count);
    int getMaxBackups() const;

    // Country file
    void setCountryFileVersion(int version);
    int getCountryFileVersion() const;

    void setCountryFilePath(const QString& path);
    QString getCountryFilePath() const;

    // Band Map filter settings
    void setShowOnlyLotwUsers(bool show);
    bool getShowOnlyLotwUsers() const;

    void setShowAllBands(bool show);
    bool getShowAllBands() const;

    // Band Map distance units
    void setUseMetricDistance(bool useMetric);
    bool getUseMetricDistance() const;  // Default: true (kilometers)

    // Band Map spot aging settings
    void setSpotExpirySeconds(int seconds);
    int getSpotExpirySeconds() const;  // Default: 600 (10 minutes)

    void setNewSpotThresholdSeconds(int seconds);
    int getNewSpotThresholdSeconds() const;  // Default: 60 (1 minute)

    void setAgingSpotThresholdSeconds(int seconds);

    // Last opened contest
    void setLastContestPath(const QString& path);
    QString getLastContestPath() const;
    int getAgingSpotThresholdSeconds() const;  // Default: 120 (2 minutes before expiry)

    int getSpotRefreshIntervalMs() const;  // Default: 5000 (5 seconds)

    // LOTW settings
    void setLotwLastUpdateTime(const QDateTime& timestamp);
    QDateTime getLotwLastUpdateTime() const;

    void setEnableLotwLookup(bool enable);
    bool getEnableLotwLookup() const;

    void setLotwMinUploadMonths(int months);
    int getLotwMinUploadMonths() const;

    // AUTO S&P automation settings
    void setAutoSPEnable(bool enable);
    bool getAutoSPEnable() const;  // Default: false

    void setAutoSPSensitivity(int hzPerSec);
    int getAutoSPSensitivity() const;  // Default: 500 Hz/sec

    // Appearance settings
    void setEntryFontSize(int size);
    int getEntryFontSize() const;

    void setTableFontSize(int size);
    int getTableFontSize() const;

    void setGridFontSize(int size);
    int getGridFontSize() const;

    void setMiscDisplayFontSize(int size);
    int getMiscDisplayFontSize() const;

    void setSCPFontSize(int size);
    int getSCPFontSize() const;

    // Band Needs Display settings
    void setNeedsDisplayWorkedColor(const QString& color);
    QString getNeedsDisplayWorkedColor() const;  // Default: #808080 (gray)

    void setNeedsDisplayNeededColor(const QString& color);
    QString getNeedsDisplayNeededColor() const;  // Default: #ffaa00 (orange)

    void setVHFBandsEnabled(bool enabled);
    bool getVHFBandsEnabled() const;  // Default: false (HF only)

    // macOS window behavior
    void setShowAllWindowsOnActivate(bool enabled);
    bool getShowAllWindowsOnActivate() const;  // Default: false (macOS only)

    // DX Cluster spot color settings
    void setClusterDupeColor(const QString& color);
    QString getClusterDupeColor() const;  // Default: #808080 (gray)

    void setClusterMultiplierColor(const QString& color);
    QString getClusterMultiplierColor() const;  // Default: #ff0000 (red)

    // UDP Broadcast settings
    void setUDPBroadcastEnabled(bool enabled);
    bool getUDPBroadcastEnabled() const;

    void setUDPRadioInfoEnabled(bool enabled);
    bool getUDPRadioInfoEnabled() const;

    void setUDPContactInfoEnabled(bool enabled);
    bool getUDPContactInfoEnabled() const;

    void setUDPThrottleInterval(int milliseconds);
    int getUDPThrottleInterval() const;

    void setUDPDestinations(const QList<UdpDestination>& destinations);
    QList<UdpDestination> getUDPDestinations() const;

    // Logging settings
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    void setFileLoggingEnabled(bool enabled);
    bool getFileLoggingEnabled() const;

    void setConsoleLoggingEnabled(bool enabled);
    bool getConsoleLoggingEnabled() const;

    void setHamlibDebugEnabled(bool enabled);
    bool getHamlibDebugEnabled() const;

    void setLogFilePath(const QString& path);
    QString getLogFilePath() const;

    void setLogMaxFileSize(qint64 bytes);
    qint64 getLogMaxFileSize() const;

    void setLogMaxBackupFiles(int count);
    int getLogMaxBackupFiles() const;

    // QSO table column widths
    void saveQSOTableColumnWidths(const QString& contestId, const QList<int>& widths);
    QList<int> loadQSOTableColumnWidths(const QString& contestId) const;

    // Web server settings
    void setWebServerAutoStart(bool autoStart);
    bool getWebServerAutoStart() const;

    void setWebServerPort(quint16 port);
    quint16 getWebServerPort() const;  // Default: 14140

    void setWebServerAddress(const QString& address);
    QString getWebServerAddress() const;  // Default: "127.0.0.1" (localhost)

    // Super Check Partial (SCP) settings
    void setSCPEnabled(bool enabled);
    bool getSCPEnabled() const;  // Default: true

    void setSCPVersion(const QString& version);
    QString getSCPVersion() const;

    void setSCPLastUpdate(const QDateTime& dt);
    QDateTime getSCPLastUpdate() const;

    void setSCPIncludeLocalLogs(bool include);
    bool getSCPIncludeLocalLogs() const;  // Default: true

    // Generic settings access
    /**
     * Get a settings value by key path
     * @param key Settings key (e.g., "Station/firstName", "Station/state")
     * @param defaultValue Default value if key doesn't exist
     * @return Settings value as QString
     */
    QString getValue(const QString& key, const QString& defaultValue = QString()) const;

private slots:
    void onAutoSaveTimer();

private:
    AppSettings();
    ~AppSettings();

    // Prevent copying
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    /**
     * @brief Migrate settings pointing to legacy ~/.tr4qt paths
     *
     * Updates saved paths for logs, backups, and country file from
     * legacy ~/.tr4qt location to platform-native AppData/Local/TR4QT
     */
    void migrateLegacyPaths();

    /**
     * @brief Migrate single RadioConfig to profile system
     *
     * Converts legacy single radio configuration to "Default" profile
     * in the new profile system. Called automatically on first run.
     */
    void migrateToRadioProfiles();

    /**
     * @brief Migrate to StationProfile system
     *
     * Creates a "Default" StationProfile from existing radio profiles.
     * If SO2R was enabled with old system, preserves radio assignments.
     * Otherwise, assigns the active radio profile to Radio 1.
     * Called automatically on first run after migrateToRadioProfiles().
     */
    void migrateToStationProfiles();

    /**
     * @brief Migrate plain-text passwords from QSettings to OS-native credential store
     *
     * Called once from startAutoSave() (NOT constructor — QEventLoop needs QApplication running).
     * Reads icomPassword from QSettings, saves to CredentialStore, verifies round-trip,
     * then removes from QSettings. Gate flag prevents re-running on subsequent launches.
     * Partial migration retries on next launch.
     */
    void migrateCredentialsToSecureStore();

    /**
     * @brief Save a password to the secure credential store with QSettings fallback.
     * @param storageKey Credential key (e.g., CredentialKeys::ICOM_RADIO)
     * @param username Username (empty for password-only auth)
     * @param password Password to save (empty to delete from both stores)
     * @param settingsKey QSettings key for plaintext fallback (e.g., "icomPassword")
     * @return true if saved to secure store, false if fell back to QSettings
     */
    bool savePasswordSecurely(const QString& storageKey, const QString& username,
                              const QString& password, const QString& settingsKey);

    /**
     * @brief Load a password from the secure credential store with QSettings fallback.
     * @param storageKey Credential key
     * @param username Username
     * @param settingsKey QSettings key for legacy plaintext fallback
     * @return The password (empty if not found in either store)
     */
    QString loadPasswordSecurely(const QString& storageKey, const QString& username,
                                 const QString& settingsKey) const;

    /**
     * @brief Get the path to the settings file
     * Platform-specific: plist on macOS, registry/ini on Windows
     */
    QString getSettingsFilePath() const;

    /**
     * @brief Get the path to the settings backup directory
     */
    QString getSettingsBackupDir() const;

    mutable QSettings m_settings;
    QTimer* m_autoSaveTimer{nullptr};

    // Auto-save interval in milliseconds (60 seconds)
    static constexpr int AUTO_SAVE_INTERVAL_MS = 60000;

    // Maximum number of backup files to keep
    static constexpr int MAX_BACKUP_FILES = 5;
};

} // namespace TR4QT

#endif // APPSETTINGS_H
