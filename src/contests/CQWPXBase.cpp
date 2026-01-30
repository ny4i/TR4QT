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

#include "CQWPXBase.h"
#include "../models/QSO.h"
#include "RSTValidator.h"
#include <QRegularExpression>

namespace TR4QT {

QList<ExchangeField> CQWPXBase::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (auto-filled based on mode)
    ExchangeField rstField;
    rstField.name = "RST";
    rstField.hint = RSTValidator::getDefault(getContestMode());
    rstField.autoFill = true;
    rstField.maxLength = 3;
    fields.append(rstField);

    // Serial number
    ExchangeField serialField;
    serialField.name = "Serial";
    serialField.hint = "001";
    serialField.autoFill = false;
    serialField.maxLength = 4;
    fields.append(serialField);

    return fields;
}

QList<ExchangeField> CQWPXBase::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (auto-filled based on mode)
    ExchangeField rstField;
    rstField.name = "RST";
    rstField.hint = RSTValidator::getDefault(getContestMode());
    rstField.autoFill = true;
    rstField.maxLength = 3;
    fields.append(rstField);

    // Serial number (auto-increment)
    ExchangeField serialField;
    serialField.name = "Serial";
    serialField.hint = "001";
    serialField.autoFill = true;  // Auto-increment
    serialField.maxLength = 4;
    fields.append(serialField);

    return fields;
}

QList<TableColumn> CQWPXBase::getTableColumns() const {
    return {
        TableColumn("Serial", "#", 50, TableColumn::Alignment::Right)
    };
}

QString CQWPXBase::formatSentExchange(int serialNumber, const QString& rst) const {
    // Format: RST + Serial (e.g., "599 001")
    return QString("%1 %2").arg(rst).arg(serialNumber, 3, 10, QChar('0'));
}

bool CQWPXBase::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (Serial or RST + Serial, e.g., '001' or '599 001')";
        return false;
    }

    QString serial;
    if (parts.size() == 1) {
        // Only serial provided - RST will be auto-filled
        // Single number is always assumed to be serial, not RST
        serial = parts[0];
    } else if (parts.size() == 2) {
        // Two fields: detect which is RST and which is Serial
        // Be smart about order - check patterns to determine fields
        QString first = parts[0];
        QString second = parts[1];

        ModeType mode = getContestMode();
        bool firstIsRST = RSTValidator::isValid(first, mode);
        bool secondIsRST = RSTValidator::isValid(second, mode);

        if (firstIsRST && !secondIsRST) {
            // First is RST, second is serial (e.g., "599 3")
            serial = second;
        } else if (!firstIsRST && secondIsRST) {
            // Second is RST, first is serial (e.g., "4 59")
            serial = first;
        } else if (firstIsRST && secondIsRST) {
            // Both could be RST - assume first is RST, second is serial
            serial = second;
        } else {
            // Neither is valid RST
            QString expectedFormat = (mode == ModeType::CW || mode == ModeType::CWR) ?
                "3 digits (e.g., 599, 579)" : "2-3 digits (e.g., 59, 599)";
            errorMsg = QString("Invalid RST format. Expected %1 (Pattern: [1-5][1-9][1-9]?)")
                .arg(expectedFormat);
            return false;
        }

        // Validate serial is provided
        if (serial.isEmpty()) {
            errorMsg = "Serial number required";
            return false;
        }
    } else {
        // Too many fields
        errorMsg = "Exchange must be: Serial (e.g., '001') or RST + Serial (e.g., '599 001')";
        return false;
    }

    // Validate serial number (1-9999)
    bool ok;
    int serialNum = serial.toInt(&ok);
    if (!ok || serialNum < 1 || serialNum > 9999) {
        errorMsg = "Invalid serial number (must be 1-9999)";
        return false;
    }

    return true;
}

void CQWPXBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() == 1) {
        // Only serial provided - auto-fill RST based on mode
        qso.rstReceived = RSTValidator::getDefault(getContestMode());
        qso.serialNumberReceived = parts[0].toInt();
    } else if (parts.size() >= 2) {
        // Two fields: detect which is RST and which is Serial (order-agnostic)
        QString first = parts[0];
        QString second = parts[1];

        ModeType mode = getContestMode();
        bool firstIsRST = RSTValidator::isValid(first, mode);
        bool secondIsRST = RSTValidator::isValid(second, mode);

        if (firstIsRST && !secondIsRST) {
            // First is RST, second is serial (e.g., "599 3")
            qso.rstReceived = first;
            qso.serialNumberReceived = second.toInt();
        } else if (!firstIsRST && secondIsRST) {
            // Second is RST, first is serial (e.g., "4 59")
            qso.rstReceived = second;
            qso.serialNumberReceived = first.toInt();
        } else if (firstIsRST && secondIsRST) {
            // Both could be RST - assume first is RST, second is serial
            qso.rstReceived = first;
            qso.serialNumberReceived = second.toInt();
        } else {
            // Neither is valid RST - use defaults
            qso.rstReceived = RSTValidator::getDefault(getContestMode());
            qso.serialNumberReceived = first.toInt();  // Assume first is serial
        }
    }

    // Format exchangeReceived with RST prepended (e.g., "599 001")
    formatExchangeReceived(exchange, qso);
}

