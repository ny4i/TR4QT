#include "ARRLDXBase.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
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

    QString stateOrPower;
    if (parts.size() == 1) {
        stateOrPower = parts[0];
    } else if (parts.size() == 2) {
        // RST + State/Power
        QString rst = parts[0];
        ModeType mode = getContestMode();
        if (!RSTValidator::isValid(rst, mode)) {
            QString expectedFormat = (mode == ModeType::CW || mode == ModeType::CWR) ?
                "3 digits (e.g., 599)" : "2-3 digits (e.g., 59)";
            errorMsg = QString("Invalid RST format. Expected %1").arg(expectedFormat);
            return false;
        }
        stateOrPower = parts[1];
    } else {
        errorMsg = "Exchange must be: State/Power (e.g., 'FL' or '100') or RST + State/Power";
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

void ARRLDXBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    QString rst = RSTValidator::getDefault(getContestMode());
    QString stateOrPower;

    if (parts.size() == 1) {
        stateOrPower = parts[0];
    } else if (parts.size() >= 2) {
        rst = parts[0];
        stateOrPower = parts[1];
    }

    // Populate QSO fields directly
    qso.rstReceived = rst;

    // Detect if it's a state/province or power
    if (Arrl::isValidSection(stateOrPower.toUpper())) {
        qso.state = stateOrPower.toUpper();
        qso.power = "";
    } else {
        qso.state = "";
        qso.power = stateOrPower;
    }

    // Format exchangeReceived with RST prepended (e.g., "599 FL" or "599 100")
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
    bool ok;
    int p = power.toInt(&ok);
    return ok && p >= 1 && p <= 2000;  // Typical range: 5, 10, 50, 100, 500, 1000, 1500
}

} // namespace TR4QT
