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
