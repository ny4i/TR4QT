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

#include "AppSettings.h"
#include "CredentialStore.h"
#include "../core/Constants.h"
#include "../network/UdpBroadcaster.h"
#include "PathManager.h"
#include "../logging/LogMacros.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QDateTime>
#include <algorithm>

namespace {

/**
 * RAII scope guard for QSettings::beginGroup/endGroup
 * Ensures endGroup() is called even if an exception is thrown
 */
class QSettingsGroupGuard {
public:
    explicit QSettingsGroupGuard(QSettings& settings, const QString& group)
        : m_settings(settings) {
        m_settings.beginGroup(group);
    }
    ~QSettingsGroupGuard() {
        m_settings.endGroup();
    }
    // Non-copyable
    QSettingsGroupGuard(const QSettingsGroupGuard&) = delete;
    QSettingsGroupGuard& operator=(const QSettingsGroupGuard&) = delete;
private:
    QSettings& m_settings;
};

/**
 * RAII scope guard for QSettings::beginReadArray/endArray
 * Ensures endArray() is called even if an exception is thrown
 */
class QSettingsReadArrayGuard {
public:
    explicit QSettingsReadArrayGuard(QSettings& settings, const QString& prefix)
        : m_settings(settings) {
        m_size = m_settings.beginReadArray(prefix);
    }
    ~QSettingsReadArrayGuard() {
        m_settings.endArray();
    }
    int size() const { return m_size; }
    // Non-copyable
    QSettingsReadArrayGuard(const QSettingsReadArrayGuard&) = delete;
    QSettingsReadArrayGuard& operator=(const QSettingsReadArrayGuard&) = delete;
private:
    QSettings& m_settings;
    int m_size{0};
};

/**
 * RAII scope guard for QSettings::beginWriteArray/endArray
 * Ensures endArray() is called even if an exception is thrown
 */
class QSettingsWriteArrayGuard {
public:
    explicit QSettingsWriteArrayGuard(QSettings& settings, const QString& prefix, int size = -1)
        : m_settings(settings) {
        m_settings.beginWriteArray(prefix, size);
    }
    ~QSettingsWriteArrayGuard() {
        m_settings.endArray();
    }
    // Non-copyable
    QSettingsWriteArrayGuard(const QSettingsWriteArrayGuard&) = delete;
    QSettingsWriteArrayGuard& operator=(const QSettingsWriteArrayGuard&) = delete;
private:
    QSettings& m_settings;
};

// Backward-compatible alias
using QSettingsArrayGuard = QSettingsReadArrayGuard;

/**
 * Helper to ensure QSettings is not stuck in a group/array context
 * Call this at the start of load functions to ensure clean state
 */
void ensureCleanSettingsState(QSettings& settings) {
    QString currentGroup = settings.group();
    if (!currentGroup.isEmpty()) {
        LOG_ERROR("AppSettings", QString("QSettings STUCK in group '%1' - escaping before read").arg(currentGroup));
        int maxAttempts = 10;
        int attempts = 0;
        while (!settings.group().isEmpty() && attempts < maxAttempts) {
            QString before = settings.group();
            settings.endArray();
            if (settings.group() == before) {
                settings.endGroup();
            }
            attempts++;
        }
        if (settings.group().isEmpty()) {
            LOG_INFO("AppSettings", QString("Successfully escaped group context after %1 escapes").arg(attempts));
        }
    }
}

} // anonymous namespace

namespace TR4QT {

AppSettings& AppSettings::instance() {
    static AppSettings instance;
    return instance;
}

AppSettings::AppSettings()
    : QObject(nullptr)
    , m_settings(APP_ORG, APP_NAME)
{
    // Force sync from disk immediately to ensure we have fresh values
    // This helps prevent cfprefsd cache issues on macOS
    m_settings.sync();

    migrateLegacyPaths();
    migrateToRadioProfiles();
    migrateToStationProfiles();
    migrateToCWOutputProfiles();

    // Verify settings integrity on startup
    if (!verifySettingsIntegrity()) {
        LOG_WARN("AppSettings", "Settings file may be corrupted or missing, attempting to restore from backup");
        if (restoreFromBackup()) {
            LOG_INFO("AppSettings", "Successfully restored settings from backup");
            // Re-read settings after restore
            m_settings.sync();
        } else {
            LOG_WARN("AppSettings", "No backup available, starting with default settings");
        }
    }

    LOG_INFO("AppSettings", QString("Settings file location: %1").arg(getSettingsFilePath()));

    // Debug: Log grid square value at initialization
    QString gridAtInit = m_settings.value("Station/gridSquare", "").toString();
    QString callAtInit = m_settings.value("Station/callsign", "").toString();
    int keyCount = m_settings.allKeys().size();
    LOG_INFO("AppSettings", QString("INIT: gridSquare='%1', callsign='%2', status=%3, keyCount=%4")
        .arg(gridAtInit)
        .arg(callAtInit)
        .arg(static_cast<int>(m_settings.status()))
        .arg(keyCount));

    // Debug: If key count is suspiciously low, log what keys we DO have
    if (keyCount < 50) {
        QStringList keys = m_settings.allKeys();
        LOG_WARN("AppSettings", QString("LOW KEY COUNT! Keys found: %1").arg(keys.join(", ")));
        LOG_WARN("AppSettings", QString("Settings file path: %1").arg(m_settings.fileName()));
    }
}

AppSettings::~AppSettings() {
    stopAutoSave();
    forceSync();
}

void AppSettings::startAutoSave() {
    if (m_autoSaveTimer) {
        return;  // Already started
    }

    // Migrate plain-text passwords to secure store (requires QApplication event loop)
    migrateCredentialsToSecureStore();

    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &AppSettings::onAutoSaveTimer);
    m_autoSaveTimer->start(AUTO_SAVE_INTERVAL_MS);

    LOG_INFO("AppSettings", QString("Auto-save timer started (interval: %1 seconds)")
             .arg(AUTO_SAVE_INTERVAL_MS / 1000));
}

void AppSettings::stopAutoSave() {
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
        delete m_autoSaveTimer;
        m_autoSaveTimer = nullptr;
        LOG_DEBUG("AppSettings", "Auto-save timer stopped");
    }
}

