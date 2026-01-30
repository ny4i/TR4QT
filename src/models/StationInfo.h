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

#ifndef STATIONINFO_H
#define STATIONINFO_H

#include <QString>

namespace TR4QT {

/**
 * Information about the operating station
 *
 * This encapsulates all station-specific data needed by contest scoring
 * and validation logic. Passed to contest methods instead of individual
 * parameters for better modularity and extensibility.
 *
 * Usage:
 *   StationInfo myStation;
 *   myStation.callsign = "W1AW";
 *   myStation.country = "United States";
 *   // etc.
 *
 *   int points = contest->calculateQSOPoints(qso, myStation);
 */
struct StationInfo {
    // Operating callsign
    QString callsign;

    // Geographic location
    QString country;            // Country name (e.g., "United States")
    QString dxccPrefix;         // DXCC prefix (e.g., "K")
    int dxccEntity{0};          // DXCC entity number
    QString continent;          // Continent code (NA, SA, EU, AF, AS, OC)
    int cqZone{0};              // CQ Zone (1-40)
    int ituZone{0};             // ITU Zone
    double latitude{0.0};       // Latitude (decimal degrees)
    double longitude{0.0};      // Longitude (decimal degrees, + = West)

    // Administrative divisions (if applicable)
    QString state;              // US state or Canadian province
    QString county;             // US county (for QSO parties)
    QString section;            // ARRL/RAC section (for WFD, etc.)

    // Grid locator
    QString grid;               // Maidenhead grid square (e.g., "FN31pr")

    // Contest-specific info
    QString contestClass;       // Class for WFD (e.g., "3O")
    int power{100};             // Power level in watts (default: 100W = High power)
                                // TODO: Add Power field to Preferences → Station Info UI
                                // TODO: Add getMyPower()/setMyPower() to AppSettings

    /**
     * Check if basic station info is configured
     */
    bool isValid() const {
        return !callsign.isEmpty() &&
               !country.isEmpty() &&
               !continent.isEmpty() &&
               cqZone > 0;
    }

    /**
     * Load from application settings
     * (Will be implemented when AppSettings is extended)
     */
    static StationInfo fromSettings();

    /**
     * Save to application settings
     */
    void saveToSettings() const;
};

} // namespace TR4QT

#endif // STATIONINFO_H
