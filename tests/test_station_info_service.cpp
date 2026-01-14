/**
 * Unit tests for StationInfoService
 *
 * Tests station information calculations:
 * - Geographic calculations (bearing, distance)
 * - Grayline detection
 * - SCP match formatting
 * - Multiplier value lookup
 */

#include <QtTest/QtTest>
#include "../src/services/StationInfoService.h"
#include "../src/utils/CountryFile.h"
#include "../src/core/Types.h"

using namespace TR4QT;

class TestStationInfoService : public QObject {
    Q_OBJECT

private:
    CountryFile m_countryFile;
    StationInfoService* m_service = nullptr;

private slots:
    void initTestCase() {
        // Load the country file for tests
        // Try common paths for cty.dat
        QStringList paths = {
            QDir::homePath() + "/.tr4qt/cty.dat",
            "../../data/cty.dat",
            "../data/cty.dat",
            "./cty.dat"
        };

        bool loaded = false;
        for (const QString& path : paths) {
            if (QFile::exists(path)) {
                loaded = m_countryFile.loadFromFile(path);
                if (loaded) break;
            }
        }

        // Create service regardless (tests will handle missing country file)
        m_service = new StationInfoService(&m_countryFile);
    }

    void cleanupTestCase() {
        delete m_service;
        m_service = nullptr;
    }

    /**
     * Test: Null country file returns invalid result
     */
    void testNullCountryFile() {
        StationInfoService serviceWithNull(nullptr);
        StationInfoResult result = serviceWithNull.calculateStationInfo("W1AW", "FN31", true);

        QVERIFY(!result.valid);
        QVERIFY(result.displayInfo.isEmpty());
    }

    /**
     * Test: Empty callsign handling
     */
    void testEmptyCallsign() {
        StationInfoResult result = m_service->calculateStationInfo("", "FN31", true);

        QVERIFY(!result.valid);
    }

    /**
     * Test: Valid US callsign with grid (if country file loaded)
     */
    void testValidUSCallsign() {
        if (!!m_countryFile.getAllCountries().isEmpty()) {
            QSKIP("Country file not loaded, skipping test");
        }

        StationInfoResult result = m_service->calculateStationInfo("W1AW", "FN31", true);

        QVERIFY(result.valid);
        QVERIFY(!result.countryName.isEmpty());
        QVERIFY(!result.primaryPrefix.isEmpty());
    }

    /**
     * Test: Empty grid returns basic info
     */
    void testEmptyGrid() {
        if (!!m_countryFile.getAllCountries().isEmpty()) {
            QSKIP("Country file not loaded, skipping test");
        }

        StationInfoResult result = m_service->calculateStationInfo("W1AW", "", true);

        QVERIFY(result.valid);
        QCOMPARE(result.displayInfo, result.primaryPrefix);
        QCOMPARE(result.bearing, 0.0);
        QCOMPARE(result.distance, 0.0);
    }

    /**
     * Test: Invalid grid returns basic info
     */
    void testInvalidGrid() {
        if (!!m_countryFile.getAllCountries().isEmpty()) {
            QSKIP("Country file not loaded, skipping test");
        }

        StationInfoResult result = m_service->calculateStationInfo("W1AW", "INVALID", true);

        QVERIFY(result.valid);
        // Invalid grid should return just prefix, no bearing/distance
        QCOMPARE(result.displayInfo, result.primaryPrefix);
    }

    /**
     * Test: Distance units - metric vs imperial
     */
    void testDistanceUnits() {
        if (!!m_countryFile.getAllCountries().isEmpty()) {
            QSKIP("Country file not loaded, skipping test");
        }

        StationInfoResult metricResult = m_service->calculateStationInfo("JA1ABC", "FN31", true);
        StationInfoResult imperialResult = m_service->calculateStationInfo("JA1ABC", "FN31", false);

        if (metricResult.valid && imperialResult.valid) {
            // Metric distance should be larger than imperial (km > miles)
            // The ratio should be approximately 1.609
            double ratio = metricResult.distance / imperialResult.distance;
            QVERIFY(ratio > 1.5 && ratio < 1.7);
        }
    }

    /**
     * Test: Bearing is within valid range
     */
    void testBearingRange() {
        if (!!m_countryFile.getAllCountries().isEmpty()) {
            QSKIP("Country file not loaded, skipping test");
        }

        StationInfoResult result = m_service->calculateStationInfo("VK3ABC", "FN31", true);

        if (result.valid && !result.displayInfo.isEmpty()) {
            QVERIFY(result.bearing >= 0.0 && result.bearing <= 360.0);
        }
    }

    /**
     * Test: SCP matches - empty list
     */
    void testSCPMatchesEmpty() {
        QStringList matches;
        QSet<QString> worked;
        auto dupeChecker = [](const QString&, BandType, ModeType, QString&) { return false; };

        SCPDisplayResult result = m_service->formatSCPMatches(
            matches, worked, BandType::Band20M, ModeType::CW, dupeChecker);

        QCOMPARE(result.totalMatches, 0);
        QCOMPARE(result.workedCount, 0);
        QCOMPARE(result.dupeCount, 0);
        QVERIFY(result.htmlContent.isEmpty());
    }