void AppSettings::onAutoSaveTimer() {
    LOG_DEBUG("AppSettings", "Auto-save timer fired, syncing settings to disk");
    forceSync();
}

void AppSettings::forceSync() {
    // Create backup before sync (in case sync corrupts the file)
    createBackup();

    // Force sync to disk
    m_settings.sync();

    // Verify the sync succeeded
    QSettings::Status status = m_settings.status();
    if (status != QSettings::NoError) {
        LOG_ERROR("AppSettings", QString("Settings sync failed with status: %1").arg(static_cast<int>(status)));
    } else {
        LOG_DEBUG("AppSettings", "Settings synced to disk successfully");
    }
}

bool AppSettings::createBackup() {
    QString settingsPath = getSettingsFilePath();
    if (settingsPath.isEmpty() || !QFile::exists(settingsPath)) {
        return false;  // Nothing to backup
    }

    QString backupDir = getSettingsBackupDir();
    QDir().mkpath(backupDir);

    // Create timestamped backup filename
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backupPath = QString("%1/settings_backup_%2.plist").arg(backupDir, timestamp);

    // Copy current settings file to backup
    if (QFile::copy(settingsPath, backupPath)) {
        LOG_DEBUG("AppSettings", QString("Created settings backup: %1").arg(backupPath));

        // Clean up old backups (keep only MAX_BACKUP_FILES)
        QDir backupDirObj(backupDir);
        QStringList backups = backupDirObj.entryList(QStringList() << "settings_backup_*.plist",
                                                      QDir::Files, QDir::Time);
        while (backups.size() > MAX_BACKUP_FILES) {
            QString oldestBackup = backups.takeLast();
            QFile::remove(backupDir + "/" + oldestBackup);
            LOG_DEBUG("AppSettings", QString("Removed old backup: %1").arg(oldestBackup));
        }

        return true;
    } else {
        LOG_WARN("AppSettings", QString("Failed to create settings backup to: %1").arg(backupPath));
        return false;
    }
}

bool AppSettings::restoreFromBackup() {
    QString backupDir = getSettingsBackupDir();
    QDir backupDirObj(backupDir);

    // Find most recent backup
    QStringList backups = backupDirObj.entryList(QStringList() << "settings_backup_*.plist",
                                                  QDir::Files, QDir::Time);
    if (backups.isEmpty()) {
        LOG_WARN("AppSettings", "No backup files found");
        return false;
    }

    QString newestBackup = backupDir + "/" + backups.first();
    QString settingsPath = getSettingsFilePath();

    if (settingsPath.isEmpty()) {
        LOG_ERROR("AppSettings", "Cannot determine settings file path for restore");
        return false;
    }

    // Remove corrupted settings file
    if (QFile::exists(settingsPath)) {
        QFile::remove(settingsPath);
    }

    // Copy backup to settings location
    if (QFile::copy(newestBackup, settingsPath)) {
        LOG_INFO("AppSettings", QString("Restored settings from backup: %1").arg(newestBackup));
        return true;
    } else {
        LOG_ERROR("AppSettings", QString("Failed to restore settings from backup: %1").arg(newestBackup));
        return false;
    }
}

bool AppSettings::verifySettingsIntegrity() const {
    // Debug: Log what we see at integrity check time
    QString gridAtIntegrity = m_settings.value("Station/gridSquare", "").toString();
    QString callAtIntegrity = m_settings.value("Station/callsign", "").toString();
    LOG_INFO("AppSettings", QString("INTEGRITY CHECK: gridSquare='%1', callsign='%2', keyCount=%3")
        .arg(gridAtIntegrity)
        .arg(callAtIntegrity)
        .arg(m_settings.allKeys().size()));

    // Check if QSettings can read without errors
    QSettings::Status status = m_settings.status();
    if (status != QSettings::NoError) {
        LOG_WARN("AppSettings", QString("QSettings status error: %1").arg(static_cast<int>(status)));
        return false;
    }

    // Check if we can read at least one known key
    // If the settings file is corrupted, this will fail
    QVariant testValue = m_settings.value("Station/callsign", QVariant());

    // The file exists but we got an error reading it
    QString settingsPath = getSettingsFilePath();
    if (!settingsPath.isEmpty() && QFile::exists(settingsPath)) {
        QFile file(settingsPath);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_WARN("AppSettings", QString("Cannot read settings file: %1").arg(settingsPath));
            return false;
        }

        // Check if file is not empty but QSettings found no keys
        qint64 fileSize = file.size();
        file.close();

        if (fileSize > 0 && m_settings.allKeys().isEmpty()) {
            LOG_WARN("AppSettings", "Settings file exists but contains no readable keys - may be corrupted");
            return false;
        }
    }

    return true;
}

QString AppSettings::getSettingsFilePath() const {
#ifdef Q_OS_MACOS
    // macOS uses plist files in ~/Library/Preferences/
    // Format: com.organization.appname.plist (organization is empty, so just com.TR4QT.plist)
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return homePath + "/Library/Preferences/com." + QString(APP_ORG).toLower()
           + "." + QString(APP_NAME) + ".plist";
#elif defined(Q_OS_WIN)
    // Windows uses registry, but we can get the INI file path if using IniFormat
    // For native format, return empty (registry-based)
    return QString();
#else
    // Linux uses ~/.config/
    return m_settings.fileName();
#endif
}

QString AppSettings::getSettingsBackupDir() const {
    return PathManager::getAppDataDir() + "/settings_backups";
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
    {
        QSettingsGroupGuard groupGuard(m_settings, "Radio");
        m_settings.setValue("modelId", config.hamlibModelId);
        m_settings.setValue("port", config.port);
        m_settings.setValue("baudRate", config.baudRate);
        m_settings.setValue("civAddress", config.civAddress);
        m_settings.setValue("pollInterval", config.pollInterval);
        m_settings.setValue("radioType", config.radioType);  // Save radio interface type
        m_settings.setValue("icomUsername", config.icomUsername);
        m_settings.setValue("icomClientName", config.icomClientName);
    }

    {
        QSettingsGroupGuard groupGuard(m_settings, "Radio");
        savePasswordSecurely(CredentialKeys::ICOM_RADIO, config.icomUsername,
                             config.icomPassword, "icomPassword");
    }

    m_settings.sync();
}

