#include "SCPMatcher.h"
#include "../data/SCPRepository.h"

namespace TR4QT {

SCPMatcher::SCPMatcher() {
}

QStringList SCPMatcher::findMatches(const QString& partial) {
    if (!m_enabled || partial.length() < 2) {
        return QStringList();
    }

    // Delegate to SCPRepository for actual matching
    SCPRepository repo;
    return repo.findMatches(partial);
}

} // namespace TR4QT
