#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>
#include <QList>
#include <QDateTime>
#include "../radio/RadioInterface.h"
#include "../logging/LogLevel.h"

namespace TR4QT {

// Forward declaration
struct UdpDestination;

/**
 * Application settings wrapper using QSettings
 * Provides persistent storage for radio config, user preferences, etc.
 */
class AppSettings {
public:
    // Singleton access
    static AppSettings& instance();

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

    // Radio auto-connect
    void setRadioAutoConnect(bool autoConnect);
    bool getRadioAutoConnect() const;

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

private:
    AppSettings();
    ~AppSettings() = default;

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

    mutable QSettings m_settings;
};

} // namespace TR4QT

#endif // APPSETTINGS_H
