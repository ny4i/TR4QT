/**
 * ContestRepository - Database access layer for contests table
 */

#include "ContestRepository.h"
#include "Database.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace TR4QT {

ContestRecord ContestRepository::findById(int contestId) {
    m_lastError.clear();

    Database& db = Database::instance();
    if (!db.isOpen()) {
        m_lastError = "Database not open";
        LOG_WARN("ContestRepository", m_lastError);
        return ContestRecord();
    }

    QSqlQuery query = db.execute(
        "SELECT id, contest_id, contest_name, start_time, contest_type, "
        "my_call, my_grid, my_continent, my_cq_zone, my_itu_zone, "
        "current_serial, exchange_sent, created_at "
        "FROM contests WHERE id = ?",
        {contestId}
    );

    if (query.lastError().isValid()) {
        m_lastError = query.lastError().text();
        LOG_WARN("ContestRepository", QString("Failed to find contest %1: %2")
            .arg(contestId).arg(m_lastError));
        return ContestRecord();
    }

    if (!query.next()) {
        m_lastError = QString("Contest with id %1 not found").arg(contestId);
        LOG_DEBUG("ContestRepository", m_lastError);
        return ContestRecord();
    }

    ContestRecord record;
    record.id = query.value(0).toInt();
    record.contestId = query.value(1).toString();
    record.contestName = query.value(2).toString();
    record.startTime = QDateTime::fromSecsSinceEpoch(query.value(3).toLongLong());
    record.contestType = query.value(4).toString();
    record.myCall = query.value(5).toString();
    record.myGrid = query.value(6).toString();
    record.myContinent = query.value(7).toString();
    record.myCqZone = query.value(8).toInt();
    record.myItuZone = query.value(9).toInt();
    record.currentSerial = query.value(10).toInt();
    record.exchangeSent = query.value(11).toString();
    record.createdAt = QDateTime::fromSecsSinceEpoch(query.value(12).toLongLong());

    return record;
}

ContestRecord ContestRepository::findFirst() {
    m_lastError.clear();

    Database& db = Database::instance();
    if (!db.isOpen()) {
        m_lastError = "Database not open";
        LOG_WARN("ContestRepository", m_lastError);
        return ContestRecord();
    }

    QSqlQuery query = db.execute(
        "SELECT id, contest_id, contest_name, start_time, contest_type, "
        "my_call, my_grid, my_continent, my_cq_zone, my_itu_zone, "
        "current_serial, exchange_sent, created_at "
        "FROM contests LIMIT 1",
        {}
    );

    if (query.lastError().isValid()) {
        m_lastError = query.lastError().text();
        LOG_WARN("ContestRepository", QString("Failed to query contests: %1").arg(m_lastError));
        return ContestRecord();
    }

    if (!query.next()) {
        m_lastError = "No contest found in database";
        LOG_DEBUG("ContestRepository", m_lastError);
        return ContestRecord();
    }

    ContestRecord record;
    record.id = query.value(0).toInt();
    record.contestId = query.value(1).toString();
    record.contestName = query.value(2).toString();
    record.startTime = QDateTime::fromSecsSinceEpoch(query.value(3).toLongLong());
    record.contestType = query.value(4).toString();
    record.myCall = query.value(5).toString();
    record.myGrid = query.value(6).toString();
    record.myContinent = query.value(7).toString();
    record.myCqZone = query.value(8).toInt();
    record.myItuZone = query.value(9).toInt();
    record.currentSerial = query.value(10).toInt();
    record.exchangeSent = query.value(11).toString();
    record.createdAt = QDateTime::fromSecsSinceEpoch(query.value(12).toLongLong());

    return record;
}

QList<ContestRecord> ContestRepository::findAll(const QString& databasePath) {
    m_lastError.clear();
    QList<ContestRecord> records;

    // Open the specified database file using singleton
    Database& db = Database::instance();
    if (!db.open(databasePath)) {
        m_lastError = QString("Failed to open database: %1").arg(databasePath);
        LOG_WARN("ContestRepository", m_lastError);
        return records;
    }

    QSqlQuery query = db.execute(
        "SELECT id, contest_id, contest_name, start_time, contest_type, "
        "my_call, my_grid, my_continent, my_cq_zone, my_itu_zone, "
        "current_serial, exchange_sent, created_at "
        "FROM contests",
        {}
    );

    if (query.lastError().isValid()) {
        m_lastError = query.lastError().text();
        LOG_WARN("ContestRepository", QString("Failed to query contests from %1: %2")
            .arg(databasePath).arg(m_lastError));
        db.close();
        return records;
    }

    while (query.next()) {
        ContestRecord record;
        record.id = query.value(0).toInt();
        record.contestId = query.value(1).toString();
        record.contestName = query.value(2).toString();
        record.startTime = QDateTime::fromSecsSinceEpoch(query.value(3).toLongLong());
        record.contestType = query.value(4).toString();
        record.myCall = query.value(5).toString();
        record.myGrid = query.value(6).toString();
        record.myContinent = query.value(7).toString();
        record.myCqZone = query.value(8).toInt();
        record.myItuZone = query.value(9).toInt();
        record.currentSerial = query.value(10).toInt();
        record.exchangeSent = query.value(11).toString();
        record.createdAt = QDateTime::fromSecsSinceEpoch(query.value(12).toLongLong());
        records.append(record);
    }

    db.close();
    LOG_DEBUG("ContestRepository", QString("Found %1 contest(s) in %2").arg(records.size()).arg(databasePath));
    return records;
}

bool ContestRepository::updateExchange(int contestId, const QString& exchange) {
    m_lastError.clear();

    Database& db = Database::instance();
    if (!db.isOpen()) {
        m_lastError = "Database not open";
        LOG_WARN("ContestRepository", m_lastError);
        return false;
    }

    QSqlQuery query = db.execute(
        "UPDATE contests SET exchange_sent = ? WHERE id = ?",
        {exchange, contestId}
    );

    if (query.lastError().isValid()) {
        m_lastError = query.lastError().text();
        LOG_WARN("ContestRepository", QString("Failed to update exchange for contest %1: %2")
            .arg(contestId).arg(m_lastError));
        return false;
    }

    LOG_DEBUG("ContestRepository", QString("Updated exchange for contest %1: \"%2\"")
        .arg(contestId).arg(exchange));
    return true;
}

} // namespace TR4QT
