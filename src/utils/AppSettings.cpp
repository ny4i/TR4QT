#include "AppSettings.h"
#include "../core/Constants.h"
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

} // namespace TR4QT
