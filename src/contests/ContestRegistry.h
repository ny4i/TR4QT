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
