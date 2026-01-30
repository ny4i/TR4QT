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
