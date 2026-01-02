#include "CountryFile.h"
#include "../models/QSO.h"
#include "../data/DXCCRepository.h"
#include "../logging/LogMacros.h"
#include "GeographicUtils.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace TR4QT {

CountryFile::CountryFile() {
}

bool CountryFile::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("CountryFile", QString("Failed to open country file: %1").arg(filePath));
        return false;
    }

    // Exclusive lock for writing - blocks all readers until reload completes
    QWriteLocker locker(&m_lock);

    m_countries.clear();
    m_prefixMap.clear();
    m_exactMatches.clear();

    QTextStream in(&file);
    QString mainLine;
    QStringList aliasLines;
    bool inAliases = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith("#")) {
            continue;  // Skip empty lines and comments
        }

        // Check if this is a main country line (starts with country name, has colons)
        if (!inAliases && line.contains(':')) {
            // This is a new country entry
            if (!mainLine.isEmpty()) {
                // Process previous country
                parseCountryEntry(mainLine, aliasLines);
                aliasLines.clear();
            }
            mainLine = line;
            inAliases = true;
        } else if (inAliases) {
            // This is an alias/prefix line
            aliasLines.append(line);

            // Check if this line ends with semicolon (end of entry)
            if (line.endsWith(';')) {
                parseCountryEntry(mainLine, aliasLines);
                mainLine.clear();
                aliasLines.clear();
                inAliases = false;
            }
        }
    }

    // Process last entry if needed
    if (!mainLine.isEmpty()) {
        parseCountryEntry(mainLine, aliasLines);
    }

    file.close();

    LOG_DEBUG("CountryFile", QString("Loaded %1 countries from %2").arg(m_countries.size()).arg(filePath));
    return !m_countries.isEmpty();
}

bool CountryFile::parseCountryEntry(const QString& mainLine, const QStringList& aliasLines) {
    CountryData country;

    if (!parseMainLine(mainLine, country)) {
        return false;
    }

    parseAliases(aliasLines, country);

    // Store in hash table
    m_countries[country.primaryPrefix] = country;

    return true;
}

bool CountryFile::parseMainLine(const QString& line, CountryData& country) {
    // CTY.DAT format (colon-delimited):
    // Column 1-26: Country Name
    // Column 27-31: CQ Zone
    // Column 32-36: ITU Zone
    // Column 37-41: Continent (2-letter)
    // Column 42-50: Latitude
    // Column 51-60: Longitude
    // Column 61-69: GMT offset
    // Column 70+: Primary DXCC Prefix

    QStringList parts = line.split(':');
    if (parts.size() < 8) {
        LOG_WARN("CountryFile", QString("Invalid main line format: %1").arg(line));
        return false;
    }

    country.name = parts[0].trimmed();
    country.cqZone = parts[1].trimmed().toInt();
    country.ituZone = parts[2].trimmed().toInt();

    // Parse continent
    QString contStr = parts[3].trimmed();
    if (contStr == "AF") country.continent = Continent::AF;
    else if (contStr == "AS") country.continent = Continent::AS;
    else if (contStr == "EU") country.continent = Continent::EU;
    else if (contStr == "NA") country.continent = Continent::NA;
    else if (contStr == "SA") country.continent = Continent::SA;
    else if (contStr == "OC") country.continent = Continent::OC;

    country.latitude = parts[4].trimmed().toDouble();
    country.longitude = parts[5].trimmed().toDouble();

    // CTY.DAT format quirk: Longitudes use opposite sign from standard geographic convention
    // Western longitudes (Americas) are stored as positive (should be negative)
    // Eastern longitudes (most others) are stored as negative (should be positive)
    // Exception: A few European countries near Greenwich are stored with correct positive values
    // Solution: Negate ALL longitudes EXCEPT those that are small positive values (< 20° indicates Europe/Africa near Greenwich)
    if (country.longitude < -20.0 || country.longitude > 20.0) {
        country.longitude = -country.longitude;
    }

    country.gmtOffset = parts[6].trimmed().toDouble();  // May be fractional
    country.primaryPrefix = parts[7].trimmed();

    country.allPrefixes.append(country.primaryPrefix);

    // Map country name to ADIF DXCC Entity Code
    country.dxccEntity = getDXCCEntityCode(country.name);

    if (country.dxccEntity == 0) {
        LOG_DEBUG("CountryFile", QString("No DXCC code found for country: '%1' (deleted entity or alias)").arg(country.name));
    }

    return true;
}

