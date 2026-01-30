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

#ifndef CONTESTMETADATA_H
#define CONTESTMETADATA_H

#include <QString>
#include <QList>
#include <QDate>
#include <functional>
#include "../core/Types.h"
#include "../models/StationInfo.h"

namespace TR4QT {

class ContestBase;

/**
 * Floating date specification for contests
 * Supports rules like "2nd Saturday", "3rd full weekend", "Last full weekend"
 *
 * IMPORTANT: Floating dates are used for UI SORTING ONLY (to show upcoming
 * contests first in the selection dropdown). They do NOT enforce contest
 * activation dates - users can select and activate any contest at any time,
 * regardless of the contest's scheduled dates. This allows users to:
 * - Operate contests that changed their schedule
 * - Practice logging outside contest windows
 * - Log historical contests with flexible dates
 */
struct FloatingDate {
    int month;                  // 1-12
    QString rule;               // "2nd Saturday", "3rd Saturday", "Last full weekend", etc.

    FloatingDate() : month(0) {}
    FloatingDate(int m, const QString& r) : month(m), rule(r) {}

    /**
     * Calculate next occurrence of this floating date from a given reference date
     * @param fromDate Reference date (defaults to today)
     * @return Next occurrence of this floating date, or invalid QDate if rule cannot be parsed
     */
    QDate calculateNextOccurrence(const QDate& fromDate = QDate::currentDate()) const;

    bool isValid() const { return month >= 1 && month <= 12 && !rule.isEmpty(); }

private:
    QDate calculateOccurrenceInMonth(const QDate& firstOfMonth,
                                    const QString& ordinal,
                                    const QString& target) const;
    QDate calculateWeekday(const QDate& firstOfMonth,
                          const QString& ordinal,
                          Qt::DayOfWeek targetDay) const;
    QDate calculateFullWeekend(const QDate& firstOfMonth,
                              const QString& ordinal) const;
};

/**
 * Contest metadata for factory registration
 * Each contest provides this information for UI display and creation
 */
struct ContestMetadata {
    // Identification
    QString id;                      // Unique factory ID (e.g., "CQWW", "CQWPX")
    QString displayName;             // Human-readable name (e.g., "CQ World Wide DX Contest")
    QString shortName;               // Short name for UI (e.g., "CQ WW")
    
    // Mode support
    QList<ModeType> supportedModes;  // Modes this contest supports
    bool hasSeparateContests;        // true if CW/SSB are separate contests
    
    // Contest identifiers (for exports)
    int wa7bnmIdCW;                  // WA7BNM Contest Calendar ID for CW
    int wa7bnmIdSSB;                 // WA7BNM Contest Calendar ID for SSB
    int wa7bnmIdMixed;               // WA7BNM Contest Calendar ID for mixed (0 if N/A)
    
    QString cabrilloNameCW;          // Cabrillo contest name for CW
    QString cabrilloNameSSB;         // Cabrillo contest name for SSB
    QString cabrilloNameMixed;       // Cabrillo contest name for mixed
    
    QString adifContestIdCW;         // ADIF Contest-ID for CW
    QString adifContestIdSSB;        // ADIF Contest-ID for SSB
    QString adifContestIdMixed;      // ADIF Contest-ID for mixed
    
    // Contest information
    QString schedule;                // When it runs (e.g., "Last full weekend of November")
    QList<FloatingDate> floatingDates; // Calculated contest dates (can be multiple per year)
    QString website;                 // Official contest website URL
    QString description;             // Brief description

    // Constants
    static constexpr const char* WA7BNM_BASE_URL = "https://www.contestcalendar.com/contestdetails.php?ref=";

    // Helper methods
    bool isSSBMode(ModeType mode) const {
        return mode == ModeType::USB || mode == ModeType::LSB;
    }

    int getWA7BNMId(ModeType mode) const {
        if (mode == ModeType::CW) return wa7bnmIdCW;
        if (isSSBMode(mode)) return wa7bnmIdSSB;
        return wa7bnmIdMixed;
    }

    /**
     * Get WA7BNM Contest Calendar URL for this contest
     * @param mode Contest mode (CW, SSB, or Mixed)
     * @return Full URL to contest details, or empty string if no WA7BNM ID
     */
    QString getWA7BNMUrl(ModeType mode) const {
        int id = getWA7BNMId(mode);
        if (id == 0) return QString();
        return QString("%1%2").arg(WA7BNM_BASE_URL).arg(id);
    }

    QString getCabrilloName(ModeType mode) const {
        if (mode == ModeType::CW) return cabrilloNameCW;
        if (isSSBMode(mode)) return cabrilloNameSSB;
        return cabrilloNameMixed;
    }

    QString getADIFContestId(ModeType mode) const {
        if (mode == ModeType::CW) return adifContestIdCW;
        if (isSSBMode(mode)) return adifContestIdSSB;
        return adifContestIdMixed;
    }

    QString getDisplayName(ModeType mode) const {
        if (!hasSeparateContests || mode == ModeType::None) {
            return displayName;
        }
        QString modeStr = (mode == ModeType::CW) ? "CW" :
                         isSSBMode(mode) ? "SSB" : modeToString(mode);
        return QString("%1 (%2)").arg(displayName).arg(modeStr);
    }

    /**
     * Get next occurrence of this contest
     * Returns the earliest date from all floating dates
     * @param fromDate Reference date (defaults to today)
     * @return Next occurrence, or invalid QDate if no floating dates defined
     */
    QDate getNextOccurrence(const QDate& fromDate = QDate::currentDate()) const {
        QDate nextDate;
        for (const FloatingDate& fd : floatingDates) {
            if (!fd.isValid()) continue;
            QDate candidate = fd.calculateNextOccurrence(fromDate);
            if (candidate.isValid() && (!nextDate.isValid() || candidate < nextDate)) {
                nextDate = candidate;
            }
        }
        return nextDate;
    }
};

/**
 * Contest entry in registry
 * Combines metadata with factory function
 */
struct ContestEntry {
    ContestMetadata metadata;
    std::function<ContestBase*(ModeType, const StationInfo&)> factory;
};

} // namespace TR4QT

#endif // CONTESTMETADATA_H
