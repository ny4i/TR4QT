#include "ContestRegistry.h"
#include <QDebug>

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
        qWarning() << "Contest already registered:" << id;
        return;
    }
    
    ContestEntry entry;
    entry.metadata = metadata;
    entry.factory = factory;
    
    m_contests[id] = entry;
    
    qDebug() << "Registered contest:" << id << "-" << metadata.displayName;
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
        qWarning() << "Contest not found:" << id;
        return nullptr;
    }
    
    return m_contests[id].factory(mode);
}

void ContestRegistry::printRegistry() const {
    qDebug() << "=== Contest Registry ===";
    qDebug() << "Total contests:" << m_contests.size();
    for (const QString& id : m_contests.keys()) {
        const ContestMetadata& meta = m_contests[id].metadata;
        qDebug() << "  -" << id << ":" << meta.displayName;
        qDebug() << "    Modes:" << meta.supportedModes.size();
        qDebug() << "    Website:" << meta.website;
    }
}

} // namespace TR4QT
