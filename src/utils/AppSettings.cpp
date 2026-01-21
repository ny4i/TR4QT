#include "AppSettings.h"
#include "../core/Constants.h"
#include "../network/UdpBroadcaster.h"
#include "PathManager.h"
#include "../logging/LogMacros.h"
#include <QDir>
#include <QFileInfo>

namespace TR4QT {

AppSettings& AppSettings::instance() {
    static AppSettings instance;
    return instance;
}

AppSettings::AppSettings()
    : m_settings(APP_ORG, APP_NAME)
{
    migrateLegacyPaths();
    migrateToRadioProfiles();
}

void AppSettings::migrateLegacyPaths() {
    // Check if any settings point to legacy ~/.tr4qt paths and update to platform-native paths
    QString legacyMarker = ".tr4qt";

    // Migrate log file path
    if (m_settings.contains("Logging/filePath")) {
        QString logPath = m_settings.value("Logging/filePath").toString();
        if (logPath.contains(legacyMarker)) {
            QString newPath = PathManager::getLogsDir() + "/tr4qt.log";
            m_settings.setValue("Logging/filePath", newPath);
            m_settings.sync();
        }
    }

    // Migrate backup directory
    if (m_settings.contains("Backup/backupDirectory")) {
        QString backupDir = m_settings.value("Backup/backupDirectory").toString();
        if (backupDir.contains(legacyMarker)) {
            QString newPath = PathManager::getBackupsDir();
            m_settings.setValue("Backup/backupDirectory", newPath);
            m_settings.sync();
        }
    }

    // Migrate country file path
    if (m_settings.contains("CountryFile/path")) {
        QString ctyPath = m_settings.value("CountryFile/path").toString();
        if (ctyPath.contains(legacyMarker)) {
            QString newPath = PathManager::getCountryFilePath();
            m_settings.setValue("CountryFile/path", newPath);
            m_settings.sync();
        }
    }

    // Migrate contest database path
    // Also handles nested TR4QT\TR4QT paths from incorrect organization name
    if (m_settings.contains("Contest/lastContestPath")) {
        QString contestPath = m_settings.value("Contest/lastContestPath").toString();
        // Check for legacy marker OR nested TR4QT\TR4QT pattern
        if (contestPath.contains(legacyMarker) || contestPath.contains("TR4QT/TR4QT") || contestPath.contains("TR4QT\\TR4QT")) {
            // Extract just the filename from the old path
            QFileInfo fileInfo(contestPath);
            QString filename = fileInfo.fileName();
            // Construct new path using PathManager
            QString newPath = PathManager::getLogsDir() + "/" + filename;
            m_settings.setValue("Contest/lastContestPath", newPath);
            m_settings.sync();
        }
    }
}

void AppSettings::saveRadioConfig(const RadioConfig& config) {
    m_settings.beginGroup("Radio");
    m_settings.setValue("modelId", config.hamlibModelId);
    m_settings.setValue("port", config.port);
    m_settings.setValue("baudRate", config.baudRate);
    m_settings.setValue("civAddress", config.civAddress);
    m_settings.setValue("pollInterval", config.pollInterval);
    m_settings.setValue("radioType", config.radioType);  // Save radio interface type
    m_settings.setValue("icomUsername", config.icomUsername);
    m_settings.setValue("icomPassword", config.icomPassword);
    m_settings.setValue("icomClientName", config.icomClientName);
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
    config.pollInterval = m_settings.value("pollInterval", 5000).toInt();  // Default 5s (transceive provides instant updates)
    config.radioType = m_settings.value("radioType", -1).toInt();  // Default: -1 (Auto)
    config.icomUsername = m_settings.value("icomUsername", "").toString();
    config.icomPassword = m_settings.value("icomPassword", "").toString();
    config.icomClientName = m_settings.value("icomClientName", "TR4QT").toString();
    m_settings.endGroup();
    return config;
}

bool AppSettings::hasRadioConfig() const {
    m_settings.beginGroup("Radio");
    bool hasConfig = m_settings.contains("modelId") && m_settings.contains("port");
    m_settings.endGroup();
    return hasConfig;
}

// Radio profiles (multi-config system)
void AppSettings::saveRadioProfiles(const QList<RadioProfile>& profiles) {
    m_settings.beginGroup("RadioProfiles");
    m_settings.remove("Profiles");  // Clear old entries

    m_settings.beginWriteArray("Profiles");
    for (int i = 0; i < profiles.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("name", profiles[i].name);
        m_settings.setValue("hamlibModelId", profiles[i].config.hamlibModelId);
        m_settings.setValue("port", profiles[i].config.port);
        m_settings.setValue("baudRate", profiles[i].config.baudRate);
        m_settings.setValue("dataBits", profiles[i].config.dataBits);
        m_settings.setValue("stopBits", profiles[i].config.stopBits);
        m_settings.setValue("parity", profiles[i].config.parity);
        m_settings.setValue("civAddress", profiles[i].config.civAddress);
        m_settings.setValue("pollInterval", profiles[i].config.pollInterval);
        m_settings.setValue("radioType", profiles[i].config.radioType);
        m_settings.setValue("icomUsername", profiles[i].config.icomUsername);
        m_settings.setValue("icomPassword", profiles[i].config.icomPassword);
        m_settings.setValue("icomClientName", profiles[i].config.icomClientName);
        m_settings.setValue("lastUsed", profiles[i].lastUsed);
        m_settings.setValue("notes", profiles[i].notes);
    }
    m_settings.endArray();
    m_settings.endGroup();
    m_settings.sync();
}

QList<RadioProfile> AppSettings::loadRadioProfiles() const {
    QList<RadioProfile> profiles;

    m_settings.beginGroup("RadioProfiles");
    int size = m_settings.beginReadArray("Profiles");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        RadioProfile profile;
        profile.name = m_settings.value("name").toString();
        profile.config.hamlibModelId = m_settings.value("hamlibModelId", 0).toInt();
        profile.config.port = m_settings.value("port", "").toString();
        profile.config.baudRate = m_settings.value("baudRate", 38400).toInt();
        profile.config.dataBits = m_settings.value("dataBits", 8).toInt();
        profile.config.stopBits = m_settings.value("stopBits", 1).toInt();
        profile.config.parity = m_settings.value("parity", 0).toInt();
        profile.config.civAddress = m_settings.value("civAddress", 0).toInt();
        profile.config.pollInterval = m_settings.value("pollInterval", 5000).toInt();  // Default 5s (transceive provides instant updates)
        profile.config.radioType = m_settings.value("radioType", -1).toInt();
        profile.config.icomUsername = m_settings.value("icomUsername", "").toString();
        profile.config.icomPassword = m_settings.value("icomPassword", "").toString();
        profile.config.icomClientName = m_settings.value("icomClientName", "TR4QT").toString();
        profile.lastUsed = m_settings.value("lastUsed", QDateTime()).toDateTime();
        profile.notes = m_settings.value("notes", "").toString();
        profiles.append(profile);
    }
    m_settings.endArray();
    m_settings.endGroup();

