#ifndef SCPCALLSIGNEXTRACTOR_H
#define SCPCALLSIGNEXTRACTOR_H

#include <QString>
#include <QStringList>

namespace TR4QT {

/**
 * Extracts unique callsigns from contest logs for SCP augmentation
 *
 * Scans all contest databases in ~/.tr4qt/logs/ and extracts unique
 * callsigns to supplement the MASTER.SCP database with locally worked stations.
 */
class SCPCallsignExtractor {
public:
    SCPCallsignExtractor();
    ~SCPCallsignExtractor() = default;

    /**
     * Extract unique callsigns from all contest logs
     *
     * Scans:
     * - All *.db files in ~/.tr4qt/logs/
     * - Queries: SELECT DISTINCT callsign FROM qsos WHERE deleted=0
     *
     * @return List of unique callsigns across all logs
     */
    QStringList extractFromAllContests();

    /**
     * Extract unique callsigns from specific contest database
     *
     * @param dbPath Path to contest database file
     * @return List of unique callsigns
     */
    QStringList extractFromContest(const QString& dbPath);

    /**
     * Update SCP database with local callsigns
     * Clears old local_log entries and re-imports from all contests.
     *
     * @return Number of callsigns added to SCP database
     */
    int updateSCPFromLocalLogs();

    /**
     * Last error message
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
};

} // namespace TR4QT

#endif // SCPCALLSIGNEXTRACTOR_H
