#include "PathManager.h"
#include "../logging/LogMacros.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace TR4QT {

QString PathManager::getAppDataDir() {
    // Use Qt's AppLocalDataLocation which provides platform-native paths:
    // Windows: C:\Users\<user>\AppData\Local\TR4QT
    // macOS:   ~/Library/Application Support/TR4QT
    // Linux:   ~/.local/share/TR4QT
    //
    // Note: We use AppLocalDataLocation (not AppDataLocation) because:
    // - AppDataLocation returns AppData\Roaming (for roaming profiles)
    // - AppLocalDataLocation returns AppData\Local (for local-only data)
    // TR4QT data doesn't need to roam between machines.
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    // Ensure the directory exists
    ensureDirectoryExists(appDataDir);

    return appDataDir;
}

QString PathManager::getLogsDir() {
    QString logsDir = getAppDataDir() + "/logs";
    ensureDirectoryExists(logsDir);
    return logsDir;
}

QString PathManager::getBackupsDir() {
    QString backupsDir = getAppDataDir() + "/backups";
    ensureDirectoryExists(backupsDir);
    return backupsDir;
}

QString PathManager::getGlobalDatabasePath() {
    return getAppDataDir() + "/tr4qt_global.db";
}

QString PathManager::getCountryFilePath() {
    return getAppDataDir() + "/cty.dat";
}

QString PathManager::getLOTWUserFilePath() {
    return getAppDataDir() + "/lotw-user-activity.csv";
}

QString PathManager::getLegacyPath() {
    // Old Unix-style path: ~/.tr4qt
    return QDir::homePath() + "/.tr4qt";
}

bool PathManager::ensureDirectoryExists(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("PathManager", QString("Failed to create directory: %1").arg(path));
            return false;
        }
        LOG_DEBUG("PathManager", QString("Created directory: %1").arg(path));
    }
    return true;
}

bool PathManager::migrateFromLegacyPath() {
    QString legacyPath = getLegacyPath();
    QString newPath = getAppDataDir();

    // Check if legacy path exists
    QDir legacyDir(legacyPath);
    if (!legacyDir.exists()) {
        // No legacy data to migrate
        LOG_DEBUG("PathManager", "No legacy ~/.tr4qt directory found, skipping migration");
        return true;
    }

    // Check if new path already has data (migration already done)
    QDir newDir(newPath);
    if (newDir.exists() && !newDir.isEmpty()) {
        LOG_DEBUG("PathManager", QString("Platform-native directory already exists: %1").arg(newPath));
        LOG_INFO("PathManager", QString("Legacy directory still exists at: %1").arg(legacyPath));
        LOG_INFO("PathManager", "You can safely delete the legacy directory if migration was successful");
        return true;
    }

#ifdef Q_OS_WIN
    // Only perform automatic migration on Windows (where the change is most significant)
    // On Unix/macOS, the paths are similar enough that we don't force migration

    LOG_INFO("PathManager", QString("Migrating data from %1 to %2").arg(legacyPath, newPath));

    // Ensure target directory exists
    if (!ensureDirectoryExists(newPath)) {
        LOG_ERROR("PathManager", "Failed to create target directory for migration");
        return false;
    }

    // List of items to migrate
    QStringList itemsToMigrate;
    itemsToMigrate << "logs" << "backups" << "cty.dat" << "lotw-user-activity.csv"
                   << "tr4qt_global.db" << "tr4qt_global.db-shm" << "tr4qt_global.db-wal";

    bool migrationSuccess = true;
    int itemsMigrated = 0;

    for (const QString& item : itemsToMigrate) {
        QString sourcePath = legacyPath + "/" + item;
        QString targetPath = newPath + "/" + item;

        QFileInfo sourceInfo(sourcePath);
        if (!sourceInfo.exists()) {
            // Item doesn't exist in legacy location, skip
            continue;
        }

        if (sourceInfo.isDir()) {
            // Copy directory recursively
            QDir sourceDir(sourcePath);
            QDir targetDir(targetPath);

            if (!targetDir.exists() && !targetDir.mkpath(".")) {
                LOG_ERROR("PathManager", QString("Failed to create target directory: %1").arg(targetPath));
                migrationSuccess = false;
                continue;
            }

            // Copy all files in directory
            QFileInfoList files = sourceDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
            for (const QFileInfo& fileInfo : files) {
                QString sourceFile = fileInfo.absoluteFilePath();
                QString targetFile = targetPath + "/" + fileInfo.fileName();

                if (QFile::copy(sourceFile, targetFile)) {
                    LOG_DEBUG("PathManager", QString("Copied: %1 -> %2").arg(sourceFile, targetFile));
                    itemsMigrated++;
                } else {
                    LOG_WARN("PathManager", QString("Failed to copy: %1").arg(sourceFile));
                    migrationSuccess = false;
                }
            }
        } else {
            // Copy single file
            if (QFile::copy(sourcePath, targetPath)) {
                LOG_DEBUG("PathManager", QString("Copied: %1 -> %2").arg(sourcePath, targetPath));
                itemsMigrated++;
            } else {
                LOG_WARN("PathManager", QString("Failed to copy: %1").arg(sourcePath));
                migrationSuccess = false;
            }
        }
    }

    if (migrationSuccess && itemsMigrated > 0) {
        LOG_INFO("PathManager", QString("Migration complete: %1 items migrated").arg(itemsMigrated));
        LOG_INFO("PathManager", QString("Legacy directory preserved at: %1").arg(legacyPath));
        LOG_INFO("PathManager", "You can safely delete the legacy directory after verifying migration");
        return true;
    } else if (itemsMigrated == 0) {
        LOG_DEBUG("PathManager", "No items to migrate from legacy directory");
        return true;
    } else {
        LOG_ERROR("PathManager", QString("Migration completed with errors (%1 items migrated)").arg(itemsMigrated));
        return false;
    }
#else
    // On Unix/macOS, just log that legacy path exists
    LOG_DEBUG("PathManager", QString("Legacy path exists: %1").arg(legacyPath));
    LOG_DEBUG("PathManager", QString("Using platform-native path: %1").arg(newPath));
    LOG_INFO("PathManager", "Automatic migration is Windows-only. Manually move data if desired.");
    return true;
#endif
}

} // namespace TR4QT
