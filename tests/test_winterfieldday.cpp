#include <QTest>
#include "../src/contests/WinterFieldDayContest.h"
#include "../src/contests/ContestMetadata.h"
#include "../src/models/QSO.h"
#include "../src/models/StationInfo.h"

using namespace TR4QT;

/**
 * Unit tests for Winter Field Day Contest
 * Tests smart exchange parsing, field reordering, and category validation
 */
class TestWinterFieldDay : public QObject {
    Q_OBJECT

private:
    // Helper to create a test station
    StationInfo testStation() {
        StationInfo station;
        station.callsign = "W1AW";
        station.country = "United States";
        station.continent = "NA";
        station.cqZone = 5;
        station.ituZone = 8;
        return station;
    }

private slots:
    // Contest identity tests
    void testGetContestId();
    void testGetContestName();
    void testUsesSerialNumbers();

    // Class validation tests - All 4 categories
    void testValidateClass_IndoorCategory();
    void testValidateClass_OutdoorCategory();
    void testValidateClass_HomeCategory();
    void testValidateClass_MobileCategory();

    // Transmitter count limits (1-99)
    void testValidateClass_SingleTransmitter();
    void testValidateClass_TwoDigitTransmitter();
    void testValidateClass_MaxTransmitter_99();
    void testValidateClass_InvalidTransmitter_100();
    void testValidateClass_InvalidTransmitter_Zero();
    void testValidateClass_InvalidCategory();

    // Section validation
    void testValidateSection_Valid_US();
    void testValidateSection_Valid_Canadian();
    void testValidateSection_Valid_DX();
    void testValidateSection_Invalid();

    // Exchange validation - Order agnostic
    void testValidateExchange_ClassFirst();
    void testValidateExchange_SectionFirst();
    void testValidateExchange_AllCategories_OrderAgnostic();
    void testValidateExchange_InvalidFormat_OnePart();
    void testValidateExchange_InvalidFormat_ThreeParts();
    void testValidateExchange_InvalidClass();
    void testValidateExchange_InvalidSection();

    // Exchange parsing - Field reordering
    void testParseExchange_ClassFirst();
    void testParseExchange_SectionFirst();
    void testParseExchange_AllCombinations();
    void testParseExchange_TwoDigitTransmitter();
    void testParseExchange_UppercaseConversion();

    // QSO points calculation (mode-based scoring as of v3.8.4)
    void testCalculatePoints_Phone();
    void testCalculatePoints_CW();
    void testCalculatePoints_Digital_RTTY();
    void testCalculatePoints_Digital_PSK();

    // Multipliers
    void testGetMultiplierTypes();
    void testGetMultiplierValue_CQZone();
    void testGetMultiplierValue_Country();
};

// ===== Contest Identity Tests =====

void TestWinterFieldDay::testGetContestId() {
    WinterFieldDayContest contest(testStation());
    QCOMPARE(contest.getContestId(), QString("WFD"));
}

void TestWinterFieldDay::testGetContestName() {
    WinterFieldDayContest contest(testStation());
    QCOMPARE(contest.getContestName(), QString("Winter Field Day"));
}

void TestWinterFieldDay::testUsesSerialNumbers() {
    WinterFieldDayContest contest(testStation());
    QVERIFY(!contest.usesSerialNumbers());
}

// ===== Class Validation Tests =====

void TestWinterFieldDay::testValidateClass_IndoorCategory() {
    QVERIFY(WinterFieldDayContest::isValidClass("1I"));
    QVERIFY(WinterFieldDayContest::isValidClass("2i"));  // Lowercase
    QVERIFY(WinterFieldDayContest::isValidClass("10I"));
    QVERIFY(WinterFieldDayContest::isValidClass("99I"));
}

void TestWinterFieldDay::testValidateClass_OutdoorCategory() {
    QVERIFY(WinterFieldDayContest::isValidClass("1O"));
    QVERIFY(WinterFieldDayContest::isValidClass("2o"));  // Lowercase
    QVERIFY(WinterFieldDayContest::isValidClass("10O"));
    QVERIFY(WinterFieldDayContest::isValidClass("99O"));
}

