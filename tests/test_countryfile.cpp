#include <QTest>
#include <QTemporaryFile>
#include "../src/utils/CountryFile.h"

using namespace TR4QT;

/**
 * Unit tests for CountryFile
 * Tests CTY.DAT parsing and callsign lookup
 */
class TestCountryFile : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // File loading tests
    void testLoadFromFile_Valid();
    void testLoadFromFile_Invalid();
    void testLoadFromFile_Empty();

    // stripPortable() tests
    void testStripPortable_NoSlash();
    void testStripPortable_P();
    void testStripPortable_M();
    void testStripPortable_MM();
    void testStripPortable_QRP();
    void testStripPortable_Numeric();
    void testStripPortable_MultipleParts();

    // extractWPXPrefix() tests
    void testExtractWPXPrefix_Simple();
    void testExtractWPXPrefix_CallDistrict();
    void testExtractWPXPrefix_Portable();
    void testExtractWPXPrefix_International();
    void testExtractWPXPrefix_NoDigit();
    void testExtractWPXPrefix_MultipleDigits();

    // lookup() tests - simple cases
    void testLookup_EmptyCallsign();
    void testLookup_SimpleUS();
    void testLookup_SimpleCanada();
    void testLookup_SimpleGermany();
    void testLookup_SimpleJapan();
    void testLookup_NotFound();

    // lookup() tests - exact matches
    void testLookup_ExactMatch_KH6();
    void testLookup_ExactMatch_3D2RR();

    // lookup() tests - longest prefix matching
    void testLookup_LongestPrefix_Hawaii();
    void testLookup_LongestPrefix_Rotuma();

    // lookup() tests - portable callsigns
    void testLookup_Portable_P();
    void testLookup_Portable_M();
    void testLookup_Portable_QRP();
    void testLookup_Portable_Slash4();

    // lookup() tests - zone overrides
    void testLookup_ZoneOverride_CQ();
    void testLookup_ZoneOverride_ITU();

    // lookup() tests - case insensitivity
    void testLookup_CaseInsensitive();

    // lookup() tests - US call area zones
    void testLookup_USCallAreaZones();

    // CountryData validation
    void testCountryData_IsValid_Valid();
    void testCountryData_IsValid_Empty();

    // getAllCountries() test
    void testGetAllCountries();

private:
    CountryFile m_countryFile;
    QString m_testFilePath;
};

void TestCountryFile::initTestCase() {
    // Load test CTY.DAT file
    m_testFilePath = QString(TESTS_SOURCE_DIR) + "/fixtures/test_cty.dat";
    bool loaded = m_countryFile.loadFromFile(m_testFilePath);
    QVERIFY2(loaded, "Failed to load test CTY.DAT file");
}

// File loading tests

void TestCountryFile::testLoadFromFile_Valid() {
    CountryFile cf;
    bool loaded = cf.loadFromFile(m_testFilePath);
    QVERIFY(loaded);

    // Should have loaded 10 countries from test file
    auto countries = cf.getAllCountries();
    QCOMPARE(countries.size(), 10);
}

void TestCountryFile::testLoadFromFile_Invalid() {
    CountryFile cf;
    bool loaded = cf.loadFromFile("/nonexistent/file.dat");
    QVERIFY(!loaded);
}

void TestCountryFile::testLoadFromFile_Empty() {
    // Create empty temporary file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempPath = tempFile.fileName();
    tempFile.close();

    CountryFile cf;
    bool loaded = cf.loadFromFile(tempPath);
    QVERIFY(!loaded);  // Empty file should fail
}

// stripPortable() tests

void TestCountryFile::testStripPortable_NoSlash() {
    QString result = CountryFile::stripPortable("W1AW");
    QCOMPARE(result, QString("W1AW"));
}

void TestCountryFile::testStripPortable_P() {
    QString result = CountryFile::stripPortable("W1AW/P");
    QCOMPARE(result, QString("W1AW"));
}

void TestCountryFile::testStripPortable_M() {
    QString result = CountryFile::stripPortable("W1AW/M");
    QCOMPARE(result, QString("W1AW"));
}

void TestCountryFile::testStripPortable_MM() {
    QString result = CountryFile::stripPortable("W1AW/MM");
    QCOMPARE(result, QString("W1AW"));
}

