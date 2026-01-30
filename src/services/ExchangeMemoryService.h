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
 * ExchangeMemoryService - Manage exchange memory for predictions
 *
 * Extracted from MainWindow::onLogQSO() as part of Phase 3 god class refactoring.
 *
 * Responsibility: Save and predict exchanges using exchange memory system.
 *
 * Design: Service layer wrapping ExchangeMemoryRepository and InitialExchangeManager.
 * - Saves exchanges after successful QSOs
 * - Predicts exchanges for callsigns (delegates to InitialExchangeManager)
 * - Provides exchange history lookup
 *
 * Why extracted:
 * - Exchange memory is a distinct feature (used in multiple places)
 * - Has its own repository and prediction engine
 * - Testable independently (can mock repository)
 * - Reusable across QSO logging, editing, import
 */

#ifndef EXCHANGEMEMORYSERVICE_H
#define EXCHANGEMEMORYSERVICE_H

#include <QString>
#include "../core/Types.h"
#include "../data/ExchangeMemoryRepository.h"

namespace TR4QT {

// Forward declarations
class ContestBase;
class InitialExchangeManager;

/**
 * Service for managing exchange memory (save, predict, lookup)
 *
 * Usage:
 *   ExchangeMemoryService service;
 *
 *   // After logging QSO
 *   ExchangeMemoryService::SaveExchangeParams params;
 *   params.callsign = "W1AW";
 *   params.exchange = "1O MA";
 *   params.contestId = "WFD";
 *   params.mode = ModeType::CW;
 *   params.wasAutopopulated = false;
 *   service.saveExchange(params);
 *
 *   // Before logging QSO
 *   QString predicted = service.predictExchange(
 *       "W1AW", contest, ModeType::CW
 *   );
 */
class ExchangeMemoryService {
public:
    /**
     * Parameters for saving exchange to memory
     */
    struct SaveExchangeParams {
        QString callsign;         // Station callsign (e.g., "W1AW")
        QString exchange;         // Full exchange string (e.g., "1O MA")
        QString contestId;        // Contest identifier (e.g., "WFD", "CQWW")
        ModeType mode;            // Operating mode
        bool wasAutopopulated;    // true = auto, false = manual

        SaveExchangeParams()
            : mode(ModeType::CW)
            , wasAutopopulated(false)
        {}
    };

    /**
     * Construct service with default repository
     */
    ExchangeMemoryService();

    /**
     * Destructor
     */
    ~ExchangeMemoryService();

    /**
     * Save exchange to memory for future predictions
     *
     * Creates ExchangeMemoryEntry and saves to repository.
     * Automatically extracts prefix and sets source (auto/manual).
     *
     * @param params Exchange save parameters
     * @return true if saved successfully, false on error
     */
    bool saveExchange(const SaveExchangeParams& params);

    /**
     * Predict exchange for callsign
     *
     * Delegates to InitialExchangeManager which tries:
     * 1. Exchange memory (exact match)
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
     * Get exchange history for callsign
     *
     * Returns all exchange memory entries for this callsign,
     * sorted by hit count (most used first).
     *
     * @param callsign Station callsign
     * @param contestId Optional contest filter (empty = all contests)
     * @return List of exchange memory entries
     */
    QList<ExchangeMemoryEntry> getHistory(const QString& callsign,
                                          const QString& contestId = QString());

    /**
     * Get last error message from repository
     * @return Error message, or empty if no error
     */
    QString lastError() const;

private:
    ExchangeMemoryRepository* m_repository;
    QString m_lastError;

    /**
     * Extract callsign prefix for indexing
     * E.g., "W1AW" → "W1", "K6XX" → "K6"
     */
    QString extractPrefix(const QString& callsign) const;
};

} // namespace TR4QT

#endif // EXCHANGEMEMORYSERVICE_H
