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

#ifndef INITIALEXCHANGEMANAGER_H
#define INITIALEXCHANGEMANAGER_H

#include <QObject>
#include <QString>
#include <QCache>
#include "../contests/ContestBase.h"
#include "../core/Types.h"

namespace TR4QT {

// Forward declarations
class ExchangeMemoryRepository;

/**
 * Manages initial exchange prediction/auto-population
 *
 * Implements TR4W's "Initial Exchange" system with multiple prediction sources:
 * 1. Exchange Memory - Prior contacts with same callsign in this contest type
 * 2. CTY.DAT Lookup - Zone/section prediction from callsign prefix
 * 3. Master Database - Cross-contest exchange knowledge (future)
 * 4. Contest Defaults - RST based on mode
 *
 * Respects contest modularity - uses ContestBase::getReceivedExchangeFields()
 * to understand what fields each contest expects.
 */
class InitialExchangeManager : public QObject {
    Q_OBJECT

public:
    static InitialExchangeManager& instance();

    /**
     * Predict exchange for a callsign
     *
     * Tries sources in priority order:
     * 1. Exchange memory (exact callsign match)
     * 2. Exchange memory (prefix match)
     * 3. CTY.DAT zone lookup
     * 4. Contest defaults (RST)
     *
     * @param callsign Station callsign
     * @param contest Active contest (for field definitions)
     * @param mode Operating mode (for RST defaults)
     * @return Predicted exchange string, or empty if no prediction
     */
    QString predictExchange(const QString& callsign,
                           ContestBase* contest,
                           ModeType mode);

    /**
     * Lookup exchange from memory (exact callsign match)
     * @param callsign Station callsign
     * @param contestType Contest identifier (e.g., "CQWW", "WFD")
     * @return Exchange string from memory, or empty if not found
     */
    QString lookupMemory(const QString& callsign, const QString& contestType);

    /**
     * Lookup exchange from CTY.DAT country file
     *
     * Populates zone fields based on callsign prefix.
     * Uses contest->getReceivedExchangeFields() to determine what to populate.
     *
     * @param callsign Station callsign
     * @param contest Active contest
     * @return Partial exchange with zone/section, or empty
     */
    QString lookupCTY(const QString& callsign, ContestBase* contest);

    /**
     * Get default exchange for contest
     *
     * Returns RST based on mode if contest expects RST field.
     *
     * @param contest Active contest
     * @param mode Operating mode
     * @return Default exchange (e.g., "599" for CW, "59" for SSB)
     */
    QString getDefaults(ContestBase* contest, ModeType mode);

private:
    InitialExchangeManager();
    ~InitialExchangeManager() override;

    // Singleton - prevent copying
    InitialExchangeManager(const InitialExchangeManager&) = delete;
    InitialExchangeManager& operator=(const InitialExchangeManager&) = delete;

    /**
     * Extract callsign prefix for partial matching
     * E.g., "W1AW" → "W1", "K6XX" → "K6"
     */
    QString extractPrefix(const QString& callsign) const;

    /**
     * Build exchange string from contest fields
     * @param fields Map of field name → value (e.g., {"RST": "599", "Zone": "14"})
     * @param contest Active contest (for field order)
     * @return Formatted exchange string
     */
    QString buildExchangeString(const QMap<QString, QString>& fields,
                                ContestBase* contest) const;

    // LRU cache for performance (100 most recent predictions)
    QCache<QString, QString> m_cache;

    // Exchange memory repository
    ExchangeMemoryRepository* m_memoryRepo;

    // Country file for zone lookups
    class CountryFile* m_countryFile;
};

} // namespace TR4QT

#endif // INITIALEXCHANGEMANAGER_H
