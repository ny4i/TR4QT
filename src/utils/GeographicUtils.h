#ifndef GEOGRAPHICUTILS_H
#define GEOGRAPHICUTILS_H

#include <QString>
#include <QTime>
#include <QDate>
#include <QDateTime>

namespace TR4QT {

class CountryFile;

/**
 * Geographic utility functions for distance and bearing calculations
 *
 * Provides calculations for:
 * - Maidenhead grid square distances
 * - Country-to-country distances and bearings (via CTY.DAT lookups)
 * - Great circle distance and bearing computations
 */
class GeographicUtils {
public:
    /**
     * Calculate distance between two Maidenhead grid squares
     *
     * Supports 4-character (e.g., "FN31") and 6-character (e.g., "FN31pr") formats
     *
     * @param grid1 First Maidenhead grid square
     * @param grid2 Second Maidenhead grid square
     * @param useKilometers If true, return kilometers; if false, return miles
     * @return Distance in km or miles, or -1.0 if grid squares are invalid
     */
    static double distanceBetweenGrids(const QString& grid1, const QString& grid2, bool useKilometers = true);

    /**
     * Calculate distance and bearing between two countries by callsign
     *
     * Uses CTY.DAT country file to look up lat/lon for each country
     *
     * @param callsign1 First callsign (country determined from prefix)
     * @param callsign2 Second callsign (country determined from prefix)
     * @param distance Output parameter for distance (km or miles based on useKilometers)
     * @param bearing Output parameter for great circle bearing from callsign1 to callsign2 (degrees)
     * @param useKilometers If true, return distance in km; if false, return miles
     * @return true if successful, false if either callsign's country cannot be determined
     */
    static bool distanceAndBearingBetweenCallsigns(const QString& callsign1,
                                                    const QString& callsign2,
                                                    double& distance,
                                                    double& bearing,
                                                    bool useKilometers = true);

    /**
     * Calculate distance and bearing between two countries by callsign (with CountryFile)
     *
     * Overload that accepts an existing CountryFile reference to avoid reloading cty.dat
     *
     * @param countryFile Reference to existing CountryFile instance
     * @param callsign1 First callsign (country determined from prefix)
     * @param callsign2 Second callsign (country determined from prefix)
     * @param distance Output parameter for distance (km or miles based on useKilometers)
     * @param bearing Output parameter for great circle bearing from callsign1 to callsign2 (degrees)
     * @param useKilometers If true, return distance in km; if false, return miles
     * @return true if successful, false if either callsign's country cannot be determined
     */
    static bool distanceAndBearingBetweenCallsigns(const CountryFile& countryFile,
                                                    const QString& callsign1,
                                                    const QString& callsign2,
                                                    double& distance,
                                                    double& bearing,
                                                    bool useKilometers = true);

    /**
     * Calculate great circle distance between two lat/lon points
     *
     * Uses Haversine formula for accurate distance calculation
     *
     * @param lat1 Latitude of first point (degrees, -90 to 90)
     * @param lon1 Longitude of first point (degrees, -180 to 180)
     * @param lat2 Latitude of second point (degrees, -90 to 90)
     * @param lon2 Longitude of second point (degrees, -180 to 180)
     * @param useKilometers If true, return km; if false, return miles
     * @return Distance in km or miles
     */
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2, bool useKilometers = true);

    /**
     * Calculate great circle bearing from point 1 to point 2
     *
     * @param lat1 Latitude of first point (degrees)
     * @param lon1 Longitude of first point (degrees)
     * @param lat2 Latitude of second point (degrees)
     * @param lon2 Longitude of second point (degrees)
     * @return Bearing in degrees (0-360, where 0 = North, 90 = East, etc.)
     */
    static double calculateBearing(double lat1, double lon1, double lat2, double lon2);

    /**
     * Convert Maidenhead grid square to latitude/longitude
     *
     * Supports 4-character (FN31) and 6-character (FN31pr) formats
     * Returns center point of the grid square
     *
     * @param grid Maidenhead grid square (4 or 6 characters)
     * @param lat Output parameter for latitude
     * @param lon Output parameter for longitude
     * @return true if conversion successful, false if invalid grid format
     */
    static bool gridToLatLon(const QString& grid, double& lat, double& lon);

    /**
     * Calculate sunrise time for a given location and date
     *
     * Uses NOAA sunrise equation algorithm for accurate calculation
     *
     * @param lat Latitude in degrees (-90 to 90)
     * @param lon Longitude in degrees (-180 to 180)
     * @param date Date for calculation
     * @return Sunrise time in UTC, or invalid QTime if calculation fails
     */
    static QTime calculateSunrise(double lat, double lon, const QDate& date);

    /**
     * Calculate sunset time for a given location and date
     *
     * Uses NOAA sunrise equation algorithm for accurate calculation
     *
     * @param lat Latitude in degrees (-90 to 90)
     * @param lon Longitude in degrees (-180 to 180)
     * @param date Date for calculation
     * @return Sunset time in UTC, or invalid QTime if calculation fails
     */
    static QTime calculateSunset(double lat, double lon, const QDate& date);

    /**
     * Check if current time is within grayline window (±30 min of sunrise/sunset)
     *
     * Grayline propagation window occurs around sunrise and sunset transitions
     *
     * @param currentTime Current UTC time to check
     * @param sunrise Sunrise time in UTC
     * @param sunset Sunset time in UTC
     * @param windowMinutes Window size in minutes (default: 30)
     * @return true if currentTime is within window of sunrise or sunset
     */
    static bool isInGraylineWindow(const QDateTime& currentTime,
                                   const QTime& sunrise,
                                   const QTime& sunset,
                                   int windowMinutes = 30);

private:
    // Earth radius in kilometers and miles
    static constexpr double EARTH_RADIUS_KM = 6371.0;
    static constexpr double EARTH_RADIUS_MILES = 3959.0;

    // Helper: Convert degrees to radians
    static double degToRad(double degrees);

    // Helper: Convert radians to degrees
    static double radToDeg(double radians);
};

} // namespace TR4QT

#endif // GEOGRAPHICUTILS_H
