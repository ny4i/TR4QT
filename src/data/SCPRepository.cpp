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

#include "SCPRepository.h"
#include "GlobalDatabase.h"
#include "DatabaseTransaction.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>

namespace TR4QT {

SCPRepository::SCPRepository() {
}

QStringList SCPRepository::findMatches(const QString& partial, const QString& contestDbPath) const {
    LOG_DEBUG("SCPRepository", QString("findMatches called with partial='%1', contestDbPath='%2'")
        .arg(partial).arg(contestDbPath));

    if (partial.length() < 2) {
        LOG_DEBUG("SCPRepository", "findMatches: partial too short, returning empty");
        return QStringList();
    }

    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("SCPRepository", m_lastError);
        return QStringList();
    }

    QString normalized = partial.toUpper().trimmed();
    LOG_DEBUG("SCPRepository", QString("findMatches: normalized='%1'").arg(normalized));

    QStringList allMatches;

    // Query 1: Prefix matches (callsign starts with partial)
    QString prefixSql;
    QVariantList prefixParams;

    if (!contestDbPath.isEmpty()) {
        // Contest mode: UNION worked calls from contest DB + MASTER.SCP from global DB
        // Prioritize worked calls first, then MASTER.SCP
        prefixSql = R"(
            SELECT callsign, priority FROM (
                SELECT DISTINCT UPPER(callsign) as callsign, 0 as priority
                FROM contest.qsos
                WHERE UPPER(callsign) LIKE ? || '%'
                UNION ALL
                SELECT callsign, 1 as priority
                FROM scp_callsigns
                WHERE callsign LIKE ? || '%'
                AND source = 'master_scp'
            )
            ORDER BY priority ASC, LENGTH(callsign) ASC
            LIMIT 5
        )";
        prefixParams = {normalized, normalized};
    } else {
        // Non-contest mode: Just query global MASTER.SCP
        prefixSql = R"(
            SELECT callsign FROM scp_callsigns
            WHERE callsign LIKE ? || '%'
            AND source = 'master_scp'
            ORDER BY LENGTH(callsign) ASC
            LIMIT 5
        )";
        prefixParams = {normalized};
    }

    // Attach contest database if provided
    if (!contestDbPath.isEmpty()) {
        QString attachSql = QString("ATTACH DATABASE '%1' AS contest").arg(contestDbPath);
        QSqlQuery attachQuery = db.execute(attachSql);
        if (!attachQuery.isActive()) {
            m_lastError = QString("Failed to attach contest database: %1").arg(db.lastError());
            LOG_WARN("SCPRepository", m_lastError);
            return QStringList();
        }
        LOG_DEBUG("SCPRepository", QString("Attached contest database: %1").arg(contestDbPath));
    }

    LOG_DEBUG("SCPRepository", QString("findMatches: executing prefix query"));
    QSqlQuery prefixQuery = db.execute(prefixSql, prefixParams);

    if (!prefixQuery.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SCPRepository", QString("Prefix query failed: %1").arg(m_lastError));
        return QStringList();
    }

    while (prefixQuery.next()) {
        QString match = prefixQuery.value(0).toString();
        allMatches << match;
        LOG_DEBUG("SCPRepository", QString("findMatches: prefix match found: %1").arg(match));
    }

    LOG_DEBUG("SCPRepository", QString("findMatches: prefix query returned %1 matches").arg(allMatches.size()));

    // Query 2: Suffix matches (callsign ends with partial)
    // Fill remaining slots up to 5 total
    int suffixLimit = 5 - allMatches.size();
    if (suffixLimit > 0) {
        QString suffixSql;
        QVariantList suffixParams;

        if (!contestDbPath.isEmpty()) {
            // Contest mode: UNION worked calls + MASTER.SCP
            suffixSql = R"(
                SELECT callsign, priority FROM (
                    SELECT DISTINCT UPPER(callsign) as callsign, 0 as priority
                    FROM contest.qsos
                    WHERE UPPER(callsign) LIKE '%' || ?
                    AND UPPER(callsign) NOT LIKE ? || '%'
                    UNION ALL
                    SELECT callsign, 1 as priority
                    FROM scp_callsigns
                    WHERE callsign LIKE '%' || ?
                    AND callsign NOT LIKE ? || '%'
                    AND source = 'master_scp'
                )
                ORDER BY priority ASC, LENGTH(callsign) ASC
                LIMIT ?
            )";
            suffixParams = {normalized, normalized, normalized, normalized, suffixLimit};
        } else {
            // Non-contest mode: Just query global MASTER.SCP
            suffixSql = R"(
                SELECT callsign FROM scp_callsigns
                WHERE callsign LIKE '%' || ?
                AND callsign NOT LIKE ? || '%'
                AND source = 'master_scp'
                ORDER BY LENGTH(callsign) ASC
                LIMIT ?
            )";
            suffixParams = {normalized, normalized, suffixLimit};
        }

        LOG_DEBUG("SCPRepository", QString("findMatches: executing suffix query, limit=%1").arg(suffixLimit));
        QSqlQuery suffixQuery = db.execute(suffixSql, suffixParams);

        if (!suffixQuery.isActive()) {
            m_lastError = db.lastError();
            LOG_WARN("SCPRepository", QString("Suffix query failed: %1").arg(m_lastError));
            // Detach before returning
            if (!contestDbPath.isEmpty()) {
                db.execute("DETACH DATABASE contest");
            }
            return allMatches;
        }

        while (suffixQuery.next() && allMatches.size() < 5) {
            QString match = suffixQuery.value(0).toString();
            allMatches << match;
            LOG_DEBUG("SCPRepository", QString("findMatches: suffix match found: %1").arg(match));
        }

        LOG_DEBUG("SCPRepository", QString("findMatches: suffix query added %1 matches").arg(allMatches.size() - (5 - suffixLimit)));
    }

    // Detach contest database
    if (!contestDbPath.isEmpty()) {
        QSqlQuery detachQuery = db.execute("DETACH DATABASE contest");
        if (detachQuery.isActive()) {
            LOG_DEBUG("SCPRepository", "Detached contest database");
        }
    }

    LOG_DEBUG("SCPRepository", QString("findMatches: returning %1 total matches: %2")
        .arg(allMatches.size()).arg(allMatches.join(", ")));
    return allMatches;
}

