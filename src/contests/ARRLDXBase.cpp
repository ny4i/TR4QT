#include "ARRLDXBase.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
#include "../exchanges/SmartExchangeParser.h"
#include "RSTValidator.h"
#include <QRegularExpression>

namespace TR4QT {

QList<ExchangeField> ARRLDXBase::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = RSTValidator::getDefault(getContestMode());
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // State/Province OR Power (smart detection)
    ExchangeField exchange;
    exchange.name = "State/Power";
    exchange.hint = "FL or 100";
    exchange.autoFill = false;
    exchange.maxLength = 4;
    fields.append(exchange);

    return fields;
}

QList<ExchangeField> ARRLDXBase::getSentExchangeFields() const {
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

QList<TableColumn> ARRLDXBase::getTableColumns() const {
    return {
        TableColumn("State", "QTH/Pwr", 80, TableColumn::Alignment::Left)
    };
}

QString ARRLDXBase::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    return rst + " {STATE}";
}

bool ARRLDXBase::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (State/Province or Power)";
        return false;
    }

    // Use SmartExchangeParser for order-agnostic parsing
    QSO tempQSO;
    parseReceivedExchange(exchange, tempQSO);

    // Validate we got either state or power
    if (tempQSO.state.isEmpty() && tempQSO.power.isEmpty()) {
        errorMsg = "Exchange must include State/Province (e.g., 'FL') or Power (e.g., '100')";
        return false;
    }

    // If we got a state, validate it
    if (!tempQSO.state.isEmpty()) {
        // Use looksLikeState for US states/Canadian provinces (NOT isValidSection)
        if (!SmartExchangeParser::looksLikeState(tempQSO.state)) {
            errorMsg = QString("Invalid state/province: %1").arg(tempQSO.state);
            return false;
        }
    }

    // If we got a power, validate it
    if (!tempQSO.power.isEmpty() && !isValidPower(tempQSO.power)) {
        errorMsg = QString("Invalid power: %1 (must be 1-2000 watts)").arg(tempQSO.power);
        return false;
    }

    return true;
}

void ARRLDXBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    // Use SmartExchangeParser for order-agnostic field detection
    // Allows "599 FL", "FL 599", "100 599", "599 100" etc.
    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<ARRLDXBase*>(this)
    );

    // Get RST (auto-fill if not provided)
    qso.rstReceived = parsed.value("RST", RSTValidator::getDefault(getContestMode()));

    // Get State/Power field
    QString stateOrPower = parsed.value("State/Power");

    // Detect if it's a state/province or power using SmartExchangeParser helpers
    if (SmartExchangeParser::looksLikeState(stateOrPower)) {
        qso.state = stateOrPower.toUpper();
        qso.power = "";
    } else if (!stateOrPower.isEmpty()) {
        qso.state = "";
        // Normalize power: convert K/KW to watts (e.g., "1K" -> "1000")
        qso.power = SmartExchangeParser::normalizePower(stateOrPower);
    }

    // Format exchangeReceived (e.g., "599 FL" or "599 100")
    formatExchangeReceived(exchange, qso);
}

int ARRLDXBase::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    Q_UNUSED(qso);
    Q_UNUSED(myStation);
    return 3;  // 3 points per QSO
}

int ARRLDXBase::calculateTotalScore(
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

QList<MultiplierDefinition> ARRLDXBase::getMultiplierTypes() const {
    // Location-dependent multipliers:
    // - W/VE stations work DX countries (DXCC)
    // - DX stations work W/VE states/provinces
    bool imWVE = isWVEStation(m_myStation);

    if (imWVE) {
        // W/VE: Work DX countries
        return {{MultiplierType::Country, MultiplierScope::PerBand, "Countries"}};
    } else {
        // DX: Work W/VE states/provinces
        return {{MultiplierType::State, MultiplierScope::PerBand, "States/Provinces"}};
    }
}

QString ARRLDXBase::getMultiplierValue(
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

bool ARRLDXBase::isValidQSO(
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

bool ARRLDXBase::isWVEStation(const StationInfo& station) {
    QString country = station.country.toUpper();
    return (country.contains("UNITED STATES") ||
            country.contains("CANADA") ||
            country == "USA" ||
            country == "US" ||
            country == "CANADA");
}

bool ARRLDXBase::isValidPower(const QString& power) {
    // Use SmartExchangeParser for consistent power validation
    // Accepts: numeric (100, 1500), K suffix (1K, 1.5K), KW suffix (1KW)
    return SmartExchangeParser::looksLikePower(power);
}

} // namespace TR4QT