void TestWinterFieldDay::testValidateClass_HomeCategory() {
    QVERIFY(WinterFieldDayContest::isValidClass("1H"));
    QVERIFY(WinterFieldDayContest::isValidClass("2h"));  // Lowercase
    QVERIFY(WinterFieldDayContest::isValidClass("10H"));
    QVERIFY(WinterFieldDayContest::isValidClass("22H"));  // User's test case
}

void TestWinterFieldDay::testValidateClass_MobileCategory() {
    QVERIFY(WinterFieldDayContest::isValidClass("1M"));
    QVERIFY(WinterFieldDayContest::isValidClass("2m"));  // Lowercase
    QVERIFY(WinterFieldDayContest::isValidClass("10M"));
    QVERIFY(WinterFieldDayContest::isValidClass("99M"));
}

void TestWinterFieldDay::testValidateClass_SingleTransmitter() {
    QVERIFY(WinterFieldDayContest::isValidClass("1O"));
    QVERIFY(WinterFieldDayContest::isValidClass("1I"));
    QVERIFY(WinterFieldDayContest::isValidClass("1H"));
    QVERIFY(WinterFieldDayContest::isValidClass("1M"));
}

void TestWinterFieldDay::testValidateClass_TwoDigitTransmitter() {
    QVERIFY(WinterFieldDayContest::isValidClass("10O"));
    QVERIFY(WinterFieldDayContest::isValidClass("22O"));  // User's test case
    QVERIFY(WinterFieldDayContest::isValidClass("50I"));
}

void TestWinterFieldDay::testValidateClass_MaxTransmitter_99() {
    QVERIFY(WinterFieldDayContest::isValidClass("99O"));
    QVERIFY(WinterFieldDayContest::isValidClass("99I"));
    QVERIFY(WinterFieldDayContest::isValidClass("99H"));
    QVERIFY(WinterFieldDayContest::isValidClass("99M"));
}

void TestWinterFieldDay::testValidateClass_InvalidTransmitter_100() {
    // 100 transmitters exceeds 99 limit
    QVERIFY(!WinterFieldDayContest::isValidClass("100O"));
}

void TestWinterFieldDay::testValidateClass_InvalidTransmitter_Zero() {
    QVERIFY(!WinterFieldDayContest::isValidClass("0O"));
}

void TestWinterFieldDay::testValidateClass_InvalidCategory() {
    // Only I/O/H/M are valid for WFD
    QVERIFY(!WinterFieldDayContest::isValidClass("1A"));  // ARRL FD category
    QVERIFY(!WinterFieldDayContest::isValidClass("1X"));  // Invalid
    QVERIFY(!WinterFieldDayContest::isValidClass("HOME")); // Not number+letter format
}

// ===== Section Validation Tests =====

void TestWinterFieldDay::testValidateSection_Valid_US() {
    QVERIFY(WinterFieldDayContest::isValidSection("WMA"));
    QVERIFY(WinterFieldDayContest::isValidSection("STX"));  // User's test case
    QVERIFY(WinterFieldDayContest::isValidSection("SCV"));  // User's test case
    QVERIFY(WinterFieldDayContest::isValidSection("wma"));  // Lowercase
}

void TestWinterFieldDay::testValidateSection_Valid_Canadian() {
    QVERIFY(WinterFieldDayContest::isValidSection("BC"));
    QVERIFY(WinterFieldDayContest::isValidSection("ON"));
    QVERIFY(WinterFieldDayContest::isValidSection("QC"));
}

void TestWinterFieldDay::testValidateSection_Valid_DX() {
    QVERIFY(WinterFieldDayContest::isValidSection("DX"));
}

void TestWinterFieldDay::testValidateSection_Invalid() {
    QVERIFY(!WinterFieldDayContest::isValidSection("ZZZ"));
    QVERIFY(!WinterFieldDayContest::isValidSection("123"));
    QVERIFY(!WinterFieldDayContest::isValidSection(""));
}

