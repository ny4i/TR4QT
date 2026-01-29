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
