#include <QTest>
#include "../src/utils/GeographicUtils.h"

using namespace TR4QT;

/**
 * Unit tests for GeographicUtils
 * Tests Maidenhead grid conversions, distance calculations, and bearing computations
 */
class TestGeographicUtils : public QObject {
    Q_OBJECT

private slots:
    // Grid to Lat/Lon conversion tests
    void testGridToLatLon_FourCharacter();
    void testGridToLatLon_SixCharacter();
    void testGridToLatLon_InvalidLength();
    void testGridToLatLon_InvalidFieldCharacters();
    void testGridToLatLon_InvalidSquareCharacters();
    void testGridToLatLon_InvalidSubsquareCharacters();

    // Haversine distance tests
    void testHaversineDistance_SamePoint();
    void testHaversineDistance_Equator();
    void testHaversineDistance_Antipodes();
    void testHaversineDistance_KilometersVsMiles();

    // Bearing calculation tests
    void testCalculateBearing_North();
    void testCalculateBearing_East();
    void testCalculateBearing_South();
    void testCalculateBearing_West();

    // Grid square distance tests
    void testDistanceBetweenGrids_SameGrid();
    void testDistanceBetweenGrids_AdjacentGrids();
    void testDistanceBetweenGrids_KnownDistance();
    void testDistanceBetweenGrids_InvalidGrid1();
    void testDistanceBetweenGrids_InvalidGrid2();

    // Callsign distance and bearing tests
    void testDistanceAndBearingBetweenCallsigns_ValidCallsigns();
    void testDistanceAndBearingBetweenCallsigns_SameCountry();
    void testDistanceAndBearingBetweenCallsigns_InvalidCallsign1();
    void testDistanceAndBearingBetweenCallsigns_InvalidCallsign2();

    // Sunrise/sunset calculation tests
    void testCalculateSunrise_KnownLocation();
    void testCalculateSunset_KnownLocation();
    void testCalculateSunrise_PolarRegion();
    void testCalculateSunset_PolarRegion();
    void testCalculateSunrise_Equator();
    void testCalculateSunset_Equator();

    // Grayline window tests
    void testIsInGraylineWindow_BeforeSunrise();
    void testIsInGraylineWindow_AtSunrise();
    void testIsInGraylineWindow_AfterSunrise();
    void testIsInGraylineWindow_BeforeSunset();
    void testIsInGraylineWindow_AtSunset();
    void testIsInGraylineWindow_AfterSunset();
    void testIsInGraylineWindow_MidnightCrossing();
    void testIsInGraylineWindow_InvalidTimes();
};

// Grid to Lat/Lon conversion tests

void TestGeographicUtils::testGridToLatLon_FourCharacter() {
    double lat, lon;

    // FN31 - Hartford, CT area
    // F(5) N(13) 3 1 -> lon=-180+5*20+3*2+1=-73, lat=-90+13*10+1*1+0.5=41.5
    QVERIFY(GeographicUtils::gridToLatLon("FN31", lat, lon));
    QCOMPARE(qRound(lat * 10) / 10.0, 41.5);  // ~41.5°N
    QCOMPARE(qRound(lon * 10) / 10.0, -73.0); // ~-73.0°W (corrected)

    // JO01 - UK (Greenwich area)
    // J(9) O(14) 0 1 -> lon=-180+9*20+0*2+1=1, lat=-90+14*10+1*1+0.5=51.5
    QVERIFY(GeographicUtils::gridToLatLon("JO01", lat, lon));
    QCOMPARE(qRound(lat * 10) / 10.0, 51.5);  // ~51.5°N
    QCOMPARE(qRound(lon * 10) / 10.0, 1.0);   // ~1.0°E (Greenwich)

    // Case insensitive
    QVERIFY(GeographicUtils::gridToLatLon("fn31", lat, lon));
    QCOMPARE(qRound(lat * 10) / 10.0, 41.5);
}