// ===== Exchange Validation Tests - Order Agnostic =====

void TestWinterFieldDay::testValidateExchange_ClassFirst() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    // Traditional order: Class + Section
    QVERIFY(contest.validateReceivedExchange("1O WMA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("2I STX", errorMsg));
    QVERIFY(contest.validateReceivedExchange("22O WCF", errorMsg));
}

void TestWinterFieldDay::testValidateExchange_SectionFirst() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    // Reversed order: Section + Class
    QVERIFY(contest.validateReceivedExchange("WMA 1O", errorMsg));
    QVERIFY(contest.validateReceivedExchange("STX 2I", errorMsg));
    QVERIFY(contest.validateReceivedExchange("SCV 1H", errorMsg));  // User's test case
    QVERIFY(contest.validateReceivedExchange("WCF 22O", errorMsg));  // User's test case
}

void TestWinterFieldDay::testValidateExchange_AllCategories_OrderAgnostic() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    // All 4 categories, both orders
    QVERIFY(contest.validateReceivedExchange("1I WMA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("WMA 1I", errorMsg));

    QVERIFY(contest.validateReceivedExchange("1O WMA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("WMA 1O", errorMsg));

    QVERIFY(contest.validateReceivedExchange("1H WMA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("WMA 1H", errorMsg));

    QVERIFY(contest.validateReceivedExchange("1M WMA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("WMA 1M", errorMsg));
}

void TestWinterFieldDay::testValidateExchange_InvalidFormat_OnePart() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("1O", errorMsg));
    QVERIFY(errorMsg.contains("must be"));
}

void TestWinterFieldDay::testValidateExchange_InvalidFormat_ThreeParts() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("1O WMA EXTRA", errorMsg));
    QVERIFY(errorMsg.contains("must be"));
}

void TestWinterFieldDay::testValidateExchange_InvalidClass() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("100O WMA", errorMsg));  // Over 99
    QVERIFY(errorMsg.contains("Invalid class"));
    QVERIFY(errorMsg.contains("[1-99][I/O/H/M]"));
}

void TestWinterFieldDay::testValidateExchange_InvalidSection() {
    WinterFieldDayContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("1O ZZZ", errorMsg));
    QVERIFY(errorMsg.contains("Invalid section"));
}

// ===== Exchange Parsing Tests - Field Reordering =====

void TestWinterFieldDay::testParseExchange_ClassFirst() {
    WinterFieldDayContest contest(testStation());

    QSO qso;
    contest.parseReceivedExchange("1O WMA", qso);
    QCOMPARE(qso.contestClass, QString("1O"));
    QCOMPARE(qso.arrlSection, QString("WMA"));
}

void TestWinterFieldDay::testParseExchange_SectionFirst() {
    WinterFieldDayContest contest(testStation());

    // Parser should detect field types and assign correctly
    QSO qso;
    contest.parseReceivedExchange("WMA 1O", qso);
    QCOMPARE(qso.contestClass, QString("1O"));
    QCOMPARE(qso.arrlSection, QString("WMA"));
}

void TestWinterFieldDay::testParseExchange_AllCombinations() {
    WinterFieldDayContest contest(testStation());

    // Test all 4 categories in both orders
    QSO qso;

    contest.parseReceivedExchange("SCV 1H", qso);  // User's test case
    QCOMPARE(qso.contestClass, QString("1H"));
    QCOMPARE(qso.arrlSection, QString("SCV"));

    qso = QSO();  // Reset
    contest.parseReceivedExchange("22O WCF", qso);  // User's test case
    QCOMPARE(qso.contestClass, QString("22O"));
    QCOMPARE(qso.arrlSection, QString("WCF"));

    qso = QSO();  // Reset
    contest.parseReceivedExchange("STX 1H", qso);
    QCOMPARE(qso.contestClass, QString("1H"));
    QCOMPARE(qso.arrlSection, QString("STX"));
}

