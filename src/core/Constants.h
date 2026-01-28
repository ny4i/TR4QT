#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace TR4QT {

// Application info
constexpr const char* APP_NAME = "TR4QT";
constexpr const char* APP_VERSION = "3.38.85";  // Fix amp window overlay positioning on Windows
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

// K4-specific CW speed limits (Elecraft K4 hardware constraints)
namespace K4Limits {
    constexpr int CW_WPM_MIN = 8;    // K4 minimum CW speed
    constexpr int CW_WPM_MAX = 100;  // K4 maximum CW speed
}

// Logging
constexpr qint64 DEFAULT_MAX_LOG_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB

// AUTO S&P (Search & Pounce) sensitivity limits
constexpr int AUTO_SP_SENSITIVITY_MIN_HZ = 100;      // Minimum frequency change threshold
constexpr int AUTO_SP_SENSITIVITY_MAX_HZ = 100000;   // Maximum frequency change threshold
constexpr int AUTO_SP_SENSITIVITY_STEP_HZ = 100;     // Step size for input dialog

// Grayline propagation window
constexpr int GRAYLINE_WINDOW_MINUTES = 30;  // Minutes before/after sunrise/sunset

// LED/indicator colors (for amplifier panels, meters, etc.) - Hardware colors, not themed
namespace LedColors {
    constexpr const char* GREEN = "#00ff00";         // Hardware: Operating, good status
    constexpr const char* AMBER = "#ffaa00";         // Hardware: Standby, caution
    constexpr const char* RED = "#ff0000";           // Hardware: Fault, TX, error
    constexpr const char* YELLOW = "#ffff00";        // Hardware: Warning, moderate SWR
    constexpr const char* OFF = "#363636";           // Hardware: LED off state
}

// Elecraft brand colors (official palette) - Hardware colors, not themed
namespace ElecraftColors {
    constexpr const char* RED_DAMASK = "#E1783F";    // Hardware: Orange/amber - standby
    constexpr const char* STARSHIP = "#F0E24D";      // Hardware: Yellow - warnings
    constexpr const char* MINE_SHAFT = "#363636";    // Hardware: Dark gray - LED off
}

// UI timing constants (milliseconds)
namespace UITiming {
    constexpr int DEFERRED_ACTION_DELAY_MS = 2000;   // Delay for non-critical startup tasks (CTY check)
    constexpr int QUICK_DELAY_MS = 100;              // Short delay for UI state updates
    constexpr int RECONNECT_DELAY_MS = 500;          // Delay before auto-reconnect after settings change
}

// UI window positioning
namespace UIPositioning {
    constexpr int WINDOW_INITIAL_OFFSET = 50;        // Initial offset from main window for child windows
    constexpr int CASCADE_START_OFFSET = 100;        // Initial offset for cascade repositioning
    constexpr int CASCADE_STEP = 30;                 // Step between cascaded windows
}

// UI window/dialog dimensions
namespace UIDefaults {
    // Layout constraints
    constexpr int BOTTOM_PANEL_MIN_WIDTH = 900;      // Minimum width for bottom panel

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
