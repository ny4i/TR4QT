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

#include "GeographicUtils.h"
#include "CountryFile.h"
#include "../logging/LogMacros.h"
#include <cmath>

namespace TR4QT {

double GeographicUtils::degToRad(double degrees) {
    return degrees * M_PI / 180.0;
}

double GeographicUtils::radToDeg(double radians) {
    return radians * 180.0 / M_PI;
}

bool GeographicUtils::gridToLatLon(const QString& grid, double& lat, double& lon) {
    // Validate length (must be 4 or 6 characters)
    if (grid.length() != 4 && grid.length() != 6) {
        LOG_WARN("GeographicUtils", QString("Invalid grid length: %1 (must be 4 or 6)").arg(grid));
        return false;
    }

    QString gridUpper = grid.toUpper();

    // Parse field (first 2 characters: AA-RR)
    QChar field1 = gridUpper[0];
    QChar field2 = gridUpper[1];
    if (!field1.isLetter() || !field2.isLetter() ||
        field1 < 'A' || field1 > 'R' || field2 < 'A' || field2 > 'R') {
        LOG_WARN("GeographicUtils", QString("Invalid field characters: %1%2").arg(field1).arg(field2));
        return false;
    }

    // Parse square (characters 3-4: 00-99)
    QChar square1 = gridUpper[2];
    QChar square2 = gridUpper[3];
    if (!square1.isDigit() || !square2.isDigit()) {
        LOG_WARN("GeographicUtils", QString("Invalid square characters: %1%2").arg(square1).arg(square2));
        return false;
    }

    // Calculate longitude from field and square
    lon = -180.0 + (field1.toLatin1() - 'A') * 20.0 + (square1.toLatin1() - '0') * 2.0;

    // Calculate latitude from field and square
    lat = -90.0 + (field2.toLatin1() - 'A') * 10.0 + (square2.toLatin1() - '0') * 1.0;

    // If 6-character grid, add subsquare precision
    if (gridUpper.length() == 6) {
        QChar subsquare1 = gridUpper[4].toLower();
        QChar subsquare2 = gridUpper[5].toLower();

        if (!subsquare1.isLetter() || !subsquare2.isLetter() ||
            subsquare1 < 'a' || subsquare1 > 'x' || subsquare2 < 'a' || subsquare2 > 'x') {
            LOG_WARN("GeographicUtils", QString("Invalid subsquare characters: %1%2").arg(subsquare1).arg(subsquare2));
            return false;
        }

        lon += (subsquare1.toLatin1() - 'a') * (2.0 / 24.0);
        lat += (subsquare2.toLatin1() - 'a') * (1.0 / 24.0);
    }

    // Add offset to get center of grid square
    if (gridUpper.length() == 4) {
        lon += 1.0;  // Center of 2-degree square
        lat += 0.5;  // Center of 1-degree square
    } else {
        lon += (2.0 / 24.0) / 2.0;  // Center of subsquare
        lat += (1.0 / 24.0) / 2.0;
    }

    LOG_DEBUG("GeographicUtils", QString("Grid %1 -> lat=%2, lon=%3").arg(grid).arg(lat).arg(lon));
    return true;
}

double GeographicUtils::haversineDistance(double lat1, double lon1, double lat2, double lon2, bool useKilometers) {
    // Convert to radians
    double lat1Rad = degToRad(lat1);
    double lon1Rad = degToRad(lon1);
    double lat2Rad = degToRad(lat2);
    double lon2Rad = degToRad(lon2);

    // Haversine formula
    double dLat = lat2Rad - lat1Rad;
    double dLon = lon2Rad - lon1Rad;

    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1Rad) * std::cos(lat2Rad) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    double radius = useKilometers ? EARTH_RADIUS_KM : EARTH_RADIUS_MILES;
    double distance = radius * c;

    return distance;
}

double GeographicUtils::calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    // Convert to radians
    double lat1Rad = degToRad(lat1);
    double lon1Rad = degToRad(lon1);
    double lat2Rad = degToRad(lat2);
    double lon2Rad = degToRad(lon2);

    double dLon = lon2Rad - lon1Rad;

    // Calculate bearing using forward azimuth formula
    double y = std::sin(dLon) * std::cos(lat2Rad);
    double x = std::cos(lat1Rad) * std::sin(lat2Rad) -
               std::sin(lat1Rad) * std::cos(lat2Rad) * std::cos(dLon);

    double bearingRad = std::atan2(y, x);
    double bearingDeg = radToDeg(bearingRad);

    // Normalize to 0-360
    bearingDeg = std::fmod(bearingDeg + 360.0, 360.0);

    return bearingDeg;
}

