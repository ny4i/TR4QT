#include <QtTest>
#include "../src/utils/ArrlSectionHelper.h"

using namespace TR4QT::Arrl;

class TestArrlSection : public QObject
{
    Q_OBJECT

private slots:
    // Florida tests
    void testFlorida_NFL();
    void testFlorida_WCF();
    void testFlorida_SFL();

    // California tests
    void testCalifornia_LAX();
    void testCalifornia_SDG();
    void testCalifornia_SF();
    void testCalifornia_SCV();

    // Texas tests
    void testTexas_STX();
    void testTexas_NTX();
    void testTexas_WTX();

    // New York tests
    void testNewYork_NLI();
    void testNewYork_NNY();
    void testNewYork_WNY();
    void testNewYork_ENY();

    // New Jersey tests
    void testNewJersey_NNJ();
    void testNewJersey_SNJ();

    // Massachusetts tests
    void testMassachusetts_EMA();
    void testMassachusetts_WMA();

    // Pennsylvania tests
    void testPennsylvania_EPA();
    void testPennsylvania_WPA();

    // Washington tests
    void testWashington_WWA();
    void testWashington_EWA();

    // Simple state tests
    void testSimpleStates();

    // Case insensitivity and normalization
    void testCaseInsensitive();
    void testCountyNormalization();
    void testStateNormalization();

    // Edge cases
    void testUnknownState();
    void testUnknownCounty();
    void testEmptyInputs();
};

// ============================================================================
// FLORIDA TESTS
// ============================================================================

void TestArrlSection::testFlorida_NFL()
{
    QCOMPARE(sectionForStateCounty("FL", "Duval"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Escambia"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Leon"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "St. Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Nassau"), QString("NFL"));
}

void TestArrlSection::testFlorida_WCF()
{
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Hillsborough"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Orange"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Seminole"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Volusia"), QString("WCF"));
}

void TestArrlSection::testFlorida_SFL()
{
    QCOMPARE(sectionForStateCounty("FL", "Broward"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Miami-Dade"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Palm Beach"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Monroe"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Collier"), QString("SFL"));
}

// ============================================================================
// CALIFORNIA TESTS
// ============================================================================

void TestArrlSection::testCalifornia_LAX()
{
    QCOMPARE(sectionForStateCounty("CA", "Los Angeles"), QString("LAX"));
}

void TestArrlSection::testCalifornia_SDG()
{
    QCOMPARE(sectionForStateCounty("CA", "San Diego"), QString("SDG"));
    QCOMPARE(sectionForStateCounty("CA", "Imperial"), QString("SDG"));
}

void TestArrlSection::testCalifornia_SF()
{
    QCOMPARE(sectionForStateCounty("CA", "San Francisco"), QString("SF"));
    QCOMPARE(sectionForStateCounty("CA", "Marin"), QString("SF"));
    QCOMPARE(sectionForStateCounty("CA", "San Mateo"), QString("SF"));
}

void TestArrlSection::testCalifornia_SCV()
{
    QCOMPARE(sectionForStateCounty("CA", "Santa Clara"), QString("SCV"));
}

// ============================================================================
// TEXAS TESTS
// ============================================================================

void TestArrlSection::testTexas_STX()
{
    QCOMPARE(sectionForStateCounty("TX", "Harris"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Travis"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Bexar"), QString("STX"));
}

void TestArrlSection::testTexas_NTX()
{
    QCOMPARE(sectionForStateCounty("TX", "Dallas"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Tarrant"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Collin"), QString("NTX"));
}

void TestArrlSection::testTexas_WTX()
{
    QCOMPARE(sectionForStateCounty("TX", "El Paso"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Midland"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Lubbock"), QString("WTX"));
}

// ============================================================================
// NEW YORK TESTS
// ============================================================================