void TestCountryFile::testStripPortable_QRP() {
    QString result = CountryFile::stripPortable("W1AW/QRP");
    QCOMPARE(result, QString("W1AW"));
}

void TestCountryFile::testStripPortable_Numeric() {
    // W1AW/4 should return W1AW (longer part)
    QString result = CountryFile::stripPortable("W1AW/4");
    QCOMPARE(result, QString("W1AW"));

    // VE3/W1AW should return W1AW (longer part)
    result = CountryFile::stripPortable("VE3/W1AW");
    QCOMPARE(result, QString("W1AW"));
}

void TestCountryFile::testStripPortable_MultipleParts() {
    // Take longest part
    QString result = CountryFile::stripPortable("W1AW/P/QRP");
    QCOMPARE(result, QString("W1AW"));
}

// extractWPXPrefix() tests

void TestCountryFile::testExtractWPXPrefix_Simple() {
    QCOMPARE(CountryFile::extractWPXPrefix("W1AW"), QString("W1"));
    QCOMPARE(CountryFile::extractWPXPrefix("K3LR"), QString("K3"));
    QCOMPARE(CountryFile::extractWPXPrefix("N6TR"), QString("N6"));
}

void TestCountryFile::testExtractWPXPrefix_CallDistrict() {
    QCOMPARE(CountryFile::extractWPXPrefix("W1ABC"), QString("W1"));
    QCOMPARE(CountryFile::extractWPXPrefix("K1XYZ"), QString("K1"));
}

void TestCountryFile::testExtractWPXPrefix_Portable() {
    // Should strip portable first, then extract prefix
    QCOMPARE(CountryFile::extractWPXPrefix("W1AW/P"), QString("W1"));
    QCOMPARE(CountryFile::extractWPXPrefix("W1AW/M"), QString("W1"));
}

void TestCountryFile::testExtractWPXPrefix_International() {
    QCOMPARE(CountryFile::extractWPXPrefix("DL1ABC"), QString("DL1"));
    QCOMPARE(CountryFile::extractWPXPrefix("JA1XYZ"), QString("JA1"));
    QCOMPARE(CountryFile::extractWPXPrefix("G3ABC"), QString("G3"));
}

void TestCountryFile::testExtractWPXPrefix_NoDigit() {
    // No digit found - return whole callsign
    QString result = CountryFile::extractWPXPrefix("ABC");
    QCOMPARE(result, QString("ABC"));
}

void TestCountryFile::testExtractWPXPrefix_MultipleDigits() {
    // Extract up to and including FIRST digit
    QCOMPARE(CountryFile::extractWPXPrefix("JA1234XYZ"), QString("JA1"));
}

// lookup() tests - simple cases

void TestCountryFile::testLookup_EmptyCallsign() {
    CountryData result = m_countryFile.lookup("");
    QVERIFY(!result.isValid());
}

void TestCountryFile::testLookup_SimpleUS() {
    CountryData result = m_countryFile.lookup("W1AW");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("United States"));
    QCOMPARE(result.primaryPrefix, QString("K"));
    QCOMPARE(result.cqZone, 5);
    QCOMPARE(result.ituZone, 8);
    QCOMPARE(result.continent, Continent::NA);
}

void TestCountryFile::testLookup_SimpleCanada() {
    CountryData result = m_countryFile.lookup("VE3ABC");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Canada"));
    QCOMPARE(result.primaryPrefix, QString("VE"));
    QCOMPARE(result.cqZone, 5);
    QCOMPARE(result.ituZone, 9);
    QCOMPARE(result.continent, Continent::NA);
}

void TestCountryFile::testLookup_SimpleGermany() {
    CountryData result = m_countryFile.lookup("DL1ABC");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Germany"));
    QCOMPARE(result.primaryPrefix, QString("DL"));
    QCOMPARE(result.cqZone, 14);
    QCOMPARE(result.ituZone, 28);
    QCOMPARE(result.continent, Continent::EU);
}

void TestCountryFile::testLookup_SimpleJapan() {
    CountryData result = m_countryFile.lookup("JA1XYZ");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Japan"));
    QCOMPARE(result.primaryPrefix, QString("JA"));
    QCOMPARE(result.cqZone, 25);
    QCOMPARE(result.ituZone, 45);
    QCOMPARE(result.continent, Continent::AS);
}