double GeographicUtils::distanceBetweenGrids(const QString& grid1, const QString& grid2, bool useKilometers) {
    double lat1, lon1, lat2, lon2;

    if (!gridToLatLon(grid1, lat1, lon1)) {
        LOG_WARN("GeographicUtils", QString("Failed to convert grid1: %1").arg(grid1));
        return -1.0;
    }

    if (!gridToLatLon(grid2, lat2, lon2)) {
        LOG_WARN("GeographicUtils", QString("Failed to convert grid2: %1").arg(grid2));
        return -1.0;
    }

    double distance = haversineDistance(lat1, lon1, lat2, lon2, useKilometers);

    LOG_DEBUG("GeographicUtils", QString("Distance %1 -> %2: %3 %4")
        .arg(grid1).arg(grid2).arg(distance, 0, 'f', 1).arg(useKilometers ? "km" : "mi"));

    return distance;
}

bool GeographicUtils::distanceAndBearingBetweenCallsigns(const QString& callsign1,
                                                         const QString& callsign2,
                                                         double& distance,
                                                         double& bearing,
                                                         bool useKilometers) {
    // Load country file (deprecated - use overload with CountryFile& for better performance)
    CountryFile countryFile;

    // Delegate to the overload that accepts a CountryFile reference
    return distanceAndBearingBetweenCallsigns(countryFile, callsign1, callsign2, distance, bearing, useKilometers);
}

bool GeographicUtils::distanceAndBearingBetweenCallsigns(const CountryFile& countryFile,
                                                         const QString& callsign1,
                                                         const QString& callsign2,
                                                         double& distance,
                                                         double& bearing,
                                                         bool useKilometers) {
    // Look up countries for both callsigns
    CountryData country1 = countryFile.lookup(callsign1);
    CountryData country2 = countryFile.lookup(callsign2);

    if (!country1.isValid()) {
        LOG_WARN("GeographicUtils", QString("Could not determine country for callsign: %1").arg(callsign1));
        return false;
    }

    if (!country2.isValid()) {
        LOG_WARN("GeographicUtils", QString("Could not determine country for callsign: %1").arg(callsign2));
        return false;
    }

    // Calculate distance and bearing
    distance = haversineDistance(country1.latitude, country1.longitude,
                                 country2.latitude, country2.longitude,
                                 useKilometers);

    bearing = calculateBearing(country1.latitude, country1.longitude,
                               country2.latitude, country2.longitude);

    LOG_DEBUG("GeographicUtils", QString("Distance %1 (%2) -> %3 (%4): %5 %6, bearing %7°")
        .arg(callsign1).arg(country1.name)
        .arg(callsign2).arg(country2.name)
        .arg(distance, 0, 'f', 1)
        .arg(useKilometers ? "km" : "mi")
        .arg(bearing, 0, 'f', 1));

    return true;
}

