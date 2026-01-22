/**
 * QSOLoggingCoordinator - Implementation
 */

#include "QSOLoggingCoordinator.h"
#include "../network/UdpBroadcastManager.h"  // Full header needed for method calls
#include "../data/BackupManager.h"           // Full header needed for method calls
#include "../controllers/DataIntegrityManager.h"  // Full header needed for method calls
#include "../logging/LogMacros.h"
#include <QString>
#include <QStringList>

namespace TR4QT {

QSOLoggingCoordinator::QSOLoggingCoordinator(
    UdpBroadcastManager* udpManager,
    BackupManager* backupManager,
    DataIntegrityManager* integrityManager)
    : m_udpManager(udpManager)
    , m_backupManager(backupManager)
    , m_integrityManager(integrityManager)
{
}

QStringList QSOLoggingCoordinator::executePostLoggingActions(const PostLoggingParams& params) {
    QStringList actions;

    // 1. UDP broadcast
    QString udpAction = broadcastQSO(params.qso, params.stationCallsign,
                                     params.adifContestId, params.wa7bnmContestId);
    if (!udpAction.isEmpty()) {
        actions.append(udpAction);
    }

    // 2. Auto-backup check
    QString backupAction = checkAutoBackup(params.databasePath, params.totalQSOCount);
    if (!backupAction.isEmpty()) {
        actions.append(backupAction);
    }

    // 3. Integrity check (every 50 QSOs)
    QString integrityAction = checkIntegrity(
        params.qsosSinceLastCheck,
        params.contestDbId,
        params.memoryQSOCount
    );
    if (!integrityAction.isEmpty()) {
        actions.append(integrityAction);
    }

    return actions;
}

QString QSOLoggingCoordinator::broadcastQSO(
    const QSO& qso,
    const QString& stationCall,
    const QString& adifContestId,
    int wa7bnmContestId)
{
    if (!m_udpManager) {
        return QString();  // No UDP manager
    }

    if (!m_udpManager->isEnabled()) {
        return QString();  // UDP disabled
    }

    if (!m_udpManager->isContactInfoEnabled()) {
        return QString();  // ContactInfo messages disabled
    }

    // Broadcast QSO with ADIF Contest-ID and WA7BNM Contest Calendar ID
    m_udpManager->onQSOLogged(qso, stationCall, adifContestId, wa7bnmContestId);

    LOG_DEBUG("QSOLoggingCoordinator",
             QString("UDP broadcast sent for %1").arg(qso.callsign));

    return QString("UDP broadcast sent");
}

QString QSOLoggingCoordinator::checkAutoBackup(const QString& dbPath, int qsoCount) {
    if (!m_backupManager) {
        return QString();  // No backup manager
    }

    if (dbPath.isEmpty()) {
        return QString();  // No database path
    }

    // Check if auto-backup is needed
    bool backupCreated = m_backupManager->autoBackupIfNeeded(dbPath, qsoCount);

    if (backupCreated) {
        LOG_INFO("QSOLoggingCoordinator",
                QString("Auto-backup created at %1 QSOs").arg(qsoCount));
        return QString("Auto-backup created");
    }

    return QString();  // No backup needed
}

QString QSOLoggingCoordinator::checkIntegrity(
    int qsosSinceLastCheck,
    int contestDbId,
    int memoryCount)
{
    if (!m_integrityManager) {
        return QString();  // No integrity manager
    }

    // Integrity check triggered every 50 QSOs
    if (qsosSinceLastCheck < 50) {
        return QString();  // Not time yet
    }

    if (contestDbId < 0) {
        return QString();  // No active contest
    }

    // Run quick integrity check
    DataIntegrityManager::QuickCheckResult result = m_integrityManager->quickIntegrityCheck(memoryCount);

    if (result.passed) {
        LOG_DEBUG("QSOLoggingCoordinator",
                 QString("Integrity check passed (memory=%1, db=%2)").arg(result.memoryCount).arg(result.dbCount));
        return QString("Integrity check passed");
    } else {
        LOG_WARN("QSOLoggingCoordinator",
                QString("Integrity check FAILED (memory=%1, db=%2)").arg(result.memoryCount).arg(result.dbCount));
        return QString("Integrity check FAILED");
    }
}

} // namespace TR4QT
