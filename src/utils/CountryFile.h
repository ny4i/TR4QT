#ifndef COUNTRYFILE_H
#define COUNTRYFILE_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>
#include <QReadWriteLock>
#include "../core/Types.h"

namespace TR4QT {

// Country data structure
struct CountryData {
    QString name;              // "United States"
    int dxccEntity{0};         // DXCC entity number
    int cqZone{0};             // Default CQ zone (1-40)
    int ituZone{0};            // Default ITU zone
    Continent continent{Continent::None};  // Continent
    double latitude{0.0};      // Latitude (+ = North)
    double longitude{0.0};     // Longitude (+ = West)
    int gmtOffset{0};          // GMT offset in hours
    QString primaryPrefix;     // Primary prefix (e.g., "K")

    // All prefixes for this country (including aliases)
    QStringList allPrefixes;

    // Override maps for specific prefixes
    // Key: prefix, Value: override value
    QHash<QString, int> cqZoneOverrides;    // e.g., "KH6" → 31
    QHash<QString, int> ituZoneOverrides;   // e.g., "KH6" → 61

    // Exact match callsigns (prefixed with = in cty.dat)
    QStringList exactMatchCallsigns;

    bool isValid() const { return !name.isEmpty(); }
};

// Main country file parser and lookup class
class CountryFile {
public:
    CountryFile();
    ~CountryFile() = default;

    // Load country file from disk
    bool loadFromFile(const QString& filePath);

    // Get current version
    QString getVersion() const { return m_version; }

    // Set version (called after loading from download)
    void setVersion(const QString& version) { m_version = version; }

    // Lookup a callsign and return country data
    CountryData lookup(const QString& callsign) const;

    // Get all countries (for UI display, etc.)
    QVector<CountryData> getAllCountries() const;

    // Extract prefix from callsign for WPX contests
    static QString extractWPXPrefix(const QString& callsign);

    // Strip portable indicators (/P, /M, /MM, /QRP, etc.)
    static QString stripPortable(const QString& callsign);

    // Get CQ zone from US call area (K6→3, K5→4, K1→5, etc.)
    // Returns -1 if not a US call or unable to determine
    static int getUSCallAreaZone(const QString& callsign);

    // Map CTY.DAT country name to ADIF DXCC Entity Code
    // Returns 0 if country not found in mapping
    static int getDXCCEntityCode(const QString& countryName);

private:
    // Parse a single country entry (can span multiple lines)
    bool parseCountryEntry(const QString& mainLine, const QStringList& aliasLines);

    // Parse the main country line (first line with country name and zones)
    bool parseMainLine(const QString& line, CountryData& country);

    // Parse alias/prefix lines
    void parseAliases(const QStringList& aliasLines, CountryData& country);

    // Handle special prefix markers: =call, (zone), [zone], etc.
    QString parseSpecialPrefix(const QString& prefix, CountryData& country);

    // Find the longest matching prefix for a callsign
    QString findMatchingPrefix(const QString& callsign) const;

    // Storage
    QHash<QString, CountryData> m_countries;  // Key: primary prefix
    QHash<QString, QString> m_prefixMap;      // Alias prefix → Primary prefix
    QHash<QString, QString> m_exactMatches;   // Exact callsign → Primary prefix

    QString m_version;

    // Thread safety: Protects all data structures during reload
    mutable QReadWriteLock m_lock;
};

} // namespace TR4QT

#endif // COUNTRYFILE_H
