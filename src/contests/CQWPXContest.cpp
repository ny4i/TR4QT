#include "CQWPXContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata CQWPXContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "CQWPX";
    meta.displayName = "CQ WPX Contest";
    meta.shortName = "CQ WPX";
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

    meta.schedule = "Last full weekend of March (SSB) and May (CW)";
    meta.website = "https://www.cqwpx.com/";
    meta.description = "Work as many callsign prefixes as possible. Exchange: RST + Serial Number.";

    return meta;
}

ContestBase* CQWPXContest::create(ModeType mode) {
    return new CQWPXContest(mode);
}

CQWPXContest::CQWPXContest(ModeType mode)
    : m_mode(mode)
{
}

QString CQWPXContest::getContestId() const {
    return (m_mode == ModeType::CW) ? "CQWPX_CW" : "CQWPX_SSB";
}

QString CQWPXContest::getContestName() const {
    return (m_mode == ModeType::CW) ? "CQ WPX Contest - CW" : "CQ WPX Contest - SSB";
}

QList<ExchangeField> CQWPXContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (auto-filled based on mode)
    ExchangeField rstField;
    rstField.name = "RST";
    rstField.hint = (m_mode == ModeType::CW) ? "599" : "59";
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

QList<ExchangeField> CQWPXContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (auto-filled based on mode)
    ExchangeField rstField;
    rstField.name = "RST";
    rstField.hint = (m_mode == ModeType::CW) ? "599" : "59";
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

QList<TableColumn> CQWPXContest::getTableColumns() const {
    return {
        TableColumn("Serial", "#", 50, TableColumn::Alignment::Right)
    };
}

QString CQWPXContest::formatSentExchange(int serialNumber, const QString& rst) const {
    // Format: RST + Serial (e.g., "599 001")
    return QString("%1 %2").arg(rst).arg(serialNumber, 3, 10, QChar('0'));
}

bool CQWPXContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.isEmpty()) {
        errorMsg = "Exchange required (Serial or RST + Serial, e.g., '001' or '599 001')";
        return false;
    }

    QString serial;
    if (parts.size() == 1) {
        // Only serial provided - RST will be auto-filled
        serial = parts[0];
    } else if (parts.size() >= 2) {
        // RST + Serial provided - validate RST
        QString rst = parts[0];
        if (rst.length() < 2 || rst.length() > 3) {
            errorMsg = "Invalid RST format";
            return false;
        }
        serial = parts[1];
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

QMap<QString, QString> CQWPXContest::parseReceivedExchange(const QString& exchange) const {
    QMap<QString, QString> result;
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() == 1) {
        // Only serial provided - auto-fill RST based on mode
        result["RST"] = (m_mode == ModeType::CW) ? "599" : "59";
        result["Serial"] = parts[0];
    } else if (parts.size() >= 2) {
        // Full exchange: RST + Serial
        result["RST"] = parts[0];
        result["Serial"] = parts[1];
    }

    return result;
}

int CQWPXContest::calculateQSOPoints(const QSO& qso, const StationInfo& myStation) const {
    // CQWPX Scoring rules (from cqwpx.com official rules):
    //
    // Different Continents:
    //   - 28/21/14 MHz (10m/15m/20m): 3 points
    //   - 7/3.5/1.8 MHz (40m/80m/160m): 6 points
    //
    // Same Continent, Different Countries:
    //   - 28/21/14 MHz: 1 point
    //   - 7/3.5/1.8 MHz: 2 points
    //   - North America exception: doubled (2 and 4 points)
    //
    // Same Country: 1 point (all bands)

    const QString& theirCountry = qso.dxccEntity;
    const QString& theirContinent = qso.continent;

    // Determine if we're on low bands (160m, 80m, 40m) or high bands (20m, 15m, 10m)
    bool isLowBand = (qso.band == BandType::Band160M ||
                      qso.band == BandType::Band80M ||
                      qso.band == BandType::Band40M);

    int points = 0;

    // Same country: 1 point (all bands)
    if (myStation.country == theirCountry) {
        points = 1;
    }
    // Different continent
    else if (myStation.continent != theirContinent) {
        points = isLowBand ? 6 : 3;
    }
    // Same continent, different country
    else {
        points = isLowBand ? 2 : 1;

        // North America exception: double points
        if (myStation.continent == "NA") {
            points *= 2;  // 4 points (low bands) or 2 points (high bands)
        }
    }

    return points;
}

int CQWPXContest::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // CQ WPX: Score = QSO Points × Total Prefixes
    int totalPrefixes = multiplierCounts.value(MultiplierType::Prefix, 0);
    return totalQSOPoints * totalPrefixes;
}

QList<MultiplierDefinition> CQWPXContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    MultiplierDefinition prefixMult;
    prefixMult.type = MultiplierType::Prefix;
    prefixMult.scope = MultiplierScope::AllBands;  // Prefixes count once across all bands
    prefixMult.displayName = "Prefixes";
    mults.append(prefixMult);

    return mults;
}

QString CQWPXContest::getMultiplierValue(
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

QMap<QString, QString> CQWPXContest::getCabrilloHeaders() const {
    QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();
    headers["CONTEST"] = (m_mode == ModeType::CW) ? "CQ-WPX-CW" : "CQ-WPX-SSB";
    return headers;
}

QString CQWPXContest::extractPrefix(const QString& callsign) {
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

// Auto-register with factory
REGISTER_CONTEST(TR4QT::CQWPXContest, "CQWPX");
