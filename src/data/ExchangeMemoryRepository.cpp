#include "ExchangeMemoryRepository.h"
#include "Database.h"
#include "../logging/LogMacros.h"
#include <QVariant>
#include <QRegularExpression>
#include <QSqlQuery>

namespace TR4QT {

ExchangeMemoryRepository::ExchangeMemoryRepository() {
}

QString ExchangeMemoryRepository::extractPrefix(const QString& callsign) const {
    if (callsign.length() < 2) {
        return callsign;
    }

    // Extract prefix: W1, K6, G3, etc.
    // Pattern: Letters followed by digit
    QRegularExpression re("^([A-Z]+\\d+)");
    QRegularExpressionMatch match = re.match(callsign.toUpper());

    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Fallback: first 2 characters
    return callsign.left(2).toUpper();
}

bool ExchangeMemoryRepository::save(const ExchangeMemoryEntry& entry) {
    Database& db = Database::instance();

    QString sql = R"(
        INSERT INTO exchange_memory (
            callsign, callsign_prefix, exchange, rst,
            contest_type, mode, timestamp, source, hit_count
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(callsign, contest_type) DO UPDATE SET
            exchange = excluded.exchange,
            rst = excluded.rst,
            mode = excluded.mode,
            timestamp = excluded.timestamp,
            source = excluded.source,
            hit_count = exchange_memory.hit_count + 1
    )";

    QString prefix = extractPrefix(entry.callsign);
    QString modeStr = modeToString(entry.mode);

    QVariantList values{
        entry.callsign.toUpper().trimmed(),
        prefix,
        entry.exchange,
        entry.rst,
        entry.contestType.isEmpty() ? QVariant() : entry.contestType,
        modeStr,
        entry.timestamp.toSecsSinceEpoch(),
        entry.source,
        entry.hitCount
    };

    QSqlQuery query = db.execute(sql, values);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("ExchangeMemoryRepository",
                 QString("Failed to save exchange memory: %1").arg(m_lastError));
        return false;
    }

    LOG_DEBUG("ExchangeMemoryRepository",
              QString("Saved exchange memory: %1 → %2")
              .arg(entry.callsign, entry.exchange));
    return true;
}

ExchangeMemoryEntry ExchangeMemoryRepository::findExact(const QString& callsign,
                                                        const QString& contestType) {
    Database& db = Database::instance();
    ExchangeMemoryEntry entry;

    QString sql;
    QVariantList values;

    if (contestType.isEmpty()) {
        // Find any contest
        sql = R"(
            SELECT callsign, exchange, rst, contest_type, mode, timestamp, source, hit_count
            FROM exchange_memory
            WHERE callsign = ? COLLATE NOCASE
            ORDER BY timestamp DESC
            LIMIT 1
        )";
        values = {callsign.toUpper().trimmed()};
    } else {
        // Find specific contest
        sql = R"(
            SELECT callsign, exchange, rst, contest_type, mode, timestamp, source, hit_count
            FROM exchange_memory
            WHERE callsign = ? COLLATE NOCASE AND contest_type = ?
            LIMIT 1
        )";
        values = {callsign.toUpper().trimmed(), contestType};
    }

    QSqlQuery query = db.execute(sql, values);
    if (!query.next()) {
        return entry;  // Not found
    }

    entry.callsign = query.value(0).toString();
    entry.exchange = query.value(1).toString();
    entry.rst = query.value(2).toString();
    entry.contestType = query.value(3).toString();
    entry.mode = stringToMode(query.value(4).toString());
    entry.timestamp = QDateTime::fromSecsSinceEpoch(query.value(5).toLongLong());
    entry.source = query.value(6).toString();
    entry.hitCount = query.value(7).toInt();

    return entry;
}

QList<ExchangeMemoryEntry> ExchangeMemoryRepository::findByPrefix(const QString& prefix) {
    Database& db = Database::instance();
    QList<ExchangeMemoryEntry> entries;

    QString sql = R"(
        SELECT callsign, exchange, rst, contest_type, mode, timestamp, source, hit_count
        FROM exchange_memory
        WHERE callsign_prefix = ? COLLATE NOCASE
        ORDER BY hit_count DESC, timestamp DESC
        LIMIT 10
    )";

    QVariantList values{prefix.toUpper().trimmed()};
    QSqlQuery query = db.execute(sql, values);

    while (query.next()) {
        ExchangeMemoryEntry entry;
        entry.callsign = query.value(0).toString();
        entry.exchange = query.value(1).toString();
        entry.rst = query.value(2).toString();
        entry.contestType = query.value(3).toString();
        entry.mode = stringToMode(query.value(4).toString());
        entry.timestamp = QDateTime::fromSecsSinceEpoch(query.value(5).toLongLong());
        entry.source = query.value(6).toString();
        entry.hitCount = query.value(7).toInt();
        entries.append(entry);
    }

    return entries;
}

int ExchangeMemoryRepository::deleteOlderThan(int days) {
    Database& db = Database::instance();

    QDateTime cutoff = QDateTime::currentDateTime().addDays(-days);
    QString sql = "DELETE FROM exchange_memory WHERE timestamp < ?";
    QVariantList values{cutoff.toSecsSinceEpoch()};

    QSqlQuery query = db.execute(sql, values);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return -1;
    }

    // SQLite doesn't return rows affected directly via QSqlQuery,
    // so we'll return 0 for success
    return 0;
}

int ExchangeMemoryRepository::count() const {
    Database& db = Database::instance();

    QString sql = "SELECT COUNT(*) FROM exchange_memory";
    QSqlQuery query = db.execute(sql, {});

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

int ExchangeMemoryRepository::getHitRate() const {
    Database& db = Database::instance();

    QString sql = R"(
        SELECT
            SUM(hit_count) as total_hits,
            COUNT(*) as total_entries
        FROM exchange_memory
    )";

    QSqlQuery query = db.execute(sql, {});

    if (!query.next()) {
        return 0;
    }

    int totalHits = query.value(0).toInt();
    int totalEntries = query.value(1).toInt();

    if (totalEntries == 0) {
        return 0;
    }

    // Hit rate: average hits per entry as percentage
    return (totalHits * 100) / totalEntries;
}

} // namespace TR4QT
