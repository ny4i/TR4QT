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

#ifndef LOTWUSERREPOSITORY_H
#define LOTWUSERREPOSITORY_H

#include <QString>
#include <QDateTime>
#include <QList>

namespace TR4QT {

/**
 * LOTW user information
 * Data structure matching the lotw_users table in global database
 */
struct LOTWUser {
    int id{-1};                    // Database ID (-1 if not in database)
    QString callsign;              // Callsign (normalized uppercase)
    QString lastUploadDate;        // Date in YYYY-MM-DD format
    QString lastUploadTime;        // Time in HH:MM:SS format (UTC)
    QDateTime lastUpdated;         // When we downloaded this record

    LOTWUser() = default;

    LOTWUser(const QString& call, const QString& date, const QString& time)
        : callsign(call), lastUploadDate(date), lastUploadTime(time) {}
};

/**
 * Repository for LOTW user database operations
 *
 * Provides fast callsign lookup and database management for LOTW users.
 * Uses the GlobalDatabase singleton for all operations.
 *
 * LOTW user data is global (shared across all contest logs) and stored
 * in the global database at ~/.tr4qt/tr4qt_global.db
 */
class LOTWUserRepository {
public:
    LOTWUserRepository();
    ~LOTWUserRepository() = default;

    /**
     * Check if callsign is a LOTW user
     * Case-insensitive lookup using database index
     *
     * This is the most common operation and is optimized for speed.
     *
     * @param callsign Callsign to check (any case)
     * @return true if callsign exists in LOTW database
     */
    bool isLotwUser(const QString& callsign) const;

    /**
     * Get LOTW user information for a callsign
     * Returns full user record including last upload date/time
     *
     * @param callsign Callsign to lookup (any case)
     * @return LOTWUser struct (check id >= 0 for success)
     */
    LOTWUser findByCallsign(const QString& callsign) const;

    /**
     * Get total count of LOTW users in database
     *
     * @return Number of LOTW users
     */
    int getUserCount() const;

    /**
     * Get timestamp of last database update
     * Returns the most recent last_updated timestamp from any record
     *
     * @return QDateTime of last update, invalid if database is empty
     */
    QDateTime getLastUpdateTime() const;

    /**
     * Clear all LOTW users from database
     * Used before importing new data
     *
     * @return true if successful
     */
    bool clearAll();

    /**
     * Bulk insert LOTW users (transaction-optimized)
     * Much faster than individual inserts for large datasets
     *
     * All users should have lastUpdated set to current time before calling
     *
     * @param users List of LOTWUser structs to insert
     * @return true if successful
     */
    bool bulkInsert(const QList<LOTWUser>& users);

    /**
     * Insert or update a single LOTW user
     * Slower than bulkInsert for large datasets
     *
     * @param user LOTWUser to insert/update
     * @return true if successful
     */
    bool save(LOTWUser& user);

    /**
     * Get last error message
     */
    QString lastError() const { return m_lastError; }

private:
    mutable QString m_lastError;
};

} // namespace TR4QT

#endif // LOTWUSERREPOSITORY_H
