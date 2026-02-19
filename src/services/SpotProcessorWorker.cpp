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

#include "SpotProcessorWorker.h"
#include "../logging/LogMacros.h"
#include "../utils/AppSettings.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>

namespace TR4QT {

// DX Cluster band colors (same as DXClusterWindow)
static const QColor COLOR_BAND_160M(102, 51, 153);
static const QColor COLOR_BAND_80M(153, 76, 0);
static const QColor COLOR_BAND_40M(204, 0, 102);
static const QColor COLOR_BAND_20M(0, 102, 204);
static const QColor COLOR_BAND_15M(0, 153, 0);
static const QColor COLOR_BAND_10M(204, 102, 0);
static const QColor COLOR_BAND_DEFAULT(102, 102, 102);

static QColor getBandColor(BandType band) {
    switch (band) {
        case BandType::Band160M: return COLOR_BAND_160M;
        case BandType::Band80M:  return COLOR_BAND_80M;
        case BandType::Band40M:  return COLOR_BAND_40M;
        case BandType::Band20M:  return COLOR_BAND_20M;
        case BandType::Band15M:  return COLOR_BAND_15M;
        case BandType::Band10M:  return COLOR_BAND_10M;
        default:                 return COLOR_BAND_DEFAULT;
    }
}

SpotProcessorWorker::SpotProcessorWorker(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<ProcessedSpot>("ProcessedSpot");
    qRegisterMetaType<TR4QT::ProcessedSpot>("TR4QT::ProcessedSpot");
}

SpotProcessorWorker::~SpotProcessorWorker()
{
    // Close worker-thread database connections
    if (m_contestDb.isOpen()) {
        m_contestDb.close();
    }
    if (m_globalDb.isOpen()) {
        m_globalDb.close();
    }
    // Remove connections (must use static method with connection name)
    QSqlDatabase::removeDatabase(CONTEST_CONN_NAME);
    QSqlDatabase::removeDatabase(GLOBAL_CONN_NAME);
}

void SpotProcessorWorker::initDatabase(const QString& contestDbPath, const QString& globalDbPath)
{
    m_contestDbPath = contestDbPath;
    m_globalDbPath = globalDbPath;

    // Open contest database connection for this thread
    if (!contestDbPath.isEmpty()) {
        m_contestDb = QSqlDatabase::addDatabase("QSQLITE", CONTEST_CONN_NAME);
        m_contestDb.setDatabaseName(contestDbPath);
        if (!m_contestDb.open()) {
            LOG_WARN("SpotProcessorWorker", QString("Failed to open contest DB: %1")
                .arg(m_contestDb.lastError().text()));
        } else {
            // Enable WAL mode for better concurrent read performance
            QSqlQuery query(m_contestDb);
            query.exec("PRAGMA journal_mode=WAL");
            query.exec("PRAGMA read_uncommitted=1");
            LOG_INFO("SpotProcessorWorker", "Contest DB connection opened");
        }
    }

    // Open global database connection for LOTW lookups
    if (!globalDbPath.isEmpty()) {
        m_globalDb = QSqlDatabase::addDatabase("QSQLITE", GLOBAL_CONN_NAME);
        m_globalDb.setDatabaseName(globalDbPath);
        if (!m_globalDb.open()) {
            LOG_WARN("SpotProcessorWorker", QString("Failed to open global DB: %1")
                .arg(m_globalDb.lastError().text()));
        } else {
            QSqlQuery query(m_globalDb);
            query.exec("PRAGMA journal_mode=WAL");
            query.exec("PRAGMA read_uncommitted=1");
            LOG_INFO("SpotProcessorWorker", "Global DB connection opened");
        }
    }

    // Load country file
    QString countryFilePath = AppSettings::instance().getCountryFilePath();
    if (!countryFilePath.isEmpty()) {
        if (!m_countryFile.loadFromFile(countryFilePath)) {
            LOG_WARN("SpotProcessorWorker", QString("Failed to load country file: %1").arg(countryFilePath));
        }
    }
}

void SpotProcessorWorker::setContestContext(int contestDbId,
                                             const QList<MultiplierDefinition>& multDefs)
{
    m_contestDbId = contestDbId;
    m_multDefs = multDefs;
    LOG_INFO("SpotProcessorWorker", QString("Contest context updated: dbId=%1, %2 mult types")
        .arg(contestDbId).arg(multDefs.size()));
}

void SpotProcessorWorker::processSpot(const QString& callsign, double frequency,
                                       const QString& spotter, const QString& comment,
                                       const QString& timestamp)
{
    ProcessedSpot result;
    result.callsign = callsign;
    result.spotFrequency = frequency;

    // --- Build band map Spot ---
    Spot& spot = result.bandMapSpot;
    spot.callsign = callsign;
    spot.frequency = static_cast<freq_t>(frequency);
    spot.timestamp = QDateTime::currentDateTime();
    spot.comment = comment;
    spot.source = QString("DX Cluster (%1)").arg(spotter);

    // Parse split frequency
    double listenFrequency = parseSplitInfo(comment, frequency);
    result.isSplit = (listenFrequency > 0);
    result.listenFrequency = listenFrequency;

    // Also parse QSX for band map spot
    spot.qsx = parseQSX(comment, spot.frequency);
    if (spot.qsx == 0) {
        spot.qsx = parseUP(comment, spot.frequency);
    }

    // Check LOTW status
    spot.isLotwUser = checkLotwUser(callsign);

    // Determine dupe/multiplier color
    QColor callsignColor = getSpotColor(callsign, frequency);

    // Set dupe/mult flags on spot based on color result
    QString dupeColorStr = AppSettings::instance().getClusterDupeColor();
    QString multColorStr = AppSettings::instance().getClusterMultiplierColor();
    if (callsignColor == QColor(dupeColorStr)) {
        spot.isWorked = true;
    } else if (callsignColor == QColor(multColorStr)) {
        spot.isMultiplier = true;
    }

    // --- Build display text ---
    BandType band = frequencyToBand(static_cast<unsigned long>(frequency));
    QColor freqColor = getBandColor(band);

    double freqKHz = frequency / 1000.0;
    QString freqStr = QString::number(freqKHz, 'f', 1);

    const int SPLIT_INDICATOR_WIDTH = 2;
    const int CALLSIGN_INDENT = 3;

    QString splitIcon = result.isSplit ? QString::fromUtf8("● ") : "  ";

    result.displayText = QString("%1%2 %3%4%5 %6Z %7")
        .arg(splitIcon)
        .arg(spotter, -12)
        .arg(freqStr, 10)
        .arg(QString(CALLSIGN_INDENT, ' '))
        .arg(callsign, -12)
        .arg(timestamp, 4)
        .arg(comment);

    // Build format ranges
    int pos = 0;

    // Split indicator
    if (result.isSplit) {
        result.formats.append({pos, SPLIT_INDICATOR_WIDTH, QColor(0, 206, 209), false});
    }
    pos += SPLIT_INDICATOR_WIDTH;

    // Spotter (12 chars) - gray
    result.formats.append({pos, 12, QColor(102, 102, 102), false});
    pos += 12;

    // Space
    pos += 1;

    // Frequency (10 chars, right-aligned)
    const int FREQUENCY_FIELD_WIDTH = 10;
    int freqPadding = FREQUENCY_FIELD_WIDTH - freqStr.length();
    int freqStart = pos + freqPadding;
    result.formats.append({freqStart, static_cast<int>(freqStr.length()), freqColor, false});
    pos += FREQUENCY_FIELD_WIDTH;

    // Indent
    pos += CALLSIGN_INDENT;

    // Callsign (12 chars) - bold, color based on dupe/multiplier
    int callsignPos = pos;
    int callsignLen = callsign.length();
    result.formats.append({callsignPos, callsignLen, callsignColor, true});
    pos += 12;

    // Space
    pos += 1;

    // Timestamp (4 chars + Z) - light gray
    result.formats.append({pos, 5, QColor(153, 153, 153), false});
    pos += 5;

    // Space
    pos += 1;

    // Comment - dark gray
    result.formats.append({pos, static_cast<int>(comment.length()), QColor(51, 51, 51), false});

    m_spotRowCount++;

    emit spotProcessed(result);
}

QColor SpotProcessorWorker::getSpotColor(const QString& callsign, double frequency)
{
    if (m_contestDbId < 0 || !m_contestDb.isOpen()) {
        return Qt::black;
    }

    BandType band = frequencyToBand(static_cast<unsigned long>(frequency));
    ModeType mode = (frequency >= 1800000 && frequency < 10000000) ? ModeType::CW : ModeType::USB;

    QString bandStr = bandToString(band);
    QString modeStr = modeToString(mode);

    // Check dupe
    QSqlQuery dupeQuery(m_contestDb);
    dupeQuery.prepare(R"(
        SELECT COUNT(*)
        FROM qsos
        WHERE contest_id = ?
          AND callsign = ?
          AND band = ?
          AND mode = ?
          AND deleted = 0
    )");
    dupeQuery.addBindValue(m_contestDbId);
    dupeQuery.addBindValue(callsign);
    dupeQuery.addBindValue(bandStr);
    dupeQuery.addBindValue(modeStr);

    if (dupeQuery.exec() && dupeQuery.next() && dupeQuery.value(0).toInt() > 0) {
        QString dupeColorStr = AppSettings::instance().getClusterDupeColor();
        return QColor(dupeColorStr);
    }

    // Check multipliers
    if (m_multDefs.isEmpty()) {
        return Qt::black;
    }

    // Build temp QSO for mult checking
    QSO tempQso;
    tempQso.callsign = callsign;
    tempQso.band = band;
    tempQso.mode = mode;
    tempQso.frequency = frequency;

    // Populate country/zone from CountryFile
    CountryData countryData = m_countryFile.lookup(callsign);
    if (countryData.isValid()) {
        tempQso.dxccPrefix = countryData.primaryPrefix;
        tempQso.dxccEntity = countryData.name;
        tempQso.continent = continentToString(countryData.continent);
        tempQso.cqZone = countryData.cqZone;
        tempQso.ituZone = countryData.ituZone;
    }

    for (const MultiplierDefinition& multDef : m_multDefs) {
        QString bandParam = (multDef.scope == MultiplierScope::PerBand)
                            ? bandStr : QString();

        // Get worked multipliers from DB
        QSqlQuery multQuery(m_contestDb);
        QString multTypeStr;
        switch (multDef.type) {
            case MultiplierType::Country: multTypeStr = "Country"; break;
            case MultiplierType::CQZone:  multTypeStr = "CQZone"; break;
            case MultiplierType::ITUZone: multTypeStr = "ITUZone"; break;
            case MultiplierType::State:   multTypeStr = "State"; break;
            case MultiplierType::Section: multTypeStr = "Section"; break;
            case MultiplierType::Prefix:  multTypeStr = "Prefix"; break;
            case MultiplierType::Grid:    multTypeStr = "Grid"; break;
            case MultiplierType::County:  multTypeStr = "County"; break;
            case MultiplierType::Custom:  multTypeStr = "Custom"; break;
        }

        QString sql = "SELECT mult_value FROM multipliers WHERE contest_id = ? AND mult_type = ?";
        QVariantList params;
        params << m_contestDbId << multTypeStr;
        if (!bandParam.isEmpty()) {
            sql += " AND band = ?";
            params << bandParam;
        }

        multQuery.prepare(sql);
        for (int i = 0; i < params.size(); ++i) {
            multQuery.addBindValue(params[i]);
        }

        QStringList workedMults;
        if (multQuery.exec()) {
            while (multQuery.next()) {
                workedMults.append(multQuery.value(0).toString());
            }
        }

        // Check if this spot is a new multiplier
        QString multValue = extractMultiplierValue(tempQso, multDef.type);
        if (!multValue.isEmpty() && !workedMults.contains(multValue)) {
            QString multColorStr = AppSettings::instance().getClusterMultiplierColor();
            return QColor(multColorStr);
        }
    }

    return Qt::black;
}

QString SpotProcessorWorker::extractMultiplierValue(const QSO& qso, MultiplierType type)
{
    switch (type) {
        case MultiplierType::Country:
            return qso.dxccPrefix;
        case MultiplierType::CQZone:
            return (qso.cqZone > 0) ? QString::number(qso.cqZone) : QString();
        case MultiplierType::ITUZone:
            return (qso.ituZone > 0) ? QString::number(qso.ituZone) : QString();
        default:
            // State/Section/Prefix/Grid/County/Custom require exchange data
            // which spots don't have, so we can't determine these
            return {};
    }
}

double SpotProcessorWorker::parseSplitInfo(const QString& comment, double spotFrequency)
{
    QString upperComment = comment.toUpper();

    // QSX (absolute frequency)
    static QRegularExpression qsxRegex(R"(QSX\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch qsxMatch = qsxRegex.match(upperComment);
    if (qsxMatch.hasMatch()) {
        double listenKHz = qsxMatch.captured(1).toDouble();
        return listenKHz * 1000.0;
    }

    // UP (relative offset, positive)
    static QRegularExpression upRegex(R"(\bUP\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch upMatch = upRegex.match(upperComment);
    if (upMatch.hasMatch()) {
        double offsetKHz = upMatch.captured(1).toDouble();
        return spotFrequency + (offsetKHz * 1000.0);
    }

    // DOWN/DN (relative offset, negative)
    static QRegularExpression downRegex(R"(\b(?:DOWN|DN)\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch downMatch = downRegex.match(upperComment);
    if (downMatch.hasMatch()) {
        double offsetKHz = downMatch.captured(1).toDouble();
        return spotFrequency - (offsetKHz * 1000.0);
    }

    return 0;
}

freq_t SpotProcessorWorker::parseQSX(const QString& comment, freq_t spotFrequency)
{
    static QRegularExpression qsxRegex(R"(\bQSX\s+(\d+(?:\.\d+)?)\b)",
                                        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = qsxRegex.match(comment);
    if (!match.hasMatch()) return 0;

    double qsxValue = match.captured(1).toDouble();
    if (qsxValue < 1000) {
        freq_t spotMHz = (spotFrequency / 1000000) * 1000000;
        return spotMHz + static_cast<freq_t>(qsxValue * 1000);
    } else {
        return static_cast<freq_t>(qsxValue * 1000000);
    }
}

freq_t SpotProcessorWorker::parseUP(const QString& comment, freq_t spotFrequency)
{
    static QRegularExpression upRegex(R"(\bUP\s+(\d+(?:\.\d+)?)\b)",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = upRegex.match(comment);
    if (!match.hasMatch()) return 0;

    double offsetKHz = match.captured(1).toDouble();
    return spotFrequency + static_cast<freq_t>(offsetKHz * 1000);
}

bool SpotProcessorWorker::checkLotwUser(const QString& callsign)
{
    AppSettings& settings = AppSettings::instance();
    if (!settings.getEnableLotwLookup()) {
        return false;
    }

    if (!m_globalDb.isOpen()) {
        return false;
    }

    // Direct SQL query instead of LOTWUserRepository (which uses GlobalDatabase singleton)
    QSqlQuery query(m_globalDb);
    query.prepare("SELECT COUNT(*) FROM lotw_users WHERE callsign = ?");
    query.addBindValue(callsign.toUpper());

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

} // namespace TR4QT