void TestCountryFile::testLookup_NotFound() {
    CountryData result = m_countryFile.lookup("ZZZINVALID");
    QVERIFY(!result.isValid());
}

// lookup() tests - exact matches

void TestCountryFile::testLookup_ExactMatch_KH6() {
    // =KH6 means match "KH6" exactly (not KH6ABC)
    CountryData result = m_countryFile.lookup("KH6");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Hawaii"));
}

void TestCountryFile::testLookup_ExactMatch_3D2RR() {
    // =3D2RR should match Rotuma
    CountryData result = m_countryFile.lookup("3D2RR");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Rotuma"));
}

// lookup() tests - longest prefix matching

void TestCountryFile::testLookup_LongestPrefix_Hawaii() {
    // KH6ABC should match Hawaii (KH6), not just "K" from USA
    CountryData result = m_countryFile.lookup("KH6ABC");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Hawaii"));
    QCOMPARE(result.primaryPrefix, QString("KH6"));
}

void TestCountryFile::testLookup_LongestPrefix_Rotuma() {
    // 3D2/R should match Rotuma's prefix, not Fiji
    CountryData result = m_countryFile.lookup("3D2RABC");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Fiji"));  // 3D2R matches, but 3D2RABC doesn't (no exact match)

    // But 3D2RR should match Rotuma (exact match)
    result = m_countryFile.lookup("3D2RR");
    QCOMPARE(result.name, QString("Rotuma"));
}

// lookup() tests - portable callsigns

void TestCountryFile::testLookup_Portable_P() {
    CountryData result = m_countryFile.lookup("W1AW/P");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("United States"));
}

void TestCountryFile::testLookup_Portable_M() {
    CountryData result = m_countryFile.lookup("W1AW/M");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("United States"));
}

void TestCountryFile::testLookup_Portable_QRP() {
    CountryData result = m_countryFile.lookup("W1AW/QRP");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("United States"));
}

void TestCountryFile::testLookup_Portable_Slash4() {
    // W1AW/4 should still be USA (takes longest part = W1AW)
    CountryData result = m_countryFile.lookup("W1AW/4");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("United States"));
}

// lookup() tests - zone overrides

void TestCountryFile::testLookup_ZoneOverride_CQ() {
    // KG4 has CQ zone override (8) in test file
    CountryData result = m_countryFile.lookup("KG4AA");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Guantanamo Bay"));
    QCOMPARE(result.cqZone, 8);  // Override from (8)
}

void TestCountryFile::testLookup_ZoneOverride_ITU() {
    // KG4 has ITU zone override [11] in test file
    CountryData result = m_countryFile.lookup("KG4AA");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("Guantanamo Bay"));
    QCOMPARE(result.ituZone, 11);  // Override from [11]
}

// lookup() tests - case insensitivity

void TestCountryFile::testLookup_CaseInsensitive() {
    // Lowercase
    CountryData lower = m_countryFile.lookup("w1aw");
    QVERIFY(lower.isValid());
    QCOMPARE(lower.name, QString("United States"));

    // Uppercase
    CountryData upper = m_countryFile.lookup("W1AW");
    QVERIFY(upper.isValid());
    QCOMPARE(upper.name, QString("United States"));

    // Mixed case
    CountryData mixed = m_countryFile.lookup("W1Aw");
    QVERIFY(mixed.isValid());
    QCOMPARE(mixed.name, QString("United States"));
}

// lookup() tests - US call area zones