// Get DXCC Entity Code from country name using global database
// Based on ADIF specification: https://adif.org.uk/316/ADIF_316.htm#DXCC_Entity_Code_Enumeration
int CountryFile::getDXCCEntityCode(const QString& countryName) {
    // CTY.DAT uses different country names than ADIF spec
    // Try multiple strategies to find the correct DXCC entity code

    DXCCRepository& dxccRepo = DXCCRepository::instance();

    // Strategy 1: Try exact match (rare, but fast)
    int code = dxccRepo.getEntityCode(countryName);
    if (code > 0) {
        return code;
    }

    // Strategy 2: Try uppercase match (ADIF uses all caps)
    code = dxccRepo.getEntityCode(countryName.toUpper());
    if (code > 0) {
        return code;
    }

    // Strategy 3: Try expanding abbreviations first
    QString expandedName = countryName.toUpper();

    // Expand common abbreviations used in CTY.DAT
    if (expandedName.startsWith("FED. REP. OF ")) {
        expandedName.replace("FED. REP. OF ", "FEDERAL REPUBLIC OF ");
    } else if (expandedName.startsWith("DEM. REP. OF ")) {
        expandedName.replace("DEM. REP. OF ", "DEMOCRATIC REPUBLIC OF ");
    } else if (expandedName.startsWith("DPR OF ")) {
        // "DPR of Korea" → "DEMOCRATIC PEOPLE'S REP. OF KOREA"
        expandedName.replace("DPR OF ", "DEMOCRATIC PEOPLE'S REP. OF ");
    } else if (expandedName.startsWith("REP. OF ")) {
        expandedName.replace("REP. OF ", "REPUBLIC OF ");
    }

    code = dxccRepo.getEntityCode(expandedName);
    if (code > 0) {
        return code;
    }

    // Strategy 4: Try fuzzy match - strip common prefixes/suffixes
    // Remove common political designations that differ between CTY.DAT and ADIF
    QString fuzzyName = countryName.toUpper();

    // Remove common prefixes
    QStringList prefixesToRemove = {
        "FED. REP. OF ",
        "DEM. REP. OF THE ",
        "DEM. REP. OF ",
        "DEMOCRATIC REPUBLIC OF THE ",
        "DEMOCRATIC REPUBLIC OF ",
        "REPUBLIC OF THE ",
        "REPUBLIC OF ",
        "PEOPLE'S REP. OF ",
        "PEOPLE'S REPUBLIC OF ",
        "ISLAMIC REP. OF ",
        "ISLAMIC REPUBLIC OF ",
        "KINGDOM OF ",
        "STATE OF ",
        "TERRITORY OF ",
        "THE "
    };

    for (const QString& prefix : prefixesToRemove) {
        if (fuzzyName.startsWith(prefix)) {
            fuzzyName = fuzzyName.mid(prefix.length());
            code = dxccRepo.getEntityCode(fuzzyName);
            if (code > 0) {
                return code;
            }
        }
    }

    // Strategy 5: Try a few common known mappings that don't fit the pattern
    static QMap<QString, QString> specialCases;
    if (specialCases.isEmpty()) {
        specialCases["UNITED STATES"] = "UNITED STATES OF AMERICA";
        specialCases["HAWAII"] = "HAWAII";
        specialCases["ALASKA"] = "ALASKA";
        specialCases["CANADA"] = "CANADA";
        // Add other special cases as discovered
    }

    QString upperName = countryName.toUpper();
    if (specialCases.contains(upperName)) {
        code = dxccRepo.getEntityCode(specialCases[upperName]);
        if (code > 0) {
            return code;
        }
    }

    // Not found - this is expected for deleted DXCC entities
    LOG_DEBUG("CountryFile", QString("No DXCC code found for country: '%1' (deleted entity, expected)").arg(countryName));
    return 0;
}

void CountryFile::parseAliases(const QStringList& aliasLines, CountryData& country) {
    // Join all alias lines and split by comma
    QString allAliases = aliasLines.join("");
    allAliases.remove(';');  // Remove trailing semicolon

    QStringList prefixes = allAliases.split(',', Qt::SkipEmptyParts);

    for (QString prefix : prefixes) {
        prefix = prefix.trimmed();
        if (prefix.isEmpty()) continue;

        // Parse special prefix and add to appropriate list
        QString cleanPrefix = parseSpecialPrefix(prefix, country);

        if (!cleanPrefix.isEmpty()) {
            country.allPrefixes.append(cleanPrefix);
            m_prefixMap[cleanPrefix] = country.primaryPrefix;
        }
    }
}

