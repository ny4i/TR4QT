#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace TR4QT {

// Application info
constexpr const char* APP_NAME = "TR4QT";
constexpr const char* APP_VERSION = "2.81.0";  // Add morse code sending feature
constexpr const char* APP_ORG = "TR4QT";

// Country file
constexpr const char* COUNTRY_FILE_URL = "https://www.country-files.com/";
constexpr const char* COUNTRY_FILE_NAME = "cty.dat";
constexpr int CURRENT_CTY_VERSION = 3540;  // Update as needed

// Database
constexpr const char* DB_DIR = ".tr4qt/logs";
constexpr const char* CONFIG_DIR = ".tr4qt";

// Radio polling
constexpr int DEFAULT_POLL_INTERVAL_MS = 500;  // 2 Hz

// UI defaults
constexpr int DEFAULT_RST = 599;

} // namespace TR4QT

#endif // CONSTANTS_H
