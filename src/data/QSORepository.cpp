#include "QSORepository.h"
#include "Database.h"
#include "../core/Types.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

namespace TR4QT {

QSORepository::QSORepository() {
}

// ===== QSO CRUD Operations =====

bool QSORepository::saveQSO(QSO& qso, int contestId) {
    Database& db = Database::instance();

    if (!db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    // Begin transaction for atomic save
    if (!db.beginTransaction()) {
        m_lastError = "Failed to begin transaction: " + db.lastError();
        LOG_ERROR("QSORepository", QString("Failed to begin transaction for QSO save: %1").arg(db.lastError()));
        return false;
    }

    // Generate GUID if not already set
    if (qso.guid.isEmpty()) {
        qso.guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Convert enums to strings
    QString modeStr = modeToString(qso.mode);
    QString bandStr = bandToString(qso.band);

    // Convert multipliers list to JSON
    QJsonArray multipliersJson;
    for (const QString& mult : qso.multipliers) {
        multipliersJson.append(mult);
    }
    QString multipliersStr = QJsonDocument(multipliersJson).toJson(QJsonDocument::Compact);

    QString sql = R"(
        INSERT INTO qsos (
            contest_id, guid, timestamp, callsign, frequency, mode, band,
            rst_sent, rst_received, exchange_sent, exchange_received,
            dxcc_entity, dxcc_prefix, dxcc_entity_code, cq_zone, itu_zone, continent, state, county, arrl_section, grid_square, iota_reference, contest_class,
            qso_points, is_dupe, is_multiplier, multipliers,
            serial_number, operator_call, notes
        ) VALUES (
            ?, ?, ?, ?, ?, ?, ?,
            ?, ?, ?, ?,
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
            ?, ?, ?, ?,
            ?, ?, ?
        )
    )";

    QVariantList values{
        contestId,
        qso.guid,
        qso.timestamp.toSecsSinceEpoch(),
        qso.callsign,
        static_cast<qint64>(qso.frequency),
        modeStr,
        bandStr,
        qso.rstSent,
        qso.rstReceived,
        qso.exchangeSent,
        qso.exchangeReceived,
        qso.dxccEntity,
        qso.dxccPrefix,
        qso.dxccEntityCode,
        qso.cqZone,
        qso.ituZone,
        qso.continent,
        qso.state,
        qso.county,
        qso.arrlSection,
        qso.gridSquare,
        qso.iotaReference,
        qso.contestClass,
        qso.qsoPoints,
        qso.isDupe,
        qso.isMultiplier,
        multipliersStr,
        qso.serialNumber > 0 ? qso.serialNumber : QVariant(),
        qso.operatorCall,
        qso.notes
    };

    QSqlQuery query = db.execute(sql, values);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        db.rollbackTransaction();
        LOG_ERROR("QSORepository", QString("Failed to insert QSO, rolled back: %1").arg(db.lastError()));
        return false;
    }

    // Set ID on the QSO object
    qso.id = db.lastInsertId();

    // Tier 1 Integrity Check: Verify the QSO was actually saved
    QSO verification = findById(qso.id);
    if (verification.id < 0 || verification.callsign != qso.callsign) {
        m_lastError = "Save verification failed - QSO not found in database after insert";
        LOG_ERROR("QSORepository", QString("INTEGRITY ERROR: Failed to verify saved QSO id=%1 callsign=%2")
            .arg(qso.id).arg(qso.callsign));
        db.rollbackTransaction();
        return false;
    }

    // Commit transaction - QSO successfully saved and verified
    if (!db.commitTransaction()) {
        m_lastError = "Failed to commit transaction: " + db.lastError();
        LOG_ERROR("QSORepository", QString("Failed to commit QSO transaction: %1").arg(db.lastError()));
        db.rollbackTransaction();
        return false;
    }

    return true;
}

bool QSORepository::updateQSO(const QSO& qso) {
    if (qso.id < 0) {
        m_lastError = "Cannot update QSO without valid ID";
        return false;
    }

    Database& db = Database::instance();

    // Begin transaction for atomic update
    if (!db.beginTransaction()) {
        m_lastError = "Failed to begin transaction: " + db.lastError();
        LOG_ERROR("QSORepository", QString("Failed to begin transaction for QSO update: %1").arg(db.lastError()));
        return false;
    }

    QString modeStr = modeToString(qso.mode);
    QString bandStr = bandToString(qso.band);

    QJsonArray multipliersJson;
    for (const QString& mult : qso.multipliers) {
        multipliersJson.append(mult);
    }
    QString multipliersStr = QJsonDocument(multipliersJson).toJson(QJsonDocument::Compact);

    QString sql = R"(
        UPDATE qsos SET
            timestamp = ?, callsign = ?, frequency = ?, mode = ?, band = ?,
            rst_sent = ?, rst_received = ?, exchange_sent = ?, exchange_received = ?,
            dxcc_entity = ?, dxcc_prefix = ?, dxcc_entity_code = ?, cq_zone = ?, itu_zone = ?, continent = ?, state = ?, county = ?, arrl_section = ?, grid_square = ?, iota_reference = ?, contest_class = ?,
            qso_points = ?, is_dupe = ?, is_multiplier = ?, multipliers = ?,
            serial_number = ?, operator_call = ?, notes = ?
        WHERE id = ?
    )";

