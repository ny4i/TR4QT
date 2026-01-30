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

#ifndef QSOPARTYCONTESTBASE_H
#define QSOPARTYCONTESTBASE_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * Base class for QSO Party contests
 *
 * QSO Parties share common characteristics:
 * - In-state vs out-of-state rules (state-dependent exchanges and multipliers)
 * - County-based multipliers
 * - State/province exchanges
 * - Station validation requirements
 *
 * This base class provides common functionality that all QSO Party contests need,
 * avoiding duplication across Florida QP, California QP, Pennsylvania QP, etc.
 */
class QSOPartyContestBase : public ContestBase {
public:
    QSOPartyContestBase(const StationInfo& myStation) : ContestBase(myStation) {}
    virtual ~QSOPartyContestBase() = default;

    /**
     * Validate that station has all required information for QSO Party contests
     * Returns list of warning/error messages (empty if all OK)
     *
     * Critical checks (errors):
     *   - Callsign must be set
     *   - State must be set (determines in-state vs out-of-state rules)
     *   - Basic geographic info (continent, CQ zone)
     *
     * Recommended checks (warnings):
     *   - In-state stations: County should be set (for auto-fill exchange)
     *
     * Subclasses can override to add contest-specific validations
     */
    virtual QStringList validateStationInfo() const;

protected:
    /**
     * Determine if the operator is in the QSO Party's state
     * Subclasses must implement this to specify which state is "in-state"
     *
     * Example for Florida QSO Party:
     *   return m_myStation.state == "FL";
     *
     * Example for California QSO Party:
     *   return m_myStation.state == "CA";
     */
    virtual bool isInState() const = 0;

    /**
     * Get the QSO Party's state abbreviation (e.g., "FL", "CA", "PA")
     * Used for validation messages and county checks
     */
    virtual QString getQSOPartyState() const = 0;
};

} // namespace TR4QT

#endif // QSOPARTYCONTESTBASE_H
