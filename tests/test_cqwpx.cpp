#include <QTest>
#include "../src/contests/CQWPXContest.h"
#include "../src/contests/ContestMetadata.h"
#include "../src/models/QSO.h"
#include "../src/models/StationInfo.h"

using namespace TR4QT;

/**
 * Unit tests for CQ WPX Contest
 * Tests exchange validation, scoring, prefix extraction, and multipliers
 *
 * Note: These tests serve as a template for testing other contest implementations.
 * All contest classes should pass similar tests for:
 * - Exchange validation and parsing
 * - QSO point calculation
 * - Multiplier handling
 * - Factory methods
 */
class TestCQWPX : public QObject {
    Q_OBJECT

private slots:
    // Contest identity and factory tests
    void testGetContestId_CW();
    void testGetContestId_SSB();
    void testGetContestName();
    void testUsesSerialNumbers();
    void testCreate_Factory();
    void testGetMetadata_Completeness();

    // Prefix extraction tests
    void testExtractPrefix_Simple();
    void testExtractPrefix_CallDistrict();
    void testExtractPrefix_Portable_P();
    void testExtractPrefix_Portable_SlashNumber();
    void testExtractPrefix_International();
    void testExtractPrefix_MultipleDigits();
    void testExtractPrefix_NoDigit();

    // Exchange validation tests
    void testValidateExchange_Valid();
    void testValidateExchange_InvalidFormat();
    void testValidateExchange_InvalidRST();
    void testValidateExchange_InvalidSerial_Low();
    void testValidateExchange_InvalidSerial_High();
    void testValidateExchange_InvalidSerial_NonNumeric();

    // Exchange parsing tests
    void testParseExchange_Valid();
    void testParseExchange_MultipleSpaces();
    void testFormatSentExchange_Padding();

    // QSO points - CW mode
    void testCalculatePoints_CW_SameContinent();
    void testCalculatePoints_CW_DifferentContinent();
    void testCalculatePoints_CW_160m_Double();
    void testCalculatePoints_CW_10m_Double();
    void testCalculatePoints_CW_20m_Normal();

    // QSO points - SSB mode
    void testCalculatePoints_SSB_SameContinent();
    void testCalculatePoints_SSB_DifferentContinent();
    void testCalculatePoints_SSB_160m_Double();

    // Total score calculation
    void testCalculateTotalScore_Formula();
    void testCalculateTotalScore_NoPrefixes();

    // Multiplier tests
    void testGetMultiplierTypes();
    void testGetMultiplierValue_Prefix_New();
    void testGetMultiplierValue_Prefix_Duplicate();
};

// Contest identity and factory tests

void TestCQWPX::testGetContestId_CW() {
    CQWPXContest contest(ModeType::CW);
    QCOMPARE(contest.getContestId(), QString("CQWPX_CW"));
}

void TestCQWPX::testGetContestId_SSB() {
    CQWPXContest contest(ModeType::USB);
    QCOMPARE(contest.getContestId(), QString("CQWPX_SSB"));
}

void TestCQWPX::testGetContestName() {
    CQWPXContest contestCW(ModeType::CW);
    QString nameCW = contestCW.getContestName();
    QVERIFY(nameCW.contains("CQ WPX"));
    QVERIFY(nameCW.contains("CW"));

    CQWPXContest contestSSB(ModeType::USB);
    QString nameSSB = contestSSB.getContestName();
    QVERIFY(nameSSB.contains("CQ WPX"));
    QVERIFY(nameSSB.contains("SSB"));
}

void TestCQWPX::testUsesSerialNumbers() {
    CQWPXContest contest(ModeType::CW);
    QVERIFY(contest.usesSerialNumbers());
}

void TestCQWPX::testCreate_Factory() {
    // Test factory method
    ContestBase* contestCW = CQWPXContest::create(ModeType::CW);
    QVERIFY(contestCW != nullptr);
    QCOMPARE(contestCW->getContestId(), QString("CQWPX_CW"));
    delete contestCW;

    ContestBase* contestSSB = CQWPXContest::create(ModeType::USB);
    QVERIFY(contestSSB != nullptr);
    QCOMPARE(contestSSB->getContestId(), QString("CQWPX_SSB"));
    delete contestSSB;
}

void TestCQWPX::testGetMetadata_Completeness() {
    // Test that metadata is complete (template for other contests)
    auto meta = CQWPXContest::getMetadata();

    QCOMPARE(meta.id, QString("CQWPX"));
    QVERIFY(!meta.displayName.isEmpty());
    QVERIFY(!meta.shortName.isEmpty());
    QVERIFY(meta.supportedModes.contains(ModeType::CW));
    QVERIFY(meta.supportedModes.contains(ModeType::USB));
    QVERIFY(meta.hasSeparateContests);
    QVERIFY(!meta.website.isEmpty());
    QVERIFY(!meta.description.isEmpty());
}

// Prefix extraction tests

void TestCQWPX::testExtractPrefix_Simple() {
    QCOMPARE(CQWPXContest::extractPrefix("W1AW"), QString("W1"));
    QCOMPARE(CQWPXContest::extractPrefix("K3LR"), QString("K3"));
    QCOMPARE(CQWPXContest::extractPrefix("N6TR"), QString("N6"));
}

