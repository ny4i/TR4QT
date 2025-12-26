#include "CountryFile.h"
#include "../data/DXCCRepository.h"
#include "../logging/LogMacros.h"
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
    country.gmtOffset = parts[6].trimmed().toDouble();  // May be fractional
    country.primaryPrefix = parts[7].trimmed();

    country.allPrefixes.append(country.primaryPrefix);

    // Map country name to ADIF DXCC Entity Code
    country.dxccEntity = getDXCCEntityCode(country.name);

    return true;
}

// Get DXCC Entity Code from country name using global database
// Based on ADIF specification: https://adif.org.uk/316/ADIF_316.htm#DXCC_Entity_Code_Enumeration
int CountryFile::getDXCCEntityCode(const QString& countryName) {
    DXCCRepository repo;
    return repo.getEntityCode(countryName);
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

QVector<CountryData> CountryFile::getAllCountries() const {
    QVector<CountryData> countries;
    for (const auto& country : m_countries) {
        countries.append(country);
    }
    return countries;
}

} // namespace TR4QT