QString CountryFile::parseSpecialPrefix(const QString& prefix, CountryData& country) {
    QString clean = prefix;

    // Handle exact match (=KH6 means match "KH6" exactly, not "KH6ABC")
    if (clean.startsWith('=')) {
        QString exactCall = clean.mid(1);  // Remove =
        m_exactMatches[exactCall] = country.primaryPrefix;
        country.exactMatchCallsigns.append(exactCall);
        return "";  // Don't add to regular prefix list
    }

    // Handle CQ zone override: KH6(31) means KH6 is in zone 31
    static QRegularExpression cqZoneRe(R"(^(.+?)\((\d+)\))");
    QRegularExpressionMatch cqMatch = cqZoneRe.match(clean);
    if (cqMatch.hasMatch()) {
        QString pfx = cqMatch.captured(1);
        int zone = cqMatch.captured(2).toInt();
        country.cqZoneOverrides[pfx] = zone;
        clean = pfx;
    }

    // Handle ITU zone override: KH6[61] means KH6 is in ITU zone 61
    static QRegularExpression ituZoneRe(R"(^(.+?)\[(\d+)\])");
    QRegularExpressionMatch ituMatch = ituZoneRe.match(clean);
    if (ituMatch.hasMatch()) {
        QString pfx = ituMatch.captured(1);
        int zone = ituMatch.captured(2).toInt();
        country.ituZoneOverrides[pfx] = zone;
        clean = pfx;
    }

    // Note: cty.dat format doesn't support continent overrides <xx>
    // or lat/lon overrides, but we could add them if needed

    return clean;
}

CountryData CountryFile::lookup(const QString& callsign) const {
    if (callsign.isEmpty()) {
        return CountryData();
    }

    // Shared lock for reading - multiple readers can run concurrently
    QReadLocker locker(&m_lock);

    QString cleanCall = stripPortable(callsign).toUpper();

    // First, check exact matches
    if (m_exactMatches.contains(cleanCall)) {
        QString primaryPrefix = m_exactMatches[cleanCall];
        if (m_countries.contains(primaryPrefix)) {
            CountryData country = m_countries[primaryPrefix];

            // Check for zone overrides for this exact callsign
            bool hasExactCQOverride = false;
            if (country.cqZoneOverrides.contains(cleanCall)) {
                country.cqZone = country.cqZoneOverrides[cleanCall];
                hasExactCQOverride = true;
            }
            if (country.ituZoneOverrides.contains(cleanCall)) {
                country.ituZone = country.ituZoneOverrides[cleanCall];
            }

            // Apply US call area zone logic ONLY if no cty.dat override exists
            if (!hasExactCQOverride && country.name == "United States") {
                int usZone = getUSCallAreaZone(cleanCall);
                if (usZone > 0) {
                    country.cqZone = usZone;
                }
            }

            return country;
        }
    }

    // Find longest matching prefix
    QString matchedPrefix = findMatchingPrefix(cleanCall);

    if (!matchedPrefix.isEmpty() && m_prefixMap.contains(matchedPrefix)) {
        QString primaryPrefix = m_prefixMap[matchedPrefix];
        if (m_countries.contains(primaryPrefix)) {
            CountryData country = m_countries[primaryPrefix];

            // Apply zone overrides if they exist for this prefix
            bool hasPrefixCQOverride = false;
            if (country.cqZoneOverrides.contains(matchedPrefix)) {
                country.cqZone = country.cqZoneOverrides[matchedPrefix];
                hasPrefixCQOverride = true;
            }
            if (country.ituZoneOverrides.contains(matchedPrefix)) {
                country.ituZone = country.ituZoneOverrides[matchedPrefix];
            }

            // Apply US call area zone logic ONLY if no cty.dat override exists
            if (!hasPrefixCQOverride && country.name == "United States") {
                int usZone = getUSCallAreaZone(cleanCall);
                if (usZone > 0) {
                    country.cqZone = usZone;
                }
            }

            return country;
        }
    }

    // Not found
    return CountryData();
}

QString CountryFile::findMatchingPrefix(const QString& callsign) const {
    // Find the longest matching prefix
    QString longestMatch;
    int longestLength = 0;

    for (auto it = m_prefixMap.constBegin(); it != m_prefixMap.constEnd(); ++it) {
        const QString& prefix = it.key();

        // Check if callsign starts with this prefix
        if (callsign.startsWith(prefix)) {
            if (prefix.length() > longestLength) {
                longestMatch = prefix;
                longestLength = prefix.length();
            }
        }
    }

    return longestMatch;
}

