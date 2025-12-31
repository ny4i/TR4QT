#ifndef SCPREPOSITORY_H
#define SCPREPOSITORY_H

#include <QString>
#include <QStringList>
#include <QDateTime>

namespace TR4QT {

/**
 * Repository for Super Check Partial database operations
 *
 * Manages the scp_callsigns table in GlobalDatabase.
 * Provides fast prefix/suffix matching for real-time SCP display.
 *
 * SCP data is global (shared across all contest logs) and stored
 * in the global database at ~/.tr4qt/tr4qt_global.db
 */
class SCPRepository {
public:
    SCPRepository();
    ~SCPRepository() = default;

    /**
     * Find matching callsigns using smart prefix+suffix search
     *
     * Matches both:
     * - Prefix: "W1" matches W1AW, W1ABC, W1XYZ
     * - Suffix: "AW" matches W1AW, K9AW, VE3AW
     *
     * Returns top 5 matches combined, prefix matches prioritized.
     *
     * @param partial Partial callsign (minimum 2 characters)
     * @return List of matching callsigns, max 5 entries
     */
    QStringList findMatches(const QString& partial) const;

    /**
     * Bulk insert callsigns (transaction-optimized)
     * Used for MASTER.SCP import and local log extraction.
     *
     * Uses INSERT OR IGNORE to skip duplicates without error.
     *
     * @param callsigns List of callsigns to insert
     * @param source Source identifier ("master_scp" or "local_log")
     * @param contestId Contest ID (optional, for local logs only)
     * @return Number of callsigns successfully inserted
     */
    int bulkInsert(const QStringList& callsigns,
                   const QString& source,
                   const QString& contestId = QString());

    /**
     * Clear all callsigns from a specific source
     * Used before re-importing MASTER.SCP or when clearing local logs.
     *
     * @param source Source identifier ("master_scp" or "local_log")
     * @return Number of callsigns deleted
     */
    int clearBySource(const QString& source);

    /**
     * Clear all SCP callsigns
     * Used for complete database reset
     *
     * @return true if successful
     */
    bool clearAll();

    /**
     * Get total callsign count
     * @return Number of callsigns in database
     */
    int getCallsignCount() const;

    /**
     * Get callsign count by source
     * @param source Source identifier
     * @return Number of callsigns from this source
     */
    int getCallsignCountBySource(const QString& source) const;

    /**
     * Set SCP metadata (e.g., version, last update time)
     * @param key Metadata key (e.g., "master_scp_version")
     * @param value Metadata value
     * @return true if successful
     */
    bool setMetadata(const QString& key, const QString& value);

    /**
     * Get SCP metadata
     * @param key Metadata key
     * @return Metadata value, or empty string if not found
     */
    QString getMetadata(const QString& key) const;

    /**
     * Get metadata updated timestamp
     * @param key Metadata key
     * @return QDateTime of last update, invalid if not found
     */
    QDateTime getMetadataTimestamp(const QString& key) const;

    /**
     * Last error message from database operation
     */
    QString lastError() const { return m_lastError; }

private:
    mutable QString m_lastError;
};

} // namespace TR4QT

#endif // SCPREPOSITORY_H
