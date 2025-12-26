#include "CQWWContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata CQWWContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "CQWW";
    meta.displayName = "CQ World Wide DX Contest";
    meta.shortName = "CQ WW";
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

    meta.schedule = "Last full weekend of November";
    meta.website = "https://www.cqww.com/";
    meta.description = "Work as many countries and CQ zones as possible. Exchange: RST + CQ Zone.";

    return meta;
}

ContestBase* CQWWContest::create(ModeType mode) {
    return new CQWWContest(mode);
}

CQWWContest::CQWWContest(ModeType mode)
    : m_mode(mode)
{
}

QString CQWWContest::getContestId() const {
    return m_mode == ModeType::CW ? "CQWW_CW" : "CQWW_SSB";
}

QString CQWWContest::getContestName() const {
    return m_mode == ModeType::CW ?
           "CQ World Wide DX Contest - CW" :
           "CQ World Wide DX Contest - SSB";
}

QList<ExchangeField> CQWWContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = m_mode == ModeType::CW ? "599" : "59";
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // CQ Zone
    ExchangeField zone;
    zone.name = "Zone";
    zone.hint = "CQ Zone (1-40)";
    zone.autoFill = false;
    zone.maxLength = 2;
    fields.append(zone);

    return fields;
}

QList<ExchangeField> CQWWContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // Just CQ Zone (RST is always sent, no need to configure)
    ExchangeField zone;
    zone.name = "Zone";
    zone.hint = "Your CQ Zone";
    zone.autoFill = true;  // From station settings
    zone.maxLength = 2;
    fields.append(zone);

    return fields;
}

QList<TableColumn> CQWWContest::getTableColumns() const {
    return {
        TableColumn("Zone", "Zn", 50, TableColumn::Alignment::Right)
    };
}

QString CQWWContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);  // CQ WW doesn't use serial numbers

    // Will be filled from settings, but format is "RST Zone"
    // e.g., "599 05" or "59 14"
    return rst + " {ZONE}";  // {ZONE} will be replaced by actual zone from settings
}

bool CQWWContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (Zone or RST + Zone, e.g., '14' or '599 14')";
        return false;
    }

    // Determine if RST was provided
    QString zoneStr;
    if (parts.size() == 1) {
        // Only zone provided - RST will be auto-filled
        // But reject if it looks like RST (59x pattern) to avoid ambiguity
        QString field = parts[0];
        bool ok;
        int val = field.toInt(&ok);
        if (ok && ((field.length() == 2 && val >= 11 && val <= 59) ||
                   (field.length() == 3 && val >= 111 && val <= 599))) {
            errorMsg = "Ambiguous input - looks like RST. Please enter: RST + Zone (e.g., '599 14')";
            return false;
        }
        zoneStr = parts[0];
    } else if (parts.size() == 2) {
        // RST + Zone provided - validate RST
        QString rst = parts[0];
        if (m_mode == ModeType::CW) {
            if (rst.length() != 3) {
                errorMsg = "CW RST must be 3 digits (e.g., 599)";
                return false;
            }
        } else {
            if (rst.length() != 2 && rst.length() != 3) {
                errorMsg = "SSB RST must be 2-3 digits (e.g., 59)";
                return false;
            }
        }
        zoneStr = parts[1];
    } else {
        // Too many fields
        errorMsg = "Exchange must be: Zone (e.g., '14') or RST + Zone (e.g., '599 14')";
        return false;
    }

    // Validate Zone (1-40)
    bool ok;
    int zone = zoneStr.toInt(&ok);
    if (!ok || zone < 1 || zone > 40) {
        errorMsg = "CQ Zone must be between 1 and 40";
        return false;
    }

    return true;
}

QMap<QString, QString> CQWWContest::parseReceivedExchange(const QString& exchange) const {
    QMap<QString, QString> parsed;

    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() == 1) {
        QString field = parts[0];
        bool ok;
        int val = field.toInt(&ok);

        // Check if it looks like RST (59x pattern)
        bool looksLikeRST = ok && ((field.length() == 2 && val >= 11 && val <= 59) ||
                                   (field.length() == 3 && val >= 111 && val <= 599));

        if (looksLikeRST) {
            // Treat as RST only, no zone (incomplete exchange)
            parsed["RST"] = field;
        } else {
            // Treat as zone, auto-fill RST
            parsed["RST"] = (m_mode == ModeType::CW) ? "599" : "59";
            parsed["Zone"] = field;
        }
    } else if (parts.size() >= 2) {
        // Full exchange: RST + Zone
        parsed["RST"] = parts[0];
        parsed["Zone"] = parts[1];
    }

    return parsed;
}

int CQWWContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    // CQWW Scoring rules (from cqww.com official rules):
    // CW Mode:
    //   - Different continents: 3 points
    //   - Same continent, different countries (North America): 2 points
    //   - Same continent, different countries (other): 1 point
    //   - Same country: 0 points
    // SSB Mode:
    //   - Different continents: 2 points
    //   - Same continent, different countries (North America): 2 points
    //   - Same continent, different countries (other): 1 point
    //   - Same country: 0 points

    const QString& theirCountry = qso.dxccEntity;
    const QString& theirContinent = qso.continent;

    // Same country: 0 points (but counts as multiplier)
    if (myStation.country == theirCountry) {
        return 0;
    }

    // Different continent: 3 points (CW) or 2 points (SSB)
    if (myStation.continent != theirContinent) {
        return (m_mode == ModeType::CW) ? 3 : 2;
    }

    // Same continent, different country
    // North America gets 2 points, others get 1 point
    if (myStation.continent == "NA") {
        return 2;
    } else {
        return 1;
    }
}

int CQWWContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    int countries = multiplierCounts.value(MultiplierType::Country, 0);
    int zones = multiplierCounts.value(MultiplierType::CQZone, 0);

    return totalQSOPoints * (countries + zones);
}

QList<MultiplierDefinition> CQWWContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    // DXCC Countries (per band)
    MultiplierDefinition country;
    country.type = MultiplierType::Country;
    country.scope = MultiplierScope::PerBand;
    country.displayName = "Countries";
    mults.append(country);

    // CQ Zones (per band)
    MultiplierDefinition zone;
    zone.type = MultiplierType::CQZone;
    zone.scope = MultiplierScope::PerBand;
    zone.displayName = "CQ Zones";
    mults.append(zone);

    return mults;
}

QString CQWWContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString value;

    switch (multType) {
    case MultiplierType::Country:
        value = qso.dxccPrefix;  // Use DXCC prefix (e.g., "K", "JA", "G")
        break;

    case MultiplierType::CQZone:
        value = QString::number(qso.cqZone);
        break;

    default:
        return QString();  // Not a multiplier for this contest
    }

    // Check if already worked
    if (alreadyWorkedValues.contains(value)) {
        return QString();  // Already worked, not a new mult
    }

    return value;
}

QMap<QString, QString> CQWWContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();

    headers["CONTEST"] = m_mode == ModeType::CW ? "CQ-WW-CW" : "CQ-WW-SSB";

    return headers;
}

} // namespace TR4QT

// Auto-register with factory
REGISTER_CONTEST(TR4QT::CQWWContest, "CQWW");
