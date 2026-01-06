#include "SpotRepository.h"
#include "GlobalDatabase.h"
#include "DatabaseTransaction.h"
#include "../ui/widgets/BandMapWidget.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QVariant>

namespace TR4QT {

SpotRepository::SpotRepository() {
}

QList<Spot> SpotRepository::loadAllSpots() const {
    GlobalDatabase& db = GlobalDatabase::instance();

    if (!db.isOpen()) {
        m_lastError = "Global database not open";
        LOG_WARN("SpotRepository", m_lastError);
        return QList<Spot>();
    }

    QString sql = "SELECT * FROM dx_spots ORDER BY timestamp DESC";

    QSqlQuery query = db.execute(sql, {});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SpotRepository", QString("Failed to load spots: %1").arg(m_lastError));
        return QList<Spot>();
    }

    QList<Spot> spots;
    while (query.next()) {
        spots.append(spotFromQuery(query));
    }

    LOG_INFO("SpotRepository", QString("Loaded %1 spots from database").arg(spots.size()));

    return spots;
}

bool SpotRepository::saveAllSpots(const QList<Spot>& spots) {
    GlobalDatabase& db = GlobalDatabase::instance();

    if (!db.isOpen()) {
        m_lastError = "Global database not open";
        LOG_WARN("SpotRepository", m_lastError);
        return false;
    }

    // Start transaction for speed
    DatabaseTransaction txn(db);
    if (!txn.begin()) {
        m_lastError = "Failed to start transaction";
        LOG_WARN("SpotRepository", m_lastError);
        return false;
    }

    // Clear existing spots (full replacement)
    QSqlQuery clearQuery = db.execute("DELETE FROM dx_spots", {});
    if (!clearQuery.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SpotRepository", QString("Failed to clear spots: %1").arg(m_lastError));
        return false;  // Auto-rollback via RAII destructor
    }

    // Bulk insert all spots
    QString sql = R"(
        INSERT INTO dx_spots (
            callsign, frequency, qsx, timestamp, comment, source,
            is_multiplier, is_worked, is_lotw_user, azimuth, distance
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    for (const Spot& spot : spots) {
        QVariantList values{
            spot.callsign.toUpper().trimmed(),
            static_cast<qint64>(spot.frequency),
            static_cast<qint64>(spot.qsx),
            spot.timestamp.toSecsSinceEpoch(),
            spot.comment,
            spot.source,
            spot.isMultiplier,
            spot.isWorked,
            spot.isLotwUser,
            spot.azimuth,
            spot.distance
        };

        QSqlQuery insertQuery = db.execute(sql, values);
        if (!insertQuery.isActive()) {
            m_lastError = db.lastError();
            LOG_WARN("SpotRepository", QString("Failed to insert spot %1: %2").arg(spot.callsign).arg(m_lastError));
            return false;  // Auto-rollback via RAII destructor
        }
    }

    // Commit transaction
    if (!txn.commit()) {
        m_lastError = "Failed to commit transaction";
        LOG_WARN("SpotRepository", m_lastError);
        return false;  // Auto-rollback already done by commit()
    }

    LOG_INFO("SpotRepository", QString("Saved %1 spots to database").arg(spots.size()));
    return true;
}

bool SpotRepository::clearAll() {
    GlobalDatabase& db = GlobalDatabase::instance();

    if (!db.isOpen()) {
        m_lastError = "Global database not open";
        LOG_WARN("SpotRepository", m_lastError);
        return false;
    }

    QSqlQuery query = db.execute("DELETE FROM dx_spots", {});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("SpotRepository", QString("Failed to clear spots: %1").arg(m_lastError));
        return false;
    }

    LOG_INFO("SpotRepository", "Cleared all spots from database");
    return true;
}

int SpotRepository::getSpotCount() const {
    GlobalDatabase& db = GlobalDatabase::instance();

    if (!db.isOpen()) {
        m_lastError = "Global database not open";
        return 0;
    }

    QString sql = "SELECT COUNT(*) FROM dx_spots";
    QSqlQuery query = db.execute(sql, {});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return 0;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

Spot SpotRepository::spotFromQuery(const QSqlQuery& query) const {
    Spot spot;

    spot.callsign = query.value("callsign").toString();
    spot.frequency = query.value("frequency").toLongLong();
    spot.qsx = query.value("qsx").toLongLong();
    spot.timestamp = QDateTime::fromSecsSinceEpoch(query.value("timestamp").toLongLong());
    spot.comment = query.value("comment").toString();
    spot.source = query.value("source").toString();
    spot.isMultiplier = query.value("is_multiplier").toBool();
    spot.isWorked = query.value("is_worked").toBool();
    spot.isLotwUser = query.value("is_lotw_user").toBool();
    spot.azimuth = query.value("azimuth").toDouble();
    spot.distance = query.value("distance").toDouble();

    return spot;
}

} // namespace TR4QT
