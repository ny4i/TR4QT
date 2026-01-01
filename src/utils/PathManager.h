#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <QString>
#include <QStandardPaths>

namespace TR4QT {

/**
 * @brief Platform-native path management for TR4QT application data
 *
 * Provides cross-platform access to application data directories following
 * platform conventions:
 *
 * Windows:  C:\Users\<user>\AppData\Local\TR4QT\
 * macOS:    ~/Library/Application Support/TR4QT/
 * Linux:    ~/.local/share/TR4QT/
 *
 * This replaces the old Unix-style ~/.tr4qt/ approach with platform-native
 * locations that follow OS conventions and best practices.
 */
class PathManager {
public:
    /**
     * @brief Get the root application data directory
     * @return Platform-native application data directory path
     *
     * Windows: %LOCALAPPDATA%\TR4QT  (C:\Users\<user>\AppData\Local\TR4QT)
     * macOS:   ~/Library/Application Support/TR4QT
     * Linux:   ~/.local/share/TR4QT
     */
    static QString getAppDataDir();

    /**
     * @brief Get the database/logs directory
     * @return Path to database storage directory
     *
     * Returns: <AppDataDir>/logs
     */
    static QString getLogsDir();

    /**
     * @brief Get the backup directory
     * @return Path to backup storage directory
     *
     * Returns: <AppDataDir>/backups
     */
    static QString getBackupsDir();

    /**
     * @brief Get the global database path
     * @return Full path to global database file
     *
     * Returns: <AppDataDir>/tr4qt_global.db
     */
    static QString getGlobalDatabasePath();

    /**
     * @brief Get the country file (cty.dat) path
     * @return Full path to country file
     *
     * Returns: <AppDataDir>/cty.dat
     */
    static QString getCountryFilePath();

    /**
     * @brief Get the LOTW user file path
     * @return Full path to LOTW user activity file
     *
     * Returns: <AppDataDir>/lotw-user-activity.csv
     */
    static QString getLOTWUserFilePath();

    /**
     * @brief Migrate data from old Unix-style ~/.tr4qt to platform-native location
     *
     * This is automatically called on first run if old directory exists.
     * Only performs migration on Windows (where the change is most significant).
     *
     * @return true if migration succeeded or wasn't needed, false on error
     */
    static bool migrateFromLegacyPath();

    /**
     * @brief Get the legacy ~/.tr4qt path (for migration purposes)
     * @return Old Unix-style path
     */
    static QString getLegacyPath();

private:
    PathManager() = delete;  // Static class, no instantiation

    /**
     * @brief Ensure a directory exists, creating it if necessary
     * @param path Directory path to check/create
     * @return true if directory exists or was created successfully
     */
    static bool ensureDirectoryExists(const QString& path);
};

} // namespace TR4QT

#endif // PATHMANAGER_H
