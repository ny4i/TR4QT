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
