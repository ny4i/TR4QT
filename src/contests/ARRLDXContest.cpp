#include "ARRLDXContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
#include "RSTValidator.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata ARRLDXContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_DX";
    meta.displayName = "ARRL International DX Contest";
    meta.shortName = "ARRL DX";
    meta.supportedModes = {ModeType::CW, ModeType::USB};
    meta.hasSeparateContests = true;  // Separate CW and SSB contests

    meta.wa7bnmIdCW = WA7BNM_ID_CW;
    meta.wa7bnmIdSSB = WA7BNM_ID_SSB;
    meta.wa7bnmIdMixed = 0;

    meta.cabrilloNameCW = CABRILLO_NAME_CW;
    meta.cabrilloNameSSB = CABRILLO_NAME_SSB;
    meta.cabrilloNameMixed = "";

    meta.adifContestIdCW = ADIF_CONTEST_ID_CW;
    meta.adifContestIdSSB = ADIF_CONTEST_ID_SSB;
    meta.adifContestIdMixed = "";

    meta.schedule = "CW: 3rd full weekend of February, Phone: 1st full weekend of March";
    meta.website = "https://contests.arrl.org/";
    meta.description = "W/VE stations work DX only. W/VE send RST+State, DX sends RST+Power.";

    return meta;
}

ContestBase* ARRLDXContest::create(ModeType mode) {
    return new ARRLDXContest(mode);
}

ARRLDXContest::ARRLDXContest(ModeType mode)
    : m_mode(mode)
{
}

QString ARRLDXContest::getContestId() const {
    return m_mode == ModeType::CW ? "ARRL_DX_CW" : "ARRL_DX_SSB";
}

QString ARRLDXContest::getContestName() const {
    return m_mode == ModeType::CW ?
           "ARRL International DX Contest - CW" :
           "ARRL International DX Contest - SSB";
}

QString ARRLDXContest::getADIFContestId() const {
    return m_mode == ModeType::CW ? ADIF_CONTEST_ID_CW : ADIF_CONTEST_ID_SSB;
}

QList<ExchangeField> ARRLDXContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = RSTValidator::getDefault(m_mode);
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // State/Province OR Power (smart detection)
    ExchangeField exchange;
    exchange.name = "State/Power";
    exchange.hint = "MA or 100";
    exchange.autoFill = false;
    exchange.maxLength = 4;
    fields.append(exchange);

    return fields;
}

QList<ExchangeField> ARRLDXContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // W/VE sends State/Province
    ExchangeField state;
    state.name = "State";
    state.hint = "Your State/Province";
    state.autoFill = true;
    state.maxLength = 4;
    fields.append(state);

    return fields;
}

QList<TableColumn> ARRLDXContest::getTableColumns() const {
    return {
        TableColumn("State", "QTH/Pwr", 80, TableColumn::Alignment::Left)
    };
}

QString ARRLDXContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    return rst + " {STATE}";
}

bool ARRLDXContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (State/Province or Power)";
        return false;
    }

    QString stateOrPower;
    if (parts.size() == 1) {
        stateOrPower = parts[0];
    } else if (parts.size() == 2) {
        // RST + State/Power
        QString rst = parts[0];
        if (!RSTValidator::isValid(rst, m_mode)) {
            QString expectedFormat = (m_mode == ModeType::CW) ?
                "3 digits (e.g., 599)" : "2-3 digits (e.g., 59)";
            errorMsg = QString("Invalid RST format. Expected %1").arg(expectedFormat);
            return false;
        }
        stateOrPower = parts[1];
    } else {
        errorMsg = "Exchange must be: State/Power (e.g., 'MA' or '100') or RST + State/Power";
        return false;
    }

    // Validate as either state/province OR power
    bool isState = Arrl::isValidSection(stateOrPower.toUpper());
    bool isPower = isValidPower(stateOrPower);

    if (!isState && !isPower) {
        errorMsg = QString("Invalid state/province or power: %1").arg(stateOrPower);
        return false;
    }

    return true;
}

QMap<QString, QString> ARRLDXContest::parseReceivedExchange(const QString& exchange) const {
    QMap<QString, QString> result;
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    QString rst = RSTValidator::getDefault(m_mode);
    QString stateOrPower;

    if (parts.size() == 1) {
        stateOrPower = parts[0];
    } else if (parts.size() >= 2) {
        rst = parts[0];
        stateOrPower = parts[1];
    }

    result["RST"] = rst;

    // Detect if it's a state/province or power
    if (Arrl::isValidSection(stateOrPower.toUpper())) {
        result["State"] = stateOrPower.toUpper();
        result["Power"] = "";
    } else {
        result["State"] = "";
        result["Power"] = stateOrPower;
    }

    return result;
}

int ARRLDXContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    Q_UNUSED(qso);
    Q_UNUSED(myStation);
    return 3;  // 3 points per QSO
}

int ARRLDXContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // ARRL DX: Score = QSO Points × Multipliers
    int totalMults = 0;
    for (int count : multiplierCounts.values()) {
        totalMults += count;
    }

    return totalQSOPoints * totalMults;
}

QList<MultiplierDefinition> ARRLDXContest::getMultiplierTypes() const {
    // W/VE stations work DX countries (DXCC)
    // DX stations work W/VE states/provinces
    return {
        {MultiplierType::Country, MultiplierScope::PerBand, "Countries"},
        {MultiplierType::State, MultiplierScope::PerBand, "States/Provinces"}
    };
}

QString ARRLDXContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString value;

    if (multType == MultiplierType::Country) {
        // DX countries (for W/VE stations working DX)
        QString dxcc = qso.dxccPrefix;
        if (!dxcc.isEmpty() && dxcc != "K" && dxcc != "VE" &&
            !alreadyWorkedValues.contains(dxcc)) {
            value = dxcc;
        }
    } else if (multType == MultiplierType::State) {
        // States/Provinces (for DX stations working W/VE)
        QString state = qso.state.toUpper();
        if (!state.isEmpty() && !alreadyWorkedValues.contains(state)) {
            value = state;
        }
    }

    return value;
}

bool ARRLDXContest::isValidQSO(
    const QSO& qso,
    const StationInfo& myStation,
    QString& errorMsg) const
{
    // W/VE stations can ONLY work DX
    // DX stations can ONLY work W/VE
    bool imWVE = isWVEStation(myStation);
    bool theyWVE = (qso.dxccPrefix == "K" || qso.dxccPrefix == "VE");

    if (imWVE && theyWVE) {
        errorMsg = "W/VE stations may only work DX stations";
        return false;
    }

    if (!imWVE && !theyWVE) {
        errorMsg = "DX stations may only work W/VE stations";
        return false;
    }

    return true;
}

QMap<QString, QString> ARRLDXContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers;
    headers["CONTEST"] = (m_mode == ModeType::CW) ? CABRILLO_NAME_CW : CABRILLO_NAME_SSB;
    return headers;
}

bool ARRLDXContest::isWVEStation(const StationInfo& station) {
    QString country = station.country.toUpper();
    return (country.contains("UNITED STATES") ||
            country.contains("CANADA") ||
            country == "USA" ||
            country == "US" ||
            country == "CANADA");
}

bool ARRLDXContest::isValidPower(const QString& power) {
    bool ok;
    int p = power.toInt(&ok);
    return ok && p >= 1 && p <= 2000;  // Typical range: 5, 10, 50, 100, 500, 1000, 1500
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::ARRLDXContest, "ARRL_DX");