    QVariantList values{
        qso.timestamp.toSecsSinceEpoch(),
        qso.callsign,
        static_cast<qint64>(qso.frequency),
        modeStr,
        bandStr,
        qso.rstSent,
        qso.rstReceived,
        qso.exchangeSent,
        qso.exchangeReceived,
        qso.dxccEntity,
        qso.dxccPrefix,
        qso.dxccEntityCode,
        qso.cqZone,
        qso.ituZone,
        qso.continent,
        qso.state,
        qso.county,
        qso.arrlSection,
        qso.gridSquare,
        qso.iotaReference,
        qso.contestClass,
        qso.qsoPoints,
        qso.isDupe,
        qso.isMultiplier,
        multipliersStr,
        qso.serialNumber > 0 ? qso.serialNumber : QVariant(),
        qso.operatorCall,
        qso.notes,
        qso.id
    };

    QSqlQuery query = db.execute(sql, values);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        db.rollbackTransaction();
        LOG_ERROR("QSORepository", QString("Failed to update QSO, rolled back: %1").arg(db.lastError()));
        return false;
    }

    // Tier 1 Integrity Check: Verify the update was persisted
    QSO verification = findById(qso.id);
    if (verification.id < 0) {
        m_lastError = "Update verification failed - QSO not found after update";
        LOG_ERROR("QSORepository", QString("INTEGRITY ERROR: QSO id=%1 disappeared after update")
            .arg(qso.id));
        db.rollbackTransaction();
        return false;
    }
    // Quick check: verify a key field was updated
    if (verification.callsign != qso.callsign || verification.qsoPoints != qso.qsoPoints) {
        LOG_WARN("QSORepository", QString("Update verification: field mismatch for QSO id=%1")
            .arg(qso.id));
    }

    // Commit transaction - QSO successfully updated and verified
    if (!db.commitTransaction()) {
        m_lastError = "Failed to commit transaction: " + db.lastError();
        LOG_ERROR("QSORepository", QString("Failed to commit QSO update transaction: %1").arg(db.lastError()));
        db.rollbackTransaction();
        return false;
    }

    return true;
}

bool QSORepository::deleteQSO(int qsoId, bool hardDelete) {
    Database& db = Database::instance();

    QString sql;
    if (hardDelete) {
        sql = "DELETE FROM qsos WHERE id = ?";
    } else {
        sql = "UPDATE qsos SET deleted = 1 WHERE id = ?";
    }

    QSqlQuery query = db.execute(sql, {qsoId});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return false;
    }

    // Tier 1 Integrity Check: Verify the deletion
    if (hardDelete) {
        QSO verification = findById(qsoId);
        if (verification.id >= 0) {
            m_lastError = "Delete verification failed - QSO still exists after hard delete";
            LOG_ERROR("QSORepository", QString("INTEGRITY ERROR: QSO id=%1 still exists after hard delete")
                .arg(qsoId));
            return false;
        }
    } else {
        // Soft delete - verify deleted flag is set
        QSO verification = findById(qsoId);
        if (verification.id >= 0 && !verification.deleted) {
            m_lastError = "Delete verification failed - deleted flag not set";
            LOG_ERROR("QSORepository", QString("INTEGRITY ERROR: QSO id=%1 not marked as deleted")
                .arg(qsoId));
            return false;
        }
    }

    return true;
}

