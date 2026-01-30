/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "IARUHFContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include "RSTValidator.h"
#include "../exchanges/SmartExchangeParser.h"
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
    meta.floatingDates = {
        FloatingDate(7, "2nd full weekend")  // July
    };
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

    // Too many fields
    if (parts.size() > 2) {
        errorMsg = "Exchange must be: Zone/HQ (e.g., '46' or 'HQ') or RST + Zone/HQ (e.g., '599 46' or '599 HQ')";
        return false;
    }

    // Use SmartExchangeParser to parse the exchange order-agnostically
    QSO tempQSO;
    parseReceivedExchange(exchange, tempQSO);

    // Get zone from parsed QSO
    QString zoneStr = tempQSO.ituZoneExchange;

    // Validate Zone is present
    if (zoneStr.isEmpty()) {
        errorMsg = "Zone or special station required";
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
    // Use SmartExchangeParser for order-agnostic field detection
    // Allows "599 46" or "46 599", "HQ 599" or "599 HQ" etc.
    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<IARUHFContest*>(this)
    );

    // Get RST (auto-fill if not provided)
    qso.rstReceived = parsed.value("RST", RSTValidator::getDefault(m_mode));

    // Get Zone field (may be ITU zone number or special station code)
    QString zoneStr = parsed.value("Zone").toUpper();

    // Handle special stations (HQ, AC, R1, R2, R3) - they won't be detected
    // by looksLikeITUZone, so check for them if Zone is empty
    if (zoneStr.isEmpty()) {
        // SmartExchangeParser may have left special station unmatched
        // Look for it in the original exchange
        QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));
        for (const QString& part : parts) {
            if (isSpecialStation(part)) {
                zoneStr = part.toUpper();
                break;
            }
        }
    }

    // Populate QSO fields
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

// Auto-register with factory - single registration, hasSeparateContests handles CW/SSB split
REGISTER_CONTEST(TR4QT::IARUHFContest, "IARU_HF");
