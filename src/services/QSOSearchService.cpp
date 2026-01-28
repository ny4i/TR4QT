/**
 * QSOSearchService - Implementation
 */

#include "QSOSearchService.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

QList<QSO> QSOSearchService::search(const QSOSearchCriteria& criteria) {
    m_lastError.clear();
    m_lastResultCount = 0;

    if (!criteria.hasAnyCriteria()) {
        m_lastError = "No search criteria specified";
        LOG_WARN("QSOSearchService", m_lastError);
        return {};
    }

    LOG_INFO("QSOSearchService",
        QString("Searching: callsign='%1' operator='%2' contestId=%3")
            .arg(criteria.callsign)
            .arg(criteria.operatorCall)
            .arg(criteria.contestId));

    QSORepository repo;
    QList<QSO> results = repo.search(criteria);

    m_lastResultCount = results.size();

    LOG_INFO("QSOSearchService",
        QString("Search returned %1 results").arg(m_lastResultCount));

    return results;
}

}  // namespace TR4QT
