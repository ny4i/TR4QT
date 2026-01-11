#include "FloridaQSOPartyContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "RSTValidator.h"
#include "../logging/LogMacros.h"
#include <QRegularExpression>
#include <QSet>
#include <QMap>

namespace TR4QT {

// ===== Static Data: Florida Counties (67 total) =====
static const QSet<QString> FLORIDA_COUNTIES = {
    "ALC", "BAK", "BAY", "BRA", "BRE", "BRO", "CAH", "CHA", "CIT", "CLA",
    "CLR", "CLM", "DAD", "DES", "DIX", "DUV", "ESC", "FLG", "FRA", "GAD",
    "GIL", "GLA", "GUL", "HAM", "HAR", "HEN", "HER", "HIG", "HIL", "HOL",
    "IDR", "JAC", "JEF", "LAF", "LAK", "LEE", "LEO", "LEV", "LIB", "MAD",
    "MTE", "MAO", "MRT", "MON", "NAS", "OKA", "OKE", "ORA", "OSC", "PAL",
    "PAS", "PIN", "POL", "PUT", "SAN", "SAR", "SEM", "STJ", "STL", "SUM",
    "SUW", "TAY", "UNI", "VOL", "WAK", "WAL", "WAG"
};

// ===== Static Data: County Name Lookup =====
static const QMap<QString, QString> COUNTY_NAMES = {
    {"ALC", "Alachua"}, {"BAK", "Baker"}, {"BAY", "Bay"},
    {"BRA", "Bradford"}, {"BRE", "Brevard"}, {"BRO", "Broward"},
    {"CAH", "Calhoun"}, {"CHA", "Charlotte"}, {"CIT", "Citrus"}, {"CLA", "Clay"},
    {"CLR", "Collier"}, {"CLM", "Columbia"}, {"DAD", "Miami-Dade"}, {"DES", "DeSoto"},
    {"DIX", "Dixie"}, {"DUV", "Duval"}, {"ESC", "Escambia"}, {"FLG", "Flagler"},
    {"FRA", "Franklin"}, {"GAD", "Gadsden"}, {"GIL", "Gilchrist"}, {"GLA", "Glades"},
    {"GUL", "Gulf"}, {"HAM", "Hamilton"}, {"HAR", "Hardee"}, {"HEN", "Hendry"},
    {"HER", "Hernando"}, {"HIG", "Highlands"}, {"HIL", "Hillsborough"}, {"HOL", "Holmes"},
    {"IDR", "Indian River"}, {"JAC", "Jackson"}, {"JEF", "Jefferson"}, {"LAF", "Lafayette"},
    {"LAK", "Lake"}, {"LEE", "Lee"}, {"LEO", "Leon"}, {"LEV", "Levy"},
    {"LIB", "Liberty"}, {"MAD", "Madison"}, {"MTE", "Manatee"}, {"MAO", "Marion"},
    {"MRT", "Martin"}, {"MON", "Monroe"}, {"NAS", "Nassau"}, {"OKA", "Okaloosa"},
    {"OKE", "Okeechobee"}, {"ORA", "Orange"}, {"OSC", "Osceola"}, {"PAL", "Palm Beach"},
    {"PAS", "Pasco"}, {"PIN", "Pinellas"}, {"POL", "Polk"}, {"PUT", "Putnam"},
    {"SAN", "St. Johns"}, {"SAR", "Sarasota"}, {"SEM", "Seminole"}, {"STJ", "St. Johns"},
    {"STL", "St. Lucie"}, {"SUM", "Sumter"}, {"SUW", "Suwannee"}, {"TAY", "Taylor"},
    {"UNI", "Union"}, {"VOL", "Volusia"}, {"WAK", "Wakulla"}, {"WAL", "Walton"},
    {"WAG", "Washington"}
};

// ===== Static Data: Valid US States and Canadian Provinces =====
static const QSet<QString> VALID_STATES = {
    // US States (50 + DC)
    "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA",
    "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD",
    "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ",
    "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC",
    "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY", "DC",
    // Canadian Provinces/Territories (13)
    "AB", "BC", "MB", "NB", "NL", "NT", "NS", "NU", "ON", "PE", "QC", "SK", "YT"
};

// ===== Contest Metadata =====
ContestMetadata FloridaQSOPartyContest::getMetadata() {
    ContestMetadata meta;

    meta.id = "FL_QP";
    meta.displayName = "Florida QSO Party";
    meta.shortName = "FQP";

    // Phone and CW supported (mixed mode)
    meta.supportedModes = {ModeType::CW, ModeType::USB, ModeType::None};
    meta.hasSeparateContests = false;  // Mixed mode

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = WA7BNM_ID_MIXED;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = "";
    meta.cabrilloNameMixed = CABRILLO_NAME_MIXED;

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = ADIF_CONTEST_ID_MIXED;

    meta.schedule = "4th Saturday of April";
    meta.floatingDates = {
        FloatingDate(4, "4th Saturday")  // April
    };
    meta.website = "https://floridaqsoparty.org/";
    meta.description = "In-state: Send County. Out-of-state: Send State/Province/DX. Two 10-hour periods.";

    return meta;
}

ContestBase* FloridaQSOPartyContest::create(ModeType mode, const StationInfo& myStation) {
    return new FloridaQSOPartyContest(mode, myStation);
}

// ===== Constructor =====
FloridaQSOPartyContest::FloridaQSOPartyContest(ModeType mode, const StationInfo& myStation)
    : QSOPartyContestBase(myStation)
    , m_mode(mode)
{
    LOG_DEBUG("FloridaQP", QString("Contest created with state='%1' isInState=%2")
        .arg(m_myStation.state)
        .arg(isInState() ? "true" : "false"));
}

// ===== Contest Identity =====
QString FloridaQSOPartyContest::getContestId() const {
    return "FL_QP";
}

QString FloridaQSOPartyContest::getContestName() const {
    return "Florida QSO Party";
}

QString FloridaQSOPartyContest::getADIFContestId() const {
    return ADIF_CONTEST_ID_MIXED;
}

// ===== Exchange Configuration =====
QList<ExchangeField> FloridaQSOPartyContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (mode-dependent)
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = RSTValidator::getDefault(m_mode);
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // Exchange field (County OR State) - depends on operator location
    ExchangeField exchange;
    if (isInState()) {
        // Florida station: expect State/Province/DX from others
        exchange.name = "State";
        exchange.hint = "State/Prov/DX";
    } else {
        // Non-Florida station: expect County from FL stations
        exchange.name = "County";
        exchange.hint = "FL County";
    }
    exchange.autoFill = false;
    exchange.maxLength = 4;
    fields.append(exchange);

