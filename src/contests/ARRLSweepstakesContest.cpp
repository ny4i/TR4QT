#include "ARRLSweepstakesContest.h"
#include "ContestRegistry.h"
#include "../models/QSO.h"
#include "../exchanges/SmartExchangeParser.h"
#include "../utils/ArrlSectionHelper.h"
#include <QRegularExpression>
#include <QStringList>

namespace TR4QT {

ARRLSweepstakesContest::ARRLSweepstakesContest(ModeType mode, const StationInfo& myStation)
    : ContestBase(myStation)
    , m_mode(mode)
{
}

// ===== Factory Methods =====

ContestMetadata ARRLSweepstakesContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_SS";
    meta.displayName = "ARRL Sweepstakes";
    meta.shortName = "SS";
    meta.supportedModes = {ModeType::CW, ModeType::USB};
    meta.hasSeparateContests = true;

    meta.wa7bnmIdCW = WA7BNM_ID_CW;
    meta.wa7bnmIdSSB = WA7BNM_ID_SSB;
    meta.wa7bnmIdMixed = 0;

    meta.cabrilloNameCW = CABRILLO_NAME_CW;
    meta.cabrilloNameSSB = CABRILLO_NAME_SSB;
    meta.cabrilloNameMixed = "";

    meta.adifContestIdCW = ADIF_CONTEST_ID_CW;
    meta.adifContestIdSSB = ADIF_CONTEST_ID_SSB;
    meta.adifContestIdMixed = "";

    meta.schedule = "First full weekend of November";
    meta.website = "http://www.arrl.org/sweepstakes";

    return meta;
}

ContestBase* ARRLSweepstakesContest::create(ModeType mode, const StationInfo& myStation) {
    return new ARRLSweepstakesContest(mode, myStation);
}

// ===== Contest Identity =====

QString ARRLSweepstakesContest::getContestId() const {
    return (m_mode == ModeType::CW) ? "ARRL_SS_CW" : "ARRL_SS_SSB";
}

QString ARRLSweepstakesContest::getContestName() const {
    return (m_mode == ModeType::CW) ? "ARRL Sweepstakes - CW" : "ARRL Sweepstakes - SSB";
}

QString ARRLSweepstakesContest::getADIFContestId() const {
    return (m_mode == ModeType::CW) ? ADIF_CONTEST_ID_CW : ADIF_CONTEST_ID_SSB;
}

// ===== Exchange Configuration =====

QList<ExchangeField> ARRLSweepstakesContest::getReceivedExchangeFields() const {
    return {
        {"Serial", "Serial number", false, false},
        {"Precedence", "Q/A/B/M/U/S", false, false},
        {"Check", "Last 2 digits of year", false, false},
        {"Section", "ARRL Section", false, false}
    };
}

QList<ExchangeField> ARRLSweepstakesContest::getSentExchangeFields() const {
    return {
        {"Serial", "Serial number", true, true},  // Auto-filled
        {"Precedence", "Your precedence", false, false},
        {"Check", "Your check", false, false},
        {"Section", "Your section", false, false}
    };
}

QList<TableColumn> ARRLSweepstakesContest::getTableColumns() const {
    return {
        TableColumn("Serial", "#", 50, TableColumn::Alignment::Right),
        TableColumn("Precedence", "Prec", 50, TableColumn::Alignment::Center),
        TableColumn("Check", "Chk", 50, TableColumn::Alignment::Center),
        TableColumn("Section", "QTH", 60, TableColumn::Alignment::Left)
    };
}

QString ARRLSweepstakesContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(rst);  // SS doesn't use RST in exchange
    // Example: "123 A 95 WMA"
    // User must configure precedence, check, and section in contest setup
    return QString::number(serialNumber) + " [PREC] [CHK] [SEC]";
}

bool ARRLSweepstakesContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() != 4) {
        errorMsg = "Exchange must be: Serial# Precedence Check Section";
        return false;
    }

    // Validate serial number (1-9999)
    bool ok;
    int serial = parts[0].toInt(&ok);
    if (!ok || serial < 1 || serial > 9999) {
        errorMsg = "Invalid serial number (must be 1-9999)";
        return false;
    }

    // Validate precedence
    if (!isValidPrecedence(parts[1])) {
        errorMsg = "Invalid precedence (must be Q, A, B, M, U, or S)";
        return false;
    }

    // Validate check (00-99)
    if (!isValidCheck(parts[2])) {
        errorMsg = "Invalid check (must be 00-99)";
        return false;
    }

    // Validate section
    if (!isValidSection(parts[3])) {
        errorMsg = "Invalid ARRL section: " + parts[3];
        return false;
    }

    return true;
}

void ARRLSweepstakesContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    // Use smart parser to allow fields in any order
    // Examples that now work:
    // - "123 A 95 WMA" (traditional order)
    // - "A 95 WMA 123" (precedence first)
    // - "WMA 123 A 95" (section first)
    // - "1 M" (partial - just serial and precedence)

    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<ARRLSweepstakesContest*>(this)  // For section validation
    );

    // Populate dedicated QSO fields directly
    if (parsed.contains("Serial")) {
        qso.serialNumberReceived = parsed["Serial"].toInt();
    }
    if (parsed.contains("Precedence")) {
        qso.precedence = parsed["Precedence"];
    }
    if (parsed.contains("Check")) {
        qso.check = parsed["Check"];
    }
    if (parsed.contains("Section")) {
        qso.arrlSection = parsed["Section"];
    }

    // Format exchangeReceived (Sweepstakes does not include RST)
    formatExchangeReceived(exchange, qso);
}

// ===== Scoring =====

int ARRLSweepstakesContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    Q_UNUSED(qso);
    Q_UNUSED(myStation);

    // All QSOs are worth 2 points
    return 2;
}

int ARRLSweepstakesContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // Score = QSO Points × Sections Worked
    int sections = multiplierCounts.value(MultiplierType::Section, 0);
    return totalQSOPoints * sections;
}

// ===== Multipliers =====

QList<MultiplierDefinition> ARRLSweepstakesContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    MultiplierDefinition sectionMult;
    sectionMult.type = MultiplierType::Section;
    sectionMult.scope = MultiplierScope::AllBands;  // Sections count once across all bands
    sectionMult.displayName = "ARRL Sections";

    mults.append(sectionMult);
    return mults;
}

QString ARRLSweepstakesContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    Q_UNUSED(alreadyWorkedValues);

    if (multType == MultiplierType::Section) {
        // Extract section from dedicated field
        QString section = qso.arrlSection.toUpper();
        if (!section.isEmpty() && isValidSection(section)) {
            return section;
        }
    }

    return QString();
}

// ===== Exchange Validation Helpers =====

bool ARRLSweepstakesContest::isValidPrecedence(const QString& precedence) const {
    QString upper = precedence.toUpper();
    return (upper == "Q" || upper == "A" || upper == "B" ||
            upper == "M" || upper == "U" || upper == "S");
}

bool ARRLSweepstakesContest::isValidCheck(const QString& check) const {
    bool ok;
    int checkNum = check.toInt(&ok);
    return ok && checkNum >= 0 && checkNum <= 99 && check.length() == 2;
}

bool ARRLSweepstakesContest::isValidSection(const QString& section) const {
    return Arrl::isValidSection(section);
}

} // namespace TR4QT

// Auto-register contest with registry
REGISTER_CONTEST(TR4QT::ARRLSweepstakesContest, "ARRL_SS");
