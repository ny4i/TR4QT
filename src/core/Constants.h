#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace TR4QT {

// Application info
constexpr const char* APP_NAME = "TR4QT";
constexpr const char* APP_VERSION = "3.38.0";  // Add checkpoint system for god class prevention
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

// Logging
constexpr qint64 DEFAULT_MAX_LOG_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB

// AUTO S&P (Search & Pounce) sensitivity limits
constexpr int AUTO_SP_SENSITIVITY_MIN_HZ = 100;      // Minimum frequency change threshold
constexpr int AUTO_SP_SENSITIVITY_MAX_HZ = 100000;   // Maximum frequency change threshold
constexpr int AUTO_SP_SENSITIVITY_STEP_HZ = 100;     // Step size for input dialog

// Grayline propagation window
constexpr int GRAYLINE_WINDOW_MINUTES = 30;  // Minutes before/after sunrise/sunset

// UI window/dialog dimensions
namespace UIDefaults {
    // Main window
    constexpr int MAIN_WINDOW_WIDTH = 1024;
    constexpr int MAIN_WINDOW_HEIGHT = 768;
    constexpr int MAIN_WINDOW_MIN_HEIGHT = 600;

    // Dialog dimensions
    constexpr int NATIVE_MAP_WIDTH = 1200;
    constexpr int NATIVE_MAP_HEIGHT = 800;
    constexpr int NATIVE_MAP_MIN_WIDTH = 600;
    constexpr int NATIVE_MAP_MIN_HEIGHT = 400;

    constexpr int STATISTICS_WIDTH = 800;
    constexpr int STATISTICS_HEIGHT = 600;
    constexpr int STATISTICS_MIN_WIDTH = 600;
    constexpr int STATISTICS_MIN_HEIGHT = 400;

    constexpr int PREFERENCES_WIDTH = 800;
    constexpr int PREFERENCES_HEIGHT = 550;

    constexpr int GRAYLINE_MAP_WIDTH = 1100;
    constexpr int GRAYLINE_MAP_HEIGHT = 600;

    constexpr int FUNCTION_KEYS_WIDTH = 600;
    constexpr int FUNCTION_KEYS_HEIGHT = 450;

    constexpr int EXPORT_PREVIEW_WIDTH = 800;
    constexpr int EXPORT_PREVIEW_HEIGHT = 600;

    constexpr int SEND_MORSE_WIDTH = 500;
    constexpr int SEND_MORSE_HEIGHT = 300;

    constexpr int CONTEST_CHOOSER_WIDTH = 700;
    constexpr int CONTEST_CHOOSER_HEIGHT = 500;

    constexpr int ADIF_IMPORT_PREVIEW_WIDTH = 600;
    constexpr int ADIF_IMPORT_PREVIEW_HEIGHT = 400;

    // MainWindow widget dimensions
    constexpr int RADIO_FREQ_BAND_LABEL_WIDTH = 80;
    constexpr int RADIO_FREQ_LABEL_WIDTH = 100;
    constexpr int RADIO_WPM_LABEL_WIDTH = 80;
    constexpr int RADIO_DATE_LABEL_WIDTH = 120;
    constexpr int RADIO_TIME_LABEL_WIDTH = 120;
    constexpr int ENTRY_WIDGET_MIN_WIDTH = 350;
    constexpr int SCP_MATCHES_LABEL_WIDTH = 120;
    constexpr int TIME_LABEL_MIN_WIDTH = 70;
    constexpr int STATS_WIDGET_MIN_WIDTH = 200;
    constexpr int STATS_WIDGET_MAX_WIDTH = 300;
    constexpr int NEEDS_DISPLAY_MIN_WIDTH = 200;
}

} // namespace TR4QT

#endif // CONSTANTS_H
