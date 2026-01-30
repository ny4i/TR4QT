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

#ifndef DXCCREPOSITORY_H
#define DXCCREPOSITORY_H

#include <QString>
#include <QMap>

namespace TR4QT {

class GlobalDatabase;

/**
 * Repository for DXCC entity data
 * Manages the dxcc_entities table in the global database
 *
 * Singleton class that caches all DXCC entities in memory for fast lookups.
 */
class DXCCRepository {
public:
    /**
     * Get singleton instance
     */
    static DXCCRepository& instance();

    // Prevent copying
    DXCCRepository(const DXCCRepository&) = delete;
    DXCCRepository& operator=(const DXCCRepository&) = delete;

    /**
     * Get DXCC entity code for a given entity name
     * Returns 0 if not found
     */
    int getEntityCode(const QString& entityName) const;

    /**
     * Get entity name for a given DXCC entity code
     * Returns empty string if not found
     */
    QString getEntityName(int entityCode) const;

    /**
     * Initialize the DXCC entities table with ADIF specification data
     * This should be called once when the database is first created
     * or when the ADIF spec is updated
     * @return true if successful
     */
    bool initializeDXCCEntities();

    /**
     * Check if DXCC entities table is populated
     * @return true if table has data
     */
    bool isPopulated() const;

private:
    DXCCRepository();
    ~DXCCRepository() = default;

    GlobalDatabase& m_db;

    // Cache of entity name -> code mappings for fast lookups
    mutable QMap<QString, int> m_nameToCodeCache;
    mutable bool m_cacheLoaded{false};

    /**
     * Load cache from database
     */
    void loadCache() const;
};

} // namespace TR4QT

#endif // DXCCREPOSITORY_H
