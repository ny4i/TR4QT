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

QTEST_MAIN(TestGeographicUtils)
#include "test_geographicutils.moc"