void TestCQWPX::testExtractPrefix_CallDistrict() {
    QCOMPARE(CQWPXContest::extractPrefix("W1ABC"), QString("W1"));
    QCOMPARE(CQWPXContest::extractPrefix("K2XYZ"), QString("K2"));
    QCOMPARE(CQWPXContest::extractPrefix("N5ZZ"), QString("N5"));
}

void TestCQWPX::testExtractPrefix_Portable_P() {
    QCOMPARE(CQWPXContest::extractPrefix("W1AW/P"), QString("W1"));
    QCOMPARE(CQWPXContest::extractPrefix("W1AW/M"), QString("W1"));
}

void TestCQWPX::testExtractPrefix_Portable_SlashNumber() {
    // W1/G3XYZ should extract W1 (prefix before /)
    QCOMPARE(CQWPXContest::extractPrefix("W1/G3XYZ"), QString("W1"));

    // G3XYZ/4 should extract G3 (from the part with digit)
    QCOMPARE(CQWPXContest::extractPrefix("G3XYZ/4"), QString("G3"));
}

void TestCQWPX::testExtractPrefix_International() {
    QCOMPARE(CQWPXContest::extractPrefix("DL1ABC"), QString("DL1"));
    QCOMPARE(CQWPXContest::extractPrefix("JA1XYZ"), QString("JA1"));
    QCOMPARE(CQWPXContest::extractPrefix("G3ABC"), QString("G3"));
    QCOMPARE(CQWPXContest::extractPrefix("VE3XYZ"), QString("VE3"));
}

void TestCQWPX::testExtractPrefix_MultipleDigits() {
    // Extract up to and including FIRST digit
    QCOMPARE(CQWPXContest::extractPrefix("JA1234XYZ"), QString("JA1"));
}

void TestCQWPX::testExtractPrefix_NoDigit() {
    // No digit - return whole callsign
    QString result = CQWPXContest::extractPrefix("ABC");
    QCOMPARE(result, QString("ABC"));
}

// Exchange validation tests

void TestCQWPX::testValidateExchange_Valid() {
    CQWPXContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(contest.validateReceivedExchange("599 001", errorMsg));
    QVERIFY(contest.validateReceivedExchange("599 123", errorMsg));
    QVERIFY(contest.validateReceivedExchange("599 9999", errorMsg));
    QVERIFY(contest.validateReceivedExchange("59 001", errorMsg));  // SSB format also ok
}

void TestCQWPX::testValidateExchange_InvalidFormat() {
    CQWPXContest contest(ModeType::CW);
    QString errorMsg;

    // Too few fields
    QVERIFY(!contest.validateReceivedExchange("599", errorMsg));
    QVERIFY(errorMsg.contains("RST + Serial"));

    // Too many fields
    QVERIFY(!contest.validateReceivedExchange("599 001 extra", errorMsg));
}

void TestCQWPX::testValidateExchange_InvalidRST() {
    CQWPXContest contest(ModeType::CW);
    QString errorMsg;

    // Too short
    QVERIFY(!contest.validateReceivedExchange("5 001", errorMsg));
    QVERIFY(errorMsg.contains("Invalid RST"));

    // Too long
    QVERIFY(!contest.validateReceivedExchange("5999 001", errorMsg));
}

void TestCQWPX::testValidateExchange_InvalidSerial_Low() {
    CQWPXContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("599 0", errorMsg));
    QVERIFY(errorMsg.contains("1-9999"));
}

void TestCQWPX::testValidateExchange_InvalidSerial_High() {
    CQWPXContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("599 10000", errorMsg));
    QVERIFY(errorMsg.contains("1-9999"));
}

void TestCQWPX::testValidateExchange_InvalidSerial_NonNumeric() {
    CQWPXContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("599 ABC", errorMsg));
    QVERIFY(errorMsg.contains("Invalid serial"));
}

// Exchange parsing tests

void TestCQWPX::testParseExchange_Valid() {
    CQWPXContest contest(ModeType::CW);

    auto parsed = contest.parseReceivedExchange("599 123");
    QCOMPARE(parsed["RST"], QString("599"));
    QCOMPARE(parsed["Serial"], QString("123"));
}

void TestCQWPX::testParseExchange_MultipleSpaces() {
    CQWPXContest contest(ModeType::CW);

    auto parsed = contest.parseReceivedExchange("599   123");
    QCOMPARE(parsed["RST"], QString("599"));
    QCOMPARE(parsed["Serial"], QString("123"));
}

void TestCQWPX::testFormatSentExchange_Padding() {
    CQWPXContest contest(ModeType::CW);

    // Serial numbers should be zero-padded to 3 digits
    QString exchange1 = contest.formatSentExchange(1, "599");
    QCOMPARE(exchange1, QString("599 001"));

    QString exchange123 = contest.formatSentExchange(123, "599");
    QCOMPARE(exchange123, QString("599 123"));

    QString exchange1234 = contest.formatSentExchange(1234, "599");
    QCOMPARE(exchange1234, QString("599 1234"));
}

