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

/**
 * QSOLoggingCoordinator - Orchestrate post-logging actions
 *
 * Extracted from MainWindow::onLogQSO() as part of Phase 4 god class refactoring.
 *
 * Responsibility: Coordinate all actions that happen after a QSO is logged.
 *
 * Design: Coordinator pattern - delegates to existing managers.
 * - Broadcasts QSO via UDP (UDPBroadcastManager)
 * - Checks auto-backup threshold (BackupManager)
 * - Triggers integrity checks (DataIntegrityManager)
 *
 * Why extracted:
 * - Post-logging actions are secondary to logging itself
 * - Multiple independent actions (UDP, backup, integrity)
 * - Testable independently (can mock managers)
 * - Easy to add new post-logging actions
 * - MainWindow doesn't need to know orchestration details
 */

#ifndef QSOLOGGINGCOORDINATOR_H
#define QSOLOGGINGCOORDINATOR_H

#include <QString>
#include <QStringList>
#include "../models/QSO.h"

namespace TR4QT {

// Forward declarations
class UdpBroadcastManager;
class BackupManager;
class DataIntegrityManager;

/**
 * Coordinator for post-logging actions
 *
 * Orchestrates UDP broadcast, auto-backup, and integrity checks after
 * a QSO is successfully logged.
 *
 * Usage:
 *   QSOLoggingCoordinator coordinator(udpManager, backupManager, integrityManager);
 *
 *   QSOLoggingCoordinator::PostLoggingParams params;
 *   params.qso = loggedQSO;
 *   params.stationCallsign = "W1AW";
 *   params.contestName = "Winter Field Day 2026";
 *   params.databasePath = "/path/to/contest.db";
 *   params.totalQSOCount = 123;
 *   params.qsosSinceLastCheck = 50;
 *
 *   QStringList actions = coordinator.executePostLoggingActions(params);
 *   // Returns: ["UDP broadcast sent", "Auto-backup created", "Integrity check passed"]
 */
class QSOLoggingCoordinator {
public:
    /**
     * Parameters for post-logging actions
     */
    struct PostLoggingParams {
        QSO qso;                  // The QSO that was just logged
        QString stationCallsign;  // Station callsign (for UDP broadcast)
        QString adifContestId;    // ADIF Contest-ID (e.g., "CQ-WW-CW") for UDP broadcast
        int wa7bnmContestId;      // WA7BNM Contest Calendar ID for UDP broadcast
        QString databasePath;     // Database path (for auto-backup)
        int totalQSOCount;        // Total QSOs in contest (for auto-backup)
        int qsosSinceLastCheck;   // QSOs since last integrity check (triggers at 50)
        int contestDbId;          // Contest database ID (for integrity check)
        int memoryQSOCount;       // QSO count in memory (for integrity check)

        PostLoggingParams()
            : wa7bnmContestId(0)
            , totalQSOCount(0)
            , qsosSinceLastCheck(0)
            , contestDbId(-1)
            , memoryQSOCount(0)
        {}
    };

    /**
     * Construct coordinator with manager dependencies
     *
     * @param udpManager UDP broadcast manager (nullptr = skip UDP)
     * @param backupManager Backup manager (nullptr = skip backup)
     * @param integrityManager Integrity manager (nullptr = skip integrity)
     */
    QSOLoggingCoordinator(UdpBroadcastManager* udpManager,
                         BackupManager* backupManager,
                         DataIntegrityManager* integrityManager);

    /**
     * Execute all post-logging actions
     *
     * Performs (in order):
     * 1. UDP broadcast (if enabled)
     * 2. Auto-backup check (if threshold reached)
     * 3. Integrity check (if 50 QSOs since last check)
     *
     * @param params Post-logging parameters
     * @return List of actions performed (for logging/status display)
     */
    QStringList executePostLoggingActions(const PostLoggingParams& params);

private:
    UdpBroadcastManager* m_udpManager;
    BackupManager* m_backupManager;
    DataIntegrityManager* m_integrityManager;

    /**
     * Broadcast QSO via UDP
     * @return Action description (empty if skipped)
     */
    QString broadcastQSO(const QSO& qso, const QString& stationCall,
                        const QString& adifContestId, int wa7bnmContestId);

    /**
     * Check if auto-backup is needed
     * @return Action description (empty if skipped)
     */
    QString checkAutoBackup(const QString& dbPath, int qsoCount);

    /**
     * Check integrity if threshold reached (every 50 QSOs)
     * @return Action description (empty if skipped)
     */
    QString checkIntegrity(int qsosSinceLastCheck, int contestDbId, int memoryCount);
};

} // namespace TR4QT

#endif // QSOLOGGINGCOORDINATOR_H
