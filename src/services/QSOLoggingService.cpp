/**
 * QSOLoggingService - Implementation
 */

#include "QSOLoggingService.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

QSOLoggingService::QSOLoggingService(const Dependencies& deps)
    : m_deps(deps)
{
    // Validate dependencies on construction
    if (!validateDependencies()) {
        LOG_WARN("QSOLoggingService", "Constructed with null dependencies");
    }
}

QSOLoggingService::LogQSOResult QSOLoggingService::logQSO(const LogQSORequest& request) {
    LogQSOResult result;

    // Validate dependencies
    if (!validateDependencies()) {
        result.success = false;
        result.errorMessage = "Service not properly initialized (null dependencies)";
        LOG_ERROR("QSOLoggingService", result.errorMessage);
        return result;
    }

    // Step 1: Validate input and create QSO (via QSOLogger)
    QSOLogger::Input loggerInput;
    loggerInput.callsign = request.callsign;
    loggerInput.exchange = request.exchange;
    loggerInput.radioState = request.radioState;
    loggerInput.operatorCallsign = request.operatorCallsign;
    loggerInput.serialNumber = request.serialNumber;
    loggerInput.operatingMode = request.operatingMode;
    loggerInput.radioNumber = request.radioNumber;

    QSOLogger::Result loggerResult = m_deps.qsoLogger->logQSO(loggerInput, request.existingQSOs);

    if (!loggerResult.success) {
        // Validation failed - determine which field caused the error
        result.success = false;
        result.errorMessage = loggerResult.errorMessage;

        // Set errorField based on error message content (QSOLogger returns specific messages)
        if (result.errorMessage.contains("Callsign", Qt::CaseInsensitive)) {
            result.errorField = ErrorField::Callsign;
        } else if (result.errorMessage.contains("Exchange", Qt::CaseInsensitive)) {
            result.errorField = ErrorField::Exchange;
        } else if (result.errorMessage.contains("Band", Qt::CaseInsensitive)) {
            result.errorField = ErrorField::Frequency;
        } else if (result.errorMessage.contains("Mode", Qt::CaseInsensitive)) {
            result.errorField = ErrorField::Mode;
        }

        LOG_DEBUG("QSOLoggingService", QString("QSOLogger validation failed: %1 (field: %2)")
                  .arg(loggerResult.errorMessage)
                  .arg(static_cast<int>(result.errorField)));
        return result;
    }

    // Copy logger result to service result
    result.qso = loggerResult.qso;
    result.isDuplicate = loggerResult.isDuplicate;
    result.dupeInfo = loggerResult.dupeInfo;
    result.isNewMultiplier = loggerResult.isNewMultiplier;
    result.multiplierValues = loggerResult.multiplierValues;
    result.updatedSerialNumber = loggerResult.updatedSerialNumber;

    LOG_DEBUG("QSOLoggingService",
             QString("QSO validated: %1 on %2 %3 (dupe=%4)")
             .arg(result.qso.callsign)
             .arg(bandToString(result.qso.band))
             .arg(modeToString(result.qso.mode))
             .arg(result.isDuplicate ? "YES" : "NO"));

    // Step 2: Save QSO to database with retry (via QSOPersistenceService)
    result.persistenceResult = m_deps.persistenceService->saveQSO(result.qso, request.contestDbId);

    // Assign database ID back to QSO (critical for integrity checks)
    if (result.persistenceResult.databaseId > 0) {
        result.qso.id = result.persistenceResult.databaseId;
        LOG_DEBUG("QSOLoggingService",
                 QString("Assigned database ID %1 to QSO %2")
                 .arg(result.qso.id).arg(result.qso.callsign));
    }

    if (result.persistenceResult.status == QSOPersistenceService::SaveResult::Failed) {
        // Persistence failed completely (database + emergency file)
        result.success = false;
        result.errorMessage = "Failed to save QSO (database and emergency file failed)";
        result.errorField = ErrorField::Database;
        LOG_ERROR("QSOLoggingService", result.errorMessage);
        return result;
    }

    if (result.persistenceResult.status == QSOPersistenceService::SaveResult::SavedToEmergencyFile) {
        // Emergency file fallback - return success but let caller know
        LOG_WARN("QSOLoggingService",
                QString("QSO saved to emergency file: %1").arg(result.qso.callsign));
    } else {
        // Successful database save
        LOG_DEBUG("QSOLoggingService",
                 QString("QSO saved to database: %1").arg(result.qso.callsign));
    }

    // Step 3: Save exchange to memory (via ExchangeMemoryService)
    if (request.saveExchangeMemory && !request.autoPopulated && !request.exchange.isEmpty()) {
        ExchangeMemoryService::SaveExchangeParams exchangeParams;
        exchangeParams.callsign = result.qso.callsign;
        exchangeParams.exchange = request.exchange;
        exchangeParams.contestId = request.contestId;
        exchangeParams.mode = result.qso.mode;
        exchangeParams.wasAutopopulated = false;

        bool saved = m_deps.exchangeMemoryService->saveExchange(exchangeParams);

        if (saved) {
            LOG_DEBUG("QSOLoggingService",
                     QString("Exchange saved to memory: %1 -> %2")
                     .arg(result.qso.callsign)
                     .arg(request.exchange));
        } else {
            LOG_WARN("QSOLoggingService",
                    QString("Failed to save exchange to memory"));
        }
    }

    // Step 4: Execute post-logging actions (via QSOLoggingCoordinator)
    if (m_deps.coordinator) {
        QSOLoggingCoordinator::PostLoggingParams coordParams;
        coordParams.qso = result.qso;
        coordParams.stationCallsign = request.stationCallsign;
        coordParams.adifContestId = request.adifContestId;
        coordParams.wa7bnmContestId = request.wa7bnmContestId;
        coordParams.databasePath = request.databasePath;
        coordParams.totalQSOCount = request.totalQSOCount;
        coordParams.qsosSinceLastCheck = request.qsosSinceLastCheck;
        coordParams.contestDbId = request.contestDbId;
        coordParams.memoryQSOCount = request.memoryQSOCount;

        result.postLoggingActions = m_deps.coordinator->executePostLoggingActions(coordParams);

        if (!result.postLoggingActions.isEmpty()) {
            LOG_DEBUG("QSOLoggingService",
                     QString("Post-logging actions: %1")
                     .arg(result.postLoggingActions.join(", ")));
        }
    }

    // Success
    result.success = true;
    LOG_INFO("QSOLoggingService",
            QString("QSO logged successfully: %1 on %2 %3")
            .arg(result.qso.callsign)
            .arg(bandToString(result.qso.band))
            .arg(modeToString(result.qso.mode)));

    return result;
}

bool QSOLoggingService::validateDependencies() const {
    return m_deps.qsoLogger != nullptr
        && m_deps.persistenceService != nullptr
        && m_deps.exchangeMemoryService != nullptr;
    // Note: coordinator is optional (can be nullptr)
}

} // namespace TR4QT
