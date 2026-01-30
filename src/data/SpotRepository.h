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

#ifndef SPOTREPOSITORY_H
#define SPOTREPOSITORY_H

#include <QString>
#include <QList>
#include <QDateTime>

class QSqlQuery;

namespace TR4QT {

// Forward declaration
struct Spot;

/**
 * Repository for DX spot database operations
 *
 * Provides shutdown persistence for band map spots with automatic aging and cleanup.
 * Uses GlobalDatabase for cross-contest spot sharing.
 *
 * Spots are stored in-memory during operation and persisted to database only on
 * clean shutdown. This design optimizes for high-frequency spot arrival (10-50/sec)
 * while providing restoration after restarts.
 *
 * Usage:
 *   // On startup
 *   SpotRepository repo;
 *   QList<Spot> spots = repo.loadAllSpots();
 *
 *   // On shutdown
 *   repo.saveAllSpots(currentSpots);
 */
class SpotRepository {
public:
    SpotRepository();
    ~SpotRepository() = default;

    // ===== Core Operations =====

    /**
     * Load all spots from database (called once on startup)
     * Returns empty list if database is empty or not initialized
     *
     * @return List of all spots from database
     */
    QList<Spot> loadAllSpots() const;

    /**
     * Save all spots to database (called once on shutdown)
     * Uses single transaction for speed (atomic operation)
     * Clears existing spots before insert (full replacement)
     *
     * @param spots List of spots to save
     * @return true if successful
     */
    bool saveAllSpots(const QList<Spot>& spots);

    /**
     * Clear all spots from database
     * Used for debugging or explicit user action
     *
     * @return true if successful
     */
    bool clearAll();

    /**
     * Get total spot count in database
     *
     * @return Number of spots in database
     */
    int getSpotCount() const;

    /**
     * Get last error message
     */
    QString lastError() const { return m_lastError; }

private:
    /**
     * Convert database row to Spot object
     */
    Spot spotFromQuery(const QSqlQuery& query) const;

    mutable QString m_lastError;
};

} // namespace TR4QT

#endif // SPOTREPOSITORY_H