void TestGeographicUtils::testGridToLatLon_SixCharacter() {
    double lat, lon;

    // FN31pr - Hartford, CT (6-char precision)
    QVERIFY(GeographicUtils::gridToLatLon("FN31pr", lat, lon));
    QVERIFY(lat >= 41.0 && lat <= 42.0);   // Should be in 1-degree band
    QVERIFY(lon >= -73.0 && lon <= -71.0); // Should be in 2-degree band

    // Case insensitive
    QVERIFY(GeographicUtils::gridToLatLon("fn31PR", lat, lon));
    QVERIFY(lat >= 41.0 && lat <= 42.0);
}

void TestGeographicUtils::testGridToLatLon_InvalidLength() {
    double lat, lon;

    // Too short
    QVERIFY(!GeographicUtils::gridToLatLon("FN3", lat, lon));

    // Too long
    QVERIFY(!GeographicUtils::gridToLatLon("FN31prx", lat, lon));

    // 5 characters (not valid)
    QVERIFY(!GeographicUtils::gridToLatLon("FN31p", lat, lon));
}

void TestGeographicUtils::testGridToLatLon_InvalidFieldCharacters() {
    double lat, lon;

    // Field out of range (S > R)
    QVERIFY(!GeographicUtils::gridToLatLon("SN31", lat, lon));

    // Non-letter field characters
    QVERIFY(!GeographicUtils::gridToLatLon("F131", lat, lon));
    QVERIFY(!GeographicUtils::gridToLatLon("1N31", lat, lon));
}

void TestGeographicUtils::testGridToLatLon_InvalidSquareCharacters() {
    double lat, lon;

    // Non-digit square characters
    QVERIFY(!GeographicUtils::gridToLatLon("FNA1", lat, lon));
    QVERIFY(!GeographicUtils::gridToLatLon("FN3B", lat, lon));
}

void TestGeographicUtils::testGridToLatLon_InvalidSubsquareCharacters() {
    double lat, lon;

    // Subsquare out of range (y > x)
    QVERIFY(!GeographicUtils::gridToLatLon("FN31pz", lat, lon));

    // Non-letter subsquare
    QVERIFY(!GeographicUtils::gridToLatLon("FN31p1", lat, lon));
}

// Haversine distance tests

void TestGeographicUtils::testHaversineDistance_SamePoint() {
    // Distance from a point to itself should be 0
    double distance = GeographicUtils::haversineDistance(41.7658, -72.6734,
                                                         41.7658, -72.6734,
                                                         true);
    QCOMPARE(qRound(distance), 0);
}

void TestGeographicUtils::testHaversineDistance_Equator() {
    // Distance along equator (0°, 0°) to (0°, 1°)
    // Should be approximately 111 km (1 degree at equator)
    double distance = GeographicUtils::haversineDistance(0.0, 0.0, 0.0, 1.0, true);
    QVERIFY(distance >= 110.0 && distance <= 112.0);
}

void TestGeographicUtils::testHaversineDistance_Antipodes() {
    // Distance from (0°, 0°) to (0°, 180°) should be half Earth's circumference
    // ~20,000 km
    double distance = GeographicUtils::haversineDistance(0.0, 0.0, 0.0, 180.0, true);
    QVERIFY(distance >= 19900.0 && distance <= 20100.0);
}

void TestGeographicUtils::testHaversineDistance_KilometersVsMiles() {
    // Hartford to NYC (~100 miles / ~161 km)
    double distanceKm = GeographicUtils::haversineDistance(41.7658, -72.6734,
                                                           40.7128, -74.0060,
                                                           true);
    double distanceMi = GeographicUtils::haversineDistance(41.7658, -72.6734,
                                                           40.7128, -74.0060,
                                                           false);

    // Verify conversion factor (1 mile = ~1.609 km)
    double ratio = distanceKm / distanceMi;
    QVERIFY(ratio >= 1.60 && ratio <= 1.62);
}

// Bearing calculation tests

