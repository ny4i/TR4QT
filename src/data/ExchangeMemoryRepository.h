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

#ifndef EXCHANGEMEMORYREPOSITORY_H
#define EXCHANGEMEMORYREPOSITORY_H

#include <QString>
#include <QDateTime>
#include <QList>
#include "../core/Types.h"

namespace TR4QT {

/**
 * Exchange memory entry
 *
 * Stores exchange data from a successful QSO for future auto-population.
 * Used by InitialExchangeManager to predict exchanges for repeated contacts.
 */
struct ExchangeMemoryEntry {
    QString callsign;           // Full callsign (e.g., "W1AW")
    QString callsignPrefix;     // Prefix for partial matching (e.g., "W1")
    QString exchange;           // Full exchange string (e.g., "1O MA")
    QString rst;                // Extracted RST (optional)
    QString contestType;        // Contest ID (e.g., "CQWW", "WFD", NULL = any)
    ModeType mode;              // Operating mode
    QDateTime timestamp;        // When this exchange was logged
    QString source;             // "manual", "auto", "imported"
    int hitCount;               // How many times used for prediction

    ExchangeMemoryEntry()
        : mode(ModeType::CW)
        , hitCount(0)
    {}
};

/**
 * Repository for exchange memory database operations
 *
 * Manages the exchange_memory table which stores exchanges from successful QSOs
 * for future auto-population. Provides fast lookup by exact callsign or prefix.
 *
 * Database schema (see schema.sql):
 * - Indexed by callsign for fast exact lookup
 * - Indexed by callsign_prefix for partial matching
 * - Unique constraint on (callsign, contest_type)
 */
class ExchangeMemoryRepository {
public:
    ExchangeMemoryRepository();
    ~ExchangeMemoryRepository() = default;

    /**
     * Save or update exchange memory entry
     *
     * If entry exists for (callsign, contest_type), updates it.
     * Otherwise inserts new entry.
     *
     * @param entry Exchange memory entry to save
     * @return true if successful, false on error
     */
    bool save(const ExchangeMemoryEntry& entry);

    /**
     * Find exchange by exact callsign match
     *
     * @param callsign Station callsign
     * @param contestType Contest identifier (empty = any contest)
     * @return Exchange memory entry, or empty entry if not found
     */
    ExchangeMemoryEntry findExact(const QString& callsign,
                                  const QString& contestType = QString());

    /**
     * Find exchanges by callsign prefix
     *
     * Returns all entries matching the prefix.
     * Useful for partial callsign matching (e.g., "W1" matches "W1AW", "W1XY").
     *
     * @param prefix Callsign prefix (e.g., "W1", "K6")
     * @return List of matching entries, sorted by hit_count descending
     */
    QList<ExchangeMemoryEntry> findByPrefix(const QString& prefix);

    /**
     * Delete entries older than specified days
     *
     * Cleanup method to prune stale memory entries.
     *
     * @param days Age threshold in days
     * @return Number of entries deleted
     */
    int deleteOlderThan(int days);

    /**
     * Clear all exchange memory entries for a specific contest
     *
     * Used when the log is cleared to remove all exchange memory
     * associated with that contest.
     *
     * @param contestType Contest identifier (e.g., "CQWW", "WFD")
     * @return Number of entries deleted
     */
    int clearForContest(const QString& contestType);

    /**
     * Get total number of entries in memory
     * @return Entry count
     */
    int count() const;

    /**
     * Get hit rate statistics
     *
     * Returns percentage of successful lookups.
     * Tracked internally by incrementing hit_count on each use.
     *
     * @return Hit rate as percentage (0-100)
     */
    int getHitRate() const;

    /**
     * Last error message from database operation
     */
    QString lastError() const { return m_lastError; }

private:
    /**
     * Extract callsign prefix for indexing
     * E.g., "W1AW" → "W1", "K6XX" → "K6", "G3ABC" → "G3"
     */
    QString extractPrefix(const QString& callsign) const;

    QString m_lastError;
};

} // namespace TR4QT

#endif // EXCHANGEMEMORYREPOSITORY_H
