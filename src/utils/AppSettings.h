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

    // Radio configuration
    void saveRadioConfig(const RadioConfig& config);
    RadioConfig loadRadioConfig() const;
    bool hasRadioConfig() const;

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

    // Appearance settings
    void setEntryFontSize(int size);
    int getEntryFontSize() const;

    void setTableFontSize(int size);
    int getTableFontSize() const;

    void setGridFontSize(int size);
    int getGridFontSize() const;

    void setMiscDisplayFontSize(int size);
    int getMiscDisplayFontSize() const;

    // Band Needs Display settings
    void setNeedsDisplayWorkedColor(const QString& color);
    QString getNeedsDisplayWorkedColor() const;  // Default: #808080 (gray)

    void setNeedsDisplayNeededColor(const QString& color);
    QString getNeedsDisplayNeededColor() const;  // Default: #ffaa00 (orange)

    void setVHFBandsEnabled(bool enabled);
    bool getVHFBandsEnabled() const;  // Default: false (HF only)

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
    void saveQSOTableColumnWidths(const QList<int>& widths);
    QList<int> loadQSOTableColumnWidths() const;

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

private:
    AppSettings();
    ~AppSettings() = default;

    // Prevent copying
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    mutable QSettings m_settings;
};

} // namespace TR4QT

#endif // APPSETTINGS_H
