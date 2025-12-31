#include "SCPRepository.h"
#include "GlobalDatabase.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>

namespace TR4QT {

SCPRepository::SCPRepository() {
}

QStringList SCPRepository::findMatches(const QString& partial) const {
    LOG_DEBUG("SCPRepository", QString("findMatches called with partial='%1'").arg(partial));

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
    // These are higher priority and returned first
    QString prefixSql = R"(
        SELECT callsign FROM scp_callsigns
        WHERE callsign LIKE ? || '%'
        ORDER BY LENGTH(callsign) ASC
        LIMIT 5
    )";

    LOG_DEBUG("SCPRepository", QString("findMatches: executing prefix query with '%1'").arg(normalized));
    QSqlQuery prefixQuery = db.execute(prefixSql, {normalized});

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
        QString suffixSql = R"(
            SELECT callsign FROM scp_callsigns
            WHERE callsign LIKE '%' || ?
            AND callsign NOT LIKE ? || '%'
            ORDER BY LENGTH(callsign) ASC
            LIMIT ?
        )";

        LOG_DEBUG("SCPRepository", QString("findMatches: executing suffix query, limit=%1").arg(suffixLimit));
        QSqlQuery suffixQuery = db.execute(suffixSql, {normalized, normalized, suffixLimit});

        if (!suffixQuery.isActive()) {
            m_lastError = db.lastError();
            LOG_WARN("SCPRepository", QString("Suffix query failed: %1").arg(m_lastError));
            // Return prefix matches even if suffix query fails
            return allMatches;
        }

        while (suffixQuery.next() && allMatches.size() < 5) {
            QString match = suffixQuery.value(0).toString();
            allMatches << match;
            LOG_DEBUG("SCPRepository", QString("findMatches: suffix match found: %1").arg(match));
        }

        LOG_DEBUG("SCPRepository", QString("findMatches: suffix query added %1 matches").arg(allMatches.size() - (5 - suffixLimit)));
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
    if (!db.beginTransaction()) {
        m_lastError = "Failed to begin transaction";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;
    }

    QString sql = R"(
        INSERT OR IGNORE INTO scp_callsigns
        (callsign, source, contest_id, added_at)
        VALUES (?, ?, ?, ?)
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
            // Check if row was actually inserted (not ignored due to UNIQUE constraint)
            if (query.numRowsAffected() > 0) {
                insertCount++;
            }
        } else {
            m_lastError = query.lastError().text();
            LOG_WARN("SCPRepository", QString("Insert failed for %1: %2").arg(normalized).arg(m_lastError));
        }
    }

    if (!db.commitTransaction()) {
        m_lastError = "Failed to commit transaction";
        LOG_WARN("SCPRepository", m_lastError);
        return 0;
    }

    LOG_DEBUG("SCPRepository", QString("Bulk insert: %1 callsigns inserted from source '%2'")
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
