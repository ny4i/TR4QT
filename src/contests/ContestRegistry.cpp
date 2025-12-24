#include "ContestRegistry.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

ContestRegistry& ContestRegistry::instance() {
    static ContestRegistry registry;
    return registry;
}

void ContestRegistry::registerContest(
    const QString& id,
    const ContestMetadata& metadata,
    std::function<ContestBase*(ModeType)> factory)
{
    if (m_contests.contains(id)) {
        LOG_WARN("ContestRegistry", QString("Contest already registered: %1").arg(id));
        return;
    }
    
    ContestEntry entry;
    entry.metadata = metadata;
    entry.factory = factory;
    
    m_contests[id] = entry;

    LOG_DEBUG("ContestRegistry", QString("Registered contest: %1 - %2").arg(id, metadata.displayName));
}

QList<ContestMetadata> ContestRegistry::availableContests() const {
    QList<ContestMetadata> result;
    for (const ContestEntry& entry : m_contests) {
        result.append(entry.metadata);
    }
    return result;
}

QStringList ContestRegistry::availableContestIds() const {
    return m_contests.keys();
}

bool ContestRegistry::hasContest(const QString& id) const {
    return m_contests.contains(id);
}

ContestMetadata ContestRegistry::getMetadata(const QString& id) const {
    if (m_contests.contains(id)) {
        return m_contests[id].metadata;
    }
    return ContestMetadata();
}

ContestBase* ContestRegistry::createContest(const QString& id, ModeType mode) {
    if (!m_contests.contains(id)) {
        LOG_WARN("ContestRegistry", QString("Contest not found: %1").arg(id));
        return nullptr;
    }

    return m_contests[id].factory(mode);
}

void ContestRegistry::printRegistry() const {
    LOG_DEBUG("ContestRegistry", "=== Contest Registry ===");
    LOG_DEBUG("ContestRegistry", QString("Total contests: %1").arg(m_contests.size()));
    for (const QString& id : m_contests.keys()) {
        const ContestMetadata& meta = m_contests[id].metadata;
        LOG_DEBUG("ContestRegistry", QString("  - %1: %2").arg(id, meta.displayName));
        LOG_DEBUG("ContestRegistry", QString("    Modes: %1").arg(meta.supportedModes.size()));
        LOG_DEBUG("ContestRegistry", QString("    Website: %1").arg(meta.website));
    }
}

} // namespace TR4QT
