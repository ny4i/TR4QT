#include "ARRLRTTYRoundupContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
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
    meta.website = "https://contests.arrl.org/rttyru/";
    meta.description = "Work as many states, provinces, and DX countries as possible. W/VE send RST+State, DX sends RST+Serial.";

    return meta;
}

ContestBase* ARRLRTTYRoundupContest::create(ModeType mode) {
    Q_UNUSED(mode);
    return new ARRLRTTYRoundupContest();
}

ARRLRTTYRoundupContest::ARRLRTTYRoundupContest()
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
    rst.hint = "599";
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // State/Province OR Serial Number (smart detection)
    ExchangeField exchange;
    exchange.name = "State/Serial";
    exchange.hint = "MA or 001";
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

    // Parse exchange - could be just state/serial, or RST + state/serial
    QString stateOrSerial;
    if (parts.size() == 1) {
        stateOrSerial = parts[0];
    } else if (parts.size() == 2) {
        // RST + State/Serial
        stateOrSerial = parts[1];
    } else {
        errorMsg = "Exchange must be: State (e.g., 'MA') or RST + State (e.g., '599 MA')";
        return false;
    }

    // Validate as either state/province OR serial number
    // Serial number: 1-999 (3 digits)
    bool isSerial = false;
    if (QRegularExpression("^\\d{1,3}$").match(stateOrSerial).hasMatch()) {
        int serial = stateOrSerial.toInt();
        if (serial >= 1 && serial <= 999) {
            isSerial = true;
        }
    }

    // If not serial, must be valid state/province
    if (!isSerial && !Arrl::isValidSection(stateOrSerial.toUpper())) {
        errorMsg = QString("Invalid state/province or serial number: %1").arg(stateOrSerial);
        return false;
    }

    return true;
}

QMap<QString, QString> ARRLRTTYRoundupContest::parseReceivedExchange(const QString& exchange) const {
    QMap<QString, QString> result;
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    QString rst = "599";
    QString stateOrSerial;

    if (parts.size() == 1) {
        stateOrSerial = parts[0];
    } else if (parts.size() >= 2) {
        rst = parts[0];
        stateOrSerial = parts[1];
    }

    result["RST"] = rst;

    // Detect if it's a serial number or state/province
    if (QRegularExpression("^\\d{1,3}$").match(stateOrSerial).hasMatch()) {
        result["Serial"] = stateOrSerial;
        result["State"] = "";
    } else {
        result["State"] = stateOrSerial.toUpper();
        result["Serial"] = "";
    }

    return result;
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
        QString dxcc = qso.dxccPrefix;
        if (!dxcc.isEmpty() && dxcc != "K" && dxcc != "VE" &&
            !alreadyWorkedValues.contains(dxcc)) {
            value = dxcc;
        }
    }

    return value;
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
