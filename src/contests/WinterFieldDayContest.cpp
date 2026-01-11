#include "WinterFieldDayContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "../exchanges/SmartExchangeParser.h"
#include "../utils/ArrlSectionHelper.h"
#include "../utils/AppSettings.h"
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

    meta.schedule = "4th full weekend of January";
    meta.floatingDates = {
        FloatingDate(1, "4th full weekend")  // January
    };
    meta.website = "https://www.winterfieldday.com/";
    meta.description = "Portable operations in winter conditions. Exchange: Class + ARRL/RAC Section.";

    return meta;
}

ContestBase* WinterFieldDayContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Winter Field Day is mixed mode
    return new WinterFieldDayContest(myStation);
}

WinterFieldDayContest::WinterFieldDayContest(const StationInfo& myStation)
    : ContestBase(myStation)
{
}

QString WinterFieldDayContest::getContestId() const {
    return "WFD";
}

QString WinterFieldDayContest::getContestName() const {
    return "Winter Field Day";
}

QString WinterFieldDayContest::getADIFContestId() const {
    return ADIF_CONTEST_ID;
}

QList<ExchangeField> WinterFieldDayContest::getReceivedExchangeFields() const {
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
    sectionField.hint = "WCF";
    sectionField.autoFill = true;  // From station settings
    sectionField.maxLength = 4;
    fields.append(sectionField);

    return fields;
}

QList<TableColumn> WinterFieldDayContest::getTableColumns() const {
    return {
        TableColumn("Class", "CL", 50, TableColumn::Alignment::Center),
        TableColumn("Section", "QTH", 60, TableColumn::Alignment::Left)
    };
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
        errorMsg = "Exchange must be: Class + Section (e.g., '1O WCF' or 'WCF 1O')";
        return false;
    }

    // Use smart parser to detect which field is which (order-agnostic)
    QSO tempQSO;
    parseReceivedExchange(exchange, tempQSO);

    // Check if we got both required fields
    if (tempQSO.contestClass.isEmpty()) {
        errorMsg = "Missing Class field. Expected format like: 1O, 2I, 3H, etc.";
        return false;
    }

    if (tempQSO.arrlSection.isEmpty()) {
        errorMsg = "Missing Section field. Expected valid ARRL/RAC section.";
        return false;
    }

    // Validate class
    if (!isValidClass(tempQSO.contestClass)) {
        errorMsg = QString("Invalid class '%1'. Must be [1-99][I/O/H/M]. Examples: 1O, 2I, 3H, 22M").arg(tempQSO.contestClass);
        return false;
    }

    // Validate section
    if (!isValidSection(tempQSO.arrlSection)) {
        errorMsg = QString("Invalid section '%1'. Must be valid ARRL or RAC section.").arg(tempQSO.arrlSection);
        return false;
    }

    return true;
}

void WinterFieldDayContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    // Use smart parser to allow fields in any order
    // Examples that now work:
    // - "1O WMA" (traditional: class first, section second)
    // - "WMA 1O" (reversed: section first, class second)
    // - "3H CT" or "CT 3H" (both work)
    // - "HOME WCF" or "WCF HOME" (both work)

    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<WinterFieldDayContest*>(this)  // For section validation
    );

    // Populate dedicated QSO fields directly
    if (parsed.contains("Class")) {
        qso.contestClass = parsed["Class"];
    }
    if (parsed.contains("Section")) {
        qso.arrlSection = parsed["Section"];
    }

    // Format exchangeReceived (Winter Field Day does not include RST)
    formatExchangeReceived(exchange, qso);
}

bool WinterFieldDayContest::isValidMode(ModeType mode, QString& errorMsg) const {
    // Winter Field Day allowed modes: CW, Phone, Digital
    // NOT allowed: FT8, FT4 per official rules

    switch (mode) {
        case ModeType::FT8:
        case ModeType::FT4:
            errorMsg = "FT8 and FT4 are not allowed in Winter Field Day";
            return false;

        default:
            return true;  // All other modes are allowed
    }
}

int WinterFieldDayContest::calculateQSOPoints(const QSO& qso, const StationInfo& myStation) const {
    Q_UNUSED(myStation);  // Not used in WFD scoring

    // Winter Field Day scoring rules (mode-based):
    // - Phone/SSB: 1 point per contact
    // - CW: 2 points per contact
    // - Digital: 2 points per contact
    //
    // Note: FT4 and FT8 are NOT allowed in WFD (validated in isValidMode)

    // CW gets 2 points (includes CW and CWR)
    if (qso.mode == ModeType::CW || qso.mode == ModeType::CWR) {
        return 2;
    }

    // Digital modes get 2 points
    // Includes RTTY, PSK, and generic DATA modes
    // Note: FT8 and FT4 are NOT allowed (validated in isValidMode)
    if (qso.mode == ModeType::RTTY ||
        qso.mode == ModeType::RTTYR ||
        qso.mode == ModeType::PSK ||
        qso.mode == ModeType::PSKR ||
        qso.mode == ModeType::DATA ||
        qso.mode == ModeType::DATAR) {
        return 2;
    }

    // Phone modes get 1 point (USB, LSB, AM, FM)
    // This includes SSB, AM, FM, DMR, C4FM, etc.
    return 1;
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

    // WFD multiplier rules:
    // - ARRL/RAC Sections (all-band)
    // Per official rules: Each section counts once across all bands

    MultiplierDefinition sectionMult;
    sectionMult.type = MultiplierType::Section;  // ARRL/RAC sections for WFD
    sectionMult.scope = MultiplierScope::AllBands;
    sectionMult.displayName = "Sections";
    mults.append(sectionMult);

    return mults;
}

QString WinterFieldDayContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString multValue;

    if (multType == MultiplierType::Section) {
        // ARRL/RAC sections - read from dedicated field
        QString section = qso.arrlSection.toUpper();
        if (!section.isEmpty() && Arrl::isValidSection(section)) {
            multValue = section;
        }
    } else {
        return QString();  // Not a multiplier for this contest
    }

    // Check if this multiplier value has already been worked (all-band scope)
    if (multValue.isEmpty() || alreadyWorkedValues.contains(multValue)) {
        return QString();  // Not a new multiplier
    }

    return multValue;
}

QList<BandType> WinterFieldDayContest::getAllowedBands() const {
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

QMap<QString, QString> WinterFieldDayContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();
    headers["CONTEST"] = "WINTER-FIELD-DAY";
    return headers;
}

bool WinterFieldDayContest::isValidClass(const QString& classStr) {
    // Valid classes: 1O, 2O, ..., 1I, 2I, ..., 1H, 2H, ..., 1M, 2M, ...
    // WFD categories: I (Indoor), O (Outdoor), H (Home), M (Mobile)
    // Per 2025 WFD rules: No limit on transmitter count (using 1-99 for validation)
    QString upper = classStr.toUpper();

    // Pattern: digit(s) + letter (1-99 transmitters, I/O/H/M category)
    QRegularExpression re("^([1-9]|[1-9][0-9])[IOHM]$");
    return re.match(upper).hasMatch();
}

bool WinterFieldDayContest::isValidSection(const QString& section) {
    return Arrl::isValidSection(section);
}

QStringList WinterFieldDayContest::getValidSections() {
    return Arrl::getAllSections();
}

} // namespace TR4QT

// Auto-register with factory
REGISTER_CONTEST(TR4QT::WinterFieldDayContest, "WFD");