RadioConfig AppSettings::loadRadioConfig() const {
    RadioConfig config;
    {
        QSettingsGroupGuard groupGuard(m_settings, "Radio");
        config.hamlibModelId = m_settings.value("modelId", 0).toInt();
        config.port = m_settings.value("port", "").toString();
        config.baudRate = m_settings.value("baudRate", 38400).toInt();
        config.civAddress = m_settings.value("civAddress", 0).toInt();
        config.pollInterval = m_settings.value("pollInterval", 5000).toInt();  // Default 5s (transceive provides instant updates)
        config.radioType = m_settings.value("radioType", -1).toInt();  // Default: -1 (Auto)
        config.icomUsername = m_settings.value("icomUsername", "").toString();
        config.icomClientName = m_settings.value("icomClientName", "TR4QT").toString();
    }

    {
        QSettingsGroupGuard groupGuard(m_settings, "Radio");
        config.icomPassword = loadPasswordSecurely(
            CredentialKeys::ICOM_RADIO, config.icomUsername, "icomPassword");
    }

    return config;
}

bool AppSettings::hasRadioConfig() const {
    QSettingsGroupGuard groupGuard(m_settings, "Radio");
    bool hasConfig = m_settings.contains("modelId") && m_settings.contains("port");
    return hasConfig;
}

// Radio profiles (multi-config system)
void AppSettings::saveRadioProfiles(const QList<RadioProfile>& profiles) {
    {
        QSettingsGroupGuard groupGuard(m_settings, "RadioProfiles");
        m_settings.remove("Profiles");  // Clear old entries

        QSettingsWriteArrayGuard arrayGuard(m_settings, "Profiles");
        for (int i = 0; i < profiles.size(); ++i) {
            m_settings.setArrayIndex(i);
            m_settings.setValue("name", profiles[i].name);
            m_settings.setValue("hamlibModelId", profiles[i].config.hamlibModelId);
            m_settings.setValue("port", profiles[i].config.port);
            m_settings.setValue("baudRate", profiles[i].config.baudRate);
            m_settings.setValue("dataBits", profiles[i].config.dataBits);
            m_settings.setValue("stopBits", profiles[i].config.stopBits);
            m_settings.setValue("parity", profiles[i].config.parity);
            m_settings.setValue("handshake", profiles[i].config.handshake);
            m_settings.setValue("dtrState", profiles[i].config.dtrState);
            m_settings.setValue("rtsState", profiles[i].config.rtsState);
            m_settings.setValue("civAddress", profiles[i].config.civAddress);
            m_settings.setValue("pollInterval", profiles[i].config.pollInterval);
            m_settings.setValue("radioType", profiles[i].config.radioType);
            m_settings.setValue("icomUsername", profiles[i].config.icomUsername);
            m_settings.setValue("icomClientName", profiles[i].config.icomClientName);
            m_settings.setValue("lastUsed", profiles[i].lastUsed);
            m_settings.setValue("notes", profiles[i].notes);

            savePasswordSecurely(CredentialKeys::icomRadioProfile(profiles[i].name),
                                 profiles[i].config.icomUsername,
                                 profiles[i].config.icomPassword, "icomPassword");
        }
        // Guards automatically call endArray() and endGroup() on scope exit
    }
    m_settings.sync();
}

QList<RadioProfile> AppSettings::loadRadioProfiles() const {
    QList<RadioProfile> profiles;

    // Ensure QSettings is in a clean state before reading
    ensureCleanSettingsState(m_settings);

    // Use RAII guards to ensure endGroup/endArray are always called
    // This prevents QSettings from being stuck in wrong group if loadPasswordSecurely throws
    QSettingsGroupGuard groupGuard(m_settings, "RadioProfiles");
    QSettingsArrayGuard arrayGuard(m_settings, "Profiles");

    for (int i = 0; i < arrayGuard.size(); ++i) {
        m_settings.setArrayIndex(i);
        RadioProfile profile;
        profile.name = m_settings.value("name").toString();
        profile.config.hamlibModelId = m_settings.value("hamlibModelId", 0).toInt();
        profile.config.port = m_settings.value("port", "").toString();
        profile.config.baudRate = m_settings.value("baudRate", 38400).toInt();
        profile.config.dataBits = m_settings.value("dataBits", 8).toInt();
        profile.config.stopBits = m_settings.value("stopBits", 1).toInt();
        profile.config.parity = m_settings.value("parity", 0).toInt();
        profile.config.handshake = m_settings.value("handshake", 0).toInt();
        profile.config.dtrState = m_settings.value("dtrState", 0).toInt();
        profile.config.rtsState = m_settings.value("rtsState", 0).toInt();
        profile.config.civAddress = m_settings.value("civAddress", 0).toInt();
        profile.config.pollInterval = m_settings.value("pollInterval", 5000).toInt();  // Default 5s (transceive provides instant updates)
        profile.config.radioType = m_settings.value("radioType", -1).toInt();
        profile.config.icomUsername = m_settings.value("icomUsername", "").toString();
        profile.config.icomClientName = m_settings.value("icomClientName", "TR4QT").toString();
        profile.lastUsed = m_settings.value("lastUsed", QDateTime()).toDateTime();
        profile.notes = m_settings.value("notes", "").toString();

        profile.config.icomPassword = loadPasswordSecurely(
            CredentialKeys::icomRadioProfile(profile.name),
            profile.config.icomUsername, "icomPassword");

        profiles.append(profile);
    }
    // Guards automatically call endArray() and endGroup() on scope exit

    return profiles;
}

