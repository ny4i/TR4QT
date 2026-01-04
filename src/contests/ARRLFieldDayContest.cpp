#include "ARRLFieldDayContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "../exchanges/SmartExchangeParser.h"
#include "../utils/ArrlSectionHelper.h"
#include "../utils/AppSettings.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata ARRLFieldDayContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_FD";
    meta.displayName = "ARRL Field Day";
    meta.shortName = "FD";
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

    meta.schedule = "Fourth full weekend of June";
    meta.website = "https://www.arrl.org/field-day";
    meta.description = "Emergency preparedness event. Exchange: Class + ARRL/RAC Section.";

    return meta;
}

ContestBase* ARRLFieldDayContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // ARRL Field Day is mixed mode
    return new ARRLFieldDayContest(myStation);
}

ARRLFieldDayContest::ARRLFieldDayContest(const StationInfo& myStation)
    : ContestBase(myStation)
{
}

QString ARRLFieldDayContest::getContestId() const {
    return "ARRL_FD";
}

QString ARRLFieldDayContest::getContestName() const {
    return "ARRL Field Day";
}

QString ARRLFieldDayContest::getADIFContestId() const {
    return ADIF_CONTEST_ID;
}

QList<ExchangeField> ARRLFieldDayContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // Class (e.g., "1O", "2O", "3O", "2I", "Home")
    ExchangeField classField;
    classField.name = "Class";
    classField.hint = "Class";
    classField.autoFill = false;
    classField.maxLength = 6;
    fields.append(classField);

    // Section (ARRL/RAC section)
    ExchangeField sectionField;
    sectionField.name = "Section";
    sectionField.hint = "Section";
    sectionField.autoFill = false;
    sectionField.maxLength = 4;
    fields.append(sectionField);

    return fields;
}

QList<ExchangeField> ARRLFieldDayContest::getSentExchangeFields() const {
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
    sectionField.hint = "WCF";
    sectionField.autoFill = true;  // From station settings
    sectionField.maxLength = 4;
    fields.append(sectionField);

    return fields;
}

QList<TableColumn> ARRLFieldDayContest::getTableColumns() const {
    return {
        TableColumn("Class", "CL", 50, TableColumn::Alignment::Center),
        TableColumn("Section", "QTH", 60, TableColumn::Alignment::Left)
    };
}

QString ARRLFieldDayContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    Q_UNUSED(rst);

    // Exchange should come from station settings
    // Format: "Class Section" (e.g., "1O WMA")
    // This will be populated from StationInfo in the logging system
    return "1O WMA";  // Placeholder - actual value from settings
}

bool ARRLFieldDayContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() != 2) {
        errorMsg = "Exchange must be: Class + Section (e.g., '1O WCF' or 'WCF 1O')";
        return false;
    }

    // Use smart parser to detect which field is which (order-agnostic)
    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<ARRLFieldDayContest*>(this)
    );

    // Check if we got both required fields
    if (!parsed.contains("Class")) {
        errorMsg = "Missing Class field. Expected format like: 1O, 2I, 3H, etc.";
        return false;
    }

    if (!parsed.contains("Section")) {
        errorMsg = "Missing Section field. Expected valid ARRL/RAC section.";
        return false;
    }

    // Validate class
    QString classStr = parsed["Class"];
    if (!isValidClass(classStr)) {
        errorMsg = QString("Invalid class '%1'. Must be [1-99][I/O/H/M]. Examples: 1O, 2I, 3H, 22M").arg(classStr);
        return false;
    }

    // Validate section
    QString section = parsed["Section"];
    if (!isValidSection(section)) {
        errorMsg = QString("Invalid section '%1'. Must be valid ARRL or RAC section.").arg(section);
        return false;
    }

    return true;
}

void ARRLFieldDayContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    // Use smart parser to allow fields in any order
    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<ARRLFieldDayContest*>(this)
    );

    // Populate dedicated QSO fields
    if (parsed.contains("Class")) {
        qso.contestClass = parsed["Class"];
    }
    if (parsed.contains("Section")) {
        qso.arrlSection = parsed["Section"];
    }

    // Format exchangeReceived (Field Day does not include RST)
    formatExchangeReceived(exchange, qso);
}

int ARRLFieldDayContest::calculateQSOPoints(const QSO& qso, const StationInfo& myStation) const {
    // ARRL Field Day scoring rules:
    // - CW and digital modes: 2 points
    // - SSB and FM modes: 1 point

    // Check if mode is CW
    if (qso.mode == ModeType::CW || qso.mode == ModeType::CWR) {
        return 2;
    }

    // Check if mode is digital
    if (qso.mode == ModeType::RTTY || qso.mode == ModeType::RTTYR ||
        qso.mode == ModeType::PSK || qso.mode == ModeType::PSKR ||
        qso.mode == ModeType::FT8 || qso.mode == ModeType::FT4 ||
        qso.mode == ModeType::DATA || qso.mode == ModeType::DATAR) {
        return 2;
    }

    // Phone modes (SSB, FM, AM): 1 point
    return 1;
}

int ARRLFieldDayContest::calculateTotalScore(
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

QList<MultiplierDefinition> ARRLFieldDayContest::getMultiplierTypes() const {
    // ARRL Field Day does not use band-based multipliers
    // Scoring is based on QSO points + bonus points only
    // (This was incorrectly copied from WinterFieldDayContest)
    return QList<MultiplierDefinition>();
}

QString ARRLFieldDayContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    Q_UNUSED(qso);
    Q_UNUSED(multType);
    Q_UNUSED(alreadyWorkedValues);

    // ARRL Field Day does not use multipliers
    return QString();
}

QList<BandType> ARRLFieldDayContest::getAllowedBands() const {
    // Standard HF contest bands
    QList<BandType> bands = { BandType::Band160M, BandType::Band80M, BandType::Band40M,
                              BandType::Band20M, BandType::Band15M, BandType::Band10M };

    // Add VHF/UHF bands if enabled in preferences
    if (AppSettings::instance().getVHFBandsEnabled()) {
        bands.append(BandType::Band6M);
        bands.append(BandType::Band2M);
        bands.append(BandType::Band70CM);
    }

    return bands;
}

QMap<QString, QString> ARRLFieldDayContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();
    headers["CONTEST"] = "ARRL-FIELD-DAY";
    return headers;
}

bool ARRLFieldDayContest::isValidClass(const QString& classStr) {
    // Valid classes: 1A, 2A, ..., 1B, 2B, ..., up to 32F
    // ARRL FD categories: A (portable 3+), B (portable 1-2), C (mobile),
    //                     D (home battery), E (home commercial), F (EOC)
    // Per ARRL FD rules: 1-32 transmitters allowed
    QString upper = classStr.toUpper();

    // Pattern: digit(s) + letter (1-32 transmitters, A-F category)
    // Matches: 1A through 9A, 10A through 32A (and same for B-F)
    QRegularExpression re("^([1-9]|[12][0-9]|3[012])[ABCDEF]$");
    return re.match(upper).hasMatch();
}

bool ARRLFieldDayContest::isValidSection(const QString& section) {
    return Arrl::isValidSection(section);
}

QStringList ARRLFieldDayContest::getValidSections() {
    return Arrl::getAllSections();
}

} // namespace TR4QT

// Auto-register with factory
REGISTER_CONTEST(TR4QT::ARRLFieldDayContest, "ARRL_FD");
