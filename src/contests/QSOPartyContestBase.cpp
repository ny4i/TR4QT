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

#include "QSOPartyContestBase.h"

namespace TR4QT {

QStringList QSOPartyContestBase::validateStationInfo() const {
    QStringList issues;

    // ===== Critical Checks (Errors) =====

    // Check: Callsign must be set
    if (m_myStation.callsign.isEmpty()) {
        issues.append("ERROR: Station callsign is not configured. Please set your callsign in Preferences → Station Info.");
    }

    // Check: State must be set (determines in-state vs out-of-state rules)
    if (m_myStation.state.isEmpty()) {
        QString qpState = getQSOPartyState();
        issues.append(QString("ERROR: Station state/province is not configured. %1 QSO Party requires your state to determine exchange and multiplier rules. Please set your state in Preferences → Station Info.")
            .arg(qpState));
    }

    // Check: Basic geographic info
    if (m_myStation.continent.isEmpty()) {
        issues.append("WARNING: Station continent is not configured. This may affect multiplier calculations.");
    }

    if (m_myStation.cqZone <= 0) {
        issues.append("WARNING: Station CQ Zone is not configured. This may affect multiplier calculations.");
    }

    // ===== Recommended Checks (Warnings) =====

    // For in-state stations: warn if county is not set
    if (isInState() && m_myStation.county.isEmpty()) {
        QString qpState = getQSOPartyState();
        issues.append(QString("WARNING: You are operating from %1, but county is not configured. You will need to manually enter your county for each QSO. Please set your county in Preferences → Station Info (once the field is added to the UI).")
            .arg(qpState));
    }

    // Power check (informational)
    if (m_myStation.power <= 0) {
        issues.append("INFO: Station power is set to 0W (will default to 100W High Power). If you are running QRP or Low Power, please update your power setting when the UI is available.");
    }

    return issues;
}

} // namespace TR4QT