    return fields;
}

QList<ExchangeField> FloridaQSOPartyContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = RSTValidator::getDefault(m_mode);
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // What we send - depends on operator location
    ExchangeField exchange;
    if (isInState()) {
        // Florida station: send county
        exchange.name = "County";
        exchange.hint = m_myStation.county.isEmpty() ? "FL County" : m_myStation.county;
        exchange.autoFill = !m_myStation.county.isEmpty();
    } else {
        // Non-Florida station: send state
        exchange.name = "State";
        exchange.hint = m_myStation.state.isEmpty() ? "State" : m_myStation.state;
        exchange.autoFill = !m_myStation.state.isEmpty();
    }
    exchange.maxLength = 4;
    fields.append(exchange);

    return fields;
}

QList<TableColumn> FloridaQSOPartyContest::getTableColumns() const {
    if (isInState()) {
        // Florida station: display State/Province column
        return {
            TableColumn("State", "ST/PR", 60, TableColumn::Alignment::Left)
        };
    } else {
        // Non-Florida station: display County column
        return {
            TableColumn("County", "County", 60, TableColumn::Alignment::Left)
        };
    }
}

QString FloridaQSOPartyContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);

    if (isInState()) {
        // Florida station sends county
        return rst + " {COUNTY}";
    } else {
        // Non-Florida station sends state
        return rst + " {STATE}";
    }
}

