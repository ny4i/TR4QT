#include "BackupManager.h"
#include "Database.h"
#include "../logging/LogMacros.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>

namespace TR4QT {

BackupManager& BackupManager::instance() {
    static BackupManager instance;
    return instance;
}

BackupManager::BackupManager()
    : m_autoBackupEnabled(false)
    , m_autoBackupInterval(50)
    , m_backupDirectory()
    , m_maxBackups(10)
{
}

bool BackupManager::createBackup(const QString& sourceDatabasePath,
                                const QString& backupDirectory,
                                QString& outBackupPath) {
    m_lastError.clear();

    // Validate source database exists
    if (!QFile::exists(sourceDatabasePath)) {
        m_lastError = "Source database does not exist: " + sourceDatabasePath;
        LOG_WARN("BackupManager", m_lastError);
        return false;
    }

    // Generate backup filename
    QFileInfo sourceInfo(sourceDatabasePath);
    QString baseName = extractBaseName(sourceInfo.fileName());
    QString backupFileName = generateBackupFileName(sourceInfo.fileName());

    // Determine backup directory
    QString backupDir = backupDirectory.isEmpty()
        ? sourceInfo.absolutePath()
        : backupDirectory;

    // Create backup directory if needed
    QDir dir(backupDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = "Failed to create backup directory: " + backupDir;
            LOG_WARN("BackupManager", m_lastError);
            return false;
        }
        LOG_DEBUG("BackupManager", QString("Created backup directory: %1").arg(backupDir));
    }

    // Full path to backup
    QString backupPath = dir.absoluteFilePath(backupFileName);

    // Use VACUUM INTO to create backup
    // This creates a compacted, consistent snapshot of the database
    QString sql = QString("VACUUM INTO '%1'").arg(backupPath);

    Database& db = Database::instance();
    QSqlQuery query = db.execute(sql);

    if (!query.isActive() && db.lastError().isEmpty() == false) {
        m_lastError = "Backup failed: " + db.lastError();
        LOG_WARN("BackupManager", m_lastError);
        return false;
    }

    // Verify backup was created
    if (!QFile::exists(backupPath)) {
        m_lastError = "Backup file was not created: " + backupPath;
        LOG_WARN("BackupManager", m_lastError);
        return false;
    }

    QFileInfo backupInfo(backupPath);
    LOG_DEBUG("BackupManager", QString("Backup created: %1 Size: %2 bytes").arg(backupPath).arg(backupInfo.size()));

    outBackupPath = backupPath;

    // Rotate old backups
    rotateBackups(backupDir, baseName, m_maxBackups);

    return true;
}

bool BackupManager::autoBackupIfNeeded(const QString& sourceDatabasePath,
                                      int currentQSOCount) {
    if (!m_autoBackupEnabled) {
        return false;  // Auto-backup disabled
    }

    if (m_autoBackupInterval <= 0) {
        return false;  // Invalid interval
    }

    // Check if we've reached the backup threshold
    int lastBackupCount = m_lastBackupQSOCount.value(sourceDatabasePath, 0);
    int qsosSinceLastBackup = currentQSOCount - lastBackupCount;

    if (qsosSinceLastBackup < m_autoBackupInterval) {
        // Not enough QSOs since last backup
        return false;
    }

    LOG_DEBUG("BackupManager", QString("Auto-backup triggered: %1 QSOs since last backup").arg(qsosSinceLastBackup));

    // Perform backup
    QString backupPath;
    bool success = createBackup(sourceDatabasePath, m_backupDirectory, backupPath);

    if (success) {
        // Update last backup count
        m_lastBackupQSOCount[sourceDatabasePath] = currentQSOCount;
        LOG_DEBUG("BackupManager", QString("Auto-backup successful: %1").arg(backupPath));
    } else {
        LOG_WARN("BackupManager", QString("Auto-backup failed: %1").arg(m_lastError));
    }

    return success;
}