    return profiles;
}

bool AppSettings::hasRadioProfiles() const {
    m_settings.beginGroup("RadioProfiles");
    int size = m_settings.beginReadArray("Profiles");
    m_settings.endArray();
    m_settings.endGroup();
    return size > 0;
}

void AppSettings::setActiveRadioProfile(const QString& profileName) {
    m_settings.setValue("RadioProfiles/activeProfile", profileName);
    m_settings.sync();
}

QString AppSettings::getActiveRadioProfile() const {
    return m_settings.value("RadioProfiles/activeProfile", "Default").toString();
}

void AppSettings::migrateToRadioProfiles() {
    // Check if old "Radio/" group exists AND no profiles exist yet
    if (hasRadioConfig() && !hasRadioProfiles()) {
        LOG_INFO("AppSettings", "Migrating single RadioConfig to profile system");

        // Load existing single radio config
        RadioConfig oldConfig = loadRadioConfig();

        // Create "Default" profile from old config
        RadioProfile defaultProfile;
        defaultProfile.name = "Default";
        defaultProfile.config = oldConfig;
        defaultProfile.lastUsed = QDateTime::currentDateTime();
        defaultProfile.notes = "";

        // Save as profile list
        QList<RadioProfile> profiles;
        profiles.append(defaultProfile);
        saveRadioProfiles(profiles);

        // Set "Default" as active
        setActiveRadioProfile("Default");

        LOG_INFO("AppSettings", "Migration complete: created 'Default' profile");
    }
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

// ===== Amplifier Settings (Expanded for Hamlib Support) =====

void AppSettings::setAmplifierEnabled(bool enabled) {
    m_settings.setValue("Amplifier/enabled", enabled);
    m_settings.sync();
}

bool AppSettings::getAmplifierEnabled() const {
    return m_settings.value("Amplifier/enabled", false).toBool();
}

void AppSettings::setAmplifierModel(int hamlibModelId) {
    m_settings.setValue("Amplifier/model", hamlibModelId);
    m_settings.sync();
}

int AppSettings::getAmplifierModel() const {
    return m_settings.value("Amplifier/model", 1201).toInt();  // Default: 1201 (AMP_MODEL_ELECRAFT_KPA1500)
}

void AppSettings::setAmplifierConnectionType(const QString& type) {
    m_settings.setValue("Amplifier/connectionType", type);
    m_settings.sync();
}

QString AppSettings::getAmplifierConnectionType() const {
    return m_settings.value("Amplifier/connectionType", "direct").toString();  // Default: "direct" (KPA1500 UDP)
}

void AppSettings::setAmplifierPort(const QString& port) {
    m_settings.setValue("Amplifier/port", port);
    m_settings.sync();
}

QString AppSettings::getAmplifierPort() const {
    return m_settings.value("Amplifier/port", "192.168.1.100:1500").toString();  // Default: IP:port format
}

void AppSettings::setAmplifierBaudRate(int baudRate) {
    m_settings.setValue("Amplifier/baudRate", baudRate);
    m_settings.sync();
}

int AppSettings::getAmplifierBaudRate() const {
    return m_settings.value("Amplifier/baudRate", 38400).toInt();  // Default: 38400 (Hamlib default)
}

void AppSettings::setAmplifierAutoConnect(bool autoConnect) {
    m_settings.setValue("Amplifier/autoConnect", autoConnect);
    m_settings.sync();
}

bool AppSettings::getAmplifierAutoConnect() const {
    return m_settings.value("Amplifier/autoConnect", false).toBool();
}

// Legacy settings (for backward compatibility)
void AppSettings::setAmplifierIpAddress(const QString& ipAddress) {
    // For backward compatibility, also update the new port format
    QString currentPort = getAmplifierPort();
    QStringList parts = currentPort.split(":");
    int portNumber = parts.size() > 1 ? parts[1].toInt() : 1500;
    setAmplifierPort(QString("%1:%2").arg(ipAddress).arg(portNumber));
}

QString AppSettings::getAmplifierIpAddress() const {
    QString port = getAmplifierPort();
    QStringList parts = port.split(":");
    return parts.size() > 0 ? parts[0] : "";
}

void AppSettings::setAmplifierPortNumber(int port) {
    QString currentIp = getAmplifierIpAddress();
    setAmplifierPort(QString("%1:%2").arg(currentIp).arg(port));
}

int AppSettings::getAmplifierPortNumber() const {
    QString port = getAmplifierPort();
    QStringList parts = port.split(":");
    return parts.size() > 1 ? parts[1].toInt() : 1500;
}

// ===== Rotator Settings (New for Hamlib Support) =====

void AppSettings::setRotatorEnabled(bool enabled) {
    m_settings.setValue("Rotator/enabled", enabled);
    m_settings.sync();
}

bool AppSettings::getRotatorEnabled() const {
    return m_settings.value("Rotator/enabled", false).toBool();
}

void AppSettings::setRotatorModel(int hamlibModelId) {
    m_settings.setValue("Rotator/model", hamlibModelId);
    m_settings.sync();
}

int AppSettings::getRotatorModel() const {
    return m_settings.value("Rotator/model", 0).toInt();  // Default: 0 (not configured)
}

void AppSettings::setRotatorConnectionType(const QString& type) {
    m_settings.setValue("Rotator/connectionType", type);
    m_settings.sync();
}

QString AppSettings::getRotatorConnectionType() const {
    return m_settings.value("Rotator/connectionType", "direct").toString();  // Default: "direct"
}

void AppSettings::setRotatorIpAddress(const QString& ipAddress) {
    m_settings.setValue("Rotator/ipAddress", ipAddress);
    m_settings.sync();
}

QString AppSettings::getRotatorIpAddress() const {
    return m_settings.value("Rotator/ipAddress", "192.168.1.100").toString();
}

void AppSettings::setRotatorPort(int port) {
    m_settings.setValue("Rotator/port", port);
    m_settings.sync();
}

int AppSettings::getRotatorPort() const {
    return m_settings.value("Rotator/port", 12000).toInt();  // Default: 12000 (PSTRotator)
}

void AppSettings::setRotatorSerialPort(const QString& serialPort) {
    m_settings.setValue("Rotator/serialPort", serialPort);
    m_settings.sync();
}

QString AppSettings::getRotatorSerialPort() const {
    return m_settings.value("Rotator/serialPort", "").toString();
}

void AppSettings::setRotatorBaudRate(int baudRate) {
    m_settings.setValue("Rotator/baudRate", baudRate);
    m_settings.sync();
}

int AppSettings::getRotatorBaudRate() const {
    return m_settings.value("Rotator/baudRate", 9600).toInt();  // Default: 9600 (common for serial rotators)
}

void AppSettings::setRotatorAutoConnect(bool autoConnect) {
    m_settings.setValue("Rotator/autoConnect", autoConnect);
    m_settings.sync();
}

bool AppSettings::getRotatorAutoConnect() const {
    return m_settings.value("Rotator/autoConnect", false).toBool();
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

void AppSettings::setMacroLabel(int index, const QString& label) {
    m_settings.setValue(QString("Morse/macro%1_label").arg(index), label);
    m_settings.sync();
}

QString AppSettings::getMacroLabel(int index) const {
    return m_settings.value(QString("Morse/macro%1_label").arg(index), "").toString();
}

void AppSettings::setMacroCWText(int index, const QString& text) {
    m_settings.setValue(QString("Morse/macro%1_text").arg(index), text);
    m_settings.sync();
}

QString AppSettings::getMacroCWText(int index) const {
    return m_settings.value(QString("Morse/macro%1_text").arg(index), "").toString();
}

// TR4W-style CW Message defaults
static const QMap<int, QString> getDefaultCQMessages() {
    static QMap<int, QString> defaults;
    if (defaults.isEmpty()) {
        defaults[1] = "CQ WFD \\ \\ TEST";              // F1: CQ
        defaults[2] = "CQ^WFD CQ^WFD \\ \\ TEST";      // F2: CQ repeated
        defaults[3] = "";                               // F3: Empty
        defaults[4] = "TU \\ TEST";                     // F4: TU (thanks)
        defaults[5] = "";                               // F5: Empty
        defaults[6] = "DE \\";                          // F6: DE (this is)
        defaults[7] = "SRI QSO B4 TU \\ TEST";         // F7: Dupe
        defaults[8] = "AGN";                            // F8: Again
        defaults[9] = "?";                              // F9: Question
        defaults[10] = "";                              // F10: Empty
        defaults[11] = "%MM_GRABLASTCALL&";            // F11: Grab last call (Phase 4)
        defaults[12] = "";                              // F12: Empty
    }
    return defaults;
}

static const QMap<int, QString> getDefaultSPMessages() {
    static QMap<int, QString> defaults;
    if (defaults.isEmpty()) {
        defaults[1] = "Set_by_the_MY_CALL";            // F1: My call
        defaults[2] = "Set_by_S&P_EXCHANGE";           // F2: My exchange
        defaults[3] = "";                               // F3: Empty
        defaults[4] = "";                               // F4: Empty
        defaults[5] = "";                               // F5: Empty
        defaults[6] = "";                               // F6: Empty
        defaults[7] = "";                               // F7: Empty
        defaults[8] = "EE";                             // F8: EE (copy that)
        defaults[9] = "?";                              // F9: Question
        defaults[10] = "";                              // F10: Empty
        defaults[11] = "%MM_GRABLASTCALL&";            // F11: Grab last call (Phase 4)
        defaults[12] = "";                              // F12: Empty
    }
    return defaults;
}

void AppSettings::setCQMessage(int fKey, const QString& templateStr) {
    if (fKey < 1 || fKey > 12) return;
    m_settings.setValue(QString("CWMessages/CQ/F%1").arg(fKey), templateStr);
    m_settings.sync();
}

QString AppSettings::getCQMessage(int fKey) const {
    if (fKey < 1 || fKey > 12) return QString();
    QString key = QString("CWMessages/CQ/F%1").arg(fKey);
    return m_settings.value(key, getDefaultCQMessages().value(fKey, "")).toString();
}

void AppSettings::setSPMessage(int fKey, const QString& templateStr) {
    if (fKey < 1 || fKey > 12) return;
    m_settings.setValue(QString("CWMessages/SP/F%1").arg(fKey), templateStr);
    m_settings.sync();
}

QString AppSettings::getSPMessage(int fKey) const {
    if (fKey < 1 || fKey > 12) return QString();
    QString key = QString("CWMessages/SP/F%1").arg(fKey);
    return m_settings.value(key, getDefaultSPMessages().value(fKey, "")).toString();
}

void AppSettings::setCtrlFMessage(int fKey, bool cqMode, const QString& templateStr) {
    if (fKey < 1 || fKey > 12) return;
    QString mode = cqMode ? "CQ" : "SP";
    m_settings.setValue(QString("CWMessages/%1/CtrlF%2").arg(mode).arg(fKey), templateStr);
    m_settings.sync();
}

QString AppSettings::getCtrlFMessage(int fKey, bool cqMode) const {
    if (fKey < 1 || fKey > 12) return QString();
    QString mode = cqMode ? "CQ" : "SP";
    QString key = QString("CWMessages/%1/CtrlF%2").arg(mode).arg(fKey);
    return m_settings.value(key, "").toString();  // Default: empty
}

void AppSettings::setAltFMessage(int fKey, bool cqMode, const QString& templateStr) {
    if (fKey < 1 || fKey > 12) return;
    QString mode = cqMode ? "CQ" : "SP";
    m_settings.setValue(QString("CWMessages/%1/AltF%2").arg(mode).arg(fKey), templateStr);
    m_settings.sync();
}

QString AppSettings::getAltFMessage(int fKey, bool cqMode) const {
    if (fKey < 1 || fKey > 12) return QString();
    QString mode = cqMode ? "CQ" : "SP";
    QString key = QString("CWMessages/%1/AltF%2").arg(mode).arg(fKey);
    return m_settings.value(key, "").toString();  // Default: empty
}

void AppSettings::setCWAutoSendEnabled(bool enabled) {
    m_settings.setValue("CWMessages/autoSendEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getCWAutoSendEnabled() const {
    return m_settings.value("CWMessages/autoSendEnabled", true).toBool();  // Default: enabled
}

// TR4W-style Auto-Send CW Messages
void AppSettings::setCQCWExchange(const QString& message) {
    m_settings.setValue("CWMessages/CQ_CW_EXCHANGE", message);
    m_settings.sync();
}

QString AppSettings::getCQCWExchange() const {
    return m_settings.value("CWMessages/CQ_CW_EXCHANGE", "Set_by_S&P_EXCHANGE").toString();
}

void AppSettings::setSPCWExchange(const QString& message) {
    m_settings.setValue("CWMessages/SP_CW_EXCHANGE", message);
    m_settings.sync();
}

QString AppSettings::getSPCWExchange() const {
    return m_settings.value("CWMessages/SP_CW_EXCHANGE", "Set_by_S&P_EXCHANGE").toString();
}

void AppSettings::setQSLCWMessage(const QString& message) {
    m_settings.setValue("CWMessages/QSL_CW_MESSAGE", message);
    m_settings.sync();
}

QString AppSettings::getQSLCWMessage() const {
    return m_settings.value("CWMessages/QSL_CW_MESSAGE", "73 \\ WFD").toString();
}

void AppSettings::setQuickQSLCWMessage(const QString& message) {
    m_settings.setValue("CWMessages/QUICK_QSL_CW_MESSAGE", message);
    m_settings.sync();
}

QString AppSettings::getQuickQSLCWMessage() const {
    return m_settings.value("CWMessages/QUICK_QSL_CW_MESSAGE", "TU").toString();
}

void AppSettings::setQSOBeforeCWMessage(const QString& message) {
    m_settings.setValue("CWMessages/QSO_BEFORE_CW_MESSAGE", message);
    m_settings.sync();
}

QString AppSettings::getQSOBeforeCWMessage() const {
    return m_settings.value("CWMessages/QSO_BEFORE_CW_MESSAGE", "SRI QSO B4 TU \\ TEST").toString();
}

void AppSettings::setRepeatSPCWExchange(const QString& message) {
    m_settings.setValue("CWMessages/REPEAT_SP_CW_EXCHANGE", message);
    m_settings.sync();
}

QString AppSettings::getRepeatSPCWExchange() const {
    return m_settings.value("CWMessages/REPEAT_SP_CW_EXCHANGE", "Set_by_S&P_EXCHANGE Set_by_S&P_EXCHANGE").toString();
}

void AppSettings::setCQCWExchangeNameKnown(const QString& message) {
    m_settings.setValue("CWMessages/CQ_CW_EXCHANGE_NAME_KNOWN", message);
    m_settings.sync();
}

QString AppSettings::getCQCWExchangeNameKnown() const {
    return m_settings.value("CWMessages/CQ_CW_EXCHANGE_NAME_KNOWN", "").toString();
}

// TODO: Discuss implementation of CALL_OK_NOW_CW_MESSAGE
// This message is defined in TR4W but the specific trigger condition for auto-send is unclear.
// Need to determine:
// 1. When should this message be sent automatically? (callsign entry? exchange entry? specific mode?)
// 2. What distinguishes it from CQ_CW_EXCHANGE and S&P_CW_EXCHANGE?
// 3. Is it triggered by a specific key combination or workflow state?
// Default template: "! OK %" (serial + "OK" + name from database)
void AppSettings::setCallOkNowCWMessage(const QString& message) {
    m_settings.setValue("CWMessages/CALL_OK_NOW_CW_MESSAGE", message);
    m_settings.sync();
}

QString AppSettings::getCallOkNowCWMessage() const {
    return m_settings.value("CWMessages/CALL_OK_NOW_CW_MESSAGE", "! OK %").toString();
}

void AppSettings::setTailEndCWMessage(const QString& message) {
    m_settings.setValue("CWMessages/TAIL_END_CW_MESSAGE", message);
    m_settings.sync();
}

QString AppSettings::getTailEndCWMessage() const {
    return m_settings.value("CWMessages/TAIL_END_CW_MESSAGE", "").toString();
}

// CW Cut Numbers defaults (standard TR4W mapping)
static const QMap<int, QString> getDefaultShortMessages() {
    static QMap<int, QString> defaults;
    if (defaults.isEmpty()) {
        defaults[0] = "T";  // Zero = T
        defaults[1] = "A";  // One = A
        defaults[2] = "U";  // Two = U
        defaults[3] = "V";  // Three = V
        defaults[4] = "4";  // Four = 4 (no cut)
        defaults[5] = "E";  // Five = E
        defaults[6] = "6";  // Six = 6 (no cut)
        defaults[7] = "B";  // Seven = B
        defaults[8] = "D";  // Eight = D
        defaults[9] = "N";  // Nine = N
    }
    return defaults;
}

void AppSettings::setCutNumbersEnabled(bool enabled) {
    m_settings.setValue("CW/cutNumbersEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getCutNumbersEnabled() const {
    return m_settings.value("CW/cutNumbersEnabled", false).toBool();  // Default: disabled
}

void AppSettings::setShortMessage(int digit, const QString& message) {
    if (digit < 0 || digit > 9) return;
    m_settings.setValue(QString("CW/SHORT_%1").arg(digit), message);
    m_settings.sync();
}

QString AppSettings::getShortMessage(int digit) const {
    if (digit < 0 || digit > 9) return QString::number(digit);
    QString key = QString("CW/SHORT_%1").arg(digit);
    return m_settings.value(key, getDefaultShortMessages().value(digit, QString::number(digit))).toString();
}

void AppSettings::setSerialNumberWidth(int width) {
    if (width < 0 || width > 4) return;
    m_settings.setValue("CW/serialNumberWidth", width);
    m_settings.sync();
}

int AppSettings::getSerialNumberWidth() const {
    return m_settings.value("CW/serialNumberWidth", 3).toInt();  // Default: 3 digits (e.g., "002")
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

void AppSettings::setMyCounty(const QString& county) {
    m_settings.setValue("Station/county", county.toUpper());
    m_settings.sync();
}

QString AppSettings::getMyCounty() const {
    return m_settings.value("Station/county", "").toString();
}

void AppSettings::setMyFirstName(const QString& firstName) {
    m_settings.setValue("Station/firstName", firstName);
    m_settings.sync();
}

QString AppSettings::getMyFirstName() const {
    return m_settings.value("Station/firstName", "").toString();
}

void AppSettings::setMyLastName(const QString& lastName) {
    m_settings.setValue("Station/lastName", lastName);
    m_settings.sync();
}

QString AppSettings::getMyLastName() const {
    return m_settings.value("Station/lastName", "").toString();
}

void AppSettings::setComputerID(const QString& id) {
    m_settings.setValue("Network/computerID", id.toUpper());
    m_settings.sync();
}

QString AppSettings::getComputerID() const {
    return m_settings.value("Network/computerID", "A").toString();
}

void AppSettings::setLicenseClass(const QString& licenseClass) {
    m_settings.setValue("Station/licenseClass", licenseClass);
    m_settings.sync();
}

QString AppSettings::getLicenseClass() const {
    return m_settings.value("Station/licenseClass", "None").toString();
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

// Grayline Map window
void AppSettings::saveGraylineMapGeometry(const QByteArray& geometry) {
    m_settings.setValue("GraylineMapWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadGraylineMapGeometry() const {
    return m_settings.value("GraylineMapWindow/geometry").toByteArray();
}

void AppSettings::setGraylineMapVisible(bool visible) {
    m_settings.setValue("GraylineMapWindow/visible", visible);
    m_settings.sync();
}

bool AppSettings::getGraylineMapVisible() const {
    return m_settings.value("GraylineMapWindow/visible", false).toBool();
}

// Amplifier Control window
void AppSettings::saveAmplifierControlGeometry(const QByteArray& geometry) {
    m_settings.setValue("AmplifierControlWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadAmplifierControlGeometry() const {
    return m_settings.value("AmplifierControlWindow/geometry").toByteArray();
}

void AppSettings::setAmplifierControlVisible(bool visible) {
    LOG_DEBUG("AppSettings", QString("setAmplifierControlVisible(%1) - writing to QSettings").arg(visible));
    m_settings.setValue("AmplifierControlWindow/visible", visible);
    m_settings.sync();
    // Verify write
    bool readBack = m_settings.value("AmplifierControlWindow/visible", false).toBool();
    LOG_DEBUG("AppSettings", QString("Verification read-back: %1").arg(readBack));
}

bool AppSettings::getAmplifierControlVisible() const {
    bool value = m_settings.value("AmplifierControlWindow/visible", false).toBool();
    LOG_DEBUG("AppSettings", QString("getAmplifierControlVisible() - reading from QSettings: %1").arg(value));
    return value;
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
    // Default to platform-native backups directory
    QString defaultPath = PathManager::getBackupsDir();
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
    QString defaultPath = PathManager::getCountryFilePath();
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

// AUTO S&P automation settings
void AppSettings::setAutoSPEnable(bool enable) {
    m_settings.setValue("Automation/AutoSPEnable", enable);
    m_settings.sync();
}

bool AppSettings::getAutoSPEnable() const {
    return m_settings.value("Automation/AutoSPEnable", false).toBool();
}

void AppSettings::setAutoSPSensitivity(int hzPerSec) {
    m_settings.setValue("Automation/AutoSPSensitivity", hzPerSec);
    m_settings.sync();
}

int AppSettings::getAutoSPSensitivity() const {
    return m_settings.value("Automation/AutoSPSensitivity", 500).toInt();  // Default: 500 Hz/sec
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

void AppSettings::setSCPFontSize(int size) {
    m_settings.setValue("Appearance/scpFontSize", size);
    m_settings.sync();
}

int AppSettings::getSCPFontSize() const {
    return m_settings.value("Appearance/scpFontSize", 9).toInt();
}

// Band Needs Display settings

void AppSettings::setNeedsDisplayWorkedColor(const QString& color) {
    m_settings.setValue("Appearance/needsDisplayWorkedColor", color);
    m_settings.sync();
}

QString AppSettings::getNeedsDisplayWorkedColor() const {
    return m_settings.value("Appearance/needsDisplayWorkedColor", "#808080").toString();
}

void AppSettings::setNeedsDisplayNeededColor(const QString& color) {
    m_settings.setValue("Appearance/needsDisplayNeededColor", color);
    m_settings.sync();
}

QString AppSettings::getNeedsDisplayNeededColor() const {
    return m_settings.value("Appearance/needsDisplayNeededColor", "#ffaa00").toString();
}

void AppSettings::setVHFBandsEnabled(bool enabled) {
    m_settings.setValue("Appearance/vhfBandsEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getVHFBandsEnabled() const {
    return m_settings.value("Appearance/vhfBandsEnabled", false).toBool();
}

void AppSettings::setClusterDupeColor(const QString& color) {
    m_settings.setValue("Appearance/clusterDupeColor", color);
    m_settings.sync();
}

QString AppSettings::getClusterDupeColor() const {
    return m_settings.value("Appearance/clusterDupeColor", "#808080").toString();
}

void AppSettings::setClusterMultiplierColor(const QString& color) {
    m_settings.setValue("Appearance/clusterMultiplierColor", color);
    m_settings.sync();
}

QString AppSettings::getClusterMultiplierColor() const {
    return m_settings.value("Appearance/clusterMultiplierColor", "#ff0000").toString();
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

void AppSettings::setHamlibDebugEnabled(bool enabled) {
    m_settings.setValue("Logging/hamlibDebugEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getHamlibDebugEnabled() const {
    return m_settings.value("Logging/hamlibDebugEnabled", false).toBool();
}

void AppSettings::setLogFilePath(const QString& path) {
    m_settings.setValue("Logging/filePath", path);
    m_settings.sync();
}

QString AppSettings::getLogFilePath() const {
    QString defaultPath = PathManager::getLogsDir() + "/tr4qt.log";
    return m_settings.value("Logging/filePath", defaultPath).toString();
}

void AppSettings::setLogMaxFileSize(qint64 bytes) {
    m_settings.setValue("Logging/maxFileSize", bytes);
    m_settings.sync();
}

qint64 AppSettings::getLogMaxFileSize() const {
    return m_settings.value("Logging/maxFileSize", DEFAULT_MAX_LOG_FILE_SIZE).toLongLong();
}

void AppSettings::setLogMaxBackupFiles(int count) {
    m_settings.setValue("Logging/maxBackupFiles", count);
    m_settings.sync();
}

int AppSettings::getLogMaxBackupFiles() const {
    return m_settings.value("Logging/maxBackupFiles", 5).toInt();
}

void AppSettings::saveQSOTableColumnWidths(const QString& contestId, const QList<int>& widths) {
    if (contestId.isEmpty()) {
        return;  // Don't save if no contest is active
    }

    QStringList widthStrings;
    for (int width : widths) {
        widthStrings.append(QString::number(width));
    }

    // Save per-contest: ColumnWidths/CQWW, ColumnWidths/CQWPX, etc.
    QString key = QString("ColumnWidths/%1").arg(contestId);
    m_settings.setValue(key, widthStrings.join(","));
    m_settings.sync();
}

QList<int> AppSettings::loadQSOTableColumnWidths(const QString& contestId) const {
    QList<int> widths;

    if (contestId.isEmpty()) {
        return widths;  // Return empty list if no contest
    }

    // Load per-contest
    QString key = QString("ColumnWidths/%1").arg(contestId);
    QString widthString = m_settings.value(key, "").toString();

    if (!widthString.isEmpty()) {
        QStringList widthStrings = widthString.split(",");
        for (const QString& w : widthStrings) {
            widths.append(w.toInt());
        }
    }

    return widths;
}

void AppSettings::setLastContestPath(const QString& path) {
    m_settings.setValue("Contest/lastContestPath", path);
    m_settings.sync();
}

QString AppSettings::getLastContestPath() const {
    return m_settings.value("Contest/lastContestPath", "").toString();
}

// Web server settings

void AppSettings::setWebServerAutoStart(bool autoStart) {
    m_settings.setValue("WebServer/autoStart", autoStart);
    m_settings.sync();
}

bool AppSettings::getWebServerAutoStart() const {
    return m_settings.value("WebServer/autoStart", false).toBool();  // Default: disabled
}

void AppSettings::setWebServerPort(quint16 port) {
    m_settings.setValue("WebServer/port", port);
    m_settings.sync();
}

quint16 AppSettings::getWebServerPort() const {
    return m_settings.value("WebServer/port", 14140).toUInt();  // Default: 14140
}

void AppSettings::setWebServerAddress(const QString& address) {
    m_settings.setValue("WebServer/address", address);
    m_settings.sync();
}

QString AppSettings::getWebServerAddress() const {
    return m_settings.value("WebServer/address", "127.0.0.1").toString();  // Default: localhost
}

// Super Check Partial (SCP) settings

void AppSettings::setSCPEnabled(bool enabled) {
    m_settings.setValue("SCP/enabled", enabled);
    m_settings.sync();
}

bool AppSettings::getSCPEnabled() const {
    return m_settings.value("SCP/enabled", true).toBool();  // Default: enabled
}

void AppSettings::setSCPVersion(const QString& version) {
    m_settings.setValue("SCP/version", version);
    m_settings.sync();
}

QString AppSettings::getSCPVersion() const {
    return m_settings.value("SCP/version", "").toString();
}

void AppSettings::setSCPLastUpdate(const QDateTime& dt) {
    m_settings.setValue("SCP/lastUpdate", dt);
    m_settings.sync();
}

QDateTime AppSettings::getSCPLastUpdate() const {
    return m_settings.value("SCP/lastUpdate", QDateTime()).toDateTime();
}

void AppSettings::setSCPIncludeLocalLogs(bool include) {
    m_settings.setValue("SCP/includeLocalLogs", include);
    m_settings.sync();
}

bool AppSettings::getSCPIncludeLocalLogs() const {
    return m_settings.value("SCP/includeLocalLogs", true).toBool();  // Default: include local logs
}

QString AppSettings::getValue(const QString& key, const QString& defaultValue) const {
    return m_settings.value(key, defaultValue).toString();
}

} // namespace TR4QT
