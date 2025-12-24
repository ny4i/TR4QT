#ifndef CONTESTMETADATA_H
#define CONTESTMETADATA_H

#include <QString>
#include <QList>
#include <functional>
#include "../core/Types.h"

namespace TR4QT {

class ContestBase;

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
    QString website;                 // Official contest website URL
    QString description;             // Brief description
    
    // Helper methods
    bool isSSBMode(ModeType mode) const {
        return mode == ModeType::USB || mode == ModeType::LSB;
    }

    int getWA7BNMId(ModeType mode) const {
        if (mode == ModeType::CW) return wa7bnmIdCW;
        if (isSSBMode(mode)) return wa7bnmIdSSB;
        return wa7bnmIdMixed;
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
};

/**
 * Contest entry in registry
 * Combines metadata with factory function
 */
struct ContestEntry {
    ContestMetadata metadata;
    std::function<ContestBase*(ModeType)> factory;
};

} // namespace TR4QT

#endif // CONTESTMETADATA_H
