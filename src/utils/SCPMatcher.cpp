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

#include "SCPMatcher.h"
#include "../data/SCPRepository.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

SCPMatcher::SCPMatcher() {
}

QStringList SCPMatcher::findMatches(const QString& partial, const QString& contestDbPath) {
    LOG_DEBUG("SCPMatcher", QString("findMatches: m_enabled=%1, partial='%2', length=%3, contestDbPath='%4'")
        .arg(m_enabled ? "true" : "false").arg(partial).arg(partial.length()).arg(contestDbPath));

    if (!m_enabled || partial.length() < 2) {
        LOG_DEBUG("SCPMatcher", "findMatches: skipping (disabled or too short)");
        return QStringList();
    }

    // Delegate to SCPRepository for actual matching
    SCPRepository repo;
    QStringList matches = repo.findMatches(partial, contestDbPath);
    LOG_DEBUG("SCPMatcher", QString("findMatches: SCPRepository returned %1 matches")
        .arg(matches.size()));
    return matches;
}

} // namespace TR4QT