void TestCountryFile::testLookup_USCallAreaZones() {
    // Test all US call areas (0-9) with K, W, and N prefixes
    // Based on CQ WW zone definitions:
    // Zone 3: K6, K7 (Pacific)
    // Zone 4: K0, K5, K8, K9 (Central/Midwest)
    // Zone 5: K1, K2, K3, K4 (Eastern)
    // Note: Hawaii and Alaska are separate DXCC entities

    struct TestCase {
        QString callsign;
        int expectedZone;
        QString expectedCountry;  // Hawaii and Alaska are separate DXCC entities
    };

    QVector<TestCase> testCases = {
        // Zone 3 - Pacific
        {"N6AA", 3, "United States"},  // California
        {"K6TA", 3, "United States"},  // California
        {"W6XYZ", 3, "United States"}, // California
        {"N7AA", 3, "United States"},  // West Coast (WA, OR, etc.)
        {"K7LR", 3, "United States"},  // West Coast
        {"W7XYZ", 3, "United States"}, // West Coast

        // Zone 4 - Central/Midwest
        {"N0AA", 4, "United States"},  // Central North (CO, IA, KS, MN, MO, ND, NE, SD)
        {"K0RF", 4, "United States"},  // Central North
        {"W0XYZ", 4, "United States"}, // Central North
        {"N5AA", 4, "United States"},  // Central South (AR, LA, MS, NM, OK, TX)
        {"K5ZD", 4, "United States"},  // Central South
        {"W5XYZ", 4, "United States"}, // Central South
        {"N8AA", 4, "United States"},  // Midwest (MI, OH, WV)
        {"K8IA", 4, "United States"},  // Midwest
        {"W8XYZ", 4, "United States"}, // Midwest
        {"N9AA", 4, "United States"},  // Midwest (IL, IN, WI)
        {"K9YC", 4, "United States"},  // Midwest
        {"W9UY", 4, "United States"},  // Midwest

        // Zone 5 - Eastern
        {"N1AA", 5, "United States"},  // New England
        {"K1AR", 5, "United States"},  // New England
        {"W1AW", 5, "United States"},  // New England
        {"N2AA", 5, "United States"},  // Mid-Atlantic (NJ, NY)
        {"K2MK", 5, "United States"},  // Mid-Atlantic
        {"W2XYZ", 5, "United States"}, // Mid-Atlantic
        {"N3AA", 5, "United States"},  // Mid-Atlantic (DE, MD, PA, DC)
        {"K3LR", 5, "United States"},  // Mid-Atlantic
        {"W3XYZ", 5, "United States"}, // Mid-Atlantic
        {"N4AA", 5, "United States"},  // Southeast (AL, FL, GA, KY, NC, SC, TN, VA)
        {"K4BAI", 5, "United States"}, // Southeast
        {"W4XYZ", 5, "United States"}, // Southeast

        // Special cases - Separate DXCC entities
        {"KH6XX", 31, "Hawaii"},   // Hawaii (separate DXCC entity)
        {"AH6ABC", 31, "Hawaii"},  // Hawaii (separate DXCC entity)
        {"KL7AA", 1, "Alaska"},    // Alaska (separate DXCC entity)
        {"AL7XYZ", 1, "Alaska"}    // Alaska (separate DXCC entity)
    };

    for (const auto& test : testCases) {
        CountryData result = m_countryFile.lookup(test.callsign);
        QVERIFY2(result.isValid(),
                 QString("Failed to lookup %1").arg(test.callsign).toLatin1());
        QVERIFY2(result.name == test.expectedCountry,
                 QString("%1: Expected country '%2', got '%3'")
                 .arg(test.callsign)
                 .arg(test.expectedCountry)
                 .arg(result.name).toLatin1());
        QVERIFY2(result.cqZone == test.expectedZone,
                 QString("%1: Expected zone %2, got zone %3")
                 .arg(test.callsign)
                 .arg(test.expectedZone)
                 .arg(result.cqZone).toLatin1());
    }
}

// CountryData validation

void TestCountryFile::testCountryData_IsValid_Valid() {
    CountryData country;
    country.name = "United States";
    QVERIFY(country.isValid());
}

void TestCountryFile::testCountryData_IsValid_Empty() {
    CountryData country;
    QVERIFY(!country.isValid());
}

// getAllCountries() test

void TestCountryFile::testGetAllCountries() {
    auto countries = m_countryFile.getAllCountries();

    // Should have 10 countries from test file
    QCOMPARE(countries.size(), 10);

    // Verify we can find specific countries
    bool foundUSA = false;
    bool foundCanada = false;
    bool foundGermany = false;

    for (const auto& country : countries) {
        if (country.name == "United States") foundUSA = true;
        if (country.name == "Canada") foundCanada = true;
        if (country.name == "Germany") foundGermany = true;
    }

    QVERIFY(foundUSA);
    QVERIFY(foundCanada);
    QVERIFY(foundGermany);
}

QTEST_MAIN(TestCountryFile)
#include "test_countryfile.moc"
