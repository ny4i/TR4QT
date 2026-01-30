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

#include "CQWWBase.h"
#include "../models/QSO.h"
#include "RSTValidator.h"
#include <QRegularExpression>

namespace TR4QT {

QList<ExchangeField> CQWWBase::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = RSTValidator::getDefault(getContestMode());
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

QList<ExchangeField> CQWWBase::getSentExchangeFields() const {
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

QList<TableColumn> CQWWBase::getTableColumns() const {
    return {
        TableColumn("Zone", "Zn", 50, TableColumn::Alignment::Right)
    };
}

QString CQWWBase::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);  // CQ WW doesn't use serial numbers

    // Will be filled from settings, but format is "RST Zone"
    // e.g., "599 05" or "59 14"
    return rst + " {ZONE}";  // {ZONE} will be replaced by actual zone from settings
}

bool CQWWBase::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (Zone or RST + Zone, e.g., '14' or '599 14')";
        return false;
    }

    // Determine if RST was provided
    QString zoneStr;
    if (parts.size() == 1) {
        // Only zone provided - RST will be auto-filled
        // Single number is always assumed to be zone, not RST
        zoneStr = parts[0];
    } else if (parts.size() == 2) {
        // Two fields: detect which is RST and which is Zone
        // Be smart about order - check patterns AND zone range (1-40)
        QString first = parts[0];
        QString second = parts[1];

        ModeType mode = getContestMode();
        bool firstIsRST = RSTValidator::isValid(first, mode);
        bool secondIsRST = RSTValidator::isValid(second, mode);

        // Check if values are in valid zone range (1-40)
        bool ok1, ok2;
        int firstInt = first.toInt(&ok1);
        int secondInt = second.toInt(&ok2);
        bool firstIsValidZone = ok1 && firstInt >= 1 && firstInt <= 40;
        bool secondIsValidZone = ok2 && secondInt >= 1 && secondInt <= 40;

        if (firstIsRST && !secondIsRST) {
            // First is RST, second is zone (e.g., "599 14")
            zoneStr = second;
        } else if (!firstIsRST && secondIsRST) {
            // Second is RST, first is zone (e.g., "14 59" or "5 14")
            zoneStr = first;
        } else if (firstIsRST && secondIsRST) {
            // Both match RST pattern - use zone range to decide
            if (firstIsValidZone && !secondIsValidZone) {
                // First is valid zone → first=zone, second=RST (e.g., "14 59")
                zoneStr = first;
            } else if (!firstIsValidZone && secondIsValidZone) {
                // Second is valid zone → first=RST, second=zone (e.g., "59 14")
                zoneStr = second;
            } else {
                // Both or neither in valid zone range - assume first is RST
                zoneStr = second;
            }
        } else {
            // Neither is valid RST
            QString expectedFormat = (mode == ModeType::CW || mode == ModeType::CWR) ?
                "3 digits (e.g., 599, 579)" : "2-3 digits (e.g., 59, 599)";
            errorMsg = QString("Invalid RST format. Expected %1 (Pattern: [1-5][1-9][1-9]?)")
                .arg(expectedFormat);
            return false;
        }

        // Validate zone is provided
        if (zoneStr.isEmpty()) {
            errorMsg = "Zone required";
            return false;
        }
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

void CQWWBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() == 1) {
        // Only zone provided - auto-fill RST
        qso.rstReceived = RSTValidator::getDefault(getContestMode());
        qso.cqZone = parts[0].toInt();
    } else if (parts.size() >= 2) {
        // Two fields: detect which is RST and which is Zone (order-agnostic)
        QString first = parts[0];
        QString second = parts[1];

        ModeType mode = getContestMode();
        bool firstIsRST = RSTValidator::isValid(first, mode);
        bool secondIsRST = RSTValidator::isValid(second, mode);

        // Check if values are in valid zone range (1-40)
        bool ok1, ok2;
        int firstInt = first.toInt(&ok1);
        int secondInt = second.toInt(&ok2);
        bool firstIsValidZone = ok1 && firstInt >= 1 && firstInt <= 40;
        bool secondIsValidZone = ok2 && secondInt >= 1 && secondInt <= 40;

        if (firstIsRST && !secondIsRST) {
            // First is RST, second is zone (e.g., "599 14")
            qso.rstReceived = first;
            qso.cqZone = second.toInt();
        } else if (!firstIsRST && secondIsRST) {
            // Second is RST, first is zone (e.g., "14 59" or "5 14")
            qso.rstReceived = second;
            qso.cqZone = first.toInt();
        } else if (firstIsRST && secondIsRST) {
            // Both match RST pattern - use zone range to decide
            if (firstIsValidZone && !secondIsValidZone) {
                // First is valid zone → first=zone, second=RST (e.g., "14 59")
                qso.cqZone = first.toInt();
                qso.rstReceived = second;
            } else if (!firstIsValidZone && secondIsValidZone) {
                // Second is valid zone → first=RST, second=zone (e.g., "59 14")
                qso.rstReceived = first;
                qso.cqZone = second.toInt();
            } else {
                // Both or neither in valid zone range - assume first is RST
                qso.rstReceived = first;
                qso.cqZone = second.toInt();
            }
        } else {
            // Neither is valid RST - use defaults
            qso.rstReceived = RSTValidator::getDefault(getContestMode());
            qso.cqZone = first.toInt();  // Assume first is zone
        }
    }

    // Format exchangeReceived with RST prepended (e.g., "599 14")
    formatExchangeReceived(exchange, qso);
}

int CQWWBase::calculateQSOPoints(
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

    // Different continent
    if (myStation.continent != theirContinent) {
        // CW: 3 points, SSB: 2 points
        return static_cast<int>(3.0 * getModePointMultiplier());
    }

    // Same continent, different country
    // North America gets 2 points, others get 1 point (both modes)
    if (myStation.continent == "NA") {
        return 2;
    } else {
        return 1;
    }
}

int CQWWBase::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    int countries = multiplierCounts.value(MultiplierType::Country, 0);
    int zones = multiplierCounts.value(MultiplierType::CQZone, 0);

    return totalQSOPoints * (countries + zones);
}

QList<MultiplierDefinition> CQWWBase::getMultiplierTypes() const {
    return {
        {MultiplierType::Country, MultiplierScope::PerBand, "Countries"},
        {MultiplierType::CQZone, MultiplierScope::PerBand, "CQ Zones"}
    };
}

QString CQWWBase::getMultiplierValue(
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

} // namespace TR4QT
