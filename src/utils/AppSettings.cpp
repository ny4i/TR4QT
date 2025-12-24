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

} // namespace TR4QT
