/**
 * QSOLoggingService - High-level QSO logging workflow orchestration
 *
 * Extracted from MainWindow::onLogQSO() as part of Phase 5 god class refactoring.
 *
 * Responsibility: Orchestrate complete QSO logging workflow by delegating to:
 * - QSOLogger: Validation, QSO creation, duplicate/multiplier checking
 * - QSOPersistenceService: Database save with retry/emergency fallback
 * - ExchangeMemoryService: Save exchange to memory
 * - QSOLoggingCoordinator: Post-logging actions (UDP, backup, integrity)
 *
 * Design: Integration service pattern - composes multiple services
 * - NO UI interactions (pure business logic)
 * - Returns comprehensive result for UI to handle
 * - MainWindow becomes thin coordinator
 * - Testable end-to-end without UI
 *
 * Why extracted:
 * - Enables testing QSO logging workflow without UI
 * - MainWindow::onLogQSO() reduced from 306 lines to ~50 lines
 * - Business logic separated from UI updates
 * - Can reuse for batch imports, API logging, etc.
 */

#ifndef QSOLOGGINGSERVICE_H
#define QSOLOGGINGSERVICE_H

#include <QString>
#include <QStringList>
#include <QList>
#include "../models/QSO.h"
#include "../core/Types.h"
#include "../controllers/QSOLogger.h"
#include "QSOPersistenceService.h"
#include "ExchangeMemoryService.h"
#include "QSOLoggingCoordinator.h"

namespace TR4QT {

/**
 * Integration service for QSO logging workflow
 *
 * Orchestrates multiple services into a single cohesive workflow.
 *
 * Usage:
 *   QSOLoggingService::Dependencies deps;
 *   deps.qsoLogger = new QSOLogger(config);
 *   deps.persistenceService = &QSOPersistenceService::instance();
 *   deps.exchangeMemoryService = &ExchangeMemoryService::instance();
 *   deps.coordinator = new QSOLoggingCoordinator(udpMgr, backupMgr, integrityMgr);
 *
 *   QSOLoggingService service(deps);
 *
 *   QSOLoggingService::LogQSORequest request;
 *   request.callsign = "W1AW";
 *   request.exchange = "599 CT";
 *   request.radioState = radioState;
 *   // ... (set other fields)
 *
 *   QSOLoggingService::LogQSOResult result = service.logQSO(request);
 *   if (!result.success) {
 *       showError(result.errorMessage);
 *   } else {
 *       updateUI(result);
 *   }
 */
class QSOLoggingService {
public:
    /**
     * Service dependencies (injected via constructor)
     */
    struct Dependencies {
        QSOLogger* qsoLogger = nullptr;
        QSOPersistenceService* persistenceService = nullptr;
        ExchangeMemoryService* exchangeMemoryService = nullptr;
        QSOLoggingCoordinator* coordinator = nullptr;
    };

    /**
     * Input parameters for QSO logging
     */
    struct LogQSORequest {
        QString callsign;                   // Station callsign
        QString exchange;                   // Received exchange
        RadioState radioState;              // Current radio state (frequency, band, mode)
        QString operatorCallsign;           // Operator callsign
        int serialNumber;                   // Current serial number
        OperatingMode operatingMode;        // CQ vs S&P mode
        QList<QSO> existingQSOs;            // For duplicate/multiplier checking
        bool saveExchangeMemory = true;     // Save to exchange memory?
        bool autoPopulated = false;         // Was exchange auto-populated?

        // Contest context (for post-logging actions)
        QString stationCallsign;            // Station callsign (for UDP broadcast)
        QString adifContestId;              // ADIF Contest-ID (e.g., "CQ-WW-CW") for UDP broadcast
        int wa7bnmContestId = 0;            // WA7BNM Contest Calendar ID for UDP broadcast
        QString contestId;                  // Contest identifier (e.g., "WFD", "CQWW")
        QString databasePath;               // Database path (for auto-backup)
        int totalQSOCount = 0;              // Total QSOs in contest (for auto-backup)
        int qsosSinceLastCheck = 0;         // QSOs since last integrity check
        int contestDbId = -1;               // Contest database ID
        int memoryQSOCount = 0;             // QSO count in memory (for integrity)
    };

    /**
     * Result returned from QSO logging
     */
    struct LogQSOResult {
        bool success = false;               // true if QSO logged successfully
        QString errorMessage;               // Error message if success = false

        // Success data (if success = true)
        QSO qso;                            // Created QSO object
        bool isDuplicate = false;           // true if duplicate QSO
        QString dupeInfo;                   // Human-readable duplicate info
        bool isNewMultiplier = false;       // true if new multiplier(s)
        QStringList multiplierValues;       // New multiplier values
        int updatedSerialNumber = 0;        // Updated serial number

        // Persistence result
        QSOPersistenceService::SaveResult persistenceResult;

        // Post-logging actions performed
        QStringList postLoggingActions;     // Status messages (UDP, backup, integrity)
    };

    /**
     * Construct service with dependencies
     *
     * @param deps Service dependencies (all must be non-null)
     */
    explicit QSOLoggingService(const Dependencies& deps);

    /**
     * Execute complete QSO logging workflow
     *
     * Workflow:
     * 1. Validate input and create QSO (via QSOLogger)
     * 2. Save QSO to database with retry (via QSOPersistenceService)
     * 3. Save exchange to memory (via ExchangeMemoryService)
     * 4. Execute post-logging actions (via QSOLoggingCoordinator)
     *
     * @param request Input parameters for QSO logging
     * @return Result with success status, QSO, persistence info, post-actions
     *
     * NO UI INTERACTIONS - returns result for caller to handle
     */
    LogQSOResult logQSO(const LogQSORequest& request);

private:
    Dependencies m_deps;

    /**
     * Validate dependencies (all must be non-null)
     * @return true if valid, false if any dependency is null
     */
    bool validateDependencies() const;
};

} // namespace TR4QT

#endif // QSOLOGGINGSERVICE_H
