#include "IARUHFContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "RSTValidator.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata IARUHFContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "IARU_HF";
    meta.displayName = "IARU HF World Championship";
    meta.shortName = "IARU HF";
    meta.supportedModes = {ModeType::CW, ModeType::USB};
    meta.hasSeparateContests = true;

    meta.wa7bnmIdCW = WA7BNM_ID_CW;
    meta.wa7bnmIdSSB = WA7BNM_ID_SSB;
    meta.wa7bnmIdMixed = WA7BNM_ID_MIXED;

    meta.cabrilloNameCW = CABRILLO_NAME_CW;
    meta.cabrilloNameSSB = CABRILLO_NAME_SSB;
    meta.cabrilloNameMixed = "";

    meta.adifContestIdCW = ADIF_CONTEST_ID_CW;
    meta.adifContestIdSSB = ADIF_CONTEST_ID_SSB;
    meta.adifContestIdMixed = "";

    meta.schedule = "Second full weekend of July";
    meta.website = "https://www.arrl.org/iaru-hf-world-championship";
    meta.description = "Work as many ITU zones and IARU HQ stations as possible. Exchange: RST + ITU Zone or HQ/AC/R1/R2/R3.";

    return meta;
}

ContestBase* IARUHFContest::create(ModeType mode, const StationInfo& myStation) {
    return new IARUHFContest(mode, myStation);
}

IARUHFContest::IARUHFContest(ModeType mode, const StationInfo& myStation)
    : ContestBase(myStation)
    , m_mode(mode)
{
}

QString IARUHFContest::getContestId() const {
    return m_mode == ModeType::CW ? "IARU_HF_CW" : "IARU_HF_SSB";
}

QString IARUHFContest::getContestName() const {
    return m_mode == ModeType::CW ?
           "IARU HF World Championship - CW" :
           "IARU HF World Championship - SSB";
}

QString IARUHFContest::getADIFContestId() const {
    return m_mode == ModeType::CW ? ADIF_CONTEST_ID_CW : ADIF_CONTEST_ID_SSB;
}

QList<ExchangeField> IARUHFContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = RSTValidator::getDefault(m_mode);
    rst.autoFill = true;
    rst.maxLength = 3;
    fields.append(rst);

    // ITU Zone or special station
    ExchangeField zone;
    zone.name = "Zone";
    zone.hint = "ITU Zone (1-90) or HQ/AC/R1/R2/R3";
    zone.autoFill = false;
    zone.maxLength = 3;
    fields.append(zone);

    return fields;
}

QList<ExchangeField> IARUHFContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // Just ITU Zone (RST is always sent, no need to configure)
    ExchangeField zone;
    zone.name = "Zone";
    zone.hint = "Your ITU Zone";
    zone.autoFill = true;  // From station settings
    zone.maxLength = 2;
    fields.append(zone);

    return fields;
}

QList<TableColumn> IARUHFContest::getTableColumns() const {
    return {
        TableColumn("Zone", "Zn", 50, TableColumn::Alignment::Right)
    };
}

QString IARUHFContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);  // IARU HF doesn't use serial numbers

    // Will be filled from settings, but format is "RST Zone"
    // e.g., "599 46" or "59 46"
    return rst + " {ZONE}";  // {ZONE} will be replaced by actual zone from settings
}

bool IARUHFContest::isSpecialStation(const QString& exchange) {
    QString upper = exchange.trimmed().toUpper();
    return upper == "HQ" || upper == "AC" ||
           upper == "R1" || upper == "R2" || upper == "R3";
}

bool IARUHFContest::isValidITUZone(const QString& zone) {
    bool ok;
    int zoneNum = zone.toInt(&ok);
    return ok && zoneNum >= 1 && zoneNum <= 90;
}