bool QSORepository::deleteAllQSOs(int contestId) {
    Database& db = Database::instance();

    // Get initial count for verification
    int initialCount = getQSOCount(contestId, true);

    // Begin transaction
    if (!db.beginTransaction()) {
        m_lastError = "Failed to begin transaction for deleting all QSOs";
        LOG_ERROR("QSORepository", m_lastError);
        return false;
    }

    // Delete all multipliers for this contest
    QString deleteMults = "DELETE FROM multipliers WHERE contest_id = ?";
    QSqlQuery multsQuery = db.execute(deleteMults, {contestId});
    if (!multsQuery.isActive()) {
        m_lastError = db.lastError();
        LOG_ERROR("QSORepository", QString("Failed to delete multipliers: %1").arg(m_lastError));
        db.rollbackTransaction();
        return false;
    }

    // Delete all QSOs for this contest
    QString deleteQSOs = "DELETE FROM qsos WHERE contest_id = ?";
    QSqlQuery qsosQuery = db.execute(deleteQSOs, {contestId});
    if (!qsosQuery.isActive()) {
        m_lastError = db.lastError();
        LOG_ERROR("QSORepository", QString("Failed to delete QSOs: %1").arg(m_lastError));
        db.rollbackTransaction();
        return false;
    }

    // Commit transaction
    if (!db.commitTransaction()) {
        m_lastError = "Failed to commit transaction for deleting all QSOs";
        LOG_ERROR("QSORepository", m_lastError);
        return false;
    }

    // Tier 1 Integrity Check: Verify all QSOs are deleted
    int finalCount = getQSOCount(contestId, true);
    if (finalCount != 0) {
        m_lastError = QString("Delete verification failed - %1 QSOs still exist after deleteAllQSOs")
            .arg(finalCount);
        LOG_ERROR("QSORepository", QString("INTEGRITY ERROR: Contest id=%1 has %2 QSOs after deleteAllQSOs (started with %3)")
            .arg(contestId).arg(finalCount).arg(initialCount));
        return false;
    }

    LOG_INFO("QSORepository", QString("Deleted all %1 QSOs and multipliers for contest %2")
        .arg(initialCount).arg(contestId));
    return true;
}

QSO QSORepository::findById(int qsoId) const {
    Database& db = Database::instance();

    QString sql = "SELECT * FROM qsos WHERE id = ?";
    QSqlQuery query = db.execute(sql, {qsoId});

    if (query.next()) {
        return qsoFromQuery(query);
    }

    return QSO();  // Return invalid QSO (id = -1)
}

QList<QSO> QSORepository::findByContest(int contestId, bool includeDeleted) const {
    Database& db = Database::instance();

    QString sql = "SELECT * FROM qsos WHERE contest_id = ?";
    if (!includeDeleted) {
        sql += " AND deleted = 0";
    }
    sql += " ORDER BY timestamp ASC";

    QSqlQuery query = db.execute(sql, {contestId});

    QList<QSO> qsos;
    while (query.next()) {
        qsos.append(qsoFromQuery(query));
    }

    return qsos;
}

