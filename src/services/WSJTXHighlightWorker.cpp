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

#include "WSJTXHighlightWorker.h"
#include "WSJTXService.h"
#include "../logging/LogMacros.h"

#include <QSqlQuery>
#include <QSqlError>

static const char* LOG_TAG = "WSJTXHighlightWorker";

namespace TR4QT {

WSJTXHighlightWorker::WSJTXHighlightWorker(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<WSJTXHighlightDecision>("WSJTXHighlightDecision");
}

WSJTXHighlightWorker::~WSJTXHighlightWorker()
{
    if (m_contestDb.isOpen()) {
        m_contestDb.close();
    }
    QSqlDatabase::removeDatabase(DB_CONN_NAME);
}

void WSJTXHighlightWorker::initDatabase(const QString& contestDbPath)
{
    m_contestDbPath = contestDbPath;

    if (contestDbPath.isEmpty())
        return;

    m_contestDb = QSqlDatabase::addDatabase("QSQLITE", DB_CONN_NAME);
    m_contestDb.setDatabaseName(contestDbPath);

    if (!m_contestDb.open()) {
        LOG_ERROR(LOG_TAG, QString("Failed to open contest DB: %1")
                  .arg(m_contestDb.lastError().text()));
    }
}

void WSJTXHighlightWorker::setContestContext(int contestDbId,
                                               const QList<MultiplierDefinition>& multDefs,
                                               DuplicateCheckingRule dupeRule)
{
    m_contestDbId = contestDbId;
    m_multDefs = multDefs;
    m_dupeRule = dupeRule;

    rebuildDupeCache();
    rebuildMultiplierCache();
}

void WSJTXHighlightWorker::setColors(const QColor& dupeBg, const QColor& dupeFg,
                                       const QColor& multBg, const QColor& multFg)
{
    m_dupeBg = dupeBg;
    m_dupeFg = dupeFg;
    m_multBg = multBg;
    m_multFg = multFg;
}

void WSJTXHighlightWorker::addWorkedCallsign(const QString& callsign,
                                                const QString& band,
                                                const QString& mode)
{
    QString key = QString("%1|%2|%3").arg(callsign.toUpper(), band, mode);
    m_dupeCache.insert(key);
}

void WSJTXHighlightWorker::addWorkedMultiplier(const QString& multType,
                                                  const QString& multValue,
                                                  const QString& band)
{
    m_multiplierCache[multType][band].insert(multValue);
}

void WSJTXHighlightWorker::checkCallsign(const QString& callsign,
                                            quint64 frequencyHz,
                                            const QString& wsjtxMode)
{
    WSJTXHighlightDecision decision;
    decision.callsign = callsign;

    if (m_contestDbId < 0) {
        // No active contest — no highlighting
        emit highlightDecision(decision);
        return;
    }

    // Map WSJT-X mode to TR4QT types
    auto modeMap = WSJTXService::mapMode(wsjtxMode);
    if (modeMap.rejected) {
        emit highlightDecision(decision);
        return;
    }

    BandType band = frequencyToBand(static_cast<unsigned long>(frequencyHz));

    // Check dupe
    if (isDupe(callsign, band, modeMap.modeType)) {
        decision.isDupe = true;
        decision.bgColor = m_dupeBg;
        decision.fgColor = m_dupeFg;
    }
    // Check multiplier (only if not dupe — dupe takes priority in highlighting)
    else if (isNewMultiplier(callsign, band)) {
        decision.isMultiplier = true;
        decision.bgColor = m_multBg;
        decision.fgColor = m_multFg;
    }

    emit highlightDecision(decision);
}

// ─── Private helpers ────────────────────────────────────────────────────────

QString WSJTXHighlightWorker::dupeKey(const QString& callsign, BandType band, ModeType mode) const
{
    switch (m_dupeRule) {
    case DuplicateCheckingRule::PerBandMode:
        return QString("%1|%2|%3").arg(callsign.toUpper(),
                                        bandToString(band),
                                        modeToString(mode));
    case DuplicateCheckingRule::PerBand:
        return QString("%1|%2|*").arg(callsign.toUpper(),
                                       bandToString(band));
    case DuplicateCheckingRule::AllBandMode:
        return QString("%1|*|%2").arg(callsign.toUpper(),
                                       modeToString(mode));
    case DuplicateCheckingRule::AllBand:
        return QString("%1|*|*").arg(callsign.toUpper());
    }
    return QString();
}

bool WSJTXHighlightWorker::isDupe(const QString& callsign, BandType band, ModeType mode) const
{
    return m_dupeCache.contains(dupeKey(callsign, band, mode));
}

bool WSJTXHighlightWorker::isNewMultiplier(const QString& callsign, BandType band) const
{
    // Simple callsign-prefix-based multiplier check (WPX style)
    // Full DXCC/zone multiplier checking would require country lookup,
    // which is deferred to QSO log time via QSOPersistenceService.
    Q_UNUSED(callsign)
    Q_UNUSED(band)

    // For now, return false — multiplier highlighting will be refined
    // when we integrate with ContestBase's multiplier checking
    return false;
}

void WSJTXHighlightWorker::rebuildDupeCache()
{
    m_dupeCache.clear();

    if (!m_contestDb.isOpen() || m_contestDbId < 0)
        return;

    QSqlQuery query(m_contestDb);
    query.prepare("SELECT callsign, band, mode FROM qsos "
                  "WHERE contest_id = :contestId AND deleted = 0");
    query.bindValue(":contestId", m_contestDbId);

    if (!query.exec()) {
        LOG_ERROR(LOG_TAG, QString("Failed to rebuild dupe cache: %1")
                  .arg(query.lastError().text()));
        return;
    }

    int count = 0;
    while (query.next()) {
        QString call = query.value(0).toString().toUpper();
        QString band = query.value(1).toString();
        QString mode = query.value(2).toString();

        // Build key based on dupe rule
        QString key;
        switch (m_dupeRule) {
        case DuplicateCheckingRule::PerBandMode:
            key = QString("%1|%2|%3").arg(call, band, mode);
            break;
        case DuplicateCheckingRule::PerBand:
            key = QString("%1|%2|*").arg(call, band);
            break;
        case DuplicateCheckingRule::AllBandMode:
            key = QString("%1|*|%2").arg(call, mode);
            break;
        case DuplicateCheckingRule::AllBand:
            key = QString("%1|*|*").arg(call);
            break;
        }

        m_dupeCache.insert(key);
        ++count;
    }

    LOG_INFO(LOG_TAG, QString("Dupe cache rebuilt: %1 entries from contest %2")
             .arg(count).arg(m_contestDbId));
}

void WSJTXHighlightWorker::rebuildMultiplierCache()
{
    m_multiplierCache.clear();

    if (!m_contestDb.isOpen() || m_contestDbId < 0)
        return;

    // Load multiplier values from QSO records
    // Each QSO has a `multipliers` field (comma-separated list of mult values)
    QSqlQuery query(m_contestDb);
    query.prepare("SELECT band, multipliers FROM qsos "
                  "WHERE contest_id = :contestId AND deleted = 0 "
                  "AND is_multiplier = 1");
    query.bindValue(":contestId", m_contestDbId);

    if (!query.exec()) {
        LOG_ERROR(LOG_TAG, QString("Failed to rebuild multiplier cache: %1")
                  .arg(query.lastError().text()));
        return;
    }

    int count = 0;
    while (query.next()) {
        QString band = query.value(0).toString();
        QString mults = query.value(1).toString();

        for (const QString& mult : mults.split(',', Qt::SkipEmptyParts)) {
            QString trimmed = mult.trimmed();
            if (!trimmed.isEmpty()) {
                // Store with a generic "mult" type key
                // More specific type tracking can be added later
                m_multiplierCache["mult"][band].insert(trimmed);
                ++count;
            }
        }
    }

    LOG_INFO(LOG_TAG, QString("Multiplier cache rebuilt: %1 entries from contest %2")
             .arg(count).arg(m_contestDbId));
}

} // namespace TR4QT
