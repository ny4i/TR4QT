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

    // QSO points calculation
    void testCalculatePoints_SameCountry();
    void testCalculatePoints_SameContinent_NA();
    void testCalculatePoints_SameContinent_NonNA();
    void testCalculatePoints_DifferentContinent();

    // Multipliers
    void testGetMultiplierTypes();
    void testGetMultiplierValue_CQZone();
    void testGetMultiplierValue_Country();
};

// ===== Contest Identity Tests =====

void TestWinterFieldDay::testGetContestId() {
    WinterFieldDayContest contest;
    QCOMPARE(contest.getContestId(), QString("WFD"));
}

void TestWinterFieldDay::testGetContestName() {
    WinterFieldDayContest contest;
    QCOMPARE(contest.getContestName(), QString("Winter Field Day"));
}

void TestWinterFieldDay::testUsesSerialNumbers() {
    WinterFieldDayContest contest;
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
    WinterFieldDayContest contest;
    QString errorMsg;

    // Traditional order: Class + Section
    QVERIFY(contest.validateReceivedExchange("1O WMA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("2I STX", errorMsg));
    QVERIFY(contest.validateReceivedExchange("22O WCF", errorMsg));
}

void TestWinterFieldDay::testValidateExchange_SectionFirst() {
    WinterFieldDayContest contest;
    QString errorMsg;

    // Reversed order: Section + Class
    QVERIFY(contest.validateReceivedExchange("WMA 1O", errorMsg));
    QVERIFY(contest.validateReceivedExchange("STX 2I", errorMsg));
    QVERIFY(contest.validateReceivedExchange("SCV 1H", errorMsg));  // User's test case
    QVERIFY(contest.validateReceivedExchange("WCF 22O", errorMsg));  // User's test case
}

void TestWinterFieldDay::testValidateExchange_AllCategories_OrderAgnostic() {
    WinterFieldDayContest contest;
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
    WinterFieldDayContest contest;
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("1O", errorMsg));
    QVERIFY(errorMsg.contains("must be"));
}

void TestWinterFieldDay::testValidateExchange_InvalidFormat_ThreeParts() {
    WinterFieldDayContest contest;
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("1O WMA EXTRA", errorMsg));
    QVERIFY(errorMsg.contains("must be"));
}

void TestWinterFieldDay::testValidateExchange_InvalidClass() {
    WinterFieldDayContest contest;
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("100O WMA", errorMsg));  // Over 99
    QVERIFY(errorMsg.contains("Invalid class"));
    QVERIFY(errorMsg.contains("[1-99][I/O/H/M]"));
}

void TestWinterFieldDay::testValidateExchange_InvalidSection() {
    WinterFieldDayContest contest;
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("1O ZZZ", errorMsg));
    QVERIFY(errorMsg.contains("Invalid section"));
}

// ===== Exchange Parsing Tests - Field Reordering =====

void TestWinterFieldDay::testParseExchange_ClassFirst() {
    WinterFieldDayContest contest;

    QMap<QString, QString> result = contest.parseReceivedExchange("1O WMA");
    QCOMPARE(result["Class"], QString("1O"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestWinterFieldDay::testParseExchange_SectionFirst() {
    WinterFieldDayContest contest;

    // Parser should detect field types and assign correctly
    QMap<QString, QString> result = contest.parseReceivedExchange("WMA 1O");
    QCOMPARE(result["Class"], QString("1O"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestWinterFieldDay::testParseExchange_AllCombinations() {
    WinterFieldDayContest contest;

    // Test all 4 categories in both orders
    QMap<QString, QString> result;

    result = contest.parseReceivedExchange("SCV 1H");  // User's test case
    QCOMPARE(result["Class"], QString("1H"));
    QCOMPARE(result["Section"], QString("SCV"));

    result = contest.parseReceivedExchange("22O WCF");  // User's test case
    QCOMPARE(result["Class"], QString("22O"));
    QCOMPARE(result["Section"], QString("WCF"));

    result = contest.parseReceivedExchange("STX 1H");
    QCOMPARE(result["Class"], QString("1H"));
    QCOMPARE(result["Section"], QString("STX"));
}

void TestWinterFieldDay::testParseExchange_TwoDigitTransmitter() {
    WinterFieldDayContest contest;

    QMap<QString, QString> result = contest.parseReceivedExchange("STX 22O");
    QCOMPARE(result["Class"], QString("22O"));
    QCOMPARE(result["Section"], QString("STX"));
}

void TestWinterFieldDay::testParseExchange_UppercaseConversion() {
    WinterFieldDayContest contest;

    // Lowercase input should be converted to uppercase
    QMap<QString, QString> result = contest.parseReceivedExchange("wma 1h");
    QCOMPARE(result["Class"], QString("1H"));
    QCOMPARE(result["Section"], QString("WMA"));
}

// ===== QSO Points Tests =====

void TestWinterFieldDay::testCalculatePoints_SameCountry() {
    WinterFieldDayContest contest;

    QSO qso;
    qso.dxccEntity = "United States";
    qso.continent = "NA";

    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    // Same country: 0 points (but counts as multiplier)
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 0);
}

void TestWinterFieldDay::testCalculatePoints_SameContinent_NA() {
    WinterFieldDayContest contest;

    QSO qso;
    qso.dxccEntity = "Canada";
    qso.continent = "NA";

    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    // Different country, same continent (NA): 2 points
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 2);
}

void TestWinterFieldDay::testCalculatePoints_SameContinent_NonNA() {
    WinterFieldDayContest contest;

    QSO qso;
    qso.dxccEntity = "Germany";
    qso.continent = "EU";

    StationInfo myStation;
    myStation.country = "France";
    myStation.continent = "EU";

    // Different country, same continent (non-NA): 1 point
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 1);
}

void TestWinterFieldDay::testCalculatePoints_DifferentContinent() {
    WinterFieldDayContest contest;

    QSO qso;
    qso.dxccEntity = "Japan";
    qso.continent = "AS";

    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    // Different continent: 3 points
    QCOMPARE(contest.calculateQSOPoints(qso, myStation), 3);
}

// ===== Multiplier Tests =====

void TestWinterFieldDay::testGetMultiplierTypes() {
    WinterFieldDayContest contest;

    QList<MultiplierDefinition> mults = contest.getMultiplierTypes();
    QCOMPARE(mults.size(), 2);

    // Should have CQ Zone and Country multipliers
    QVERIFY(mults[0].type == MultiplierType::CQZone || mults[1].type == MultiplierType::CQZone);
    QVERIFY(mults[0].type == MultiplierType::Country || mults[1].type == MultiplierType::Country);
}

void TestWinterFieldDay::testGetMultiplierValue_CQZone() {
    WinterFieldDayContest contest;

    QSO qso;
    qso.cqZone = 14;
    qso.dxccPrefix = "W";

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::CQZone, QStringList());
    QCOMPARE(multValue, QString("14"));

    // Already worked
    QString multValue2 = contest.getMultiplierValue(qso, MultiplierType::CQZone, QStringList() << "14");
    QVERIFY(multValue2.isEmpty());
}

void TestWinterFieldDay::testGetMultiplierValue_Country() {
    WinterFieldDayContest contest;

    QSO qso;
    qso.cqZone = 14;
    qso.dxccPrefix = "W";

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::Country, QStringList());
    QCOMPARE(multValue, QString("W"));

    // Already worked
    QString multValue2 = contest.getMultiplierValue(qso, MultiplierType::Country, QStringList() << "W");
    QVERIFY(multValue2.isEmpty());
}

QTEST_MAIN(TestWinterFieldDay)
#include "test_winterfieldday.moc"