void TestGeographicUtils::testCalculateBearing_North() {
    // Bearing from equator (0°, 0°) to north pole (90°, 0°) should be 0° (north)
    double bearing = GeographicUtils::calculateBearing(0.0, 0.0, 90.0, 0.0);
    QVERIFY(qAbs(bearing - 0.0) < 1.0);  // Allow small tolerance
}

void TestGeographicUtils::testCalculateBearing_East() {
    // Bearing from (0°, 0°) to (0°, 10°) should be ~90° (east)
    double bearing = GeographicUtils::calculateBearing(0.0, 0.0, 0.0, 10.0);
    QVERIFY(qAbs(bearing - 90.0) < 1.0);
}

void TestGeographicUtils::testCalculateBearing_South() {
    // Bearing from north pole to equator should be 180° (south)
    double bearing = GeographicUtils::calculateBearing(90.0, 0.0, 0.0, 0.0);
    QVERIFY(qAbs(bearing - 180.0) < 1.0);
}

void TestGeographicUtils::testCalculateBearing_West() {
    // Bearing from (0°, 0°) to (0°, -10°) should be ~270° (west)
    double bearing = GeographicUtils::calculateBearing(0.0, 0.0, 0.0, -10.0);
    QVERIFY(qAbs(bearing - 270.0) < 1.0);
}

// Grid square distance tests

void TestGeographicUtils::testDistanceBetweenGrids_SameGrid() {
    // Distance from grid to itself should be 0
    double distance = GeographicUtils::distanceBetweenGrids("FN31", "FN31", true);
    QCOMPARE(qRound(distance), 0);
}

void TestGeographicUtils::testDistanceBetweenGrids_AdjacentGrids() {
    // Adjacent grids should be about 111 km apart (1 degree)
    double distance = GeographicUtils::distanceBetweenGrids("FN31", "FN32", true);
    QVERIFY(distance >= 100.0 && distance <= 120.0);
}

void TestGeographicUtils::testDistanceBetweenGrids_KnownDistance() {
    // FN31 (Hartford area) to DN70 (Philadelphia area)
    // Should be approximately 200-300 km
    double distance = GeographicUtils::distanceBetweenGrids("FN31", "FN20", true);
    QVERIFY(distance >= 100.0 && distance <= 300.0);
}

void TestGeographicUtils::testDistanceBetweenGrids_InvalidGrid1() {
    // Invalid first grid should return -1.0
    double distance = GeographicUtils::distanceBetweenGrids("INVALID", "FN31", true);
    QCOMPARE(distance, -1.0);
}

void TestGeographicUtils::testDistanceBetweenGrids_InvalidGrid2() {
    // Invalid second grid should return -1.0
    double distance = GeographicUtils::distanceBetweenGrids("FN31", "XYZ", true);
    QCOMPARE(distance, -1.0);
}

// Callsign distance and bearing tests

void TestGeographicUtils::testDistanceAndBearingBetweenCallsigns_ValidCallsigns() {
    // Skip this test if cty.dat file is not available
    // This is an integration test that requires the country file to be loaded
    QSKIP("Requires cty.dat file - integration test");
}

void TestGeographicUtils::testDistanceAndBearingBetweenCallsigns_SameCountry() {
    // Skip this test if cty.dat file is not available
    // This is an integration test that requires the country file to be loaded
    QSKIP("Requires cty.dat file - integration test");
}

void TestGeographicUtils::testDistanceAndBearingBetweenCallsigns_InvalidCallsign1() {
    double distance, bearing;

    // Invalid first callsign
    bool result = GeographicUtils::distanceAndBearingBetweenCallsigns(
        "INVALID123", "G3XYZ", distance, bearing, true);

    QVERIFY(!result);
}

void TestGeographicUtils::testDistanceAndBearingBetweenCallsigns_InvalidCallsign2() {
    double distance, bearing;

    // Invalid second callsign
    bool result = GeographicUtils::distanceAndBearingBetweenCallsigns(
        "W1AW", "999ZZZ", distance, bearing, true);

    QVERIFY(!result);
}

