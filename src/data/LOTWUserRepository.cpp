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

#include "LOTWUserRepository.h"
#include "GlobalDatabase.h"
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDate>

namespace TR4QT {

LOTWUserRepository::LOTWUserRepository() {
}

bool LOTWUserRepository::isLotwUser(const QString& callsign) const {
    if (callsign.isEmpty()) {
        return false;
    }

    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return false;
    }

    // Get minimum upload months setting
    AppSettings& settings = AppSettings::instance();
    int minMonths = settings.getLotwMinUploadMonths();

    // Calculate cutoff date (current date minus X months)
    QDate cutoffDate = QDate::currentDate().addMonths(-minMonths);
    QString cutoffDateStr = cutoffDate.toString("yyyy-MM-dd");

    // Log the first lookup to show the date constraint being applied
    static bool firstLookup = true;
    if (firstLookup) {
        LOG_DEBUG("LOTWUserRepository", QString("LOTW user lookup: considering users active if uploaded within %1 months (since %2)")
            .arg(minMonths).arg(cutoffDateStr));
        firstLookup = false;
    }

    // Fast lookup using indexed query with date constraint
    // UPPER() ensures case-insensitive search
    // Date comparison: last_upload_date >= cutoff date
    QString sql = "SELECT COUNT(*) FROM lotw_users "
                  "WHERE callsign = UPPER(?) "
                  "AND last_upload_date >= ? "
                  "LIMIT 1";
    QSqlQuery query = db.execute(sql, {callsign.trimmed(), cutoffDateStr});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return false;
    }

    if (query.next()) {
        int count = query.value(0).toInt();
        return count > 0;
    }

    return false;
}

LOTWUser LOTWUserRepository::findByCallsign(const QString& callsign) const {
    LOTWUser user;

    if (callsign.isEmpty()) {
        m_lastError = "Callsign is empty";
        return user;
    }

    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return user;
    }

    QString sql = "SELECT id, callsign, last_upload_date, last_upload_time, last_updated "
                  "FROM lotw_users WHERE callsign = UPPER(?)";
    QSqlQuery query = db.execute(sql, {callsign.trimmed()});

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return user;
    }

    if (query.next()) {
        user.id = query.value(0).toInt();
        user.callsign = query.value(1).toString();
        user.lastUploadDate = query.value(2).toString();
        user.lastUploadTime = query.value(3).toString();
        user.lastUpdated = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
    } else {
        m_lastError = QString("LOTW user not found: %1").arg(callsign);
    }

    return user;
}

int LOTWUserRepository::getUserCount() const {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return 0;
    }

    QString sql = "SELECT COUNT(*) FROM lotw_users";
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

QDateTime LOTWUserRepository::getLastUpdateTime() const {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return QDateTime();
    }

    QString sql = "SELECT MAX(last_updated) FROM lotw_users";
    QSqlQuery query = db.execute(sql);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        return QDateTime();
    }

    if (query.next()) {
        qint64 timestamp = query.value(0).toLongLong();
        if (timestamp > 0) {
            return QDateTime::fromSecsSinceEpoch(timestamp);
        }
    }

    return QDateTime();
}

bool LOTWUserRepository::clearAll() {
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return false;
    }

    LOG_DEBUG("LOTWUserRepository", "Clearing all LOTW users...");

    QString sql = "DELETE FROM lotw_users";
    QSqlQuery query = db.execute(sql);

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("LOTWUserRepository", QString("Failed to clear LOTW users: %1").arg(m_lastError));
        return false;
    }

    LOG_DEBUG("LOTWUserRepository", "LOTW users cleared successfully");
    return true;
}

bool LOTWUserRepository::bulkInsert(const QList<LOTWUser>& users) {
    if (users.isEmpty()) {
        m_lastError = "User list is empty";
        return false;
    }

    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return false;
    }

    LOG_DEBUG("LOTWUserRepository", QString("Bulk inserting %1 LOTW users...").arg(users.size()));

    // Use prepared statement for efficiency
    QString sql = "INSERT INTO lotw_users (callsign, last_upload_date, last_upload_time, last_updated) "
                  "VALUES (?, ?, ?, ?)";

    QSqlQuery query(db.connection());
    if (!query.prepare(sql)) {
        m_lastError = query.lastError().text();
        LOG_WARN("LOTWUserRepository", QString("Failed to prepare bulk insert: %1").arg(m_lastError));
        return false;
    }

    // Insert all users in a single transaction for speed
    int successCount = 0;
    int errorCount = 0;

    for (const LOTWUser& user : users) {
        query.addBindValue(user.callsign.toUpper().trimmed());
        query.addBindValue(user.lastUploadDate);
        query.addBindValue(user.lastUploadTime);
        query.addBindValue(user.lastUpdated.toSecsSinceEpoch());

        if (!query.exec()) {
            errorCount++;
            // Log first few errors, skip rest to avoid log spam
            if (errorCount <= 5) {
                LOG_WARN("LOTWUserRepository", QString("Failed to insert user %1: %2")
                    .arg(user.callsign).arg(query.lastError().text()));
            }
        } else {
            successCount++;
        }
    }

    LOG_DEBUG("LOTWUserRepository", QString("Bulk insert complete: %1 success, %2 errors")
        .arg(successCount).arg(errorCount));

    if (errorCount > 0) {
        m_lastError = QString("%1 users failed to insert").arg(errorCount);
        return false;
    }

    return true;
}

bool LOTWUserRepository::save(LOTWUser& user) {
    if (user.callsign.isEmpty()) {
        m_lastError = "Callsign is empty";
        return false;
    }

    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        m_lastError = "Global database is not open";
        LOG_WARN("LOTWUserRepository", m_lastError);
        return false;
    }

    // Normalize callsign
    user.callsign = user.callsign.toUpper().trimmed();

    // Use INSERT OR REPLACE for upsert behavior
    QString sql = "INSERT OR REPLACE INTO lotw_users "
                  "(callsign, last_upload_date, last_upload_time, last_updated) "
                  "VALUES (?, ?, ?, ?)";

    QSqlQuery query = db.execute(sql, {
        user.callsign,
        user.lastUploadDate,
        user.lastUploadTime,
        user.lastUpdated.toSecsSinceEpoch()
    });

    if (!query.isActive()) {
        m_lastError = db.lastError();
        LOG_WARN("LOTWUserRepository", QString("Failed to save LOTW user %1: %2")
            .arg(user.callsign).arg(m_lastError));
        return false;
    }

    // Update ID from database
    user.id = db.lastInsertId();

    return true;
}

} // namespace TR4QT