QTime GeographicUtils::calculateSunrise(double lat, double lon, const QDate& date) {
    // NOAA sunrise equation algorithm
    // Solar zenith angle: 90.833° (official NOAA/US Naval Observatory standard)
    const double ZENITH = 90.833;

    // Calculate day of year
    int dayOfYear = date.dayOfYear();

    // Convert longitude to hour value and calculate approximate time
    double lngHour = lon / 15.0;
    double t = dayOfYear + ((6.0 - lngHour) / 24.0);

    // Calculate Sun's mean anomaly
    double M = (0.9856 * t) - 3.289;

    // Calculate Sun's true longitude
    double L = M + (1.916 * std::sin(degToRad(M))) + (0.020 * std::sin(degToRad(2 * M))) + 282.634;

    // Normalize L to 0-360
    while (L < 0) L += 360.0;
    while (L >= 360.0) L -= 360.0;

    // Calculate Sun's right ascension
    double RA = radToDeg(std::atan(0.91764 * std::tan(degToRad(L))));

    // Normalize RA to 0-360
    while (RA < 0) RA += 360.0;
    while (RA >= 360.0) RA -= 360.0;

    // Right ascension value needs to be in same quadrant as L
    double Lquadrant = std::floor(L / 90.0) * 90.0;
    double RAquadrant = std::floor(RA / 90.0) * 90.0;
    RA = RA + (Lquadrant - RAquadrant);

    // Convert RA to hours
    RA = RA / 15.0;

    // Calculate Sun's declination
    double sinDec = 0.39782 * std::sin(degToRad(L));
    double cosDec = std::cos(std::asin(sinDec));

    // Calculate Sun's local hour angle
    double cosH = (std::cos(degToRad(ZENITH)) - (sinDec * std::sin(degToRad(lat)))) /
                  (cosDec * std::cos(degToRad(lat)));

    // Check if sun rises at this location on this date
    if (cosH > 1.0) {
        // Sun never rises (polar winter)
        LOG_WARN("GeographicUtils", QString("Sun never rises at lat=%1 on date=%2")
            .arg(lat).arg(date.toString("yyyy-MM-dd")));
        return QTime();  // Invalid time
    }
    if (cosH < -1.0) {
        // Sun never sets (polar summer) - return midnight for sunrise
        return QTime(0, 0, 0);
    }

    // Calculate hour angle for sunrise (sun rising in east)
    double H = 360.0 - radToDeg(std::acos(cosH));
    H = H / 15.0;  // Convert to hours

    // Calculate local mean time of sunrise
    double T = H + RA - (0.06571 * t) - 6.622;

    // Adjust back to UTC
    double UT = T - lngHour;

    // Normalize UT to 0-24
    while (UT < 0.0) UT += 24.0;
    while (UT >= 24.0) UT -= 24.0;

    // Convert to QTime
    int hours = static_cast<int>(UT);
    int minutes = static_cast<int>((UT - hours) * 60.0);
    int seconds = static_cast<int>(((UT - hours) * 60.0 - minutes) * 60.0);

    QTime sunrise(hours, minutes, seconds);

    LOG_DEBUG("GeographicUtils", QString("Sunrise at lat=%1, lon=%2, date=%3: %4 UTC")
        .arg(lat).arg(lon).arg(date.toString("yyyy-MM-dd")).arg(sunrise.toString("HH:mm:ss")));

    return sunrise;
}

QTime GeographicUtils::calculateSunset(double lat, double lon, const QDate& date) {
    // NOAA sunrise equation algorithm (same as sunrise, but different hour angle)
    // Solar zenith angle: 90.833° (official NOAA/US Naval Observatory standard)
    const double ZENITH = 90.833;

    // Calculate day of year
    int dayOfYear = date.dayOfYear();

    // Convert longitude to hour value and calculate approximate time
    double lngHour = lon / 15.0;
    double t = dayOfYear + ((18.0 - lngHour) / 24.0);  // 18.0 for sunset instead of 6.0

    // Calculate Sun's mean anomaly
    double M = (0.9856 * t) - 3.289;

    // Calculate Sun's true longitude
    double L = M + (1.916 * std::sin(degToRad(M))) + (0.020 * std::sin(degToRad(2 * M))) + 282.634;

    // Normalize L to 0-360
    while (L < 0) L += 360.0;
    while (L >= 360.0) L -= 360.0;

    // Calculate Sun's right ascension
    double RA = radToDeg(std::atan(0.91764 * std::tan(degToRad(L))));

    // Normalize RA to 0-360
    while (RA < 0) RA += 360.0;
    while (RA >= 360.0) RA -= 360.0;

    // Right ascension value needs to be in same quadrant as L
    double Lquadrant = std::floor(L / 90.0) * 90.0;
    double RAquadrant = std::floor(RA / 90.0) * 90.0;
    RA = RA + (Lquadrant - RAquadrant);

    // Convert RA to hours
    RA = RA / 15.0;

    // Calculate Sun's declination
    double sinDec = 0.39782 * std::sin(degToRad(L));
    double cosDec = std::cos(std::asin(sinDec));

    // Calculate Sun's local hour angle
    double cosH = (std::cos(degToRad(ZENITH)) - (sinDec * std::sin(degToRad(lat)))) /
                  (cosDec * std::cos(degToRad(lat)));

    // Check if sun sets at this location on this date
    if (cosH > 1.0) {
        // Sun never rises (polar winter) - return midnight for sunset
        return QTime(23, 59, 59);
    }
    if (cosH < -1.0) {
        // Sun never sets (polar summer)
        LOG_WARN("GeographicUtils", QString("Sun never sets at lat=%1 on date=%2")
            .arg(lat).arg(date.toString("yyyy-MM-dd")));
        return QTime();  // Invalid time
    }

    // Calculate hour angle for sunset (sun setting in west)
    double H = radToDeg(std::acos(cosH));
    H = H / 15.0;  // Convert to hours

    // Calculate local mean time of sunset
    double T = H + RA - (0.06571 * t) - 6.622;

    // Adjust back to UTC
    double UT = T - lngHour;

    // Normalize UT to 0-24
    while (UT < 0.0) UT += 24.0;
    while (UT >= 24.0) UT -= 24.0;

    // Convert to QTime
    int hours = static_cast<int>(UT);
    int minutes = static_cast<int>((UT - hours) * 60.0);
    int seconds = static_cast<int>(((UT - hours) * 60.0 - minutes) * 60.0);

    QTime sunset(hours, minutes, seconds);

    LOG_DEBUG("GeographicUtils", QString("Sunset at lat=%1, lon=%2, date=%3: %4 UTC")
        .arg(lat).arg(lon).arg(date.toString("yyyy-MM-dd")).arg(sunset.toString("HH:mm:ss")));

    return sunset;
}