int QSORepository::getQSOCount(int contestId, bool includeDeleted) const {
    Database& db = Database::instance();

    QString sql = "SELECT COUNT(*) FROM qsos WHERE contest_id = ?";
    if (!includeDeleted) {
        sql += " AND deleted = 0";
    }

    QSqlQuery query = db.execute(sql, {contestId});

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

int QSORepository::getTotalPoints(int contestId) const {
    Database& db = Database::instance();

    QString sql = R"(
        SELECT SUM(qso_points)
        FROM qsos
        WHERE contest_id = ? AND deleted = 0 AND is_dupe = 0
    )";

    QSqlQuery query = db.execute(sql, {contestId});

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

// ===== Dupe Checking =====

bool QSORepository::isDuplicate(const QString& callsign, BandType band, ModeType mode, int contestId) const {
    Database& db = Database::instance();

    QString bandStr = bandToString(band);
    QString modeStr = modeToString(mode);

    QString sql = R"(
        SELECT COUNT(*)
        FROM qsos
        WHERE contest_id = ?
          AND callsign = ?
          AND band = ?
          AND mode = ?
          AND deleted = 0
    )";

    QSqlQuery query = db.execute(sql, {contestId, callsign, bandStr, modeStr});

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

QList<QSO> QSORepository::findByCallsign(const QString& callsign, int contestId) const {
    Database& db = Database::instance();

    QString sql = R"(
        SELECT * FROM qsos
        WHERE contest_id = ? AND callsign = ? AND deleted = 0
        ORDER BY timestamp ASC
    )";

    QSqlQuery query = db.execute(sql, {contestId, callsign});

    QList<QSO> qsos;
    while (query.next()) {
        qsos.append(qsoFromQuery(query));
    }

    return qsos;
}

// ===== Multiplier Tracking =====

bool QSORepository::isNewMultiplier(
    MultiplierType multType,
    const QString& multValue,
    const QString& band,
    int contestId) const
{
    Database& db = Database::instance();

    QString multTypeStr = multiplierTypeToString(multType);

    QString sql = R"(
        SELECT COUNT(*)
        FROM multipliers
        WHERE contest_id = ?
          AND mult_type = ?
          AND mult_value = ?
    )";

    QVariantList values{contestId, multTypeStr, multValue};

    // If band is specified (per-band mult), include it in the query
    if (!band.isEmpty()) {
        sql += " AND band = ?";
        values.append(band);
    } else {
        sql += " AND band IS NULL";
    }

    QSqlQuery query = db.execute(sql, values);

    if (query.next()) {
        return query.value(0).toInt() == 0;  // New if count is 0
    }

    return true;  // Assume new if query fails
}

bool QSORepository::saveMultiplier(
    MultiplierType multType,
    const QString& multValue,
    const QString& band,
    int contestId,
    int firstQsoId)
{
    Database& db = Database::instance();

    QString multTypeStr = multiplierTypeToString(multType);

    QString sql = R"(
        INSERT INTO multipliers (contest_id, mult_type, mult_value, band, first_qso_id, qso_count)
        VALUES (?, ?, ?, ?, ?, 1)
        ON CONFLICT(contest_id, mult_type, mult_value, band)
        DO UPDATE SET qso_count = qso_count + 1
    )";

    QVariantList values{
        contestId,
        multTypeStr,
        multValue,
        band.isEmpty() ? QVariant() : band,
        firstQsoId
    };

    QSqlQuery query = db.execute(sql, values);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return false;
    }

    return true;
}

QStringList QSORepository::getWorkedMultipliers(
    MultiplierType multType,
    const QString& band,
    int contestId) const
{
    Database& db = Database::instance();

    QString multTypeStr = multiplierTypeToString(multType);

    QString sql = R"(
        SELECT mult_value
        FROM multipliers
        WHERE contest_id = ? AND mult_type = ?
    )";

    QVariantList values{contestId, multTypeStr};

    if (!band.isEmpty()) {
        sql += " AND band = ?";
        values.append(band);
    }

    sql += " ORDER BY mult_value ASC";

    QSqlQuery query = db.execute(sql, values);

    QStringList mults;
    while (query.next()) {
        mults.append(query.value(0).toString());
    }

    return mults;
}

int QSORepository::getMultiplierCount(MultiplierType multType, int contestId) const {
    Database& db = Database::instance();

    QString multTypeStr = multiplierTypeToString(multType);

    QString sql = R"(
        SELECT COUNT(*)
        FROM multipliers
        WHERE contest_id = ? AND mult_type = ?
    )";

    QSqlQuery query = db.execute(sql, {contestId, multTypeStr});

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