    /**
     * Test: SCP matches - single match not worked
     */
    void testSCPMatchesSingleNotWorked() {
        QStringList matches = {"W1ABC"};
        QSet<QString> worked;
        auto dupeChecker = [](const QString&, BandType, ModeType, QString&) { return false; };

        SCPDisplayResult result = m_service->formatSCPMatches(
            matches, worked, BandType::Band20M, ModeType::CW, dupeChecker);

        QCOMPARE(result.totalMatches, 1);
        QCOMPARE(result.workedCount, 0);
        QCOMPARE(result.dupeCount, 0);
        QVERIFY(result.htmlContent.contains("W1ABC"));
    }

    /**
     * Test: SCP matches - worked callsign (not dupe)
     */
    void testSCPMatchesWorked() {
        QStringList matches = {"W1ABC"};
        QSet<QString> worked = {"W1ABC"};
        auto dupeChecker = [](const QString&, BandType, ModeType, QString&) { return false; };

        SCPDisplayResult result = m_service->formatSCPMatches(
            matches, worked, BandType::Band20M, ModeType::CW, dupeChecker);

        QCOMPARE(result.totalMatches, 1);
        QCOMPARE(result.workedCount, 1);
        QCOMPARE(result.dupeCount, 0);
        QVERIFY(result.htmlContent.contains("W1ABC"));
    }

    /**
     * Test: SCP matches - dupe callsign
     */
    void testSCPMatchesDupe() {
        QStringList matches = {"W1ABC"};
        QSet<QString> worked = {"W1ABC"};
        auto dupeChecker = [](const QString& call, BandType, ModeType, QString&) {
            return call == "W1ABC";  // This one is a dupe
        };

        SCPDisplayResult result = m_service->formatSCPMatches(
            matches, worked, BandType::Band20M, ModeType::CW, dupeChecker);

        QCOMPARE(result.totalMatches, 1);
        QCOMPARE(result.workedCount, 1);
        QCOMPARE(result.dupeCount, 1);
    }

    /**
     * Test: SCP matches - multiple with mixed status
     */
    void testSCPMatchesMixed() {
        QStringList matches = {"W1ABC", "W2DEF", "W3GHI", "W4JKL"};
        QSet<QString> worked = {"W1ABC", "W2DEF"};
        auto dupeChecker = [](const QString& call, BandType, ModeType, QString&) {
            return call == "W1ABC";  // Only W1ABC is dupe
        };

        SCPDisplayResult result = m_service->formatSCPMatches(
            matches, worked, BandType::Band20M, ModeType::CW, dupeChecker);

        QCOMPARE(result.totalMatches, 4);
        QCOMPARE(result.workedCount, 2);
        QCOMPARE(result.dupeCount, 1);
        // HTML should contain all callsigns
        QVERIFY(result.htmlContent.contains("W1ABC"));
        QVERIFY(result.htmlContent.contains("W2DEF"));
        QVERIFY(result.htmlContent.contains("W3GHI"));
        QVERIFY(result.htmlContent.contains("W4JKL"));
    }

    /**
     * Test: SCP matches - sorted with worked first
     */
    void testSCPMatchesSorting() {
        QStringList matches = {"A1AAA", "Z9ZZZ", "W1ABC"};
        QSet<QString> worked = {"W1ABC"};
        auto dupeChecker = [](const QString&, BandType, ModeType, QString&) { return false; };

        SCPDisplayResult result = m_service->formatSCPMatches(
            matches, worked, BandType::Band20M, ModeType::CW, dupeChecker);

        // Worked call (W1ABC) should appear before not-worked calls
        int w1abcPos = result.htmlContent.indexOf("W1ABC");
        int a1aaaPos = result.htmlContent.indexOf("A1AAA");

        QVERIFY(w1abcPos < a1aaaPos);  // Worked should come first
    }

    /**
     * Test: Multiplier value - null contest
     */
    void testMultiplierValueNullContest() {
        QString mult = m_service->getMultiplierValueForCallsign("W1AW", nullptr);

        QVERIFY(mult.isEmpty());
    }

    /**
     * Test: Multiplier value - empty callsign
     */
    void testMultiplierValueEmptyCallsign() {
        // We can't easily mock a contest, but we can test edge cases
        QString mult = m_service->getMultiplierValueForCallsign("", nullptr);

        QVERIFY(mult.isEmpty());
    }

    /**
     * Test: Grayline window constant
     */
    void testGraylineWindowConstant() {
        // Verify the grayline window is 30 minutes
        QCOMPARE(StationInfoService::GRAYLINE_WINDOW_MINUTES, 30);
    }
};

QTEST_GUILESS_MAIN(TestStationInfoService)
#include "test_station_info_service.moc"
