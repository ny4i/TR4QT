#ifndef CONTESTREGISTRY_H
#define CONTESTREGISTRY_H

#include "ContestMetadata.h"
#include "ContestBase.h"
#include <QMap>
#include <QString>
#include <QList>
#include <functional>

namespace TR4QT {

class ContestRegistry {
public:
    static ContestRegistry& instance();
    
    void registerContest(
        const QString& id,
        const ContestMetadata& metadata,
        std::function<ContestBase*(ModeType, const StationInfo&)> factory
    );

    QList<ContestMetadata> availableContests() const;
    QStringList availableContestIds() const;

    bool hasContest(const QString& id) const;
    ContestMetadata getMetadata(const QString& id) const;

    ContestBase* createContest(const QString& id, ModeType mode, const StationInfo& myStation);
    
    void printRegistry() const;
    int count() const { return m_contests.size(); }
    
private:
    ContestRegistry() = default;
    ~ContestRegistry() = default;
    
    ContestRegistry(const ContestRegistry&) = delete;
    ContestRegistry& operator=(const ContestRegistry&) = delete;
    ContestRegistry(ContestRegistry&&) = delete;
    ContestRegistry& operator=(ContestRegistry&&) = delete;
    
    QMap<QString, ContestEntry> m_contests;
};

class ContestRegistrar {
public:
    ContestRegistrar(
        const QString& id,
        const ContestMetadata& metadata,
        std::function<ContestBase*(ModeType, const StationInfo&)> factory
    ) {
        ContestRegistry::instance().registerContest(id, metadata, factory);
    }
};

} // namespace TR4QT

#define CONTEST_REGISTRAR_NAME_IMPL(line) _contest_registrar_##line
#define CONTEST_REGISTRAR_NAME(line) CONTEST_REGISTRAR_NAME_IMPL(line)

#define REGISTER_CONTEST(FullClassName, Id) \
    namespace { \
        static TR4QT::ContestRegistrar CONTEST_REGISTRAR_NAME(__LINE__)( \
            Id, \
            FullClassName::getMetadata(), \
            &FullClassName::create \
        ); \
    }

#endif // CONTESTREGISTRY_H
