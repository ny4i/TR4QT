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

/**
 * @file StationInfoService.h
 * @brief Service for calculating station information (geographic, propagation)
 *
 * Phase 7 extraction from MainWindow.
 * Encapsulates geographic calculations, grayline detection, and station info formatting.
 */

#ifndef STATIONINFOSERVICE_H
#define STATIONINFOSERVICE_H

#include <QString>
#include <QDateTime>
#include <QSet>
#include <QList>
#include <functional>
#include "../utils/CountryFile.h"
#include "../core/Types.h"

namespace TR4QT {

class ContestBase;
class QSOTableModel;

/**
 * @brief Result of station information calculation
 */
struct StationInfoResult {
    bool valid = false;           ///< Whether lookup was successful
    QString countryName;          ///< Country name from CTY.DAT
    QString primaryPrefix;        ///< Primary prefix (e.g., "KP4", "HV")
    QString displayInfo;          ///< Formatted display string
    QString tooltip;              ///< Tooltip with grayline info

    // Geographic data
    double targetLat = 0.0;       ///< DX station latitude
    double targetLon = 0.0;       ///< DX station longitude
    double bearing = 0.0;         ///< Bearing from home to DX
    double distance = 0.0;        ///< Distance from home to DX

    // Grayline status
    bool homeInGrayline = false;  ///< Home station in grayline window
    bool dxInGrayline = false;    ///< DX station in grayline window
    bool doubleGrayline = false;  ///< Both stations in grayline (exceptional)
};

/**
 * @brief Result of SCP matches formatting
 */
struct SCPDisplayResult {
    QString htmlContent;          ///< HTML-formatted content for display
    int totalMatches = 0;         ///< Total number of matches
    int workedCount = 0;          ///< Number already worked
    int dupeCount = 0;            ///< Number that are dupes on current band/mode
};

/**
 * @brief Service for station information and geographic calculations
 *
 * Extracted from MainWindow Phase 7. Encapsulates:
 * - Geographic calculations (bearing, distance, sunrise/sunset)
 * - Grayline detection and status
 * - Station info display formatting
 * - SCP match display formatting
 */
class StationInfoService {
public:
    /**
     * @brief Construct service with required dependencies
     * @param countryFile Pointer to country file (for DXCC lookups)
     */
    explicit StationInfoService(const CountryFile* countryFile);

    /**
     * @brief Calculate station information for a callsign
     * @param callsign The callsign to look up
     * @param myGrid My station's grid square (e.g., "FN42")
     * @param useMetric True to use kilometers, false for miles
     * @return StationInfoResult with geographic and propagation data
     */
    StationInfoResult calculateStationInfo(
        const QString& callsign,
        const QString& myGrid,
        bool useMetric) const;

    /**
     * @brief Format SCP matches for display with color coding
     * @param matches List of callsign matches from SCP
     * @param workedCallsigns Set of callsigns already worked
     * @param currentBand Current operating band
     * @param currentMode Current operating mode
     * @param dupeChecker Function to check if a call is a dupe
     * @return SCPDisplayResult with formatted HTML
     */
    SCPDisplayResult formatSCPMatches(
        const QStringList& matches,
        const QSet<QString>& workedCallsigns,
        BandType currentBand,
        ModeType currentMode,
        std::function<bool(const QString&, BandType, ModeType, QString&)> dupeChecker) const;

    /**
     * @brief Get multiplier value for a callsign (using contest rules)
     * @param callsign The callsign to evaluate
     * @param contest The active contest (for multiplier definitions)
     * @return Multiplier value string, or empty if none
     */
    QString getMultiplierValueForCallsign(
        const QString& callsign,
        const ContestBase* contest) const;

    // Constants
    static constexpr int GRAYLINE_WINDOW_MINUTES = 30;

private:
    const CountryFile* m_countryFile;
};

} // namespace TR4QT

#endif // STATIONINFOSERVICE_H