// QSO points - CW mode

void TestCQWPX::testCalculatePoints_CW_SameContinent() {
    CQWPXContest contest(ModeType::CW);

    StationInfo myStation;
    myStation.continent = "EU";

    QSO qso;
    qso.continent = "EU";
    qso.band = BandType::Band20M;  // Normal band

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 1);  // Same continent = 1 point
}

void TestCQWPX::testCalculatePoints_CW_DifferentContinent() {
    CQWPXContest contest(ModeType::CW);

    StationInfo myStation;
    myStation.continent = "NA";

    QSO qso;
    qso.continent = "AS";
    qso.band = BandType::Band20M;  // Normal band

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 3);  // Different continent (CW) = 3 points
}

void TestCQWPX::testCalculatePoints_CW_160m_Double() {
    CQWPXContest contest(ModeType::CW);

    StationInfo myStation;
    myStation.continent = "NA";

    QSO qso;
    qso.continent = "AS";
    qso.band = BandType::Band160M;  // 160m = double points

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 6);  // (3 × 2) = 6 points
}

void TestCQWPX::testCalculatePoints_CW_10m_Double() {
    CQWPXContest contest(ModeType::CW);

    StationInfo myStation;
    myStation.continent = "NA";

    QSO qso;
    qso.continent = "AS";
    qso.band = BandType::Band10M;  // 10m = double points

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 6);  // (3 × 2) = 6 points
}

void TestCQWPX::testCalculatePoints_CW_20m_Normal() {
    CQWPXContest contest(ModeType::CW);

    StationInfo myStation;
    myStation.continent = "EU";

    QSO qso;
    qso.continent = "EU";
    qso.band = BandType::Band20M;  // 20m = normal points

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 1);  // Same continent, normal band = 1 point
}

// QSO points - SSB mode

void TestCQWPX::testCalculatePoints_SSB_SameContinent() {
    CQWPXContest contest(ModeType::USB);

    StationInfo myStation;
    myStation.continent = "EU";

    QSO qso;
    qso.continent = "EU";
    qso.band = BandType::Band20M;

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 1);  // Same continent = 1 point (same as CW)
}

void TestCQWPX::testCalculatePoints_SSB_DifferentContinent() {
    CQWPXContest contest(ModeType::USB);

    StationInfo myStation;
    myStation.continent = "NA";

    QSO qso;
    qso.continent = "AS";
    qso.band = BandType::Band20M;

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 2);  // Different continent (SSB) = 2 points (not 3 like CW)
}

void TestCQWPX::testCalculatePoints_SSB_160m_Double() {
    CQWPXContest contest(ModeType::USB);

    StationInfo myStation;
    myStation.continent = "NA";

    QSO qso;
    qso.continent = "AS";
    qso.band = BandType::Band160M;  // 160m = double points

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 4);  // (2 × 2) = 4 points
}

// Total score calculation

void TestCQWPX::testCalculateTotalScore_Formula() {
    CQWPXContest contest(ModeType::CW);

    // Formula: QSO points × Total Prefixes
    QMap<MultiplierType, int> mults;
    mults[MultiplierType::Prefix] = 250;  // 250 prefixes

    int totalQSOPoints = 2000;
    int score = contest.calculateTotalScore(totalQSOPoints, mults);

    QCOMPARE(score, 2000 * 250);  // 2000 × 250 = 500,000
    QCOMPARE(score, 500000);
}

void TestCQWPX::testCalculateTotalScore_NoPrefixes() {
    CQWPXContest contest(ModeType::CW);

    QMap<MultiplierType, int> mults;
    // No prefixes

    int totalQSOPoints = 2000;
    int score = contest.calculateTotalScore(totalQSOPoints, mults);

    QCOMPARE(score, 0);  // 2000 × 0 = 0
}

// Multiplier tests

void TestCQWPX::testGetMultiplierTypes() {
    CQWPXContest contest(ModeType::CW);

    auto mults = contest.getMultiplierTypes();

    QCOMPARE(mults.size(), 1);  // Only Prefix multiplier

    QCOMPARE(mults[0].type, MultiplierType::Prefix);
    QCOMPARE(mults[0].scope, MultiplierScope::AllBands);  // Counted once across all bands
    QVERIFY(!mults[0].displayName.isEmpty());
}

void TestCQWPX::testGetMultiplierValue_Prefix_New() {
    CQWPXContest contest(ModeType::CW);

    QSO qso;
    qso.callsign = "W1AW";

    QStringList alreadyWorked;  // Empty - first time working W1

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::Prefix, alreadyWorked);
    QCOMPARE(multValue, QString("W1"));  // New multiplier
}

void TestCQWPX::testGetMultiplierValue_Prefix_Duplicate() {
    CQWPXContest contest(ModeType::CW);

    QSO qso;
    qso.callsign = "W1AW";

    QStringList alreadyWorked;
    alreadyWorked << "W1";  // Already worked W1

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::Prefix, alreadyWorked);
    QVERIFY(multValue.isEmpty());  // Not a new multiplier
}

QTEST_MAIN(TestCQWPX)
#include "test_cqwpx.moc"
