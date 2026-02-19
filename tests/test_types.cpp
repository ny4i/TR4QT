#include <QTest>
#include "../src/core/Types.h"

using namespace TR4QT;

/**
 * Unit tests for Types (enum conversions)
 * Tests band/mode/continent string conversions
 */
class TestTypes : public QObject {
    Q_OBJECT

private slots:
    // Band tests
    void testBandToString_AllBands();
    void testStringToBand_AllBands();
    void testStringToBand_Invalid();
    void testBandRoundTrip();

    // Mode tests
    void testModeToString_AllModes();
    void testStringToMode_AllModes();
    void testStringToMode_Invalid();
    void testModeRoundTrip();

    // Continent tests
    void testContinentToString_AllContinents();
};

// Band conversion tests

void TestTypes::testBandToString_AllBands() {
    QCOMPARE(bandToString(BandType::Band160M), QString("160M"));
    QCOMPARE(bandToString(BandType::Band80M), QString("80M"));
    QCOMPARE(bandToString(BandType::Band60M), QString("60M"));
    QCOMPARE(bandToString(BandType::Band40M), QString("40M"));
    QCOMPARE(bandToString(BandType::Band30M), QString("30M"));
    QCOMPARE(bandToString(BandType::Band20M), QString("20M"));
    QCOMPARE(bandToString(BandType::Band17M), QString("17M"));
    QCOMPARE(bandToString(BandType::Band15M), QString("15M"));
    QCOMPARE(bandToString(BandType::Band12M), QString("12M"));
    QCOMPARE(bandToString(BandType::Band10M), QString("10M"));
    QCOMPARE(bandToString(BandType::Band6M), QString("6M"));
    QCOMPARE(bandToString(BandType::Band4M), QString("4M"));
    QCOMPARE(bandToString(BandType::Band2M), QString("2M"));
    QCOMPARE(bandToString(BandType::Band70CM), QString("70CM"));
    QCOMPARE(bandToString(BandType::None), QString("Unknown"));
}

void TestTypes::testStringToBand_AllBands() {
    QCOMPARE(stringToBand("160M"), BandType::Band160M);
    QCOMPARE(stringToBand("80M"), BandType::Band80M);
    QCOMPARE(stringToBand("60M"), BandType::Band60M);
    QCOMPARE(stringToBand("40M"), BandType::Band40M);
    QCOMPARE(stringToBand("30M"), BandType::Band30M);
    QCOMPARE(stringToBand("20M"), BandType::Band20M);
    QCOMPARE(stringToBand("17M"), BandType::Band17M);
    QCOMPARE(stringToBand("15M"), BandType::Band15M);
    QCOMPARE(stringToBand("12M"), BandType::Band12M);
    QCOMPARE(stringToBand("10M"), BandType::Band10M);
    QCOMPARE(stringToBand("6M"), BandType::Band6M);
    QCOMPARE(stringToBand("4M"), BandType::Band4M);
    QCOMPARE(stringToBand("2M"), BandType::Band2M);
    QCOMPARE(stringToBand("70CM"), BandType::Band70CM);
}

void TestTypes::testStringToBand_Invalid() {
    // Invalid/unknown strings should return None
    QCOMPARE(stringToBand(""), BandType::None);
    QCOMPARE(stringToBand("invalid"), BandType::None);
    QCOMPARE(stringToBand("20"), BandType::None);    // Missing "M"
    QCOMPARE(stringToBand("Unknown"), BandType::None);

    // Case-insensitive parsing - lowercase should work
    QCOMPARE(stringToBand("160m"), BandType::Band160M);
    QCOMPARE(stringToBand("20m"), BandType::Band20M);
}

void TestTypes::testBandRoundTrip() {
    // Round-trip: band → string → band should be identity
    QCOMPARE(stringToBand(bandToString(BandType::Band160M)), BandType::Band160M);
    QCOMPARE(stringToBand(bandToString(BandType::Band20M)), BandType::Band20M);
    QCOMPARE(stringToBand(bandToString(BandType::Band2M)), BandType::Band2M);
    QCOMPARE(stringToBand(bandToString(BandType::Band70CM)), BandType::Band70CM);

    // Note: None → "Unknown" → None works because stringToBand("Unknown") returns None
}

