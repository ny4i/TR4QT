/**
 * ExchangeMemoryService - Implementation
 */

#include "ExchangeMemoryService.h"
#include "../exchanges/InitialExchangeManager.h"
#include "../contests/ContestBase.h"
#include "../logging/LogMacros.h"
#include <QDateTime>

namespace TR4QT {

ExchangeMemoryService::ExchangeMemoryService()
    : m_repository(new ExchangeMemoryRepository())
{
}

ExchangeMemoryService::~ExchangeMemoryService() {
    delete m_repository;
}

bool ExchangeMemoryService::saveExchange(const SaveExchangeParams& params) {
    // Skip if exchange is empty
    if (params.exchange.isEmpty()) {
        m_lastError = "Exchange is empty";
        LOG_WARN("ExchangeMemoryService", "Skipping save - exchange is empty");
        return false;
    }

    // Skip if callsign is too short
    if (params.callsign.length() < 2) {
        m_lastError = "Callsign too short";
        LOG_WARN("ExchangeMemoryService",
                QString("Skipping save - callsign '%1' too short").arg(params.callsign));
        return false;
    }

    // Create exchange memory entry
    ExchangeMemoryEntry entry;
    entry.callsign = params.callsign.toUpper();  // Normalize to uppercase
    entry.callsignPrefix = extractPrefix(entry.callsign);
    entry.exchange = params.exchange;
    entry.contestType = params.contestId;
    entry.mode = params.mode;
    entry.timestamp = QDateTime::currentDateTime();
    entry.source = params.wasAutopopulated ? "auto" : "manual";
    entry.hitCount = 0;

    // Save to repository
    if (!m_repository->save(entry)) {
        m_lastError = m_repository->lastError();
        LOG_WARN("ExchangeMemoryService",
                QString("Failed to save exchange for %1: %2")
                .arg(params.callsign).arg(m_lastError));
        return false;
    }

    LOG_DEBUG("ExchangeMemoryService",
             QString("Saved exchange for %1: '%2' (contest=%3, source=%4)")
             .arg(entry.callsign).arg(entry.exchange)
             .arg(entry.contestType).arg(entry.source));

    return true;
}

QString ExchangeMemoryService::predictExchange(const QString& callsign,
                                               ContestBase* contest,
                                               ModeType mode) {
    if (!contest) {
        LOG_WARN("ExchangeMemoryService", "Cannot predict exchange - no active contest");
        return QString();
    }

    // Delegate to InitialExchangeManager for sophisticated prediction
    QString prediction = InitialExchangeManager::instance().predictExchange(
        callsign, contest, mode
    );

    if (!prediction.isEmpty()) {
        LOG_DEBUG("ExchangeMemoryService",
                 QString("Predicted exchange for %1: '%2'")
                 .arg(callsign).arg(prediction));
    }

    return prediction;
}

QList<ExchangeMemoryEntry> ExchangeMemoryService::getHistory(
    const QString& callsign,
    const QString& contestId)
{
    QList<ExchangeMemoryEntry> history;

    // Try exact match first
    ExchangeMemoryEntry exact = m_repository->findExact(callsign, contestId);
    if (!exact.callsign.isEmpty()) {
        history.append(exact);
    }

    // If no exact match, try prefix match
    if (history.isEmpty() && callsign.length() >= 2) {
        QString prefix = extractPrefix(callsign);
        QList<ExchangeMemoryEntry> prefixMatches = m_repository->findByPrefix(prefix);

        // Filter by contest if specified
        if (!contestId.isEmpty()) {
            for (const ExchangeMemoryEntry& entry : prefixMatches) {
                if (entry.contestType == contestId) {
                    history.append(entry);
                }
            }
        } else {
            history = prefixMatches;
        }
    }

    return history;
}

QString ExchangeMemoryService::lastError() const {
    return m_lastError;
}

QString ExchangeMemoryService::extractPrefix(const QString& callsign) const {
    // Extract prefix for partial matching
    // Examples: "W1AW" → "W1", "K6XX" → "K6", "G3ABC" → "G3"

    if (callsign.length() < 2) {
        return callsign;
    }

    // Find first digit
    for (int i = 0; i < callsign.length(); ++i) {
        if (callsign[i].isDigit()) {
            // Include digit in prefix
            return callsign.left(i + 1);
        }
    }

    // No digit found - return first 2-3 characters
    return callsign.left(qMin(3, callsign.length()));
}

} // namespace TR4QT