bool IARUHFContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (Zone/HQ or RST + Zone/HQ, e.g., '46' or '599 46' or 'HQ')";
        return false;
    }

    // Determine if RST was provided
    QString zoneStr;
    if (parts.size() == 1) {
        // Only zone/HQ provided - RST will be auto-filled
        zoneStr = parts[0];
    } else if (parts.size() == 2) {
        // Two fields: detect which is RST and which is Zone/HQ
        QString first = parts[0];
        QString second = parts[1];

        // Check for special stations first
        bool firstIsSpecial = isSpecialStation(first);
        bool secondIsSpecial = isSpecialStation(second);

        if (firstIsSpecial) {
            // First is HQ/AC/R1/R2/R3, second should be RST (unusual but possible)
            zoneStr = first;
        } else if (secondIsSpecial) {
            // Normal case: RST + HQ
            zoneStr = second;
        } else {
            // Neither is special, check for RST pattern
            // RST pattern: [1-5][1-9][1-9]? for phone, [1-5][1-9][1-9] for CW
            QRegularExpression rstPattern;
            if (m_mode == ModeType::CW || m_mode == ModeType::CWR) {
                rstPattern.setPattern("^[1-5][1-9][1-9]$");
            } else {
                rstPattern.setPattern("^[1-5][1-9][1-9]?$");
            }

            bool firstIsRST = rstPattern.match(first).hasMatch();
            bool secondIsRST = rstPattern.match(second).hasMatch();

            // Check if values are valid ITU zones (1-90)
            bool firstIsValidZone = isValidITUZone(first);
            bool secondIsValidZone = isValidITUZone(second);

            if (firstIsRST && !secondIsRST) {
                // First is RST, second is zone (e.g., "599 46")
                zoneStr = second;
            } else if (!firstIsRST && secondIsRST) {
                // Second is RST, first is zone (e.g., "46 599")
                zoneStr = first;
            } else if (firstIsRST && secondIsRST) {
                // Both match RST pattern - use zone range to decide
                if (firstIsValidZone && !secondIsValidZone) {
                    // First is valid zone → first=zone, second=RST (e.g., "46 59")
                    zoneStr = first;
                } else if (!firstIsValidZone && secondIsValidZone) {
                    // Second is valid zone → first=RST, second=zone (e.g., "599 46")
                    zoneStr = second;
                } else {
                    // Both or neither in valid zone range - assume first is RST
                    zoneStr = second;
                }
            } else {
                // Neither is valid RST
                QString expectedFormat = (m_mode == ModeType::CW || m_mode == ModeType::CWR) ?
                    "3 digits (e.g., 599, 579)" : "2-3 digits (e.g., 59, 599)";
                errorMsg = QString("Invalid RST format. Expected %1 (Pattern: [1-5][1-9][1-9]?)")
                    .arg(expectedFormat);
                return false;
            }
        }

        // Validate zone is provided
        if (zoneStr.isEmpty()) {
            errorMsg = "Zone or special station required";
            return false;
        }
    } else {
        // Too many fields
        errorMsg = "Exchange must be: Zone/HQ (e.g., '46' or 'HQ') or RST + Zone/HQ (e.g., '599 46' or '599 HQ')";
        return false;
    }

    // Validate Zone (1-90) or special station (HQ, AC, R1, R2, R3)
    if (!isSpecialStation(zoneStr) && !isValidITUZone(zoneStr)) {
        errorMsg = "ITU Zone must be between 1 and 90, or HQ/AC/R1/R2/R3 for special stations";
        return false;
    }

    return true;
}

void IARUHFContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    QString rst;
    QString zoneStr;

    if (parts.size() == 1) {
        // Only zone/HQ provided - auto-fill RST
        rst = RSTValidator::getDefault(m_mode);
        zoneStr = parts[0].toUpper();
    } else if (parts.size() >= 2) {
        // Two fields: detect which is RST and which is Zone/HQ (order-agnostic)
        QString first = parts[0];
        QString second = parts[1];

        // Check for special stations first
        bool firstIsSpecial = isSpecialStation(first);
        bool secondIsSpecial = isSpecialStation(second);

        if (firstIsSpecial) {
            // First is HQ/AC/R1/R2/R3, second should be RST (unusual but possible)
            zoneStr = first.toUpper();
            rst = second;
        } else if (secondIsSpecial) {
            // Normal case: RST + HQ
            rst = first;
            zoneStr = second.toUpper();
        } else {
            // Neither is special, check for RST pattern
            QRegularExpression rstPattern;
            if (m_mode == ModeType::CW || m_mode == ModeType::CWR) {
                rstPattern.setPattern("^[1-5][1-9][1-9]$");
            } else {
                rstPattern.setPattern("^[1-5][1-9][1-9]?$");
            }

            bool firstIsRST = rstPattern.match(first).hasMatch();
            bool secondIsRST = rstPattern.match(second).hasMatch();

            // Check if values are valid ITU zones (1-90)
            bool firstIsValidZone = isValidITUZone(first);
            bool secondIsValidZone = isValidITUZone(second);

            if (firstIsRST && !secondIsRST) {
                // First is RST, second is zone (e.g., "599 46")
                rst = first;
                zoneStr = second;
            } else if (!firstIsRST && secondIsRST) {
                // Second is RST, first is zone (e.g., "46 599")
                rst = second;
                zoneStr = first;
            } else if (firstIsRST && secondIsRST) {
                // Both match RST pattern - use zone range to decide
                if (firstIsValidZone && !secondIsValidZone) {
                    // First is valid zone → first=zone, second=RST (e.g., "46 59")
                    zoneStr = first;
                    rst = second;
                } else if (!firstIsValidZone && secondIsValidZone) {
                    // Second is valid zone → first=RST, second=zone (e.g., "599 46")
                    rst = first;
                    zoneStr = second;
                } else {
                    // Both or neither in valid zone range - assume first is RST
                    rst = first;
                    zoneStr = second;
                }
            } else {
                // Neither is valid RST - use defaults
                rst = RSTValidator::getDefault(m_mode);
                zoneStr = first;  // Assume first is zone
            }
        }
    }

    // Populate QSO fields directly
    qso.rstReceived = rst;
    qso.ituZoneExchange = zoneStr;

    // If it's a numeric zone, also populate ituZone integer field
    if (!isSpecialStation(zoneStr)) {
        bool ok;
        int zone = zoneStr.toInt(&ok);
        if (ok) {
            qso.ituZone = zone;
        }
    }

    // Format exchangeReceived with RST prepended (e.g., "599 46" or "599 HQ")
    formatExchangeReceived(exchange, qso);
}

