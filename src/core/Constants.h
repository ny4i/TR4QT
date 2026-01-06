#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace TR4QT {

// Application info
constexpr const char* APP_NAME = "TR4QT";
constexpr const char* APP_VERSION = "3.31.13";  // Disable performance profiling + fix K4Radio verbose logging
constexpr const char* APP_ORG = "TR4QT";

// Country file
constexpr const char* COUNTRY_FILE_URL = "https://www.country-files.com/";
constexpr const char* COUNTRY_FILE_NAME = "cty.dat";
constexpr int CURRENT_CTY_VERSION = 3540;  // Update as needed

// Database
// DEPRECATED: Use PathManager::getLogsDir() for platform-native paths
// These constants are kept for reference only
constexpr const char* DB_DIR = ".tr4qt/logs";           // Legacy Unix-style path
constexpr const char* CONFIG_DIR = ".tr4qt";            // Legacy Unix-style path
constexpr const char* GLOBAL_DB_NAME = "tr4qt_global.db";

// LoTW
constexpr const char* LOTW_USERS_FILE = "lotw-user-activity.csv";
constexpr const char* LOTW_USERS_URL = "https://lotw.arrl.org/lotw-user-activity.csv";

// Backup
// DEPRECATED: Use PathManager::getBackupsDir() for platform-native paths
constexpr const char* BACKUP_DIR = ".tr4qt/backups";    // Legacy Unix-style path
constexpr int DEFAULT_BACKUP_INTERVAL = 10;  // QSOs between auto-backups
constexpr int DEFAULT_MAX_BACKUPS = 50;       // Max backup files to keep

// UI defaults
constexpr int DEFAULT_ENTRY_FONT_SIZE = 14;
constexpr int DEFAULT_TABLE_FONT_SIZE = 12;
constexpr int DEFAULT_GRID_FONT_SIZE = 11;
constexpr int DEFAULT_MISC_DISPLAY_FONT_SIZE = 11;

// Zone validation ranges
constexpr int CQ_ZONE_MIN = 1;
constexpr int CQ_ZONE_MAX = 40;
constexpr int ITU_ZONE_MIN = 1;
constexpr int ITU_ZONE_MAX = 90;

// Integrity checks
constexpr int INTEGRITY_CHECK_INTERVAL_MS = 5 * 60 * 1000;  // 5 minutes
constexpr int INTEGRITY_CHECK_QSO_THRESHOLD = 50;            // Minimum QSOs before integrity check

// CW speed limits (WPM - Words Per Minute)
constexpr int CW_SPEED_MIN = 5;
constexpr int CW_SPEED_MAX = 60;
constexpr int CW_SPEED_DEFAULT = 25;

} // namespace TR4QT

#endif // CONSTANTS_H