bool FloridaQSOPartyContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = isInState() ?
            "Exchange required (State/Province/DX)" :
            "Exchange required (FL County)";
        return false;
    }

    // Find RST and location parts
    QString rst;
    QString location;

    if (parts.size() == 1) {
        // Only location provided (assume RST is default)
        location = parts[0];
    } else if (parts.size() >= 2) {
        // Try to find RST
        QString first = parts[0];
        QString second = parts[1];

        bool firstIsRST = RSTValidator::isValid(first, m_mode);
        bool secondIsRST = RSTValidator::isValid(second, m_mode);

        if (firstIsRST && !secondIsRST) {
            rst = first;
            location = second;
        } else if (!firstIsRST && secondIsRST) {
            rst = second;
            location = first;
        } else if (firstIsRST && secondIsRST) {
            // Both look like RST - assume traditional order (RST first)
            rst = first;
            location = second;
        } else {
            // Neither looks like RST - take first as location
            location = first;
        }
    }

    // Validate RST if provided
    if (!rst.isEmpty() && !RSTValidator::isValid(rst, m_mode)) {
        QString expectedFormat = (m_mode == ModeType::CW) ?
            "3 digits (e.g., 599)" : "2-3 digits (e.g., 59)";
        errorMsg = QString("Invalid RST format. Expected %1").arg(expectedFormat);
        return false;
    }

    // Validate location based on operator's state
    QString upper = location.toUpper();

    if (isInState()) {
        // Florida station: expect state/province/DX from others
        // Accept any non-empty value (could be state, province, or DX prefix)
        if (upper.isEmpty()) {
            errorMsg = "State/Province/DX required";
            return false;
        }
        // Basic validation: 2-4 characters
        if (upper.length() < 2 || upper.length() > 4) {
            errorMsg = QString("Invalid State/Province/DX: %1").arg(location);
            return false;
        }
    } else {
        // Non-Florida station: expect Florida county
        if (!isValidFloridaCounty(upper)) {
            errorMsg = QString("Invalid Florida county: %1. Must be one of the 67 FL county abbreviations.").arg(location);
            return false;
        }
    }

    return true;
}

void FloridaQSOPartyContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        return;
    }

    // Auto-detect order: "599 PAL" or "PAL 599"
    QString rst;
    QString location;

    if (parts.size() == 1) {
        // Only location provided (assume RST is default)
        rst = RSTValidator::getDefault(m_mode);
        location = parts[0];
    } else if (parts.size() >= 2) {
        // Two or more parts: detect which is RST
        QString first = parts[0];
        QString second = parts[1];

        bool firstIsRST = RSTValidator::isValid(first, m_mode);
        bool secondIsRST = RSTValidator::isValid(second, m_mode);

        if (firstIsRST && !secondIsRST) {
            rst = first;
            location = second;
        } else if (!firstIsRST && secondIsRST) {
            rst = second;
            location = first;
        } else if (firstIsRST && secondIsRST) {
            // Both look like RST - assume traditional order (RST first)
            rst = first;
            location = second;
        } else {
            // Neither looks like RST - take first as location
            rst = RSTValidator::getDefault(m_mode);
            location = first;
        }
    }

    // Store RST
    qso.rstReceived = rst;

    // Determine if location is a county or state
    QString upper = location.toUpper();

    if (isValidFloridaCounty(upper)) {
        // It's a Florida county
        qso.county = upper;
        qso.state = "FL";  // County implies Florida
    } else if (isValidStateOrProvince(upper)) {
        // It's a US state or Canadian province
        qso.state = upper;
        qso.county = "";  // State/Province has no county
    } else {
        // Might be DX prefix or maritime mobile
        qso.state = upper;  // Store as-is in state field
        qso.county = "";
    }

    // Format exchangeReceived with RST prepended (e.g., "599 PAL" or "599 GA")
    formatExchangeReceived(exchange, qso);
}

// ===== Scoring =====
int FloridaQSOPartyContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const {
    Q_UNUSED(myStation);

    // Florida QSO Party scoring:
    // - Phone: 1 point per QSO
    // - CW: 2 points per QSO

    if (qso.mode == ModeType::CW || qso.mode == ModeType::CWR) {
        return 2;
    }

    // Phone modes (SSB, FM, AM)
    return 1;
}

int FloridaQSOPartyContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const {

    // Florida QSO Party formula: QSO Points × Multipliers × Power Multiplier

    // Total multipliers (sum of all multiplier types)
    int totalMults = 0;
    for (int count : multiplierCounts.values()) {
        totalMults += count;
    }

    // Avoid division by zero
    if (totalMults == 0) {
        totalMults = 1;
    }

    // Base score (before power multiplier)
    int baseScore = totalQSOPoints * totalMults;

    // Apply power multiplier (configured in station settings)
    int powerMultiplier = getPowerMultiplier();

    return baseScore * powerMultiplier;
}

