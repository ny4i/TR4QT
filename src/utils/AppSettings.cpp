#include "AppSettings.h"
#include "../core/Constants.h"
#include "../network/UdpBroadcaster.h"
#include <QDir>

namespace TR4QT {

AppSettings& AppSettings::instance() {
    static AppSettings instance;
    return instance;
}

AppSettings::AppSettings()
    : m_settings(APP_ORG, APP_NAME)
{
}

void AppSettings::saveRadioConfig(const RadioConfig& config) {
    m_settings.beginGroup("Radio");
    m_settings.setValue("modelId", config.hamlibModelId);
    m_settings.setValue("port", config.port);
    m_settings.setValue("baudRate", config.baudRate);
    m_settings.setValue("civAddress", config.civAddress);
    m_settings.setValue("pollInterval", config.pollInterval);
    m_settings.endGroup();
    m_settings.sync();
}

RadioConfig AppSettings::loadRadioConfig() const {
    RadioConfig config;
    m_settings.beginGroup("Radio");
    config.hamlibModelId = m_settings.value("modelId", 0).toInt();
    config.port = m_settings.value("port", "").toString();
    config.baudRate = m_settings.value("baudRate", 38400).toInt();
    config.civAddress = m_settings.value("civAddress", 0).toInt();
    config.pollInterval = m_settings.value("pollInterval", 100).toInt();
    m_settings.endGroup();
    return config;
}

bool AppSettings::hasRadioConfig() const {
    m_settings.beginGroup("Radio");
    bool hasConfig = m_settings.contains("modelId") && m_settings.contains("port");
    m_settings.endGroup();
    return hasConfig;
}

void AppSettings::setRadioAutoConnect(bool autoConnect) {
    m_settings.setValue("Radio/autoConnect", autoConnect);
    m_settings.sync();
}

bool AppSettings::getRadioAutoConnect() const {
    return m_settings.value("Radio/autoConnect", true).toBool();  // Default: true
}

void AppSettings::setShowStableRadios(bool show) {
    m_settings.setValue("Radio/showStableRadios", show);
    m_settings.sync();
}

bool AppSettings::getShowStableRadios() const {
    return m_settings.value("Radio/showStableRadios", true).toBool();  // Default: true (show stable)
}

void AppSettings::setShowBetaRadios(bool show) {
    m_settings.setValue("Radio/showBetaRadios", show);
    m_settings.sync();
}

bool AppSettings::getShowBetaRadios() const {
    return m_settings.value("Radio/showBetaRadios", false).toBool();  // Default: false
}

void AppSettings::setShowAlphaRadios(bool show) {
    m_settings.setValue("Radio/showAlphaRadios", show);
    m_settings.sync();
}

bool AppSettings::getShowAlphaRadios() const {
    return m_settings.value("Radio/showAlphaRadios", false).toBool();  // Default: false
}

void AppSettings::setShowUntestedRadios(bool show) {
    m_settings.setValue("Radio/showUntestedRadios", show);
    m_settings.sync();
}

bool AppSettings::getShowUntestedRadios() const {
    return m_settings.value("Radio/showUntestedRadios", false).toBool();  // Default: false
}

void AppSettings::setMorseWPM(int wpm) {
    m_settings.setValue("Morse/wpm", wpm);
    m_settings.sync();
}

int AppSettings::getMorseWPM() const {
    return m_settings.value("Morse/wpm", 25).toInt();  // Default: 25 WPM
}

void AppSettings::setMorseWPMIncrement(int increment) {
    m_settings.setValue("Morse/wpmIncrement", increment);
    m_settings.sync();
}

int AppSettings::getMorseWPMIncrement() const {
    return m_settings.value("Morse/wpmIncrement", 3).toInt();  // Default: 3 WPM
}

void AppSettings::setAutoSendCW(bool enabled) {
    m_settings.setValue("Morse/autoSendCW", enabled);
    m_settings.sync();
}

bool AppSettings::getAutoSendCW() const {
    return m_settings.value("Morse/autoSendCW", true).toBool();  // Default: true (enabled)
}

void AppSettings::setMyCallsign(const QString& callsign) {
    m_settings.setValue("Station/callsign", callsign.toUpper());
    m_settings.sync();
}

QString AppSettings::getMyCallsign() const {
    return m_settings.value("Station/callsign", "").toString();
}

void AppSettings::setMyGridSquare(const QString& grid) {
    m_settings.setValue("Station/gridSquare", grid.toUpper());
    m_settings.sync();
}

QString AppSettings::getMyGridSquare() const {
    return m_settings.value("Station/gridSquare", "").toString();
}

void AppSettings::setMyContinent(const QString& continent) {
    m_settings.setValue("Station/continent", continent.toUpper());
    m_settings.sync();
}

QString AppSettings::getMyContinent() const {
    return m_settings.value("Station/continent", "NA").toString();
}

void AppSettings::setMyCQZone(int zone) {
    m_settings.setValue("Station/cqZone", zone);
    m_settings.sync();
}

int AppSettings::getMyCQZone() const {
    return m_settings.value("Station/cqZone", 5).toInt();
}

void AppSettings::setMyITUZone(int zone) {
    m_settings.setValue("Station/ituZone", zone);
    m_settings.sync();
}

int AppSettings::getMyITUZone() const {
    return m_settings.value("Station/ituZone", 8).toInt();
}

void AppSettings::setMyState(const QString& state) {
    m_settings.setValue("Station/state", state.toUpper());
    m_settings.sync();
}

QString AppSettings::getMyState() const {
    return m_settings.value("Station/state", "").toString();
}

void AppSettings::setMyARRLSection(const QString& section) {
    m_settings.setValue("Station/arrlSection", section.toUpper());
    m_settings.sync();
}

QString AppSettings::getMyARRLSection() const {
    return m_settings.value("Station/arrlSection", "").toString();
}

void AppSettings::setCurrentOperator(const QString& callsign) {
    m_settings.setValue("Station/currentOperator", callsign.toUpper());
    m_settings.sync();
}

QString AppSettings::getCurrentOperator() const {
    // Default to station callsign if not set
    QString operator_ = m_settings.value("Station/currentOperator", "").toString();
    if (operator_.isEmpty()) {
        operator_ = getMyCallsign();
    }
    return operator_;
}

void AppSettings::saveWindowGeometry(const QByteArray& geometry) {
    m_settings.setValue("MainWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadWindowGeometry() const {
    return m_settings.value("MainWindow/geometry").toByteArray();
}

void AppSettings::saveWindowState(const QByteArray& state) {
    m_settings.setValue("MainWindow/state", state);
    m_settings.sync();
}

QByteArray AppSettings::loadWindowState() const {
    return m_settings.value("MainWindow/state").toByteArray();
}

// DX Cluster window
void AppSettings::saveDXClusterGeometry(const QByteArray& geometry) {
    m_settings.setValue("DXClusterWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadDXClusterGeometry() const {
    return m_settings.value("DXClusterWindow/geometry").toByteArray();
}

void AppSettings::setDXClusterVisible(bool visible) {
    m_settings.setValue("DXClusterWindow/visible", visible);
    m_settings.sync();
}

bool AppSettings::getDXClusterVisible() const {
    return m_settings.value("DXClusterWindow/visible", false).toBool();
}

// Band Map window
void AppSettings::saveBandMapGeometry(const QByteArray& geometry) {
    m_settings.setValue("BandMapWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadBandMapGeometry() const {
    return m_settings.value("BandMapWindow/geometry").toByteArray();
}

void AppSettings::setBandMapVisible(bool visible) {
    m_settings.setValue("BandMapWindow/visible", visible);
    m_settings.sync();
}

bool AppSettings::getBandMapVisible() const {
    return m_settings.value("BandMapWindow/visible", false).toBool();
}

// Radio Control window
void AppSettings::saveRadioControlGeometry(const QByteArray& geometry) {
    m_settings.setValue("RadioControlWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadRadioControlGeometry() const {
    return m_settings.value("RadioControlWindow/geometry").toByteArray();
}

void AppSettings::setRadioControlVisible(bool visible) {
    m_settings.setValue("RadioControlWindow/visible", visible);
    m_settings.sync();
}

bool AppSettings::getRadioControlVisible() const {
    return m_settings.value("RadioControlWindow/visible", false).toBool();
}

// Multipliers window
void AppSettings::saveMultipliersGeometry(const QByteArray& geometry) {
    m_settings.setValue("MultipliersWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadMultipliersGeometry() const {
    return m_settings.value("MultipliersWindow/geometry").toByteArray();
}

void AppSettings::setMultipliersVisible(bool visible) {
    m_settings.setValue("MultipliersWindow/visible", visible);
    m_settings.sync();
}

bool AppSettings::getMultipliersVisible() const {
    return m_settings.value("MultipliersWindow/visible", false).toBool();
}

// DX Cluster settings
void AppSettings::setDXClusterCallsign(const QString& callsign) {
    m_settings.setValue("DXCluster/callsign", callsign.toUpper());
    m_settings.sync();
}

QString AppSettings::getDXClusterCallsign() const {
    // Default to station callsign if not set
    QString dxCall = m_settings.value("DXCluster/callsign", "").toString();
    if (dxCall.isEmpty()) {
        dxCall = getMyCallsign();
    }
    return dxCall;
}

void AppSettings::setDXClusterServer(const QString& server) {
    m_settings.setValue("DXCluster/server", server);
    m_settings.sync();
}

QString AppSettings::getDXClusterServer() const {
    return m_settings.value("DXCluster/server", "dxc.nc7j.com:7373").toString();
}

void AppSettings::setDXClusterAutoConnect(bool autoConnect) {
    m_settings.setValue("DXCluster/autoConnect", autoConnect);
    m_settings.sync();
}

bool AppSettings::getDXClusterAutoConnect() const {
    return m_settings.value("DXCluster/autoConnect", false).toBool();
}

void AppSettings::saveDXClusterList(const QStringList& servers) {
    m_settings.setValue("DXCluster/serverList", servers);
    m_settings.sync();
}

QStringList AppSettings::getDXClusterList() const {
    return m_settings.value("DXCluster/serverList", QStringList()).toStringList();
}

void AppSettings::setAutoBackupEnabled(bool enabled) {
    m_settings.setValue("Backup/autoBackupEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getAutoBackupEnabled() const {
    return m_settings.value("Backup/autoBackupEnabled", false).toBool();
}

void AppSettings::setAutoBackupInterval(int qsoCount) {
    m_settings.setValue("Backup/autoBackupInterval", qsoCount);
    m_settings.sync();
}

int AppSettings::getAutoBackupInterval() const {
    return m_settings.value("Backup/autoBackupInterval", 50).toInt();
}

void AppSettings::setBackupDirectory(const QString& path) {
    m_settings.setValue("Backup/backupDirectory", path);
    m_settings.sync();
}

QString AppSettings::getBackupDirectory() const {
    // Default to ~/.tr4qt/backups
    QString defaultPath = QDir::homePath() + "/.tr4qt/backups";
    return m_settings.value("Backup/backupDirectory", defaultPath).toString();
}

void AppSettings::setMaxBackups(int count) {
    m_settings.setValue("Backup/maxBackups", count);
    m_settings.sync();
}

int AppSettings::getMaxBackups() const {
    return m_settings.value("Backup/maxBackups", 10).toInt();
}

void AppSettings::setCountryFileVersion(int version) {
    m_settings.setValue("CountryFile/version", version);
    m_settings.sync();
}

int AppSettings::getCountryFileVersion() const {
    return m_settings.value("CountryFile/version", 0).toInt();
}

void AppSettings::setCountryFilePath(const QString& path) {
    m_settings.setValue("CountryFile/path", path);
    m_settings.sync();
}

QString AppSettings::getCountryFilePath() const {
    QString defaultPath = QDir::homePath() + "/.tr4qt/cty.dat";
    return m_settings.value("CountryFile/path", defaultPath).toString();
}

// LOTW settings
void AppSettings::setShowOnlyLotwUsers(bool show) {
    m_settings.setValue("BandMap/ShowOnlyLotwUsers", show);
    m_settings.sync();
}

bool AppSettings::getShowOnlyLotwUsers() const {
    return m_settings.value("BandMap/ShowOnlyLotwUsers", false).toBool();
}

void AppSettings::setShowAllBands(bool show) {
    m_settings.setValue("BandMap/ShowAllBands", show);
    m_settings.sync();
}

bool AppSettings::getShowAllBands() const {
    return m_settings.value("BandMap/ShowAllBands", false).toBool();  // Default: false (show current band only)
}

void AppSettings::setUseMetricDistance(bool useMetric) {
    m_settings.setValue("BandMap/UseMetricDistance", useMetric);
    m_settings.sync();
}

bool AppSettings::getUseMetricDistance() const {
    return m_settings.value("BandMap/UseMetricDistance", true).toBool();  // Default: true (kilometers)
}

void AppSettings::setSpotExpirySeconds(int seconds) {
    m_settings.setValue("BandMap/SpotExpirySeconds", seconds);
    m_settings.sync();
}

int AppSettings::getSpotExpirySeconds() const {
    return m_settings.value("BandMap/SpotExpirySeconds", 600).toInt();  // Default: 10 minutes
}

void AppSettings::setNewSpotThresholdSeconds(int seconds) {
    m_settings.setValue("BandMap/NewSpotThresholdSeconds", seconds);
    m_settings.sync();
}

int AppSettings::getNewSpotThresholdSeconds() const {
    return m_settings.value("BandMap/NewSpotThresholdSeconds", 60).toInt();  // Default: 1 minute
}

void AppSettings::setAgingSpotThresholdSeconds(int seconds) {
    m_settings.setValue("BandMap/AgingSpotThresholdSeconds", seconds);
    m_settings.sync();
}

int AppSettings::getAgingSpotThresholdSeconds() const {
    return m_settings.value("BandMap/AgingSpotThresholdSeconds", 120).toInt();  // Default: 2 minutes before expiry
}

int AppSettings::getSpotRefreshIntervalMs() const {
    return m_settings.value("BandMap/RefreshIntervalMs", 5000).toInt();  // Default: 5 seconds
}

void AppSettings::setLotwLastUpdateTime(const QDateTime& timestamp) {
    m_settings.setValue("LOTW/LastUpdateTime", timestamp.toSecsSinceEpoch());
    m_settings.sync();
}

QDateTime AppSettings::getLotwLastUpdateTime() const {
    qint64 secs = m_settings.value("LOTW/LastUpdateTime", 0).toLongLong();
    if (secs > 0) {
        return QDateTime::fromSecsSinceEpoch(secs);
    }
    return QDateTime();
}

void AppSettings::setEnableLotwLookup(bool enable) {
    m_settings.setValue("LOTW/EnableLookup", enable);
    m_settings.sync();
}

bool AppSettings::getEnableLotwLookup() const {
    return m_settings.value("LOTW/EnableLookup", true).toBool();  // Default: enabled
}

void AppSettings::setLotwMinUploadMonths(int months) {
    m_settings.setValue("LOTW/MinUploadMonths", months);
    m_settings.sync();
}

int AppSettings::getLotwMinUploadMonths() const {
    return m_settings.value("LOTW/MinUploadMonths", 24).toInt();  // Default: 24 months (2 years)
}

// Appearance settings
void AppSettings::setEntryFontSize(int size) {
    m_settings.setValue("Appearance/entryFontSize", size);
    m_settings.sync();
}

int AppSettings::getEntryFontSize() const {
    return m_settings.value("Appearance/entryFontSize", 12).toInt();
}

void AppSettings::setTableFontSize(int size) {
    m_settings.setValue("Appearance/tableFontSize", size);
    m_settings.sync();
}

int AppSettings::getTableFontSize() const {
    return m_settings.value("Appearance/tableFontSize", 9).toInt();
}

void AppSettings::setGridFontSize(int size) {
    m_settings.setValue("Appearance/gridFontSize", size);
    m_settings.sync();
}

int AppSettings::getGridFontSize() const {
    return m_settings.value("Appearance/gridFontSize", 11).toInt();
}

void AppSettings::setMiscDisplayFontSize(int size) {
    m_settings.setValue("Appearance/miscDisplayFontSize", size);
    m_settings.sync();
}

int AppSettings::getMiscDisplayFontSize() const {
    return m_settings.value("Appearance/miscDisplayFontSize", 9).toInt();
}

// UDP Broadcast settings

void AppSettings::setUDPBroadcastEnabled(bool enabled) {
    m_settings.setValue("UDPBroadcast/enabled", enabled);
    m_settings.sync();
}

bool AppSettings::getUDPBroadcastEnabled() const {
    return m_settings.value("UDPBroadcast/enabled", false).toBool();
}

void AppSettings::setUDPRadioInfoEnabled(bool enabled) {
    m_settings.setValue("UDPBroadcast/radioInfoEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getUDPRadioInfoEnabled() const {
    return m_settings.value("UDPBroadcast/radioInfoEnabled", true).toBool();
}

void AppSettings::setUDPContactInfoEnabled(bool enabled) {
    m_settings.setValue("UDPBroadcast/contactInfoEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getUDPContactInfoEnabled() const {
    return m_settings.value("UDPBroadcast/contactInfoEnabled", true).toBool();
}

void AppSettings::setUDPThrottleInterval(int milliseconds) {
    m_settings.setValue("UDPBroadcast/throttleInterval", milliseconds);
    m_settings.sync();
}

int AppSettings::getUDPThrottleInterval() const {
    return m_settings.value("UDPBroadcast/throttleInterval", 500).toInt();
}

void AppSettings::setUDPDestinations(const QList<UdpDestination>& destinations) {
    m_settings.beginGroup("UDPBroadcast");
    m_settings.remove("Destinations");  // Clear old entries

    m_settings.beginWriteArray("Destinations");
    for (int i = 0; i < destinations.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("host", destinations[i].host);
        m_settings.setValue("port", destinations[i].port);
        m_settings.setValue("enabled", destinations[i].enabled);
    }
    m_settings.endArray();
    m_settings.endGroup();
    m_settings.sync();
}

QList<UdpDestination> AppSettings::getUDPDestinations() const {
    QList<UdpDestination> destinations;

    m_settings.beginGroup("UDPBroadcast");
    int size = m_settings.beginReadArray("Destinations");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        UdpDestination dest;
        dest.host = m_settings.value("host").toString();
        dest.port = m_settings.value("port").toUInt();
        dest.enabled = m_settings.value("enabled", true).toBool();
        destinations.append(dest);
    }
    m_settings.endArray();
    m_settings.endGroup();

    // If no destinations configured, return default N1MM+ localhost destination
    if (destinations.isEmpty()) {
        UdpDestination defaultDest;
        defaultDest.host = "127.0.0.1";
        defaultDest.port = 12060;  // N1MM+ RadioInfo default port
        defaultDest.enabled = true;
        destinations.append(defaultDest);
    }

    return destinations;
}

// Logging settings
void AppSettings::setLogLevel(LogLevel level) {
    m_settings.setValue("Logging/level", static_cast<int>(level));
    m_settings.sync();
}

LogLevel AppSettings::getLogLevel() const {
    int levelInt = m_settings.value("Logging/level", static_cast<int>(LogLevel::Info)).toInt();
    return static_cast<LogLevel>(levelInt);
}

void AppSettings::setFileLoggingEnabled(bool enabled) {
    m_settings.setValue("Logging/fileEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getFileLoggingEnabled() const {
    return m_settings.value("Logging/fileEnabled", true).toBool();
}

void AppSettings::setConsoleLoggingEnabled(bool enabled) {
    m_settings.setValue("Logging/consoleEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getConsoleLoggingEnabled() const {
    return m_settings.value("Logging/consoleEnabled", true).toBool();
}

void AppSettings::setLogFilePath(const QString& path) {
    m_settings.setValue("Logging/filePath", path);
    m_settings.sync();
}

QString AppSettings::getLogFilePath() const {
    QString defaultPath = QDir::homePath() + "/.tr4qt/logs/tr4qt.log";
    return m_settings.value("Logging/filePath", defaultPath).toString();
}

void AppSettings::setLogMaxFileSize(qint64 bytes) {
    m_settings.setValue("Logging/maxFileSize", bytes);
    m_settings.sync();
}

qint64 AppSettings::getLogMaxFileSize() const {
    return m_settings.value("Logging/maxFileSize", 10 * 1024 * 1024).toLongLong(); // 10 MB default
}

void AppSettings::setLogMaxBackupFiles(int count) {
    m_settings.setValue("Logging/maxBackupFiles", count);
    m_settings.sync();
}

int AppSettings::getLogMaxBackupFiles() const {
    return m_settings.value("Logging/maxBackupFiles", 5).toInt();
}

void AppSettings::setLastContestPath(const QString& path) {
    m_settings.setValue("Contest/lastContestPath", path);
    m_settings.sync();
}

QString AppSettings::getLastContestPath() const {
    return m_settings.value("Contest/lastContestPath", "").toString();
}

} // namespace TR4QT
