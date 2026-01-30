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
 * IQSODataSource - Interface for QSO data access
 *
 * This interface abstracts QSO data access, allowing different implementations:
 * - QSOTableModel: GUI mode (wraps Qt model)
 * - WebServerContext: Headless mode (in-memory list)
 *
 * Used by WebServer to access QSO data without depending on Qt Widgets.
 */

#ifndef IQSODATASOURCE_H
#define IQSODATASOURCE_H

#include <QList>
#include "../models/QSO.h"

namespace TR4QT {

/**
 * Interface for accessing QSO data
 *
 * Implementations must be thread-safe for read operations.
 */
class IQSODataSource {
public:
    virtual ~IQSODataSource() = default;

    /**
     * Get the total number of QSOs
     * @return Number of QSOs in the data source
     */
    virtual int qsoCount() const = 0;

    /**
     * Get a QSO by index
     * @param index Zero-based index
     * @return QSO at the index, or empty QSO if out of range
     */
    virtual QSO qsoAt(int index) const = 0;

    /**
     * Get all QSOs
     * @return List of all QSOs
     */
    virtual QList<QSO> allQSOs() const = 0;
};

} // namespace TR4QT

#endif // IQSODATASOURCE_H
