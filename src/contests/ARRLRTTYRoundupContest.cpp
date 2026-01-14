#include "ARRLRTTYRoundupContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
#include "../utils/CountryFile.h"
#include "../exchanges/SmartExchangeParser.h"
#include "RSTValidator.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata ARRLRTTYRoundupContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_RTTY";
    meta.displayName = "ARRL RTTY Roundup";
    meta.shortName = "RTTY RU";
    meta.supportedModes = {ModeType::RTTY};
    meta.hasSeparateContests = false;  // Single contest, RTTY only

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = WA7BNM_ID;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = "";
    meta.cabrilloNameMixed = CABRILLO_NAME;

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = ADIF_CONTEST_ID;

    meta.schedule = "First full weekend of January";
    meta.floatingDates = {
        FloatingDate(1, "1st full weekend")  // January
    };
    meta.website = "https://contests.arrl.org/rttyru/";
    meta.description = "Work as many states, provinces, and DX countries as possible. W/VE send RST+State, DX sends RST+Serial.";

    return meta;
}

ContestBase* ARRLRTTYRoundupContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);
    return new ARRLRTTYRoundupContest(myStation);
}

ARRLRTTYRoundupContest::ARRLRTTYRoundupContest(const StationInfo& myStation)
    : ContestBase(myStation)
{
}

QString ARRLRTTYRoundupContest::getContestId() const {
    return "ARRL_RTTY";
}

QString ARRLRTTYRoundupContest::getContestName() const {
    return "ARRL RTTY Roundup";
}

QString ARRLRTTYRoundupContest::getADIFContestId() const {
    return ADIF_CONTEST_ID;
}

QList<ExchangeField> ARRLRTTYRoundupContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (auto-filled)
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = "RSTValidator::getDefault(getContestMode())";
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // State/Province OR Serial Number (smart detection)
    ExchangeField exchange;
    exchange.name = "State/Serial";
    exchange.hint = "FL or 001";
    exchange.autoFill = false;
    exchange.maxLength = 4;
    fields.append(exchange);

    return fields;
}

QList<ExchangeField> ARRLRTTYRoundupContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // State/Province (W/VE stations)
    ExchangeField state;
    state.name = "State";
    state.hint = "Your State/Province";
    state.autoFill = true;  // From station settings
    state.maxLength = 4;
    fields.append(state);

    return fields;
}

QList<TableColumn> ARRLRTTYRoundupContest::getTableColumns() const {
    return {
        TableColumn("State", "QTH", 60, TableColumn::Alignment::Left)
    };
}

QString ARRLRTTYRoundupContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    return rst + " {STATE}";  // {STATE} replaced by actual state from settings
}

bool ARRLRTTYRoundupContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (State/Province or Serial)";
        return false;
    }

    // Use SmartExchangeParser for order-agnostic parsing
    QSO tempQSO;
    parseReceivedExchange(exchange, tempQSO);

    // Validate we got either state or serial
    if (tempQSO.state.isEmpty() && tempQSO.serialNumberReceived == 0) {
        errorMsg = "Exchange must include State/Province (e.g., 'FL') or Serial (e.g., '001')";
        return false;
    }

    // If we got a state, validate it using looksLikeState (NOT isValidSection)
    if (!tempQSO.state.isEmpty()) {
        if (!SmartExchangeParser::looksLikeState(tempQSO.state)) {
            errorMsg = QString("Invalid state/province: %1").arg(tempQSO.state);
            return false;
        }
    }

    // Serial number validation (1-9999)
    if (tempQSO.serialNumberReceived > 0 && tempQSO.serialNumberReceived > 9999) {
        errorMsg = QString("Invalid serial number: %1 (must be 1-9999)").arg(tempQSO.serialNumberReceived);
        return false;
    }

    return true;
}

void ARRLRTTYRoundupContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    // Use SmartExchangeParser for order-agnostic field detection
    // Allows "599 FL", "FL 599", "599 001", "001 599" etc.
    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<ARRLRTTYRoundupContest*>(this)
    );

    // Get RST (auto-fill if not provided)
    qso.rstReceived = parsed.value("RST", RSTValidator::getDefault(getContestMode()));

    // Get State/Serial field
    QString stateOrSerial = parsed.value("State/Serial");

    // Detect if it's a state/province or serial number using SmartExchangeParser helpers
    if (SmartExchangeParser::looksLikeState(stateOrSerial)) {
        qso.state = stateOrSerial.toUpper();
        qso.serialNumberReceived = 0;
    } else if (!stateOrSerial.isEmpty()) {
        qso.state = "";
        qso.serialNumberReceived = stateOrSerial.toInt();
    }

    // Format exchangeReceived (e.g., "599 FL" or "599 001")
    formatExchangeReceived(exchange, qso);
}

int ARRLRTTYRoundupContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    Q_UNUSED(qso);
    Q_UNUSED(myStation);
    return 1;  // 1 point per QSO
}

int ARRLRTTYRoundupContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // Total multipliers = DXCC + States + Provinces
    int totalMults = 0;
    for (int count : multiplierCounts.values()) {
        totalMults += count;
    }

    return totalQSOPoints * totalMults;
}

QList<MultiplierDefinition> ARRLRTTYRoundupContest::getMultiplierTypes() const {
    return {
        {MultiplierType::Country, MultiplierScope::AllBands, "Countries"},
        {MultiplierType::State, MultiplierScope::AllBands, "States/Provinces"}
    };
}

QString ARRLRTTYRoundupContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString value;

    if (multType == MultiplierType::State) {
        // US states and Canadian provinces
        QString state = qso.state.toUpper();
        if (!state.isEmpty() && Arrl::isValidSection(state) &&
            !alreadyWorkedValues.contains(state)) {
            value = state;
        }
    } else if (multType == MultiplierType::Country) {
        // DX countries (not US or Canada)
        // Use dxccPrefix which should be the primary DXCC prefix (e.g., "I", "HB", "PY")
        // Entity code ensures proper country identification (exclude US=291, Canada=1)
        if (qso.dxccEntityCode > 0 &&
            qso.dxccEntityCode != 291 && qso.dxccEntityCode != 1 &&
            !qso.dxccPrefix.isEmpty() &&
            !alreadyWorkedValues.contains(qso.dxccPrefix)) {
            value = qso.dxccPrefix;
        }
    }

    return value;
}

QList<BandType> ARRLRTTYRoundupContest::getAllowedBands() const {
    // RTTY contests exclude 160m
    return { BandType::Band80M, BandType::Band40M, BandType::Band20M,
             BandType::Band15M, BandType::Band10M };
}

QMap<QString, QString> ARRLRTTYRoundupContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers;
    headers["CONTEST"] = CABRILLO_NAME;
    return headers;
}

bool ARRLRTTYRoundupContest::isValidState(const QString& state) {
    // Use ARRL section validation (includes both US and Canadian sections)
    return Arrl::isValidSection(state.toUpper());
}

bool ARRLRTTYRoundupContest::isValidProvince(const QString& province) {
    // Use ARRL section validation (includes both US and Canadian sections)
    return Arrl::isValidSection(province.toUpper());
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::ARRLRTTYRoundupContest, "ARRL_RTTY");