int SCPRepository::bulkInsert(const QStringList& callsigns,
                               const QString& source,
                               const QString& contestId) {
    if (callsigns.isEmpty()) {
        return 0;
    }

    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;
    }

    // Start transaction for performance
    DatabaseTransaction txn(db);
    if (!txn.begin()) {
        m_lastError = "Failed to begin transaction";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;
    }

    // Use UPSERT to update source when callsign already exists (e.g., from MASTER.SCP)
    // This ensures worked calls get prioritized in SCP queries
    QString sql = R"(
        INSERT INTO scp_callsigns (callsign, source, contest_id, added_at)
        VALUES (?, ?, ?, ?)
        ON CONFLICT(callsign) DO UPDATE SET
            source = excluded.source,
            contest_id = excluded.contest_id
    )";

    QSqlQuery query(db.connection());
    query.prepare(sql);

    qint64 now = QDateTime::currentSecsSinceEpoch();
    int insertCount = 0;

    for (const QString& callsign : callsigns) {
        QString normalized = callsign.toUpper().trimmed();
        if (normalized.isEmpty()) {
            continue;
        }

        query.addBindValue(normalized);
        query.addBindValue(source);
        query.addBindValue(contestId.isEmpty() ? QVariant() : contestId);
        query.addBindValue(now);

        if (query.exec()) {
            // Count inserts and updates (UPSERT affects rows for both operations)
            if (query.numRowsAffected() > 0) {
                insertCount++;
            }
        } else {
            m_lastError = query.lastError().text();
            LOG_WARN("SCPRepository", QString("Insert failed for %1: %2").arg(normalized).arg(m_lastError));
        }
    }

    if (!txn.commit()) {
        m_lastError = "Failed to commit transaction";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;  // Auto-rollback already done by commit()
    }

    LOG_DEBUG("SCPRepository", QString("Bulk insert: %1 callsigns inserted/updated from source '%2'")
        .arg(insertCount).arg(source));

    return insertCount;
}

