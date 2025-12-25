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
    // Load country file
    CountryFile countryFile;

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

} // namespace TR4QT