bool BackupManager::restoreFromBackup(const QString& backupPath,
                                     const QString& targetDatabasePath) {
    m_lastError.clear();

    // Validate backup file
    if (!validateBackupFile(backupPath)) {
        m_lastError = "Invalid backup file: " + backupPath;
        return false;
    }

    // Close target database if it's currently open
    Database::instance().close();

    // Create safety backup of existing database
    if (QFile::exists(targetDatabasePath)) {
        QString brokenPath = targetDatabasePath + ".broken";

        // Remove old .broken file if it exists
        if (QFile::exists(brokenPath)) {
            if (!QFile::remove(brokenPath)) {
                m_lastError = "Failed to remove old .broken file: " + brokenPath;
                LOG_WARN("BackupManager", m_lastError);
                return false;
            }
        }

        // Rename current database to .broken
        if (!QFile::rename(targetDatabasePath, brokenPath)) {
            m_lastError = "Failed to backup existing database to .broken";
            LOG_WARN("BackupManager", m_lastError);
            return false;
        }
        LOG_DEBUG("BackupManager", QString("Saved existing database as: %1").arg(brokenPath));
    }

    // Copy backup to target location
    if (!QFile::copy(backupPath, targetDatabasePath)) {
        m_lastError = "Failed to copy backup file to target location";
        LOG_WARN("BackupManager", m_lastError);

        // Try to restore original database
        QString brokenPath = targetDatabasePath + ".broken";
        if (QFile::exists(brokenPath)) {
            QFile::rename(brokenPath, targetDatabasePath);
            LOG_WARN("BackupManager", "Restored original database after failed restore");
        }
        return false;
    }

    LOG_DEBUG("BackupManager", QString("Restore successful: %1").arg(targetDatabasePath));

    // Reopen database
    if (!Database::instance().open(targetDatabasePath)) {
        m_lastError = "Failed to reopen restored database: " + Database::instance().lastError();
        LOG_WARN("BackupManager", m_lastError);
        return false;
    }

    return true;
}

QList<BackupInfo> BackupManager::listBackups(const QString& directory,
                                             const QString& baseNameFilter) {
    QList<BackupInfo> backups;

    QDir dir(directory);
    if (!dir.exists()) {
        LOG_WARN("BackupManager", QString("Backup directory does not exist: %1").arg(directory));
        return backups;
    }

    // Filter for .db files
    QStringList filters;
    filters << "*.db";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    // Regex to match backup filename pattern: *_YYYYMMDD_HHMMSS.db
    QRegularExpression backupPattern("^(.+)_(\\d{8})_(\\d{6})\\.db$");

    for (const QFileInfo& fileInfo : files) {
        QRegularExpressionMatch match = backupPattern.match(fileInfo.fileName());
        if (!match.hasMatch()) {
            continue;  // Not a backup file
        }

        QString baseName = match.captured(1);

        // Apply filter if specified
        if (!baseNameFilter.isEmpty() && baseName != baseNameFilter) {
            continue;
        }

        BackupInfo info;
        info.filePath = fileInfo.absoluteFilePath();
        info.fileName = fileInfo.fileName();
        info.fileSize = fileInfo.size();
        info.timestamp = parseBackupTimestamp(fileInfo.fileName());
        info.isValid = validateBackupFile(info.filePath);
        info.qsoCount = getBackupQSOCount(info.filePath);

        backups.append(info);
    }

    // Sort by timestamp, newest first
    std::sort(backups.begin(), backups.end(),
             [](const BackupInfo& a, const BackupInfo& b) {
                 return a.timestamp > b.timestamp;
             });

    return backups;
}

void BackupManager::rotateBackups(const QString& directory,
                                 const QString& baseName,
                                 int maxBackups) {
    if (maxBackups <= 0) {
        return;  // No rotation
    }

    // List all backups for this base name
    QList<BackupInfo> backups = listBackups(directory, baseName);

    // Delete oldest backups beyond the limit
    if (backups.size() > maxBackups) {
        int toDelete = backups.size() - maxBackups;
        LOG_DEBUG("BackupManager", QString("Rotating backups: deleting %1 old backups").arg(toDelete));

        // backups is sorted newest first, so delete from end
        for (int i = backups.size() - 1; i >= maxBackups; --i) {
            if (QFile::remove(backups[i].filePath)) {
                LOG_DEBUG("BackupManager", QString("Deleted old backup: %1").arg(backups[i].fileName));
            } else {
                LOG_WARN("BackupManager", QString("Failed to delete backup: %1").arg(backups[i].filePath));
            }
        }
    }
}