QString CountryFile::stripPortable(const QString& callsign) {
    // Remove portable indicators: /P, /M, /MM, /QRP, /AM, /1, /2, etc.
    // Take the base callsign (longest part)
    QStringList parts = callsign.split('/');

    if (parts.size() == 1) {
        return callsign;  // No slashes
    }

    // Find the longest part (usually the base callsign)
    QString longest;
    for (const QString& part : parts) {
        if (part.length() > longest.length()) {
            longest = part;
        }
    }

    return longest;
}

QString CountryFile::extractWPXPrefix(const QString& callsign) {
    // Extract WPX prefix: everything up to and including first digit
    // W1AW → W1
    // DL1ABC → DL1
    // JA1234XYZ → JA1

    QString base = stripPortable(callsign);

    // Find first digit
    int firstDigit = -1;
    for (int i = 0; i < base.length(); i++) {
        if (base[i].isDigit()) {
            firstDigit = i;
            break;
        }
    }

    if (firstDigit == -1) {
        return base;  // No digit found, return whole callsign
    }

    // Return everything up to and including first digit
    return base.left(firstDigit + 1);
}

int CountryFile::getUSCallAreaZone(const QString& callsign) {
    // Determine CQ zone from US call area
    // Based on standard ARRL/CQ zone assignments

    QString base = stripPortable(callsign).toUpper();

    // Check if this is a US callsign: starts with K, W, N, or A followed by digit
    if (base.length() < 2) {
        return -1;  // Too short
    }

    QChar firstChar = base[0];
    QChar secondChar = base[1];

    // Must start with K, W, N, or A
    if (firstChar != 'K' && firstChar != 'W' && firstChar != 'N' && firstChar != 'A') {
        return -1;  // Not a US call
    }

    // Second character must be a digit (call area number)
    if (!secondChar.isDigit()) {
        return -1;  // Not standard US format
    }

    int callArea = secondChar.digitValue();

    // Map call area to CQ zone
    // Zone 3 (Pacific): 6, 7, plus KH6/KL7 (handled separately)
    // Zone 4 (Central): 0, 5
    // Zone 5 (Eastern): 1, 2, 3, 4, 8, 9

    // Special cases for Alaska (KL7) and Hawaii (KH6, AH6, etc.)
    if (base.startsWith("KL") || base.startsWith("AL") ||
        base.startsWith("NL") || base.startsWith("WL")) {
        return 1;  // Alaska is zone 1
    }
    if (base.startsWith("KH") || base.startsWith("AH") ||
        base.startsWith("NH") || base.startsWith("WH")) {
        return 31;  // Hawaii is zone 31
    }

    // Standard continental US call areas
    // Based on official CQ WW zone definitions
    switch (callArea) {
        case 6:
        case 7:
            return 3;  // Pacific (CA, NV, OR, WA, ID, MT, WY, UT, AZ)
        case 0:
        case 5:
        case 8:
        case 9:
            return 4;  // Central (W0: CO, IA, KS, MN, MO, ND, NE, SD)
                       //         (W5: AR, LA, MS, NM, OK, TX)
                       //         (W8: MI, OH, WV)
                       //         (W9: IL, IN, WI)
        case 1:
        case 2:
        case 3:
        case 4:
            return 5;  // Eastern (W1: CT, ME, MA, NH, RI, VT)
                       //         (W2: NJ, NY)
                       //         (W3: DE, MD, PA, DC)
                       //         (W4: AL, FL, GA, KY, NC, SC, TN, VA)
        default:
            return -1;  // Unknown
    }
}