int IARUHFContest::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    // IARU HF Scoring rules:
    // - 1 point: Same ITU zone
    // - 3 points: Same continent, different ITU zone
    // - 5 points: Different continent, different ITU zone
    //
    // Note: Special stations (HQ, AC, R1, R2, R3) are scored based on their ITU zone

    int theirITUZone = qso.ituZone;
    const QString& theirContinent = qso.continent;

    // For special stations, use their ITU zone for scoring
    // (the zone was looked up from cty.dat based on callsign)

    // Same ITU zone: 1 point
    if (myStation.ituZone == theirITUZone) {
        return 1;
    }

    // Different continent: 5 points
    if (myStation.continent != theirContinent) {
        return 5;
    }

    // Same continent, different ITU zone: 3 points
    return 3;
}

int IARUHFContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // IARU HF Scoring: Total QSO points × Total multipliers across all bands
    // Multipliers are: ITU zones + HQ stations (counted separately)

    int ituZones = multiplierCounts.value(MultiplierType::ITUZone, 0);
    int hqStations = multiplierCounts.value(MultiplierType::Custom, 0);

    return totalQSOPoints * (ituZones + hqStations);
}

QList<MultiplierDefinition> IARUHFContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    // ITU Zones (per band)
    MultiplierDefinition zone;
    zone.type = MultiplierType::ITUZone;
    zone.scope = MultiplierScope::PerBand;
    zone.displayName = "ITU Zones";
    mults.append(zone);

    // HQ Stations (per band) - using Custom multiplier type
    MultiplierDefinition hq;
    hq.type = MultiplierType::Custom;
    hq.scope = MultiplierScope::PerBand;
    hq.displayName = "HQ Stations";
    mults.append(hq);

    return mults;
}

QString IARUHFContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString value;

    // Get the zone/HQ from dedicated exchange field
    QString zoneExchange = qso.ituZoneExchange.toUpper();

    switch (multType) {
    case MultiplierType::ITUZone:
        // Only regular ITU zones count, not special stations
        if (!isSpecialStation(zoneExchange)) {
            value = QString::number(qso.ituZone);
        }
        break;

    case MultiplierType::Custom:
        // HQ stations and officials (HQ, AC, R1, R2, R3)
        if (isSpecialStation(zoneExchange)) {
            // Use the exchange value as the multiplier
            // This allows tracking different HQ stations separately
            value = zoneExchange;
        }
        break;

    default:
        return QString();  // Not a multiplier for this contest
    }

    // Check if already worked
    if (!value.isEmpty() && alreadyWorkedValues.contains(value)) {
        return QString();  // Already worked, not a new mult
    }

    return value;
}

QMap<QString, QString> IARUHFContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();

    headers["CONTEST"] = m_mode == ModeType::CW ? "IARU-HF" : "IARU-HF";

    return headers;
}

} // namespace TR4QT

// Auto-register with factory - register both CW and SSB variants
REGISTER_CONTEST(TR4QT::IARUHFContest, "IARU_HF_CW");
REGISTER_CONTEST(TR4QT::IARUHFContest, "IARU_HF_SSB");
