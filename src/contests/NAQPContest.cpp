#include "NAQPContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata NAQPContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "NAQP";
    meta.displayName = "North American QSO Party";
    meta.shortName = "NAQP";
    meta.supportedModes = {ModeType::CW, ModeType::USB, ModeType::RTTY};
    meta.hasSeparateContests = true;  // Separate CW, SSB, and RTTY contests

    meta.wa7bnmIdCW = WA7BNM_ID_CW;
    meta.wa7bnmIdSSB = WA7BNM_ID_SSB;
    meta.wa7bnmIdMixed = WA7BNM_ID_RTTY;

    meta.cabrilloNameCW = CABRILLO_NAME_CW;
    meta.cabrilloNameSSB = CABRILLO_NAME_SSB;
    meta.cabrilloNameMixed = CABRILLO_NAME_RTTY;

    meta.adifContestIdCW = ADIF_CONTEST_ID_CW;
    meta.adifContestIdSSB = ADIF_CONTEST_ID_SSB;
    meta.adifContestIdMixed = ADIF_CONTEST_ID_RTTY;

    meta.schedule = "Multiple dates throughout year (Jan, May, Aug, Oct)";
    meta.website = "https://ncjweb.com/naqp/";
    meta.description = "12-hour low power sprint. Exchange: Name + State/Province. Max 100W.";

    return meta;
}

ContestBase* NAQPContest::create(ModeType mode, const StationInfo& myStation) {
    return new NAQPContest(mode, myStation);
}

NAQPContest::NAQPContest(ModeType mode, const StationInfo& myStation)
    : ContestBase(myStation)
    , m_mode(mode)
{
}

QString NAQPContest::getContestId() const {
    if (m_mode == ModeType::CW) return "NAQP_CW";
    if (m_mode == ModeType::RTTY) return "NAQP_RTTY";
    return "NAQP_SSB";
}

QString NAQPContest::getContestName() const {
    if (m_mode == ModeType::CW) return "North American QSO Party - CW";
    if (m_mode == ModeType::RTTY) return "North American QSO Party - RTTY";
    return "North American QSO Party - SSB";
}

QString NAQPContest::getADIFContestId() const {
    if (m_mode == ModeType::CW) return ADIF_CONTEST_ID_CW;
    if (m_mode == ModeType::RTTY) return ADIF_CONTEST_ID_RTTY;
    return ADIF_CONTEST_ID_SSB;
}

QList<ExchangeField> NAQPContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // Name (no RST in NAQP!)
    ExchangeField name;
    name.name = "Name";
    name.hint = "First name";
    name.autoFill = false;
    name.maxLength = 20;
    fields.append(name);

    // State/Province
    ExchangeField state;
    state.name = "State";
    state.hint = "State/Province";
    state.autoFill = false;
    state.maxLength = 4;
    fields.append(state);

    return fields;
}

QList<ExchangeField> NAQPContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // Name
    ExchangeField name;
    name.name = "Name";
    name.hint = "Your first name";
    name.autoFill = true;  // From station settings
    name.maxLength = 20;
    fields.append(name);

    // State/Province
    ExchangeField state;
    state.name = "State";
    state.hint = "Your State/Province";
    state.autoFill = true;  // From station settings
    state.maxLength = 4;
    fields.append(state);

    return fields;
}

QList<TableColumn> NAQPContest::getTableColumns() const {
    return {
        TableColumn("Name", "Name", 100, TableColumn::Alignment::Left),
        TableColumn("State", "QTH", 60, TableColumn::Alignment::Left)
    };
}

QString NAQPContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    Q_UNUSED(rst);  // No RST in NAQP exchange
    return "{NAME} {STATE}";  // Replaced by actual values from settings
}

bool NAQPContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() < 2) {
        errorMsg = "Exchange must be: Name State (e.g., 'JOHN FL')";
        return false;
    }

    // First part is name - any string is valid
    // Second part is state/province
    QString state = parts[1].toUpper();

    if (!Arrl::isValidSection(state)) {
        errorMsg = QString("Invalid state/province: %1").arg(state);
        return false;
    }

    return true;
}

void NAQPContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() >= 2) {
        qso.operatorName = parts[0];
        qso.state = parts[1].toUpper();

        // If there are more parts, they're part of the name
        if (parts.size() > 2) {
            QStringList nameParts;
            for (int i = 0; i < parts.size() - 1; i++) {
                nameParts.append(parts[i]);
            }
            qso.operatorName = nameParts.join(" ");
            qso.state = parts.last().toUpper();
        }
    }

    // Format exchangeReceived (NAQP does not include RST)
    formatExchangeReceived(exchange, qso);
}

int NAQPContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    Q_UNUSED(qso);
    Q_UNUSED(myStation);
    return 1;  // 1 point per QSO
}

int NAQPContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // NAQP special scoring: QSOs × sum of all multipliers across all bands
    // Since multipliers count per-band, the total in multiplierCounts
    // is the sum of unique mults on each band
    int totalMults = 0;
    for (int count : multiplierCounts.values()) {
        totalMults += count;
    }

    return totalQSOPoints * totalMults;
}

QList<MultiplierDefinition> NAQPContest::getMultiplierTypes() const {
    return {
        {MultiplierType::State, MultiplierScope::PerBand, "States/Provinces"}
    };
}

QString NAQPContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    Q_UNUSED(multType);

    QString state = qso.state.toUpper();
    if (!state.isEmpty() && !alreadyWorkedValues.contains(state)) {
        return state;
    }

    return "";
}

QList<BandType> NAQPContest::getAllowedBands() const {
    // Check if RTTY mode (RTTY excludes 160m)
    if (m_mode == ModeType::RTTY || m_mode == ModeType::RTTYR) {
        return { BandType::Band80M, BandType::Band40M, BandType::Band20M,
                 BandType::Band15M, BandType::Band10M };
    }

    // SSB/CW: all HF bands including 160m
    return { BandType::Band160M, BandType::Band80M, BandType::Band40M,
             BandType::Band20M, BandType::Band15M, BandType::Band10M };
}

QMap<QString, QString> NAQPContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers;
    if (m_mode == ModeType::CW) {
        headers["CONTEST"] = CABRILLO_NAME_CW;
    } else if (m_mode == ModeType::RTTY) {
        headers["CONTEST"] = CABRILLO_NAME_RTTY;
    } else {
        headers["CONTEST"] = CABRILLO_NAME_SSB;
    }
    return headers;
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::NAQPContest, "NAQP");
