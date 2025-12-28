#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace TR4QT {

// Application info
constexpr const char* APP_NAME = "TR4QT";
constexpr const char* APP_VERSION = "2.98.1";  // Preserve zoom level on sections map refresh
constexpr const char* APP_ORG = "TR4QT";

// Country file
constexpr const char* COUNTRY_FILE_URL = "https://www.country-files.com/";
constexpr const char* COUNTRY_FILE_NAME = "cty.dat";
constexpr int CURRENT_CTY_VERSION = 3540;  // Update as needed

// Database
constexpr const char* DB_DIR = ".tr4qt/logs";
constexpr const char* CONFIG_DIR = ".tr4qt";
constexpr const char* GLOBAL_DB_NAME = "tr4qt_global.db";

// LoTW
constexpr const char* LOTW_USERS_FILE = "lotw-user-activity.csv";
constexpr const char* LOTW_USERS_URL = "https://lotw.arrl.org/lotw-user-activity.csv";

// Backup
constexpr const char* BACKUP_DIR = ".tr4qt/backups";
constexpr int DEFAULT_BACKUP_INTERVAL = 10;  // QSOs between auto-backups
constexpr int DEFAULT_MAX_BACKUPS = 50;       // Max backup files to keep

// UI defaults
constexpr int DEFAULT_ENTRY_FONT_SIZE = 14;
constexpr int DEFAULT_TABLE_FONT_SIZE = 12;
constexpr int DEFAULT_GRID_FONT_SIZE = 11;
constexpr int DEFAULT_MISC_DISPLAY_FONT_SIZE = 11;

} // namespace TR4QT

#endif // CONSTANTS_H
