#include "WinterFieldDayContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata WinterFieldDayContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "WFD";
    meta.displayName = "Winter Field Day";
    meta.shortName = "WFD";
    meta.supportedModes = {ModeType::CW, ModeType::USB, ModeType::RTTY, ModeType::PSK, ModeType::None};
    meta.hasSeparateContests = false;

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = WA7BNM_ID;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = "";
    meta.cabrilloNameMixed = CABRILLO_NAME;

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = ADIF_CONTEST_ID;

    meta.schedule = "Last full weekend of January";
    meta.website = "https://www.winterfieldday.com/";
    meta.description = "Portable operations in winter conditions. Exchange: Class + ARRL/RAC Section.";

    return meta;
}

ContestBase* WinterFieldDayContest::create(ModeType mode) {
    Q_UNUSED(mode);  // Winter Field Day is mixed mode
    return new WinterFieldDayContest();
}

WinterFieldDayContest::WinterFieldDayContest()
{
}

QString WinterFieldDayContest::getContestId() const {
    return "WFD";
}

QString WinterFieldDayContest::getContestName() const {
    return "Winter Field Day";
}

QList<ExchangeField> WinterFieldDayContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // Class (e.g., "1O", "2O", "3O", "2I", "Home")
    ExchangeField classField;
    classField.name = "Class";
    classField.hint = "1O";
    classField.autoFill = false;
    classField.maxLength = 6;
    fields.append(classField);

    // Section (ARRL/RAC section)
    ExchangeField sectionField;
    sectionField.name = "Section";
    sectionField.hint = "WMA";
    sectionField.autoFill = false;
    sectionField.maxLength = 4;
    fields.append(sectionField);

    return fields;
}

QList<ExchangeField> WinterFieldDayContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // Class - to be configured in station settings
    ExchangeField classField;
    classField.name = "Class";
    classField.hint = "1O";
    classField.autoFill = true;  // From station settings
    classField.maxLength = 6;
    fields.append(classField);

    // Section - from station settings
    ExchangeField sectionField;
    sectionField.name = "Section";
    sectionField.hint = "WMA";
    sectionField.autoFill = true;  // From station settings
    sectionField.maxLength = 4;
    fields.append(sectionField);

    return fields;
}

QString WinterFieldDayContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    Q_UNUSED(rst);

    // Exchange should come from station settings
    // Format: "Class Section" (e.g., "1O WMA")
    // This will be populated from StationInfo in the logging system
    return "1O WMA";  // Placeholder - actual value from settings
}

bool WinterFieldDayContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() != 2) {
        errorMsg = "Exchange must be: Class + Section (e.g., '1O WMA')";
        return false;
    }

    QString classStr = parts[0].toUpper();
    QString section = parts[1].toUpper();

    // Validate class
    if (!isValidClass(classStr)) {
        errorMsg = QString("Invalid class '%1'. Expected: 1O, 2O, 3O, 1I, 2I, 3I, Home, etc.").arg(classStr);
        return false;
    }

    // Validate section
    if (!isValidSection(section)) {
        errorMsg = QString("Invalid section '%1'. Must be valid ARRL or RAC section.").arg(section);
        return false;
    }

    return true;
}

QMap<QString, QString> WinterFieldDayContest::parseReceivedExchange(const QString& exchange) const {
    QMap<QString, QString> result;
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() == 2) {
        result["Class"] = parts[0].toUpper();
        result["Section"] = parts[1].toUpper();
    }

    return result;
}

int WinterFieldDayContest::calculateQSOPoints(const QSO& qso, const StationInfo& myStation) const {
    // WFD scoring rules from WA7BNM:
    // - 0 points: Same country (but counts as multiplier)
    // - 1 point: Different country, same continent (except NA)
    // - 2 points: Different country, same continent (NA only)
    // - 3 points: Different continent

    QString theirCountry = qso.dxccEntity;
    QString theirContinent = qso.continent;
    QString myCountry = myStation.country;
    QString myContinent = myStation.continent;

    // Handle case where geographic data is missing
    if (theirCountry.isEmpty() || theirContinent.isEmpty() ||
        myCountry.isEmpty() || myContinent.isEmpty()) {
        // Default to 1 point if we don't have complete geographic data
        return 1;
    }

    // Same country: 0 points (but counts as multiplier)
    if (theirCountry == myCountry) {
        return 0;
    }

    // Different continent: 3 points
    if (theirContinent != myContinent) {
        return 3;
    }

    // Same continent, different country
    // North America (NA) gets 2 points, others get 1 point
    if (myContinent == "NA") {
        return 2;
    } else {
        return 1;
    }
}

int WinterFieldDayContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // Winter Field Day: Score = QSO Points + Bonus Points
    // Multipliers are NOT multiplied - they're just tracked
    // Bonus points are added separately via Cabrillo submission checkboxes
    Q_UNUSED(multiplierCounts);

    // Just return QSO points - bonus points added later
    return totalQSOPoints;
}

QList<MultiplierDefinition> WinterFieldDayContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    // WFD multiplier rules from WA7BNM:
    // - Each CQ zone once per band
    // - Each country once per band

    MultiplierDefinition cqZoneMult;
    cqZoneMult.type = MultiplierType::CQZone;
    cqZoneMult.scope = MultiplierScope::PerBand;
    cqZoneMult.displayName = "CQ Zones";
    mults.append(cqZoneMult);

    MultiplierDefinition countryMult;
    countryMult.type = MultiplierType::Country;
    countryMult.scope = MultiplierScope::PerBand;
    countryMult.displayName = "Countries";
    mults.append(countryMult);

    return mults;
}

QString WinterFieldDayContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString multValue;

    switch (multType) {
        case MultiplierType::CQZone:
            // CQ Zone from QSO
            if (qso.cqZone > 0) {
                multValue = QString::number(qso.cqZone);
            }
            break;

        case MultiplierType::Country:
            // Country (DXCC entity)
            multValue = qso.dxccPrefix;  // Use DXCC prefix as country identifier
            break;

        default:
            return QString();  // Not a multiplier for this contest
    }

    // Check if this multiplier value has already been worked (on this band)
    if (multValue.isEmpty() || alreadyWorkedValues.contains(multValue)) {
        return QString();  // Not a new multiplier
    }

    return multValue;
}

QMap<QString, QString> WinterFieldDayContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();
    headers["CONTEST"] = "WINTER-FIELD-DAY";
    return headers;
}

bool WinterFieldDayContest::isValidClass(const QString& classStr) {
    // Valid classes: 1O, 2O, 3O, 4O, 5O, 1I, 2I, 3I, 4I, 5I, Home
    QString upper = classStr.toUpper();

    if (upper == "HOME") {
        return true;
    }

    // Pattern: digit + letter (O for Outdoor, I for Indoor)
    QRegularExpression re("^[1-9][OI]$");
    return re.match(upper).hasMatch();
}

bool WinterFieldDayContest::isValidSection(const QString& section) {
    return getValidSections().contains(section.toUpper());
}

QStringList WinterFieldDayContest::getValidSections() {
    // ARRL Sections
    static QStringList sections = {
        // US Sections
        "CT", "EMA", "ME", "NH", "RI", "VT", "WMA",  // New England
        "ENY", "NLI", "NNJ", "NNY", "SNJ", "WNY",    // Atlantic
        "DE", "EPA", "MDC", "WPA",                    // Roanoke
        "AL", "GA", "KY", "NC", "NFL", "SC", "SFL", "WCF", "TN", "VA", "PR", "VI",  // Delta
        "AR", "LA", "MS", "NM", "NTX", "OK", "STX", "WTX",  // Delta
        "EB", "LAX", "ORG", "SB", "SCV", "SDG", "SF", "SJV", "SV", "PAC",  // Pacific
        "AK", "EWA", "ID", "MT", "NV", "OR", "UT", "WWA", "WY",  // Pacific Northwestern
        "MI", "OH", "WV",                             // Great Lakes
        "IL", "IN", "WI",                             // Central
        "CO", "IA", "KS", "MN", "MO", "ND", "NE", "SD",  // Midwest
        // Canadian Sections (RAC)
        "AB", "BC", "MB", "NB", "NL", "NS", "NT", "ON", "PE", "QC", "SK", "YT",
        // Other
        "DX"  // Outside US/Canada
    };

    return sections;
}

} // namespace TR4QT

// Auto-register with factory
REGISTER_CONTEST(TR4QT::WinterFieldDayContest, "WFD");
