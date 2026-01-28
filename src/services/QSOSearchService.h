/**
 * QSOSearchService - Search for QSOs by flexible criteria
 *
 * Thin service layer wrapping QSORepository::search().
 * Validates criteria, logs the query, and returns results.
 * Keeps SQL out of UI code.
 */

#ifndef QSOSEARCHSERVICE_H
#define QSOSEARCHSERVICE_H

#include <QList>
#include <QString>
#include "../data/QSORepository.h"
#include "../models/QSO.h"

namespace TR4QT {

class QSOSearchService {
public:
    QSOSearchService() = default;

    /**
     * Search QSOs matching the given criteria
     *
     * @param criteria Search criteria (all optional, ANDed together)
     * @return List of matching QSOs ordered by timestamp DESC
     */
    QList<QSO> search(const QSOSearchCriteria& criteria);

    /**
     * @return Number of results from last search
     */
    int lastResultCount() const { return m_lastResultCount; }

    /**
     * @return Last error message (empty if no error)
     */
    QString lastError() const { return m_lastError; }

private:
    int m_lastResultCount = 0;
    QString m_lastError;
};

}  // namespace TR4QT

#endif  // QSOSEARCHSERVICE_H
