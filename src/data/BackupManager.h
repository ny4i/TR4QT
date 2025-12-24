#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QMap>

namespace TR4QT {

/**
 * Information about a database backup
 */
struct BackupInfo {
    QString filePath;        // Full path to backup file
    QString fileName;        // Just the filename
    QDateTime timestamp;     // When backup was created (from filename)
    qint64 fileSize;         // Size in bytes
    int qsoCount;            // Number of QSOs in backup (read from database)
    bool isValid;            // Whether backup file is valid SQLite
};

/**
 * Manager for database backup and restore operations
 *
 * Singleton class that handles:
 * - Manual backups via VACUUM INTO
 * - Auto-backup every N QSOs
 * - Backup rotation (keep N most recent)
 * - Full database restore
 *
 * Usage:
 *   BackupManager& backup = BackupManager::instance();
 *   QString backupPath;
 *   if (backup.createBackup(dbPath, backupDir, backupPath)) {
 *       qDebug() << "Backup created:" << backupPath;
 *   }
 */
class BackupManager {
public:
    /**
     * Get singleton instance
     */
    static BackupManager& instance();

    /**
     * Create a backup of the database
     *
     * Uses SQLite VACUUM INTO to create a consistent, compacted backup.
     * Automatically rotates old backups based on maxBackups setting.
     *
     * @param sourceDatabasePath Path to source database
     * @param backupDirectory Directory to store backup (empty = same as source)
     * @param outBackupPath [out] Full path to created backup file
     * @return true if successful
     */
    bool createBackup(const QString& sourceDatabasePath,
                     const QString& backupDirectory,
                     QString& outBackupPath);

    /**
     * Auto-backup if threshold reached
     *
     * Checks if enough QSOs have been logged since last backup.
     * If so, creates a backup automatically.
     *
     * @param sourceDatabasePath Path to source database
     * @param currentQSOCount Current number of QSOs in contest
     * @return true if backup was created, false if not needed or failed
     */
    bool autoBackupIfNeeded(const QString& sourceDatabasePath,
                           int currentQSOCount);

    /**
     * Restore database from backup
     *
     * Replaces the target database with a copy of the backup.
     * Creates a safety backup (.broken) of the existing database.
     *
     * @param backupPath Path to backup file
     * @param targetDatabasePath Path where to restore database
     * @return true if successful
     */
    bool restoreFromBackup(const QString& backupPath,
                          const QString& targetDatabasePath);

    /**
     * List available backups in a directory
     *
     * Finds all backup files matching the pattern and returns their info.
     *
     * @param directory Directory to search
     * @param baseNameFilter Optional filter (e.g., "CQWW_CW" to find only CQWW_CW backups)
     * @return List of BackupInfo sorted by timestamp (newest first)
     */
    QList<BackupInfo> listBackups(const QString& directory,
                                 const QString& baseNameFilter = QString());

    /**
     * Rotate backups (delete oldest)
     *
     * Keeps only the N most recent backups with the given base name.
     *
     * @param directory Backup directory
     * @param baseName Base name for backups (e.g., "CQWW_CW_2024")
     * @param maxBackups Maximum number to keep
     */
    void rotateBackups(const QString& directory,
                      const QString& baseName,
                      int maxBackups);

    /**
     * Validate that a file is a valid SQLite database
     *
     * @param filePath Path to file
     * @return true if file is a valid SQLite database
     */
    bool validateBackupFile(const QString& filePath);

    /**
     * Get QSO count from a backup database
     *
     * Opens the backup and counts QSOs without affecting it.
     *
     * @param backupPath Path to backup file
     * @return QSO count, or -1 on error
     */
    int getBackupQSOCount(const QString& backupPath);

    // Configuration
    void setAutoBackupEnabled(bool enabled) { m_autoBackupEnabled = enabled; }
    bool getAutoBackupEnabled() const { return m_autoBackupEnabled; }

    void setAutoBackupInterval(int qsoCount) { m_autoBackupInterval = qsoCount; }
    int getAutoBackupInterval() const { return m_autoBackupInterval; }

    void setBackupDirectory(const QString& path) { m_backupDirectory = path; }
    QString getBackupDirectory() const { return m_backupDirectory; }

    void setMaxBackups(int count) { m_maxBackups = count; }
    int getMaxBackups() const { return m_maxBackups; }

    QString lastError() const { return m_lastError; }

    // Public helpers (used by UI)
    QString generateBackupFileName(const QString& databaseName);
    QString extractBaseName(const QString& databaseName);

private:
    BackupManager();
    ~BackupManager() = default;

    // Prevent copying
    BackupManager(const BackupManager&) = delete;
    BackupManager& operator=(const BackupManager&) = delete;

    /**
     * Parse timestamp from backup filename
     *
     * @param fileName Backup filename
     * @return Timestamp, or invalid QDateTime if parsing failed
     */
    QDateTime parseBackupTimestamp(const QString& fileName);

    // Tracking last backup to avoid duplicates
    QMap<QString, int> m_lastBackupQSOCount;  // dbPath -> last backup QSO count

    // Settings
    bool m_autoBackupEnabled;
    int m_autoBackupInterval;     // Backup every N QSOs
    QString m_backupDirectory;     // Empty = same directory as database
    int m_maxBackups;              // Keep N most recent backups

    QString m_lastError;
};

} // namespace TR4QT

#endif // BACKUPMANAGER_H