QMap<MultiplierType, int> QSORepository::getAllMultiplierCounts(int contestId) const {
    Database& db = Database::instance();

    QString sql = R"(
        SELECT mult_type, COUNT(*)
        FROM multipliers
        WHERE contest_id = ?
        GROUP BY mult_type
    )";

    QSqlQuery query = db.execute(sql, {contestId});

    QMap<MultiplierType, int> counts;
    while (query.next()) {
        QString typeStr = query.value(0).toString();
        int count = query.value(1).toInt();
        MultiplierType type = stringToMultiplierType(typeStr);
        counts[type] = count;
    }

    return counts;
}

// ===== Utility Methods =====

QSO QSORepository::qsoFromQuery(const QSqlQuery& query) const {
    QSO qso;

    qso.id = query.value("id").toInt();
    qso.guid = query.value("guid").toString();
    qso.timestamp = QDateTime::fromSecsSinceEpoch(query.value("timestamp").toLongLong());
    qso.callsign = query.value("callsign").toString();
    qso.frequency = query.value("frequency").toLongLong();
    qso.mode = stringToMode(query.value("mode").toString());
    qso.band = stringToBand(query.value("band").toString());

    qso.rstSent = query.value("rst_sent").toString();
    qso.rstReceived = query.value("rst_received").toString();
    qso.exchangeSent = query.value("exchange_sent").toString();
    qso.exchangeReceived = query.value("exchange_received").toString();

    qso.dxccEntity = query.value("dxcc_entity").toString();
    qso.dxccPrefix = query.value("dxcc_prefix").toString();
    qso.dxccEntityCode = query.value("dxcc_entity_code").toInt();
    qso.cqZone = query.value("cq_zone").toInt();
    qso.ituZone = query.value("itu_zone").toInt();
    qso.continent = query.value("continent").toString();
    qso.state = query.value("state").toString();
    qso.county = query.value("county").toString();
    qso.arrlSection = query.value("arrl_section").toString();
    qso.gridSquare = query.value("grid_square").toString();
    qso.iotaReference = query.value("iota_reference").toString();
    qso.contestClass = query.value("contest_class").toString();

    qso.qsoPoints = query.value("qso_points").toInt();
    qso.isDupe = query.value("is_dupe").toBool();
    qso.isMultiplier = query.value("is_multiplier").toBool();

    // Parse multipliers JSON
    QString multipliersStr = query.value("multipliers").toString();
    if (!multipliersStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(multipliersStr.toUtf8());
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            for (const QJsonValue& val : arr) {
                qso.multipliers.append(val.toString());
            }
        }
    }

    qso.serialNumber = query.value("serial_number").toInt();
    qso.operatorCall = query.value("operator_call").toString();
    qso.deleted = query.value("deleted").toBool();
    qso.notes = query.value("notes").toString();

    return qso;
}

QString QSORepository::multiplierTypeToString(MultiplierType type) const {
    switch (type) {
        case MultiplierType::Country: return "Country";
        case MultiplierType::CQZone: return "CQZone";
        case MultiplierType::ITUZone: return "ITUZone";
        case MultiplierType::State: return "State";
        case MultiplierType::Section: return "Section";
        case MultiplierType::Prefix: return "Prefix";
        case MultiplierType::Grid: return "Grid";
        case MultiplierType::Custom: return "Custom";
    }
    return "Unknown";
}

MultiplierType QSORepository::stringToMultiplierType(const QString& str) const {
    if (str == "Country") return MultiplierType::Country;
    if (str == "CQZone") return MultiplierType::CQZone;
    if (str == "ITUZone") return MultiplierType::ITUZone;
    if (str == "State") return MultiplierType::State;
    if (str == "Section") return MultiplierType::Section;
    if (str == "Prefix") return MultiplierType::Prefix;
    if (str == "Grid") return MultiplierType::Grid;
    if (str == "Custom") return MultiplierType::Custom;

    return MultiplierType::Country;  // Default fallback
}

} // namespace TR4QT