void TestArrlSection::testNewYork_NLI()
{
    QCOMPARE(sectionForStateCounty("NY", "Queens"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Kings"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Nassau"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Suffolk"), QString("NLI"));
}

void TestArrlSection::testNewYork_NNY()
{
    QCOMPARE(sectionForStateCounty("NY", "Jefferson"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "St. Lawrence"), QString("NNY"));
}

void TestArrlSection::testNewYork_WNY()
{
    QCOMPARE(sectionForStateCounty("NY", "Erie"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Monroe"), QString("WNY"));
}

void TestArrlSection::testNewYork_ENY()
{
    QCOMPARE(sectionForStateCounty("NY", "Albany"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Onondaga"), QString("ENY"));
}

// ============================================================================
// NEW JERSEY TESTS
// ============================================================================

void TestArrlSection::testNewJersey_NNJ()
{
    QCOMPARE(sectionForStateCounty("NJ", "Bergen"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Essex"), QString("NNJ"));
}

void TestArrlSection::testNewJersey_SNJ()
{
    QCOMPARE(sectionForStateCounty("NJ", "Atlantic"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Camden"), QString("SNJ"));
}

// ============================================================================
// MASSACHUSETTS TESTS
// ============================================================================

void TestArrlSection::testMassachusetts_EMA()
{
    QCOMPARE(sectionForStateCounty("MA", "Suffolk"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Middlesex"), QString("EMA"));
}

void TestArrlSection::testMassachusetts_WMA()
{
    QCOMPARE(sectionForStateCounty("MA", "Berkshire"), QString("WMA"));
    QCOMPARE(sectionForStateCounty("MA", "Hampshire"), QString("WMA"));
}

// ============================================================================
// PENNSYLVANIA TESTS
// ============================================================================

void TestArrlSection::testPennsylvania_EPA()
{
    QCOMPARE(sectionForStateCounty("PA", "Philadelphia"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Bucks"), QString("EPA"));
}

void TestArrlSection::testPennsylvania_WPA()
{
    QCOMPARE(sectionForStateCounty("PA", "Allegheny"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Erie"), QString("WPA"));
}

// ============================================================================
// WASHINGTON TESTS
// ============================================================================

void TestArrlSection::testWashington_WWA()
{
    QCOMPARE(sectionForStateCounty("WA", "King"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Pierce"), QString("WWA"));
}

void TestArrlSection::testWashington_EWA()
{
    QCOMPARE(sectionForStateCounty("WA", "Spokane"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Yakima"), QString("EWA"));
}

// ============================================================================
// SIMPLE STATE TESTS
// ============================================================================

void TestArrlSection::testSimpleStates()
{
    // Test simple 1:1 state mappings
    QCOMPARE(sectionForStateCounty("UT", "Salt Lake"), QString("UT"));
    QCOMPARE(sectionForStateCounty("UT", "Weber"), QString("UT"));
    QCOMPARE(sectionForStateCounty("CO", "Denver"), QString("CO"));
    QCOMPARE(sectionForStateCounty("AZ", "Maricopa"), QString("AZ"));
    QCOMPARE(sectionForStateCounty("OR", "Multnomah"), QString("OR"));

    // Test sectionForState() for simple states
    QCOMPARE(sectionForState("UT"), QString("UT"));
    QCOMPARE(sectionForState("CO"), QString("CO"));
    QCOMPARE(sectionForState("AZ"), QString("AZ"));
}

// ============================================================================
// CASE INSENSITIVITY AND NORMALIZATION TESTS
// ============================================================================

void TestArrlSection::testCaseInsensitive()
{
    // State case variations
    QCOMPARE(sectionForStateCounty("fl", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("Fl", "Pinellas"), QString("WCF"));

    // County case variations
    QCOMPARE(sectionForStateCounty("FL", "pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "PINELLAS"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));

    // Both lowercase
    QCOMPARE(sectionForStateCounty("ca", "los angeles"), QString("LAX"));
}

void TestArrlSection::testCountyNormalization()
{
    // Test removal of "County" suffix
    QCOMPARE(sectionForStateCounty("FL", "Pinellas County"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas county"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));

    // Test St./Saint normalization
    QCOMPARE(sectionForStateCounty("FL", "St. Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Saint Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "St Johns"), QString("NFL"));

    QCOMPARE(sectionForStateCounty("NY", "St. Lawrence"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Saint Lawrence"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "St Lawrence"), QString("NNY"));

    // Test hyphenated county names
    QCOMPARE(sectionForStateCounty("FL", "Miami-Dade"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Miami Dade"), QString("SFL"));
}

void TestArrlSection::testStateNormalization()
{
    // Test whitespace handling
    QCOMPARE(sectionForStateCounty("  FL  ", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "  Pinellas  "), QString("WCF"));
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

void TestArrlSection::testUnknownState()
{
    // Unknown state should return empty QString
    QVERIFY(sectionForStateCounty("ZZ", "Unknown").isEmpty());
    QVERIFY(sectionForStateCounty("XX", "Test").isEmpty());
}

void TestArrlSection::testUnknownCounty()
{
    // Known state but unknown county in subdivided state should return empty
    QVERIFY(sectionForStateCounty("FL", "NonexistentCounty").isEmpty());
    QVERIFY(sectionForStateCounty("CA", "FakeCounty").isEmpty());
}

void TestArrlSection::testEmptyInputs()
{
    // Empty inputs should return empty QString
    QVERIFY(sectionForStateCounty("", "").isEmpty());
    QVERIFY(sectionForStateCounty("FL", "").isEmpty());
    QVERIFY(sectionForStateCounty("", "Pinellas").isEmpty());

    QVERIFY(sectionForState("").isEmpty());
}

QTEST_MAIN(TestArrlSection)
#include "test_arrlsection.moc"
