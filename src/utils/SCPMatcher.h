#ifndef SCPMATCHER_H
#define SCPMATCHER_H

#include <QString>
#include <QStringList>

namespace TR4QT {

/**
 * Smart SCP matching engine
 *
 * Provides real-time callsign matching with <50ms latency.
 * Currently a thin wrapper around SCPRepository for future caching.
 *
 * This wrapper allows for future performance optimizations
 * (e.g., in-memory caching of frequent queries) without changing the API.
 */
class SCPMatcher {
public:
    SCPMatcher();
    ~SCPMatcher() = default;

    /**
     * Find matching callsigns for partial input
     *
     * Returns top 5 matches using smart prefix+suffix algorithm:
     * 1. Prefix matches (starts with partial)
     * 2. Suffix matches (ends with partial)
     *
     * Prioritization (when contestDbPath is provided):
     * - Calls worked in THIS contest appear first (from contest database)
     * - MASTER.SCP calls appear second (from global database)
     *
     * @param partial Partial callsign (minimum 2 characters)
     * @param contestDbPath Path to contest database for prioritization (optional)
     * @return List of matches, max 5 entries
     */
    QStringList findMatches(const QString& partial, const QString& contestDbPath = QString());

    /**
     * Enable/disable SCP matching
     * @param enabled true to enable matching
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * Check if SCP is enabled
     */
    bool isEnabled() const { return m_enabled; }

private:
    bool m_enabled{true};
};

} // namespace TR4QT

#endif // SCPMATCHER_H