// Sunrise/sunset calculation tests

void TestGeographicUtils::testCalculateSunrise_KnownLocation() {
    // Hartford, CT (41.7658°N, -72.6734°W) on June 21, 2024 (summer solstice)
    // Expected sunrise (from NOAA calculator): ~09:20 UTC (05:20 EDT)
    QDate date(2024, 6, 21);
    QTime sunrise = GeographicUtils::calculateSunrise(41.7658, -72.6734, date);

    QVERIFY(sunrise.isValid());

    // Allow ±5 minute tolerance (NOAA algorithm approximation)
    int expectedMinutes = 9 * 60 + 20;  // 09:20 UTC
    int actualMinutes = sunrise.hour() * 60 + sunrise.minute();
    int diff = qAbs(actualMinutes - expectedMinutes);

    QVERIFY2(diff <= 5, QString("Sunrise time %1 differs from expected 09:20 UTC by %2 minutes")
        .arg(sunrise.toString("HH:mm")).arg(diff).toLatin1());
}

void TestGeographicUtils::testCalculateSunset_KnownLocation() {
    // Hartford, CT (41.7658°N, -72.6734°W) on June 21, 2024 (summer solstice)
    // Expected sunset (from NOAA calculator): ~00:32 UTC (20:32 EDT)
    QDate date(2024, 6, 21);
    QTime sunset = GeographicUtils::calculateSunset(41.7658, -72.6734, date);

    QVERIFY(sunset.isValid());

    // Allow ±5 minute tolerance
    int expectedMinutes = 0 * 60 + 32;  // 00:32 UTC
    int actualMinutes = sunset.hour() * 60 + sunset.minute();
    int diff = qAbs(actualMinutes - expectedMinutes);

    QVERIFY2(diff <= 5, QString("Sunset time %1 differs from expected 00:32 UTC by %2 minutes")
        .arg(sunset.toString("HH:mm")).arg(diff).toLatin1());
}

void TestGeographicUtils::testCalculateSunrise_PolarRegion() {
    // North Pole (90°N, 0°W) on December 21 (polar winter)
    // Sun never rises in winter
    QDate date(2024, 12, 21);
    QTime sunrise = GeographicUtils::calculateSunrise(90.0, 0.0, date);

    QVERIFY(!sunrise.isValid());  // Should return invalid QTime
}

void TestGeographicUtils::testCalculateSunset_PolarRegion() {
    // North Pole (90°N, 0°W) on June 21 (polar summer)
    // Sun never sets in summer
    QDate date(2024, 6, 21);
    QTime sunset = GeographicUtils::calculateSunset(90.0, 0.0, date);

    QVERIFY(!sunset.isValid());  // Should return invalid QTime
}

void TestGeographicUtils::testCalculateSunrise_Equator() {
    // Quito, Ecuador (0°N, -78°W) on March 20 (equinox)
    // Sunrise should be approximately 11:12 UTC (06:12 local)
    QDate date(2024, 3, 20);
    QTime sunrise = GeographicUtils::calculateSunrise(0.0, -78.0, date);

    QVERIFY(sunrise.isValid());

    // At equator on equinox, sunrise should be near 06:00 local
    // -78° longitude = UTC-5.2h, so ~11:12 UTC
    int expectedMinutes = 11 * 60 + 12;  // 11:12 UTC
    int actualMinutes = sunrise.hour() * 60 + sunrise.minute();
    int diff = qAbs(actualMinutes - expectedMinutes);

    QVERIFY2(diff <= 10, QString("Equator sunrise %1 differs from expected 11:12 UTC by %2 minutes")
        .arg(sunrise.toString("HH:mm")).arg(diff).toLatin1());
}