void TestWinterFieldDay::testParseExchange_TwoDigitTransmitter() {
    WinterFieldDayContest contest(testStation());

    QSO qso;
    contest.parseReceivedExchange("STX 22O", qso);
    QCOMPARE(qso.contestClass, QString("22O"));
    QCOMPARE(qso.arrlSection, QString("STX"));
}

void TestWinterFieldDay::testParseExchange_UppercaseConversion() {
    WinterFieldDayContest contest(testStation());

    // Lowercase input should be converted to uppercase
    QSO qso;
    contest.parseReceivedExchange("wma 1h", qso);
    QCOMPARE(qso.contestClass, QString("1H"));
    QCOMPARE(qso.arrlSection, QString("WMA"));
}

// ===== QSO Points Tests (Mode-Based Scoring v3.8.4) =====

void TestWinterFieldDay::testCalculatePoints_Phone() {
    WinterFieldDayContest contest(testStation());
    StationInfo myStation = testStation();

    // Phone modes get 1 point (USB, LSB, FM, AM)
    QSO qso;
    qso.mode = ModeType::USB;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 1);

    qso.mode = ModeType::LSB;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 1);

    qso.mode = ModeType::FM;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 1);

    qso.mode = ModeType::AM;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 1);
}

void TestWinterFieldDay::testCalculatePoints_CW() {
    WinterFieldDayContest contest(testStation());
    StationInfo myStation = testStation();

    // CW modes get 2 points
    QSO qso;
    qso.mode = ModeType::CW;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);

    qso.mode = ModeType::CWR;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);
}

void TestWinterFieldDay::testCalculatePoints_Digital_RTTY() {
    WinterFieldDayContest contest(testStation());
    StationInfo myStation = testStation();

    // RTTY gets 2 points
    QSO qso;
    qso.mode = ModeType::RTTY;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);

    qso.mode = ModeType::RTTYR;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);
}

void TestWinterFieldDay::testCalculatePoints_Digital_PSK() {
    WinterFieldDayContest contest(testStation());
    StationInfo myStation = testStation();

    // Digital modes (PSK, DATA) get 2 points
    QSO qso;
    qso.mode = ModeType::PSK;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);

    qso.mode = ModeType::PSKR;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);

    qso.mode = ModeType::DATA;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);

    qso.mode = ModeType::DATAR;
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);
}

// ===== Multiplier Tests =====

void TestWinterFieldDay::testGetMultiplierTypes() {
    WinterFieldDayContest contest(testStation());

    QList<MultiplierDefinition> mults = contest.getMultiplierTypes();
    QCOMPARE(mults.size(), 1);

    // Should have State (Section) multipliers only
    QVERIFY(mults[0].type == MultiplierType::State);
}

void TestWinterFieldDay::testGetMultiplierValue_CQZone() {
    // NOTE: WFD uses Sections (State), not CQ Zones
    // This test is renamed to testGetMultiplierValue_Section
    WinterFieldDayContest contest(testStation());

    QSO qso;
    qso.arrlSection = "WMA";  // Populated by parseReceivedExchange

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::State, QStringList());
    QCOMPARE(multValue, QString("WMA"));

    // Already worked
    QString multValue2 = contest.getMultiplierValue(qso, MultiplierType::State, QStringList() << "WMA");
    QVERIFY(multValue2.isEmpty());
}

void TestWinterFieldDay::testGetMultiplierValue_Country() {
    // NOTE: WFD uses Sections (State), not Countries
    // Testing another section (using arrlSection fallback)
    WinterFieldDayContest contest(testStation());

    QSO qso;
    // Test fallback: only set arrlSection, not parsedExchange
    qso.arrlSection = "STX";

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::State, QStringList());
    QCOMPARE(multValue, QString("STX"));

    // Already worked
    QString multValue2 = contest.getMultiplierValue(qso, MultiplierType::State, QStringList() << "STX");
    QVERIFY(multValue2.isEmpty());
}

QTEST_MAIN(TestWinterFieldDay)
#include "test_winterfieldday.moc"