bool AppSettings::hasRadioProfiles() const {
    ensureCleanSettingsState(m_settings);
    QSettingsGroupGuard groupGuard(m_settings, "RadioProfiles");
    QSettingsArrayGuard arrayGuard(m_settings, "Profiles");
    return arrayGuard.size() > 0;
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

void AppSettings::migrateToStationProfiles() {
    // Check if StationProfiles already exist (without loading full profiles)
    // This avoids calling CredentialStore before QApplication is created
    int stationProfileCount;
    {
        QSettingsGroupGuard groupGuard(m_settings, "StationProfiles");
        QSettingsArrayGuard arrayGuard(m_settings, "Profiles");
        stationProfileCount = arrayGuard.size();
    }

    if (stationProfileCount > 0) {
        return;  // Already migrated
    }

    // Check if we have RadioProfiles to migrate from (without loading passwords)
    if (!hasRadioProfiles()) {
        return;  // Nothing to migrate
    }

    LOG_INFO("AppSettings", "Migrating to StationProfile system");

    // Create a "Default" station profile
    StationProfile defaultStation;
    defaultStation.name = "Default";

    // Check if SO2R was enabled with old system
    bool legacySO2R = isSO2REnabled();
    if (legacySO2R) {
        // Use the old SO2R radio assignments
        QString radio1Name = getSO2RRadioProfile(0);  // Radio 1
        QString radio2Name = getSO2RRadioProfile(1);  // Radio 2

        defaultStation.radio1Name = radio1Name;
        defaultStation.radio2Name = radio2Name;
        defaultStation.so2rEnabled = true;
        defaultStation.defaultActive = 0;  // Radio 1 is default

        LOG_INFO("AppSettings", QString("Migrated SO2R config: Radio1='%1', Radio2='%2'")
                 .arg(radio1Name).arg(radio2Name));
    } else {
        // Single-radio mode: assign the active profile to Radio 1
        QString activeProfileName = getActiveRadioProfile();
        defaultStation.radio1Name = activeProfileName;
        defaultStation.radio2Name = "";  // No Radio 2
        defaultStation.so2rEnabled = false;
        defaultStation.defaultActive = 0;

        LOG_INFO("AppSettings", QString("Migrated single-radio config: Radio1='%1'")
                 .arg(activeProfileName));
    }

    // Save the new station profile
    QList<StationProfile> stationProfiles;
    stationProfiles.append(defaultStation);
    saveStationProfiles(stationProfiles);

    // Set as active
    setActiveStationProfile("Default");

    LOG_INFO("AppSettings", "StationProfile migration complete");
}

bool AppSettings::savePasswordSecurely(const QString& storageKey, const QString& username,
                                       const QString& password, const QString& settingsKey) {
    if (password.isEmpty()) {
        // Empty password — remove from both stores
        m_settings.remove(settingsKey);
        CredentialStore::instance().deletePassword(storageKey, username);
        return true;
    }

    int rc = CredentialStore::instance().savePassword(storageKey, username, password);
    if (rc == 0) {
        // Secure save succeeded — remove plain-text from QSettings
        m_settings.remove(settingsKey);
        return true;
    }

    // Fallback: keep password in QSettings so it isn't lost
    LOG_WARN("AppSettings",
             QString("Credential store unavailable for '%1', keeping password in QSettings")
                 .arg(storageKey));
    m_settings.setValue(settingsKey, password);
    return false;
}

QString AppSettings::loadPasswordSecurely(const QString& storageKey, const QString& username,
                                          const QString& settingsKey) const {
    // Try secure credential store first
    QString password = CredentialStore::instance().getPassword(storageKey, username);
    if (!password.isEmpty()) {
        return password;
    }

    // Fallback: try legacy plaintext QSettings (pre-migration or credential store unavailable)
    return m_settings.value(settingsKey, "").toString();
}

void AppSettings::migrateCredentialsToSecureStore() {
    // Gate flag: skip if already migrated
    if (m_settings.value("Security/credentialsMigrated", false).toBool()) {
        return;
    }

    LOG_INFO("AppSettings", "Starting credential migration to secure store");
    bool allMigrated = true;

    // Migrate radio profiles - use RAII guards for exception safety
    if (hasRadioProfiles()) {
        {
            QSettingsGroupGuard groupGuard(m_settings, "RadioProfiles");
            QSettingsArrayGuard arrayGuard(m_settings, "Profiles");

            for (int i = 0; i < arrayGuard.size(); ++i) {
                m_settings.setArrayIndex(i);
                QString profileName = m_settings.value("name").toString();
                QString password = m_settings.value("icomPassword", "").toString();
                QString username = m_settings.value("icomUsername", "").toString();

                if (!password.isEmpty()) {
                    const QString storageKey = CredentialKeys::icomRadioProfile(profileName);
                    int rc = CredentialStore::instance().savePassword(storageKey, username, password);
                    if (rc == 0) {
                        // Verify round-trip before removing plain text
                        QString readBack = CredentialStore::instance().getPassword(storageKey, username);
                        if (readBack == password) {
                            LOG_INFO("AppSettings",
                                     QString("Migrated password for profile '%1' to secure store").arg(profileName));
                            // Note: can't remove individual array values inside beginReadArray,
                            // removal happens in a second pass below
                        } else {
                            LOG_WARN("AppSettings",
                                     QString("Round-trip verification failed for profile '%1', keeping in QSettings")
                                         .arg(profileName));
                            allMigrated = false;
                        }
                    } else {
                        LOG_WARN("AppSettings",
                                 QString("Failed to save profile '%1' password to secure store, keeping in QSettings")
                                     .arg(profileName));
                        allMigrated = false;
                    }
                }
            }
            // Guards automatically call endArray() and endGroup() on scope exit
        }

        // Second pass: remove plain-text passwords from successfully migrated profiles
        // We need to re-read and re-write the array to remove individual values
        if (allMigrated) {
            QList<RadioProfile> profiles = loadRadioProfiles();
            // loadRadioProfiles already prefers credential store; just re-save
            // which won't write icomPassword to QSettings
            saveRadioProfiles(profiles);
            LOG_INFO("AppSettings", "Removed plain-text passwords from profile QSettings");
        }
    }

    // Migrate legacy single radio config - use RAII guard
    QString legacyPassword;
    QString legacyUsername;
    {
        QSettingsGroupGuard groupGuard(m_settings, "Radio");
        legacyPassword = m_settings.value("icomPassword", "").toString();
        legacyUsername = m_settings.value("icomUsername", "").toString();
    }

    if (!legacyPassword.isEmpty()) {
        int rc = CredentialStore::instance().savePassword(CredentialKeys::ICOM_RADIO, legacyUsername, legacyPassword);
        if (rc == 0) {
            QString readBack = CredentialStore::instance().getPassword(CredentialKeys::ICOM_RADIO, legacyUsername);
            if (readBack == legacyPassword) {
                {
                    QSettingsGroupGuard groupGuard(m_settings, "Radio");
                    m_settings.remove("icomPassword");
                }
                m_settings.sync();
                LOG_INFO("AppSettings", "Migrated legacy radio password to secure store");
            } else {
                LOG_WARN("AppSettings", "Round-trip verification failed for legacy radio password");
                allMigrated = false;
            }
        } else {
            LOG_WARN("AppSettings", "Failed to save legacy radio password to secure store");
            allMigrated = false;
        }
    }

    // Set gate flag only if ALL passwords migrated successfully
    if (allMigrated) {
        m_settings.setValue("Security/credentialsMigrated", true);
        m_settings.sync();
        LOG_INFO("AppSettings", "Credential migration complete");
    } else {
        LOG_WARN("AppSettings", "Partial credential migration — will retry on next launch");
    }
}

RadioConfig AppSettings::getActiveRadioConfig() const {
    // Check profiles first (new system), then fall back to legacy
    if (hasRadioProfiles()) {
        QList<RadioProfile> profiles = loadRadioProfiles();
        QString activeProfileName = getActiveRadioProfile();

        for (const RadioProfile& profile : profiles) {
            if (profile.name == activeProfileName) {
                return profile.config;
            }
        }

        // Active profile not found - use first profile if available
        if (!profiles.isEmpty()) {
            return profiles.first().config;
        }
    }

    // Fall back to legacy single-radio config
    if (hasRadioConfig()) {
        return loadRadioConfig();
    }

    // No config at all - return default empty config
    return RadioConfig();
}

bool AppSettings::hasAnyRadioConfig() const {
    return hasRadioProfiles() || hasRadioConfig();
}

void AppSettings::setRadioAutoConnect(bool autoConnect) {
    m_settings.setValue("Radio/autoConnect", autoConnect);
    m_settings.sync();
}

bool AppSettings::getRadioAutoConnect() const {
    return m_settings.value("Radio/autoConnect", true).toBool();  // Default: true
}

// ===== SO2R (Single Operator Two Radio) Settings =====

void AppSettings::setSO2REnabled(bool enabled) {
    m_settings.setValue("SO2R/enabled", enabled);
    m_settings.sync();
}

bool AppSettings::isSO2REnabled() const {
    return m_settings.value("SO2R/enabled", false).toBool();  // Default: false (single radio)
}

void AppSettings::setSO2RRadioProfile(int slot, const QString& profileName) {
    if (slot < 0 || slot > 1) return;  // Only slots 0 and 1 supported
    QString key = QString("SO2R/radio%1Profile").arg(slot + 1);  // radio1Profile, radio2Profile
    m_settings.setValue(key, profileName);
    m_settings.sync();
}

QString AppSettings::getSO2RRadioProfile(int slot) const {
    if (slot < 0 || slot > 1) return QString();
    QString key = QString("SO2R/radio%1Profile").arg(slot + 1);

    // Default: Radio 1 uses the active profile, Radio 2 is empty
    if (slot == 0) {
        return m_settings.value(key, getActiveRadioProfile()).toString();
    }
    return m_settings.value(key, QString()).toString();
}

// ===== Station Profiles (Groups of Radios) =====

void AppSettings::saveStationProfiles(const QList<StationProfile>& profiles) {
    {
        QSettingsGroupGuard groupGuard(m_settings, "StationProfiles");
        m_settings.remove("Profiles");  // Clear old entries

        QSettingsWriteArrayGuard arrayGuard(m_settings, "Profiles");
        for (int i = 0; i < profiles.size(); ++i) {
            m_settings.setArrayIndex(i);
            m_settings.setValue("name", profiles[i].name);
            m_settings.setValue("radio1Name", profiles[i].radio1Name);
            m_settings.setValue("radio2Name", profiles[i].radio2Name);
            m_settings.setValue("cw1Name", profiles[i].cw1Name);
            m_settings.setValue("cw2Name", profiles[i].cw2Name);
            m_settings.setValue("defaultActive", profiles[i].defaultActive);
            m_settings.setValue("so2rEnabled", profiles[i].so2rEnabled);
        }
        // Guards automatically call endArray() and endGroup() on scope exit
    }
    m_settings.sync();

    LOG_DEBUG("AppSettings", QString("Saved %1 station profile(s)").arg(profiles.size()));
}

QList<StationProfile> AppSettings::loadStationProfiles() const {
    QList<StationProfile> profiles;

    // Ensure QSettings is in a clean state before reading
    ensureCleanSettingsState(m_settings);

    QSettingsGroupGuard groupGuard(m_settings, "StationProfiles");
    QSettingsArrayGuard arrayGuard(m_settings, "Profiles");

    for (int i = 0; i < arrayGuard.size(); ++i) {
        m_settings.setArrayIndex(i);
        StationProfile profile;
        profile.name = m_settings.value("name").toString();
        profile.radio1Name = m_settings.value("radio1Name").toString();
        profile.radio2Name = m_settings.value("radio2Name").toString();
        profile.cw1Name = m_settings.value("cw1Name").toString();
        profile.cw2Name = m_settings.value("cw2Name").toString();
        profile.defaultActive = m_settings.value("defaultActive", 0).toInt();
        profile.so2rEnabled = m_settings.value("so2rEnabled", false).toBool();
        profiles.append(profile);
    }
    // Guards automatically call endArray() and endGroup() on scope exit

    return profiles;
}

void AppSettings::setActiveStationProfile(const QString& profileName) {
    m_settings.setValue("StationProfiles/activeProfile", profileName);
    m_settings.sync();
    LOG_DEBUG("AppSettings", QString("Set active station profile: %1").arg(profileName));
}

QString AppSettings::getActiveStationProfile() const {
    return m_settings.value("StationProfiles/activeProfile", "Default").toString();
}

StationProfile AppSettings::getStationProfile(const QString& name) const {
    QList<StationProfile> profiles = loadStationProfiles();
    for (const StationProfile& profile : profiles) {
        if (profile.name == name) {
            return profile;
        }
    }
    // Not found - return empty profile
    return StationProfile();
}

// === CW Output Profiles ===

void AppSettings::saveCWOutputProfiles(const QList<CWOutputProfile>& profiles) {
    {
        QSettingsGroupGuard groupGuard(m_settings, "CWOutputProfiles");
        m_settings.remove("Profiles");  // Clear old entries

        QSettingsWriteArrayGuard arrayGuard(m_settings, "Profiles");
        for (int i = 0; i < profiles.size(); ++i) {
            m_settings.setArrayIndex(i);
            m_settings.setValue("name", profiles[i].name);
            m_settings.setValue("type", static_cast<int>(profiles[i].type));
            // WinKeyer settings (type == KeyerDevice)
            m_settings.setValue("winKeyerPortName", profiles[i].winKeyerPortName);
            m_settings.setValue("weighting", profiles[i].weighting);
            m_settings.setValue("leadInTime", profiles[i].leadInTime);
            m_settings.setValue("tailTime", profiles[i].tailTime);
            // DTR/RTS settings
            m_settings.setValue("dtrRtsPortName", profiles[i].dtrRtsPortName);
            m_settings.setValue("dtrRtsPin", static_cast<int>(profiles[i].dtrRtsPin));
        }
    }
    m_settings.sync();

    LOG_DEBUG("AppSettings", QString("Saved %1 CW output profile(s)").arg(profiles.size()));
}

QList<CWOutputProfile> AppSettings::loadCWOutputProfiles() const {
    QList<CWOutputProfile> profiles;

    ensureCleanSettingsState(m_settings);

    QSettingsGroupGuard groupGuard(m_settings, "CWOutputProfiles");
    QSettingsArrayGuard arrayGuard(m_settings, "Profiles");

    for (int i = 0; i < arrayGuard.size(); ++i) {
        m_settings.setArrayIndex(i);
        CWOutputProfile profile;
        profile.name = m_settings.value("name").toString();
        profile.type = static_cast<CWSenderFactory::Backend>(m_settings.value("type", 0).toInt());
        // WinKeyer settings — try new key first, fall back to legacy keyerPortName
        profile.winKeyerPortName = m_settings.value("winKeyerPortName").toString();
        if (profile.winKeyerPortName.isEmpty()) {
            profile.winKeyerPortName = m_settings.value("keyerPortName").toString();
        }
        profile.weighting = m_settings.value("weighting", 50).toInt();
        profile.leadInTime = m_settings.value("leadInTime", 0).toInt();
        profile.tailTime = m_settings.value("tailTime", 0).toInt();
        // DTR/RTS settings
        profile.dtrRtsPortName = m_settings.value("dtrRtsPortName").toString();
        profile.dtrRtsPin = static_cast<DtrRtsCWSender::Pin>(m_settings.value("dtrRtsPin", 0).toInt());
        profiles.append(profile);
    }

    return profiles;
}

bool AppSettings::hasCWOutputProfiles() const {
    ensureCleanSettingsState(m_settings);
    QSettingsGroupGuard groupGuard(m_settings, "CWOutputProfiles");
    QSettingsArrayGuard arrayGuard(m_settings, "Profiles");
    return arrayGuard.size() > 0;
}

void AppSettings::setActiveCWOutputProfile(const QString& profileName) {
    m_settings.setValue("CWOutputProfiles/activeProfile", profileName);
    m_settings.sync();
}

QString AppSettings::getActiveCWOutputProfile() const {
    return m_settings.value("CWOutputProfiles/activeProfile",
                            CWProfileDefaults::DEFAULT_PROFILE_NAME).toString();
}

CWOutputProfile AppSettings::getCWOutputProfile(const QString& name) const {
    QList<CWOutputProfile> profiles = loadCWOutputProfiles();
    for (const CWOutputProfile& profile : profiles) {
        if (profile.name == name) {
            return profile;
        }
    }
    return CWOutputProfile();
}

CWOutputProfile AppSettings::getCWOutputProfileForRadio(int radioIndex) const {
    QString activeStationName = getActiveStationProfile();
    StationProfile station = getStationProfile(activeStationName);
    if (station.name.isEmpty()) return CWOutputProfile();

    QString cwProfileName = (radioIndex == 0) ? station.cw1Name : station.cw2Name;
    if (cwProfileName.isEmpty()) return CWOutputProfile();

    return getCWOutputProfile(cwProfileName);
}

void AppSettings::migrateToCWOutputProfiles() {
    // Only migrate if no CW output profiles exist yet
    if (hasCWOutputProfiles()) return;

    LOG_INFO("AppSettings", "Migrating flat CW settings to CWOutputProfile system");

    // Read existing flat CW keying settings
    int keyingSource = getCWKeyingSource();  // 0=CAT, 1=Keyer, 2=DTR/RTS

    CWOutputProfile defaultProfile;
    defaultProfile.name = CWProfileDefaults::DEFAULT_PROFILE_NAME;

    switch (keyingSource) {
    case 0:  // Radio CAT
        defaultProfile.type = CWSenderFactory::Backend::Hamlib;
        break;
    case 1: {  // External Keyer — need to separate input from output
        KeyerDeviceType deviceType = static_cast<KeyerDeviceType>(getKeyerDeviceType());
        if (deviceType == KeyerDeviceType::WinKeyer) {
            // WinKeyer is a CW output device
            defaultProfile.type = CWSenderFactory::Backend::KeyerDevice;
            defaultProfile.winKeyerPortName = getKeyerPortName();
            defaultProfile.weighting = getWinKeyerWeighting();
            defaultProfile.leadInTime = getWinKeyerLeadIn();
            defaultProfile.tailTime = getWinKeyerTailTime();
        } else {
            // HaliKey (Serial or MIDI) is a paddle INPUT device, not CW output.
            // CW output falls back to Radio CAT since HaliKey can't send text.
            defaultProfile.type = CWSenderFactory::Backend::Hamlib;

            // Migrate paddle settings to PaddleInputConfig
            PaddleInputConfig paddleConfig;
            if (deviceType == KeyerDeviceType::HaliKeySerial) {
                paddleConfig.deviceType = PaddleInputConfig::DeviceType::HaliKeySerial;
            } else {
                paddleConfig.deviceType = PaddleInputConfig::DeviceType::HaliKeyMidi;
            }
            paddleConfig.portName = getKeyerPortName();
            paddleConfig.paddleSwap = getKeyerPaddleSwap();
            savePaddleInputConfig(paddleConfig);

            LOG_INFO("AppSettings", "Migrated HaliKey paddle settings to PaddleInputConfig");
        }
        break;
    }
    case 2:  // DTR/RTS
        defaultProfile.type = CWSenderFactory::Backend::DtrRts;
        defaultProfile.dtrRtsPortName = getDtrRtsPortName();
        defaultProfile.dtrRtsPin = static_cast<DtrRtsCWSender::Pin>(getDtrRtsPin());
        break;
    }

    QList<CWOutputProfile> profiles;
    profiles.append(defaultProfile);
    saveCWOutputProfiles(profiles);
    setActiveCWOutputProfile(CWProfileDefaults::DEFAULT_PROFILE_NAME);

    // Update active station profile's cw1Name to point to the migrated profile
    QList<StationProfile> stationProfiles = loadStationProfiles();
    bool updated = false;
    for (StationProfile& sp : stationProfiles) {
        if (sp.cw1Name.isEmpty()) {
            sp.cw1Name = CWProfileDefaults::DEFAULT_PROFILE_NAME;
            updated = true;
        }
    }
    if (updated) {
        saveStationProfiles(stationProfiles);
    }

    LOG_INFO("AppSettings", "CWOutputProfile migration complete: created 'Default' profile");
}

void AppSettings::savePaddleInputConfig(const PaddleInputConfig& config) {
    QSettingsGroupGuard groupGuard(m_settings, "CW/PaddleInput");
    m_settings.setValue("deviceType", static_cast<int>(config.deviceType));
    m_settings.setValue("portName", config.portName);
    m_settings.setValue("paddleSwap", config.paddleSwap);
    m_settings.sync();

    LOG_DEBUG("AppSettings", QString("Saved paddle input config (type=%1, port=%2)")
              .arg(static_cast<int>(config.deviceType)).arg(config.portName));
}

PaddleInputConfig AppSettings::loadPaddleInputConfig() const {
    PaddleInputConfig config;

    ensureCleanSettingsState(m_settings);

    QSettingsGroupGuard groupGuard(m_settings, "CW/PaddleInput");
    config.deviceType = static_cast<PaddleInputConfig::DeviceType>(
        m_settings.value("deviceType", 0).toInt());
    config.portName = m_settings.value("portName").toString();
    config.paddleSwap = m_settings.value("paddleSwap", false).toBool();

    return config;
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

void AppSettings::setAmplifierPollInterval(int intervalMs) {
    m_settings.setValue("Amplifier/pollIntervalMs", intervalMs);
    m_settings.sync();
}

int AppSettings::getAmplifierPollInterval() const {
    return m_settings.value("Amplifier/pollIntervalMs", 250).toInt();  // Default: 250ms
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

// CW Keyer Hardware settings
void AppSettings::setKeyerDeviceType(int type) {
    m_settings.setValue("Keyer/deviceType", type);
    m_settings.sync();
}

int AppSettings::getKeyerDeviceType() const {
    return m_settings.value("Keyer/deviceType", 0).toInt();
}

void AppSettings::setKeyerPortName(const QString& port) {
    m_settings.setValue("Keyer/portName", port);
    m_settings.sync();
}

QString AppSettings::getKeyerPortName() const {
    return m_settings.value("Keyer/portName", "").toString();
}

void AppSettings::setKeyerPaddleSwap(bool swap) {
    m_settings.setValue("Keyer/paddleSwap", swap);
    m_settings.sync();
}

bool AppSettings::getKeyerPaddleSwap() const {
    return m_settings.value("Keyer/paddleSwap", false).toBool();
}

void AppSettings::setKeyerIambicMode(int mode) {
    m_settings.setValue("Keyer/iambicMode", mode);
    m_settings.sync();
}

int AppSettings::getKeyerIambicMode() const {
    return m_settings.value("Keyer/iambicMode", 1).toInt();  // Default: IambicB
}

void AppSettings::setKeyerDitNote(int note) {
    m_settings.setValue("Keyer/ditNote", note);
    m_settings.sync();
}

int AppSettings::getKeyerDitNote() const {
    return m_settings.value("Keyer/ditNote", 20).toInt();
}

void AppSettings::setKeyerDahNote(int note) {
    m_settings.setValue("Keyer/dahNote", note);
    m_settings.sync();
}

int AppSettings::getKeyerDahNote() const {
    return m_settings.value("Keyer/dahNote", 21).toInt();
}

void AppSettings::setKeyerEnabled(bool enabled) {
    m_settings.setValue("Keyer/enabled", enabled);
    m_settings.sync();
}

bool AppSettings::getKeyerEnabled() const {
    return m_settings.value("Keyer/enabled", false).toBool();
}

void AppSettings::setKeyerAutoConnect(bool autoConnect) {
    m_settings.setValue("Keyer/autoConnect", autoConnect);
    m_settings.sync();
}

bool AppSettings::getKeyerAutoConnect() const {
    return m_settings.value("Keyer/autoConnect", false).toBool();
}

void AppSettings::setCWKeyingSource(int source) {
    m_settings.setValue("CW/keyingSource", source);
    m_settings.sync();
}

int AppSettings::getCWKeyingSource() const {
    return m_settings.value("CW/keyingSource", 0).toInt();  // Default: Radio (KY command)
}

// DTR/RTS CW keying settings
void AppSettings::setDtrRtsPortName(const QString& port) {
    m_settings.setValue("CW/dtrRtsPort", port);
    m_settings.sync();
}

QString AppSettings::getDtrRtsPortName() const {
    return m_settings.value("CW/dtrRtsPort", "").toString();
}

void AppSettings::setDtrRtsPin(int pin) {
    m_settings.setValue("CW/dtrRtsPin", pin);
    m_settings.sync();
}

int AppSettings::getDtrRtsPin() const {
    return m_settings.value("CW/dtrRtsPin", 0).toInt();  // Default: DTR
}

// Sidetone settings
void AppSettings::setSidetonePitch(int hz) {
    m_settings.setValue("Keyer/sidetonePitch", hz);
    m_settings.sync();
}

int AppSettings::getSidetonePitch() const {
    return m_settings.value("Keyer/sidetonePitch", 600).toInt();
}

void AppSettings::setSidetoneVolume(int percent) {
    m_settings.setValue("Keyer/sidetoneVolume", percent);
    m_settings.sync();
}

int AppSettings::getSidetoneVolume() const {
    return m_settings.value("Keyer/sidetoneVolume", 50).toInt();
}

// WinKeyer extended settings
void AppSettings::setWinKeyerWeighting(int weight) {
    m_settings.setValue("Keyer/winKeyerWeighting", weight);
    m_settings.sync();
}

int AppSettings::getWinKeyerWeighting() const {
    return m_settings.value("Keyer/winKeyerWeighting", 50).toInt();
}

void AppSettings::setWinKeyerLeadIn(int time) {
    m_settings.setValue("Keyer/winKeyerLeadIn", time);
    m_settings.sync();
}

int AppSettings::getWinKeyerLeadIn() const {
    return m_settings.value("Keyer/winKeyerLeadIn", 0).toInt();
}

void AppSettings::setWinKeyerTailTime(int time) {
    m_settings.setValue("Keyer/winKeyerTailTime", time);
    m_settings.sync();
}

int AppSettings::getWinKeyerTailTime() const {
    return m_settings.value("Keyer/winKeyerTailTime", 0).toInt();
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
    // Defensive check: ensure QSettings is not stuck in a group/array context
    // This can happen if a previous operation threw an exception before calling endGroup()/endArray()
    QString currentGroup = m_settings.group();
    if (!currentGroup.isEmpty()) {
        LOG_ERROR("AppSettings", QString("QSettings STUCK in group '%1'! Escaping to fix. This indicates a bug.").arg(currentGroup));

        // We need to escape all nested groups/arrays. QSettings::group() returns the full path
        // regardless of whether the context is from beginGroup() or beginReadArray()/beginWriteArray().
        // Unfortunately, we can't tell which it is, so we try endArray() first (which will warn if
        // we're actually in a group), then fall back to endGroup().
        QSettings& settings = const_cast<QSettings&>(m_settings);
        int maxAttempts = 10;  // Safety limit to prevent infinite loop
        int attempts = 0;
        while (!settings.group().isEmpty() && attempts < maxAttempts) {
            QString before = settings.group();
            // Try endArray first (handles array context)
            settings.endArray();
            // If that didn't change the group, we're in a group context, not array
            if (settings.group() == before) {
                settings.endGroup();
            }
            attempts++;
        }

        if (!settings.group().isEmpty()) {
            LOG_ERROR("AppSettings", QString("Failed to fully escape group context after %1 attempts. Current: '%2'")
                      .arg(attempts).arg(settings.group()));
        } else {
            LOG_INFO("AppSettings", QString("Successfully escaped group context after %1 escapes").arg(attempts));
        }
    }

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

// Radio 2 Control window (SO2R)
void AppSettings::saveRadio2ControlGeometry(const QByteArray& geometry) {
    m_settings.setValue("Radio2ControlWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadRadio2ControlGeometry() const {
    return m_settings.value("Radio2ControlWindow/geometry").toByteArray();
}

void AppSettings::setRadio2ControlVisible(bool visible) {
    m_settings.setValue("Radio2ControlWindow/visible", visible);
    m_settings.sync();
}

bool AppSettings::getRadio2ControlVisible() const {
    return m_settings.value("Radio2ControlWindow/visible", false).toBool();
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

// Statistics window
void AppSettings::saveStatisticsWindowGeometry(const QByteArray& geometry) {
    m_settings.setValue("StatisticsWindow/geometry", geometry);
    m_settings.sync();
}

QByteArray AppSettings::loadStatisticsWindowGeometry() const {
    return m_settings.value("StatisticsWindow/geometry").toByteArray();
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

// DXLab DDE integration
void AppSettings::setDXLabDDEEnabled(bool enabled) {
    m_settings.setValue("DXLab/DDEEnabled", enabled);
    m_settings.sync();
}

bool AppSettings::getDXLabDDEEnabled() const {
    return m_settings.value("DXLab/DDEEnabled", false).toBool();
}

void AppSettings::setDXLabDDEQSY(bool enabled) {
    m_settings.setValue("DXLab/DDEQSY", enabled);
    m_settings.sync();
}

bool AppSettings::getDXLabDDEQSY() const {
    return m_settings.value("DXLab/DDEQSY", false).toBool();
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

void AppSettings::setShowAllWindowsOnActivate(bool enabled) {
    m_settings.setValue("Appearance/showAllWindowsOnActivate", enabled);
    m_settings.sync();
}

bool AppSettings::getShowAllWindowsOnActivate() const {
    return m_settings.value("Appearance/showAllWindowsOnActivate", false).toBool();
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
    {
        QSettingsGroupGuard groupGuard(m_settings, "UDPBroadcast");
        m_settings.remove("Destinations");  // Clear old entries

        QSettingsWriteArrayGuard arrayGuard(m_settings, "Destinations");
        for (int i = 0; i < destinations.size(); ++i) {
            m_settings.setArrayIndex(i);
            m_settings.setValue("host", destinations[i].host);
            m_settings.setValue("port", destinations[i].port);
            m_settings.setValue("enabled", destinations[i].enabled);
        }
        // Guards automatically call endArray() and endGroup() on scope exit
    }
    m_settings.sync();
}

QList<UdpDestination> AppSettings::getUDPDestinations() const {
    QList<UdpDestination> destinations;

    {
        QSettingsGroupGuard groupGuard(m_settings, "UDPBroadcast");
        QSettingsArrayGuard arrayGuard(m_settings, "Destinations");

        for (int i = 0; i < arrayGuard.size(); ++i) {
            m_settings.setArrayIndex(i);
            UdpDestination dest;
            dest.host = m_settings.value("host").toString();
            dest.port = m_settings.value("port").toUInt();
            dest.enabled = m_settings.value("enabled", true).toBool();
            destinations.append(dest);
        }
    }

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
