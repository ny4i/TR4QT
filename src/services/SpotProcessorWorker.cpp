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
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>

namespace TR4QT {

// Helper: open a SQLite DB with WAL mode for read-heavy worker thread access
static QSqlDatabase openWorkerDb(const QString& path, const QString& connName,
                                  const QString& label)
{
    if (path.isEmpty()) {
        return {};
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(path);
    if (!db.open()) {
        LOG_WARN("SpotProcessorWorker", QString("Failed to open %1 DB: %2")
            .arg(label, db.lastError().text()));
        return {};
    }

    QSqlQuery query(db);
    query.exec("PRAGMA journal_mode=WAL");
    query.exec("PRAGMA read_uncommitted=1");
    LOG_INFO("SpotProcessorWorker", QString("%1 DB connection opened").arg(label));
    return db;
}

SpotProcessorWorker::SpotProcessorWorker(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<ProcessedSpot>("ProcessedSpot");
    qRegisterMetaType<TR4QT::ProcessedSpot>("TR4QT::ProcessedSpot");
    qRegisterMetaType<SpotProcessorConfig>("SpotProcessorConfig");
    qRegisterMetaType<TR4QT::SpotProcessorConfig>("TR4QT::SpotProcessorConfig");
}

SpotProcessorWorker::~SpotProcessorWorker()
{
    if (m_contestDb.isOpen()) {
        m_contestDb.close();
    }
    if (m_globalDb.isOpen()) {
        m_globalDb.close();
    }
    QSqlDatabase::removeDatabase(CONTEST_CONN_NAME);
    QSqlDatabase::removeDatabase(GLOBAL_CONN_NAME);
}

void SpotProcessorWorker::initDatabase(const QString& contestDbPath, const QString& globalDbPath,
                                        const QString& countryFilePath)
{
    m_contestDbPath = contestDbPath;
    m_globalDbPath = globalDbPath;

    m_contestDb = openWorkerDb(contestDbPath, CONTEST_CONN_NAME, "Contest");
    m_globalDb = openWorkerDb(globalDbPath, GLOBAL_CONN_NAME, "Global");

    // Load country file (worker needs its own instance for thread safety)
    // Path is captured from the main thread to avoid accessing AppSettings here
    if (!countryFilePath.isEmpty()) {
        if (m_countryFile.loadFromFile(countryFilePath)) {
            LOG_INFO("SpotProcessorWorker", QString("Country file loaded: %1").arg(countryFilePath));
        } else {
            LOG_WARN("SpotProcessorWorker", QString("Failed to load country file: %1").arg(countryFilePath));
        }
    }
}

void SpotProcessorWorker::setConfig(const SpotProcessorConfig& config)
{
    m_config = config;
    LOG_DEBUG("SpotProcessorWorker", "Configuration updated");
}

void SpotProcessorWorker::setContestContext(int contestDbId,
                                             const QList<MultiplierDefinition>& multDefs)
{
    m_contestDbId = contestDbId;
    m_multDefs = multDefs;

    // Rebuild in-memory caches from database
    rebuildDupeCache();
    rebuildMultiplierCache();

    LOG_INFO("SpotProcessorWorker", QString("Contest context updated: dbId=%1, %2 mult types, %3 dupes cached")
        .arg(contestDbId).arg(multDefs.size()).arg(m_dupeCache.size()));
}

void SpotProcessorWorker::addWorkedCallsign(const QString& callsign, const QString& band, const QString& mode)
{
    QString key = callsign.toUpper() + "|" + band + "|" + mode;
    m_dupeCache.insert(key);
}

void SpotProcessorWorker::addWorkedMultiplier(const QString& multType, const QString& multValue, const QString& band)
{
    m_multiplierCache[multType][band].insert(multValue);
}

void SpotProcessorWorker::rebuildDupeCache()
{
    m_dupeCache.clear();

    if (m_contestDbId < 0 || !m_contestDb.isOpen()) {
        return;
    }

    QSqlQuery query(m_contestDb);
    query.prepare("SELECT callsign, band, mode FROM qsos WHERE contest_id = ? AND deleted = 0");
    query.addBindValue(m_contestDbId);

    if (query.exec()) {
        while (query.next()) {
            QString key = query.value(0).toString().toUpper() + "|"
                        + query.value(1).toString() + "|"
                        + query.value(2).toString();
            m_dupeCache.insert(key);
        }
    } else {
        LOG_WARN("SpotProcessorWorker", QString("Failed to build dupe cache: %1")
            .arg(query.lastError().text()));
    }

    LOG_DEBUG("SpotProcessorWorker", QString("Dupe cache rebuilt: %1 entries").arg(m_dupeCache.size()));
}

void SpotProcessorWorker::rebuildMultiplierCache()
{
    m_multiplierCache.clear();

    if (m_contestDbId < 0 || !m_contestDb.isOpen()) {
        return;
    }

    QSqlQuery query(m_contestDb);
    query.prepare("SELECT mult_type, mult_value, band FROM multipliers WHERE contest_id = ?");
    query.addBindValue(m_contestDbId);

    int count = 0;
    if (query.exec()) {
        while (query.next()) {
            QString multType = query.value(0).toString();
            QString multValue = query.value(1).toString();
            QString band = query.value(2).toString();  // Empty string for AllBands
            m_multiplierCache[multType][band].insert(multValue);
            count++;
        }
    } else {
        LOG_WARN("SpotProcessorWorker", QString("Failed to build multiplier cache: %1")
            .arg(query.lastError().text()));
    }

    LOG_DEBUG("SpotProcessorWorker", QString("Multiplier cache rebuilt: %1 entries").arg(count));
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

    // Parse split frequency (single pass for both display and band map)
    double listenFrequency = parseSplitFrequency(comment, frequency);
    result.isSplit = (listenFrequency > 0);
    result.listenFrequency = listenFrequency;

    // Set QSX on band map spot
    if (listenFrequency > 0) {
        spot.qsx = static_cast<freq_t>(listenFrequency);
    }

    // Check LOTW status
    spot.isLotwUser = checkLotwUser(callsign);

    // Determine dupe/multiplier color (uses in-memory cache, no SQL)
    QColor callsignColor = getSpotColor(callsign, frequency);

    // Set dupe/mult flags on spot based on color result
    if (callsignColor == m_config.dupeColor) {
        spot.isWorked = true;
    } else if (callsignColor == m_config.multiplierColor) {
        spot.isMultiplier = true;
    }

    // --- Build display text ---
    BandType band = frequencyToBand(static_cast<unsigned long>(frequency));
    QColor freqColor = getBandColor(band);

    double freqKHz = frequency / 1000.0;
    QString freqStr = QString::number(freqKHz, 'f', 1);

    QString splitIcon = result.isSplit ? QString::fromUtf8("\u25CF ") : "  ";

    result.displayText = QString("%1%2 %3%4%5 %6Z %7")
        .arg(splitIcon)
        .arg(spotter, -SPOTTER_FIELD_WIDTH)
        .arg(freqStr, FREQUENCY_FIELD_WIDTH)
        .arg(QString(CALLSIGN_INDENT, ' '))
        .arg(callsign, -CALLSIGN_FIELD_WIDTH)
        .arg(timestamp, 4)
        .arg(comment);

    // Build format ranges
    int pos = 0;

    // Split indicator
    if (result.isSplit) {
        result.formats.append({pos, SPLIT_INDICATOR_WIDTH, m_config.splitIndicatorColor, false});
    }
    pos += SPLIT_INDICATOR_WIDTH;

    // Spotter
    result.formats.append({pos, SPOTTER_FIELD_WIDTH, m_config.spotterColor, false});
    pos += SPOTTER_FIELD_WIDTH;

    // Space
    pos += 1;

    // Frequency (right-aligned within field)
    int freqPadding = FREQUENCY_FIELD_WIDTH - freqStr.length();
    int freqStart = pos + freqPadding;
    result.formats.append({freqStart, static_cast<int>(freqStr.length()), freqColor, false});
    pos += FREQUENCY_FIELD_WIDTH;

    // Indent
    pos += CALLSIGN_INDENT;

    // Callsign (bold, color based on dupe/multiplier)
    result.formats.append({pos, static_cast<int>(callsign.length()), callsignColor, true});
    pos += CALLSIGN_FIELD_WIDTH;

    // Space
    pos += 1;

    // Timestamp
    result.formats.append({pos, TIMESTAMP_FIELD_WIDTH, m_config.timestampColor, false});
    pos += TIMESTAMP_FIELD_WIDTH;

    // Space
    pos += 1;

    // Comment
    result.formats.append({pos, static_cast<int>(comment.length()), m_config.commentColor, false});

    m_spotRowCount++;

    emit spotProcessed(result);
}

QColor SpotProcessorWorker::getSpotColor(const QString& callsign, double frequency)
{
    if (m_contestDbId < 0) {
        return m_config.defaultCallColor;
    }

    BandType band = frequencyToBand(static_cast<unsigned long>(frequency));
    QString bandStr = bandToString(band);
    QString callUpper = callsign.toUpper();

    // Spot lines don't carry mode. Check all common modes to avoid false non-dupes.
    // A station worked on CW should show as dupe even when the spot doesn't specify mode.
    QString dupeKeyCW  = callUpper + "|" + bandStr + "|CW";
    QString dupeKeySSB = callUpper + "|" + bandStr + "|USB";
    QString dupeKeyLSB = callUpper + "|" + bandStr + "|LSB";
    QString dupeKeyFM  = callUpper + "|" + bandStr + "|FM";
    if (m_dupeCache.contains(dupeKeyCW)  ||
        m_dupeCache.contains(dupeKeySSB) ||
        m_dupeCache.contains(dupeKeyLSB) ||
        m_dupeCache.contains(dupeKeyFM)) {
        return m_config.dupeColor;
    }

    // Check multipliers via in-memory cache (no SQL)
    if (m_multDefs.isEmpty()) {
        return m_config.defaultCallColor;
    }

    // Build temp QSO for mult value extraction
    QSO tempQso;
    tempQso.callsign = callsign;
    tempQso.band = band;
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
        QString multTypeStr = multiplierTypeToString(multDef.type);

        // Look up in cache
        QString bandKey = (multDef.scope == MultiplierScope::PerBand) ? bandStr : QString();

        const auto& typeCache = m_multiplierCache.value(multTypeStr);
        const auto& bandCache = typeCache.value(bandKey);

        // Check if this spot would be a new multiplier
        QString multValue = extractMultiplierValue(tempQso, multDef.type);
        if (!multValue.isEmpty() && !bandCache.contains(multValue)) {
            return m_config.multiplierColor;
        }
    }

    return m_config.defaultCallColor;
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

double SpotProcessorWorker::parseSplitFrequency(const QString& comment, double spotFrequencyHz)
{
    QString upperComment = comment.toUpper();

    // QSX (absolute frequency in kHz)
    static QRegularExpression qsxRegex(R"(\bQSX\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch qsxMatch = qsxRegex.match(upperComment);
    if (qsxMatch.hasMatch()) {
        double qsxValue = qsxMatch.captured(1).toDouble();
        // Heuristic: if value < 1000, it's kHz relative to current MHz
        // (e.g., QSX 045 on 7 MHz -> 7045 kHz)
        if (qsxValue < 1000) {
            freq_t spotMHz = (static_cast<freq_t>(spotFrequencyHz) / 1000000) * 1000000;
            return static_cast<double>(spotMHz) + (qsxValue * 1000.0);
        }
        return qsxValue * 1000.0;
    }

    // UP (relative offset, positive)
    static QRegularExpression upRegex(R"(\bUP\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch upMatch = upRegex.match(upperComment);
    if (upMatch.hasMatch()) {
        double offsetKHz = upMatch.captured(1).toDouble();
        return spotFrequencyHz + (offsetKHz * 1000.0);
    }

    // DOWN/DN (relative offset, negative)
    static QRegularExpression downRegex(R"(\b(?:DOWN|DN)\s*(\d+(?:\.\d+)?))");
    QRegularExpressionMatch downMatch = downRegex.match(upperComment);
    if (downMatch.hasMatch()) {
        double offsetKHz = downMatch.captured(1).toDouble();
        return spotFrequencyHz - (offsetKHz * 1000.0);
    }

    return 0;
}

QColor SpotProcessorWorker::getBandColor(BandType band) const
{
    switch (band) {
        case BandType::Band160M: return m_config.band160mColor;
        case BandType::Band80M:  return m_config.band80mColor;
        case BandType::Band40M:  return m_config.band40mColor;
        case BandType::Band20M:  return m_config.band20mColor;
        case BandType::Band15M:  return m_config.band15mColor;
        case BandType::Band10M:  return m_config.band10mColor;
        default:                 return m_config.bandDefaultColor;
    }
}

bool SpotProcessorWorker::checkLotwUser(const QString& callsign)
{
    if (!m_config.lotwLookupEnabled || !m_globalDb.isOpen()) {
        return false;
    }

    QSqlQuery query(m_globalDb);
    query.prepare("SELECT COUNT(*) FROM lotw_users WHERE callsign = ?");
    query.addBindValue(callsign.toUpper());

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

} // namespace TR4QT