void TestGeographicUtils::testCalculateSunset_Equator() {
    // Quito, Ecuador (0°N, -78°W) on March 20 (equinox)
    // Sunset should be approximately 23:18 UTC (18:18 local)
    QDate date(2024, 3, 20);
    QTime sunset = GeographicUtils::calculateSunset(0.0, -78.0, date);

    QVERIFY(sunset.isValid());

    // At equator on equinox, sunset should be near 18:00 local
    // -78° longitude = UTC-5.2h, so ~23:18 UTC
    int expectedMinutes = 23 * 60 + 18;  // 23:18 UTC
    int actualMinutes = sunset.hour() * 60 + sunset.minute();
    int diff = qAbs(actualMinutes - expectedMinutes);

    QVERIFY2(diff <= 10, QString("Equator sunset %1 differs from expected 23:18 UTC by %2 minutes")
        .arg(sunset.toString("HH:mm")).arg(diff).toLatin1());
}

// Grayline window tests

void TestGeographicUtils::testIsInGraylineWindow_BeforeSunrise() {
    QTime sunrise(6, 0, 0);   // 06:00 UTC
    QTime sunset(18, 0, 0);   // 18:00 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(5, 0, 0));  // 05:00 UTC (60 min before sunrise)

    // 60 minutes before sunrise is outside the 30-minute window
    QVERIFY(!GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_AtSunrise() {
    QTime sunrise(6, 0, 0);   // 06:00 UTC
    QTime sunset(18, 0, 0);   // 18:00 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(6, 0, 0));  // 06:00 UTC (exactly at sunrise)

    // Exactly at sunrise should be in grayline window
    QVERIFY(GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_AfterSunrise() {
    QTime sunrise(6, 0, 0);   // 06:00 UTC
    QTime sunset(18, 0, 0);   // 18:00 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(6, 25, 0));  // 06:25 UTC (25 min after sunrise)

    // 25 minutes after sunrise is within the 30-minute window
    QVERIFY(GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_BeforeSunset() {
    QTime sunrise(6, 0, 0);   // 06:00 UTC
    QTime sunset(18, 0, 0);   // 18:00 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(17, 40, 0));  // 17:40 UTC (20 min before sunset)

    // 20 minutes before sunset is within the 30-minute window
    QVERIFY(GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_AtSunset() {
    QTime sunrise(6, 0, 0);   // 06:00 UTC
    QTime sunset(18, 0, 0);   // 18:00 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(18, 0, 0));  // 18:00 UTC (exactly at sunset)

    // Exactly at sunset should be in grayline window
    QVERIFY(GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_AfterSunset() {
    QTime sunrise(6, 0, 0);   // 06:00 UTC
    QTime sunset(18, 0, 0);   // 18:00 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(19, 0, 0));  // 19:00 UTC (60 min after sunset)

    // 60 minutes after sunset is outside the 30-minute window
    QVERIFY(!GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_MidnightCrossing() {
    QTime sunrise(0, 15, 0);   // 00:15 UTC (just after midnight)
    QTime sunset(12, 30, 0);   // 12:30 UTC
    QDateTime currentTime(QDate(2024, 6, 21), QTime(23, 50, 0));  // 23:50 UTC (25 min before midnight)

    // 23:50 is 25 minutes before 00:15 sunrise (crossing midnight)
    // Should be within 30-minute grayline window
    QVERIFY(GeographicUtils::isInGraylineWindow(currentTime, sunrise, sunset, 30));
}

void TestGeographicUtils::testIsInGraylineWindow_InvalidTimes() {
    QTime invalidSunrise;      // Invalid QTime
    QTime validSunset(18, 0, 0);
    QDateTime currentTime(QDate(2024, 6, 21), QTime(17, 45, 0));

    // Invalid sunrise time should return false
    QVERIFY(!GeographicUtils::isInGraylineWindow(currentTime, invalidSunrise, validSunset, 30));
}

QTEST_MAIN(TestGeographicUtils)
#include "test_geographicutils.moc"