int SCPRepository::clearBySource(const QString& source) {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;
    }

    QString sql = "DELETE FROM scp_callsigns WHERE source = ?";
    QSqlQuery query = db.execute(sql, {source});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SCPRepository", QString("Clear failed for source '%1': %2").arg(source).arg(m_lastError));
        return 0;
    }

    int deletedCount = query.numRowsAffected();
    LOG_DEBUG("SCPRepository", QString("Cleared %1 callsigns from source '%2'")
        .arg(deletedCount).arg(source));

    return deletedCount;
}

int SCPRepository::clearByContestId(const QString& contestId) {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;
    }

    QString sql = "DELETE FROM scp_callsigns WHERE contest_id = ?";
    QSqlQuery query = db.execute(sql, {contestId});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SCPRepository", QString("Clear failed for contest_id '%1': %2").arg(contestId).arg(m_lastError));
        return 0;
    }

    int deletedCount = query.numRowsAffected();
    LOG_DEBUG("SCPRepository", QString("Cleared %1 callsigns from contest_id '%2'")
        .arg(deletedCount).arg(contestId));

    return deletedCount;
}

bool SCPRepository::clearAll() {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("SCPRepository", m_lastError);
        return false;
    }

    QString sql = "DELETE FROM scp_callsigns";
    QSqlQuery query = db.execute(sql);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SCPRepository", QString("Clear all failed: %1").arg(m_lastError));
        return false;
    }

    int deletedCount = query.numRowsAffected();
    LOG_DEBUG("SCPRepository", QString("Cleared all %1 callsigns").arg(deletedCount));

    return true;
}

int SCPRepository::getCallsignCount() const {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        return 0;
    }

    QString sql = "SELECT COUNT(*) FROM scp_callsigns";
    QSqlQuery query = db.execute(sql);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return 0;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

int SCPRepository::getCallsignCountBySource(const QString& source) const {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        return 0;
    }

    QString sql = "SELECT COUNT(*) FROM scp_callsigns WHERE source = ?";
    QSqlQuery query = db.execute(sql, {source});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return 0;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

bool SCPRepository::setMetadata(const QString& key, const QString& value) {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("SCPRepository", m_lastError);
        return false;
    }

    qint64 now = QDateTime::currentSecsSinceEpoch();

    QString sql = R"(
        INSERT OR REPLACE INTO scp_metadata (key, value, updated_at)
        VALUES (?, ?, ?)
    )";

    QSqlQuery query = db.execute(sql, {key, value, now});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SCPRepository", QString("Set metadata failed for key '%1': %2").arg(key).arg(m_lastError));
        return false;
    }

    return true;
}

QString SCPRepository::getMetadata(const QString& key) const {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        return QString();
    }

    QString sql = "SELECT value FROM scp_metadata WHERE key = ?";
    QSqlQuery query = db.execute(sql, {key});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return QString();
    }

    if (query.next()) {
        return query.value(0).toString();
    }

    return QString();
}

QDateTime SCPRepository::getMetadataTimestamp(const QString& key) const {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        return QDateTime();
    }

    QString sql = "SELECT updated_at FROM scp_metadata WHERE key = ?";
    QSqlQuery query = db.execute(sql, {key});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return QDateTime();
    }

    if (query.next()) {
        qint64 timestamp = query.value(0).toLongLong();
        return QDateTime::fromSecsSinceEpoch(timestamp);
    }

    return QDateTime();
}

} // namespace TR4QT