// ===== Multipliers =====
QList<MultiplierDefinition> FloridaQSOPartyContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    if (isInState()) {
        // Florida station multipliers: States + Provinces + DXCC

        MultiplierDefinition stateMult;
        stateMult.type = MultiplierType::State;
        stateMult.scope = MultiplierScope::AllBands;
        stateMult.displayName = "States/Provinces";
        mults.append(stateMult);

        MultiplierDefinition countryMult;
        countryMult.type = MultiplierType::Country;
        countryMult.scope = MultiplierScope::AllBands;
        countryMult.displayName = "DXCC";
        mults.append(countryMult);

        // Note: Maritime mobile ITU regions would be tracked separately if needed

    } else {
        // Non-Florida station multipliers: Florida counties only

        MultiplierDefinition countyMult;
        countyMult.type = MultiplierType::County;
        countyMult.scope = MultiplierScope::AllBands;
        countyMult.displayName = "FL Counties";
        mults.append(countyMult);
    }

    return mults;
}

QString FloridaQSOPartyContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const {

    QString multValue;

    if (isInState()) {
        // Florida station: count states/provinces and countries
        if (multType == MultiplierType::State) {
            multValue = qso.state.toUpper();
            // Validate: Must be valid US state or Canadian province
            if (!isValidStateOrProvince(multValue)) {
                return QString();
            }
        } else if (multType == MultiplierType::Country) {
            multValue = qso.dxccPrefix.toUpper();
            if (multValue.isEmpty()) {
                return QString();
            }
        }
    } else {
        // Non-Florida station: count Florida counties
        if (multType == MultiplierType::County) {
            multValue = qso.county.toUpper();
            // Validate: Must be valid Florida county abbreviation
            if (!isValidFloridaCounty(multValue)) {
                return QString();
            }
        }
    }

    // Check if already worked
    if (multValue.isEmpty() || alreadyWorkedValues.contains(multValue)) {
        return QString();
    }

    return multValue;
}

// ===== Band and Mode Restrictions =====
QList<BandType> FloridaQSOPartyContest::getAllowedBands() const {
    // Florida QSO Party: 40M, 20M, 15M, 10M ONLY
    // NO 160M, 80M, WARC (60M, 30M, 17M, 12M), or VHF
    return {
        BandType::Band40M,
        BandType::Band20M,
        BandType::Band15M,
        BandType::Band10M
    };
}

bool FloridaQSOPartyContest::isValidMode(ModeType mode, QString& errorMsg) const {
    // Florida QSO Party: Phone and CW ONLY
    // NO digital modes (RTTY, PSK, FT8, FT4, etc.)

    switch (mode) {
        // Valid: CW
        case ModeType::CW:
        case ModeType::CWR:
            return true;

        // Valid: Phone modes
        case ModeType::LSB:
        case ModeType::USB:
        case ModeType::FM:
        case ModeType::AM:
            return true;

        // INVALID: All digital modes
        case ModeType::RTTY:
        case ModeType::RTTYR:
        case ModeType::PSK:
        case ModeType::PSKR:
        case ModeType::FT8:
        case ModeType::FT4:
        case ModeType::DATA:
        case ModeType::DATAR:
            errorMsg = "Digital modes are not allowed in Florida QSO Party. Phone and CW only.";
            return false;

        case ModeType::None:
        default:
            errorMsg = "Invalid mode for Florida QSO Party";
            return false;
    }
}

QMap<QString, QString> FloridaQSOPartyContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers;
    headers["CONTEST"] = CABRILLO_NAME_MIXED;
    return headers;
}

// ===== Private Helper Methods =====
bool FloridaQSOPartyContest::isInState() const {
    QString state = m_myStation.state.trimmed().toUpper();
    bool result = (state == "FL");
    LOG_DEBUG("FloridaQP", QString("isInState() check: state='%1' trimmed/upper='%2' result=%3")
        .arg(m_myStation.state)
        .arg(state)
        .arg(result ? "true" : "false"));
    return result;
}

bool FloridaQSOPartyContest::isValidFloridaCounty(const QString& county) const {
    return FLORIDA_COUNTIES.contains(county.toUpper());
}

bool FloridaQSOPartyContest::isValidStateOrProvince(const QString& state) const {
    return VALID_STATES.contains(state.toUpper());
}

QString FloridaQSOPartyContest::getCountyName(const QString& abbrev) const {
    return COUNTY_NAMES.value(abbrev.toUpper(), abbrev);
}

int FloridaQSOPartyContest::getPowerMultiplier() const {
    // Power multipliers from station settings
    int watts = m_myStation.power;

    if (watts <= 5) {
        return 3;  // QRP
    } else if (watts < 100) {
        return 2;  // Low power
    } else {
        return 1;  // High power (no multiplier)
    }
}

// ===== Contest Registration =====
REGISTER_CONTEST(FloridaQSOPartyContest, "FL_QP")

} // namespace TR4QT
