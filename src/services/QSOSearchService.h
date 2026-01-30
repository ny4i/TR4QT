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