bool BackupManager::validateBackupFile(const QString& filePath) {
    if (!QFile::exists(filePath)) {
        return false;
    }

    bool isValid = false;

    // Scope ensures both db and query are destroyed before removeDatabase
    {
        QSqlDatabase testDb = QSqlDatabase::addDatabase("QSQLITE", "backup_validation");
        testDb.setDatabaseName(filePath);

        if (!testDb.open()) {
            testDb.close();
            // Fall through to removeDatabase outside scope
        } else {
            // Run integrity check
            QSqlQuery query(testDb);
            if (query.exec("PRAGMA integrity_check")) {
                if (query.next()) {
                    QString result = query.value(0).toString();
                    isValid = (result == "ok");
                }
            }
            testDb.close();
        }
    }  // testDb and query destroyed here

    QSqlDatabase::removeDatabase("backup_validation");
    return isValid;
}

int BackupManager::getBackupQSOCount(const QString& backupPath) {
    if (!QFile::exists(backupPath)) {
        return -1;
    }

    int count = -1;
    QString connectionName = QString("backup_count_%1").arg(QDateTime::currentMSecsSinceEpoch());

    // Scope ensures both db and query are destroyed before removeDatabase
    {
        QSqlDatabase backupDb = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        backupDb.setDatabaseName(backupPath);

        if (!backupDb.open()) {
            backupDb.close();
            // Fall through to removeDatabase outside scope
        } else {
            // Count QSOs
            QSqlQuery query(backupDb);
            if (query.exec("SELECT COUNT(*) FROM qsos WHERE deleted = 0")) {
                if (query.next()) {
                    count = query.value(0).toInt();
                }
            }
            backupDb.close();
        }
    }  // backupDb and query destroyed here

    QSqlDatabase::removeDatabase(connectionName);
    return count;
}

QString BackupManager::generateBackupFileName(const QString& databaseName) {
    QString baseName = extractBaseName(databaseName);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("%1_%2.db").arg(baseName, timestamp);
}

QString BackupManager::extractBaseName(const QString& databaseName) {
    QString name = databaseName;

    // Remove .db extension
    if (name.endsWith(".db", Qt::CaseInsensitive)) {
        name.chop(3);
    }

    // Remove existing timestamp suffix if present (YYYYMMDD_HHMMSS)
    QRegularExpression timestampPattern("_(\\d{8})_(\\d{6})$");
    name.remove(timestampPattern);

    return name;
}

QDateTime BackupManager::parseBackupTimestamp(const QString& fileName) {
    // Match pattern: *_YYYYMMDD_HHMMSS.db
    QRegularExpression pattern("_(\\d{8})_(\\d{6})\\.db$");
    QRegularExpressionMatch match = pattern.match(fileName);

    if (!match.hasMatch()) {
        return QDateTime();  // Invalid
    }

    QString dateStr = match.captured(1);  // YYYYMMDD
    QString timeStr = match.captured(2);  // HHMMSS

    // Parse date and time
    int year = dateStr.mid(0, 4).toInt();
    int month = dateStr.mid(4, 2).toInt();
    int day = dateStr.mid(6, 2).toInt();
    int hour = timeStr.mid(0, 2).toInt();
    int minute = timeStr.mid(2, 2).toInt();
    int second = timeStr.mid(4, 2).toInt();

    QDate date(year, month, day);
    QTime time(hour, minute, second);

    if (!date.isValid() || !time.isValid()) {
        return QDateTime();
    }

    return QDateTime(date, time, QTimeZone::UTC);
}

} // namespace TR4QT