bool GeographicUtils::isInGraylineWindow(const QDateTime& currentTime,
                                         const QTime& sunrise,
                                         const QTime& sunset,
                                         int windowMinutes) {
    // Extract time from QDateTime
    QTime currentTimeOnly = currentTime.time();

    // Check if sunrise or sunset are invalid
    if (!sunrise.isValid() || !sunset.isValid()) {
        return false;
    }

    // Calculate window in seconds
    const int WINDOW_SECONDS = windowMinutes * 60;

    // Check if within window of sunrise
    int secondsToSunrise = currentTimeOnly.secsTo(sunrise);
    if (std::abs(secondsToSunrise) <= WINDOW_SECONDS) {
        LOG_DEBUG("GeographicUtils", QString("In grayline window (sunrise): %1 seconds from sunrise")
            .arg(secondsToSunrise));
        return true;
    }

    // Check if within window of sunset
    int secondsToSunset = currentTimeOnly.secsTo(sunset);
    if (std::abs(secondsToSunset) <= WINDOW_SECONDS) {
        LOG_DEBUG("GeographicUtils", QString("In grayline window (sunset): %1 seconds from sunset")
            .arg(secondsToSunset));
        return true;
    }

    // Handle midnight crossing for sunrise window (case 1)
    // If sunrise is near midnight (e.g., 23:45), and current time is early morning (e.g., 00:10)
    if (sunrise.hour() >= 23 && currentTimeOnly.hour() <= 1) {
        int secondsFromMidnight = QTime(0, 0, 0).secsTo(currentTimeOnly);
        int secondsSunriseFromMidnight = 86400 - QTime(0, 0, 0).secsTo(sunrise);  // Seconds before midnight
        if (secondsFromMidnight + secondsSunriseFromMidnight <= WINDOW_SECONDS) {
            LOG_DEBUG("GeographicUtils", "In grayline window (sunrise, midnight crossing case 1)");
            return true;
        }
    }

    // Handle midnight crossing for sunrise window (case 2)
    // If sunrise is early morning (e.g., 00:15), and current time is late night (e.g., 23:50)
    if (sunrise.hour() <= 1 && currentTimeOnly.hour() >= 23) {
        int secondsToMidnight = currentTimeOnly.secsTo(QTime(23, 59, 59));
        int secondsSunriseFromMidnight = QTime(0, 0, 0).secsTo(sunrise);
        if (secondsToMidnight + secondsSunriseFromMidnight <= WINDOW_SECONDS) {
            LOG_DEBUG("GeographicUtils", "In grayline window (sunrise, midnight crossing case 2)");
            return true;
        }
    }

    // Handle midnight crossing for sunset window
    // If sunset is early morning (e.g., 00:15), and current time is late night (e.g., 23:50)
    if (sunset.hour() <= 1 && currentTimeOnly.hour() >= 23) {
        int secondsToMidnight = currentTimeOnly.secsTo(QTime(23, 59, 59));
        int secondsSunsetFromMidnight = QTime(0, 0, 0).secsTo(sunset);
        if (secondsToMidnight + secondsSunsetFromMidnight <= WINDOW_SECONDS) {
            LOG_DEBUG("GeographicUtils", "In grayline window (sunset, midnight crossing)");
            return true;
        }
    }

    return false;
}

} // namespace TR4QT