// Mode conversion tests

void TestTypes::testModeToString_AllModes() {
    QCOMPARE(modeToString(ModeType::CW), QString("CW"));
    QCOMPARE(modeToString(ModeType::CWR), QString("CW-R"));
    QCOMPARE(modeToString(ModeType::LSB), QString("LSB"));
    QCOMPARE(modeToString(ModeType::USB), QString("USB"));
    QCOMPARE(modeToString(ModeType::FM), QString("FM"));
    QCOMPARE(modeToString(ModeType::AM), QString("AM"));
    QCOMPARE(modeToString(ModeType::RTTY), QString("RTTY"));
    QCOMPARE(modeToString(ModeType::RTTYR), QString("RTTY-R"));
    QCOMPARE(modeToString(ModeType::PSK), QString("PSK"));
    QCOMPARE(modeToString(ModeType::PSKR), QString("PSK-R"));
    QCOMPARE(modeToString(ModeType::FT8), QString("FT8"));
    QCOMPARE(modeToString(ModeType::FT4), QString("FT4"));
    QCOMPARE(modeToString(ModeType::DATA), QString("DATA"));
    QCOMPARE(modeToString(ModeType::DATAR), QString("DATA-R"));
    QCOMPARE(modeToString(ModeType::None), QString("Unknown"));
}

void TestTypes::testStringToMode_AllModes() {
    QCOMPARE(stringToMode("CW"), ModeType::CW);
    QCOMPARE(stringToMode("CW-R"), ModeType::CWR);
    QCOMPARE(stringToMode("LSB"), ModeType::LSB);
    QCOMPARE(stringToMode("USB"), ModeType::USB);
    QCOMPARE(stringToMode("FM"), ModeType::FM);
    QCOMPARE(stringToMode("AM"), ModeType::AM);
    QCOMPARE(stringToMode("RTTY"), ModeType::RTTY);
    QCOMPARE(stringToMode("RTTY-R"), ModeType::RTTYR);
    QCOMPARE(stringToMode("PSK"), ModeType::PSK);
    QCOMPARE(stringToMode("PSK-R"), ModeType::PSKR);
    QCOMPARE(stringToMode("FT8"), ModeType::FT8);
    QCOMPARE(stringToMode("FT4"), ModeType::FT4);
    QCOMPARE(stringToMode("DATA"), ModeType::DATA);
    QCOMPARE(stringToMode("DATA-R"), ModeType::DATAR);

    // ADIF alias mappings (generic mode names that map to specific modes)
    QCOMPARE(stringToMode("SSB"), ModeType::USB);  // SSB defaults to USB per ADIF convention
}

void TestTypes::testStringToMode_Invalid() {
    // Invalid/unknown strings should return None
    QCOMPARE(stringToMode(""), ModeType::None);
    QCOMPARE(stringToMode("invalid"), ModeType::None);
    QCOMPARE(stringToMode("cw"), ModeType::None);  // Lowercase should fail (case sensitive)
    QCOMPARE(stringToMode("Unknown"), ModeType::None);
}

void TestTypes::testModeRoundTrip() {
    // Round-trip: mode → string → mode should be identity
    QCOMPARE(stringToMode(modeToString(ModeType::CW)), ModeType::CW);
    QCOMPARE(stringToMode(modeToString(ModeType::LSB)), ModeType::LSB);
    QCOMPARE(stringToMode(modeToString(ModeType::FT8)), ModeType::FT8);
    QCOMPARE(stringToMode(modeToString(ModeType::RTTY)), ModeType::RTTY);
}

// Continent conversion tests

void TestTypes::testContinentToString_AllContinents() {
    QCOMPARE(continentToString(Continent::AF), QString("AF"));
    QCOMPARE(continentToString(Continent::AS), QString("AS"));
    QCOMPARE(continentToString(Continent::EU), QString("EU"));
    QCOMPARE(continentToString(Continent::NA), QString("NA"));
    QCOMPARE(continentToString(Continent::SA), QString("SA"));
    QCOMPARE(continentToString(Continent::OC), QString("OC"));
    QCOMPARE(continentToString(Continent::None), QString(""));
}

QTEST_MAIN(TestTypes)
#include "test_types.moc"