bool CountryFile::getUSCallAreaCoordinates(const QString& callsign, int dxccEntity, double& lat, double& lon) {
    // Only apply to DXCC 291 (United States)
    // Alaska (DXCC 6) and Hawaii (DXCC 110) use country centers from CTY.DAT
    if (dxccEntity != 291) {
        return false;  // Not US mainland
    }

    // TODO: Known edge cases for future enhancement:
    // 1. Portable callsigns: Should use operating location, not home callsign
    //    Examples:
    //    - Suffix: W6ABC/7 (operating in area 7), VE7ABC/W4 (VE7 operating in W4)
    //    - Prefix: I9/NY4I (NY4I operating in Italy), W4/VE7ABC (VE7 operating in W4)
    //    Current behavior:
    //    - stripPortable() removes portable indicator entirely
    //    - We use home callsign for geographic lookup (incorrect for operating location)
    //    Correct behavior:
    //    - Log entire callsign as-is (e.g., "I9/NY4I")
    //    - Parse portable indicator (prefix or suffix) for geographic lookup
    //    - Use portable location for distance/bearing (e.g., "I9" → Italy)
    // 2. Multi-digit callsigns (e.g., 9N1AA, 4U100ABC, W100AW)
    //    - Current logic finds first digit only, may not handle multiple digits correctly
    //    - Examples: 9N1AA (Nepal), 4U100ABC (UN), W100AW (special event)
    //    - Action needed: Audit all callsign regex patterns in codebase
    //    - Ensure regex allows for multiple digits: \d+ instead of \d
    //    - Known issue: DXClusterListDownloader.cpp:161 uses "\\d" (single digit)
    //      Pattern: "^([A-Z0-9]+?)\\d" should be "^([A-Z0-9]+?)\\d+"
    //    - Search for other patterns that assume single digit and update them

    QString base = stripPortable(callsign).toUpper();

    // Check if this is a US callsign: starts with K, W, N, or A
    if (base.length() < 2) {
        return false;  // Too short
    }

    QChar firstChar = base[0];

    // Must start with K, W, N, or A
    if (firstChar != 'K' && firstChar != 'W' && firstChar != 'N' && firstChar != 'A') {
        return false;  // Not a US call
    }

    // Find the first digit (call area number) - can be at position 1 or 2
    // Examples: W6ABC (pos 1), KA6ABC (pos 2), N1XYZ (pos 1), WB2XYZ (pos 2)
    int callArea = -1;
    for (int i = 1; i < base.length() && i < 3; ++i) {  // Check positions 1-2 only
        if (base[i].isDigit()) {
            callArea = base[i].digitValue();
            break;
        }
    }

    if (callArea == -1) {
        return false;  // No digit found in expected positions
    }

    // Map call area number to grid square (closest to center of call area)
    // Grid squares provided for accurate distance/bearing calculations
    QString gridSquare;
    switch (callArea) {
        case 1: gridSquare = "FN43"; break;  // W1: New England
        case 2: gridSquare = "FN22"; break;  // W2: NY/NJ
        case 3: gridSquare = "FN10"; break;  // W3: Mid-Atlantic
        case 4: gridSquare = "EL83"; break;  // W4: Southeast
        case 5: gridSquare = "EM13"; break;  // W5: South Central
        case 6: gridSquare = "DM06"; break;  // W6: California
        case 7: gridSquare = "DN42"; break;  // W7: Pacific NW
        case 8: gridSquare = "EN80"; break;  // W8: Great Lakes East
        case 9: gridSquare = "EN52"; break;  // W9: Great Lakes West
        case 0: gridSquare = "EN04"; break;  // W0: Central
        default:
            return false;  // Unknown call area
    }

    // Convert grid square to lat/lon coordinates
    return GeographicUtils::gridToLatLon(gridSquare, lat, lon);
}

QVector<CountryData> CountryFile::getAllCountries() const {
    // Shared lock for reading - multiple readers can run concurrently
    QReadLocker locker(&m_lock);

    QVector<CountryData> countries;
    for (const auto& country : m_countries) {
        countries.append(country);
    }
    return countries;
}

QStringList CountryFile::getAllPrimaryPrefixes() const {
    // Shared lock for reading - multiple readers can run concurrently
    QReadLocker locker(&m_lock);

    // m_countries key is the primary prefix
    QStringList prefixes = m_countries.keys();
    prefixes.sort();
    return prefixes;
}

void CountryFile::populateQSODXCCFields(QSO& qso) const {
    if (qso.callsign.isEmpty()) {
        return;
    }

    CountryData countryData = lookup(qso.callsign);

    if (!countryData.isValid()) {
        return;
    }

    // Populate all DXCC-related fields
    // This is the SINGLE source of truth for DXCC data population
    qso.dxccEntity = countryData.name;
    qso.dxccEntityCode = countryData.dxccEntity;
    qso.dxccPrefix = countryData.primaryPrefix;  // Always use PRIMARY prefix (e.g., "F" not "TM6")
    qso.continent = continentToString(countryData.continent);
    qso.cqZone = countryData.cqZone;
    qso.ituZone = countryData.ituZone;
}

} // namespace TR4QT