int CQWPXBase::calculateQSOPoints(const QSO& qso, const StationInfo& myStation) const {
    // CQWPX Scoring rules (from cqwpx.com official rules):
    //
    // Different Continents:
    //   - 28/21/14 MHz (10m/15m/20m): 3 points (CW) or 2 points (SSB)
    //   - 7/3.5/1.8 MHz (40m/80m/160m): 6 points (CW) or 4 points (SSB)
    //
    // Same Continent, Different Countries:
    //   - 28/21/14 MHz: 1 point (both modes)
    //   - 7/3.5/1.8 MHz: 2 points (CW) or 1 point (SSB)
    //   - North America exception: For NA stations, contacts within NA are doubled
    //     (2 points on high bands, 4 points on low bands for CW; 2 points on high, 2 points on low for SSB)
    //
    // Same Country: 1 point (all bands, all modes)

    const QString& theirCountry = qso.dxccEntity;
    const QString& theirContinent = qso.continent;

    // Determine if we're on low bands (160m, 80m, 40m) or high bands (20m, 15m, 10m)
    bool isLowBand = (qso.band == BandType::Band160M ||
                      qso.band == BandType::Band80M ||
                      qso.band == BandType::Band40M);

    double modeMultiplier = getModePointMultiplier();  // CW = 1.0, SSB = 0.67
    int points = 0;

    // Same country: 1 point (all bands)
    // Only check country match if both countries are set (not empty)
    if (!myStation.country.isEmpty() && !theirCountry.isEmpty() &&
        myStation.country == theirCountry) {
        points = 1;
    }
    // Different continent
    else if (myStation.continent != theirContinent) {
        // Per official rules:
        // - High bands: 3 points CW, 2 points SSB
        // - Low bands: 6 points CW, 4 points SSB
        int basePoints = isLowBand ? 6 : 3;
        points = static_cast<int>(basePoints * modeMultiplier);
        if (points < 1) points = 1;  // Safety: minimum 1 point
    }
    // Same continent, different country
    else {
        // Per official rules:
        // - High bands: 1 point (both modes)
        // - Low bands: 2 points CW, 1 point SSB
        if (isLowBand) {
            int basePoints = 2;
            points = static_cast<int>(basePoints * modeMultiplier);
            if (points < 1) points = 1;  // Minimum 1 point
        } else {
            points = 1;  // High bands: 1 point regardless of mode
        }

        // North America exception: double points for contacts within NA
        if (myStation.continent == "NA") {
            points *= 2;
        }
    }

    return points;
}

int CQWPXBase::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // CQ WPX: Score = QSO Points × Total Prefixes
    int totalPrefixes = multiplierCounts.value(MultiplierType::Prefix, 0);
    return totalQSOPoints * totalPrefixes;
}

QList<MultiplierDefinition> CQWPXBase::getMultiplierTypes() const {
    return {
        {MultiplierType::Prefix, MultiplierScope::AllBands, "Prefixes"}
    };
}

QString CQWPXBase::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    if (multType != MultiplierType::Prefix) {
        return QString();  // Not a multiplier for this contest
    }

    QString prefix = extractPrefix(qso.callsign);

    // Check if this prefix has already been worked
    if (alreadyWorkedValues.contains(prefix)) {
        return QString();  // Not a new multiplier
    }

    return prefix;
}

QString CQWPXBase::extractPrefix(const QString& callsign) {
    QString call = callsign.trimmed().toUpper();

    // Remove portable indicators (/P, /M, /MM, /QRP, /A, etc.)
    // Keep only the base call or the prefix before the /
    if (call.contains('/')) {
        QStringList parts = call.split('/');

        // If first part has a digit, use it (e.g., W1/G3XYZ → W1)
        // Otherwise use second part (e.g., G3XYZ/P → G3)
        for (const QString& part : parts) {
            if (part.contains(QRegularExpression("[0-9]"))) {
                call = part;
                break;
            }
        }
    }

    // Find the first digit in the callsign
    int firstDigitPos = -1;
    for (int i = 0; i < call.length(); ++i) {
        if (call[i].isDigit()) {
            firstDigitPos = i;
            break;
        }
    }

    if (firstDigitPos == -1) {
        // No digit found - unusual, but return the whole call
        return call;
    }

    // Prefix is everything up to and including the first digit
    // W1AW → W1
    // DL1ABC → DL1
    // JA1234XYZ → JA1
    return call.left(firstDigitPos + 1);
}

} // namespace TR4QT
