#include <QTest>
#include "../src/contests/CQWWContest.h"
#include "../src/contests/ContestMetadata.h"
#include "../src/models/QSO.h"
#include "../src/models/StationInfo.h"

using namespace TR4QT;

/**
 * Unit tests for CQ WW DX Contest
 * Tests exchange validation, scoring, and multipliers
 */
class TestCQWW : public QObject {
    Q_OBJECT

private slots:
    // Contest identity tests
    void testGetContestId_CW();
    void testGetContestId_SSB();
    void testGetContestName_CW();
    void testGetContestName_SSB();
    void testUsesSerialNumbers();

    // Exchange validation tests
    void testValidateExchange_Valid_CW();
    void testValidateExchange_Valid_SSB();
    void testValidateExchange_InvalidFormat();
    void testValidateExchange_InvalidRST_CW();
    void testValidateExchange_InvalidRST_SSB();
    void testValidateExchange_InvalidZone_Low();
    void testValidateExchange_InvalidZone_High();
    void testValidateExchange_InvalidZone_NonNumeric();

    // Exchange parsing tests
    void testParseExchange_Valid();
    void testParseExchange_MultipleSpaces();
    void testParseExchange_OrderAgnostic();

    // QSO points - CW mode
    void testCalculatePoints_CW_SameContinent();
    void testCalculatePoints_CW_DifferentContinent();
    void testCalculatePoints_CW_WVE_Rule();
    void testCalculatePoints_CW_SameCountry();

    // QSO points - SSB mode
    void testCalculatePoints_SSB_SameContinent();
    void testCalculatePoints_SSB_DifferentContinent();
    void testCalculatePoints_SSB_WVE_Rule();

    // Total score calculation
    void testCalculateTotalScore_Formula();
    void testCalculateTotalScore_NoMultipliers();

    // Multiplier tests
    void testGetMultiplierTypes();
    void testGetMultiplierValue_Country_New();
    void testGetMultiplierValue_Country_Duplicate();
    void testGetMultiplierValue_Zone_New();
    void testGetMultiplierValue_Zone_Duplicate();

    // Metadata tests
    void testGetMetadata();
};

// Contest identity tests

void TestCQWW::testGetContestId_CW() {
    CQWWContest contest(ModeType::CW);
    QCOMPARE(contest.getContestId(), QString("CQWW_CW"));
}

void TestCQWW::testGetContestId_SSB() {
    CQWWContest contest(ModeType::USB);
    QCOMPARE(contest.getContestId(), QString("CQWW_SSB"));
}

void TestCQWW::testGetContestName_CW() {
    CQWWContest contest(ModeType::CW);
    QString name = contest.getContestName();
    QVERIFY(name.contains("CQ World Wide"));
    QVERIFY(name.contains("CW"));
}

void TestCQWW::testGetContestName_SSB() {
    CQWWContest contest(ModeType::USB);
    QString name = contest.getContestName();
    QVERIFY(name.contains("CQ World Wide"));
    QVERIFY(name.contains("SSB"));
}

void TestCQWW::testUsesSerialNumbers() {
    CQWWContest contest(ModeType::CW);
    QVERIFY(!contest.usesSerialNumbers());
}

// Exchange validation tests

void TestCQWW::testValidateExchange_Valid_CW() {
    CQWWContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(contest.validateReceivedExchange("599 05", errorMsg));
    QVERIFY(contest.validateReceivedExchange("599 14", errorMsg));
    QVERIFY(contest.validateReceivedExchange("599 1", errorMsg));
    QVERIFY(contest.validateReceivedExchange("599 40", errorMsg));
}

void TestCQWW::testValidateExchange_Valid_SSB() {
    CQWWContest contest(ModeType::USB);
    QString errorMsg;

    QVERIFY(contest.validateReceivedExchange("59 05", errorMsg));
    QVERIFY(contest.validateReceivedExchange("59 14", errorMsg));
    QVERIFY(contest.validateReceivedExchange("599 14", errorMsg));  // 3 digits also valid for SSB
}

void TestCQWW::testValidateExchange_InvalidFormat() {
    CQWWContest contest(ModeType::CW);
    QString errorMsg;

    // Single number is accepted as zone (RST auto-filled)
    QVERIFY(contest.validateReceivedExchange("14", errorMsg));
    QVERIFY(contest.validateReceivedExchange("1", errorMsg));
    QVERIFY(contest.validateReceivedExchange("40", errorMsg));

    // Zone out of range
    QVERIFY(!contest.validateReceivedExchange("599", errorMsg));
    QVERIFY(errorMsg.contains("between 1 and 40"));

    // Empty
    QVERIFY(!contest.validateReceivedExchange("", errorMsg));
}

void TestCQWW::testValidateExchange_InvalidRST_CW() {
    CQWWContest contest(ModeType::CW);
    QString errorMsg;

    // CW requires 3-digit RST
    QVERIFY(!contest.validateReceivedExchange("59 14", errorMsg));
    QVERIFY(errorMsg.contains("3 digits"));

    QVERIFY(!contest.validateReceivedExchange("5999 14", errorMsg));
}

void TestCQWW::testValidateExchange_InvalidRST_SSB() {
    CQWWContest contest(ModeType::USB);
    QString errorMsg;

    // SSB requires 2-3 digit RST with pattern [1-5][1-9][1-9]?
    // Neither field is valid RST - both fail pattern
    QVERIFY(!contest.validateReceivedExchange("60 70", errorMsg));
    QVERIFY(errorMsg.contains("Pattern") || errorMsg.contains("RST"));

    // "80 90" - both have invalid first digit (8,9 > 5)
    QVERIFY(!contest.validateReceivedExchange("80 90", errorMsg));
    QVERIFY(errorMsg.contains("Pattern") || errorMsg.contains("RST"));
}

void TestCQWW::testValidateExchange_InvalidZone_Low() {
    CQWWContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("599 0", errorMsg));
    QVERIFY(errorMsg.contains("1 and 40"));
}

void TestCQWW::testValidateExchange_InvalidZone_High() {
    CQWWContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("599 41", errorMsg));
    QVERIFY(errorMsg.contains("1 and 40"));

    QVERIFY(!contest.validateReceivedExchange("599 99", errorMsg));
}

void TestCQWW::testValidateExchange_InvalidZone_NonNumeric() {
    CQWWContest contest(ModeType::CW);
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("599 AB", errorMsg));
    QVERIFY(errorMsg.contains("1 and 40"));
}

// Exchange parsing tests

void TestCQWW::testParseExchange_Valid() {
    CQWWContest contest(ModeType::CW);

    auto parsed = contest.parseReceivedExchange("599 14");
    QCOMPARE(parsed["RST"], QString("599"));
    QCOMPARE(parsed["Zone"], QString("14"));

    parsed = contest.parseReceivedExchange("599 5");
    QCOMPARE(parsed["RST"], QString("599"));
    QCOMPARE(parsed["Zone"], QString("5"));
}

void TestCQWW::testParseExchange_MultipleSpaces() {
    CQWWContest contest(ModeType::CW);

    // Should handle multiple spaces
    auto parsed = contest.parseReceivedExchange("599   14");
    QCOMPARE(parsed["RST"], QString("599"));
    QCOMPARE(parsed["Zone"], QString("14"));
}

void TestCQWW::testParseExchange_OrderAgnostic() {
    CQWWContest ssbContest(ModeType::USB);
    CQWWContest cwContest(ModeType::CW);

    // Single number treated as zone (RST auto-filled)
    auto parsed1 = ssbContest.parseReceivedExchange("14");
    QCOMPARE(parsed1["Zone"], QString("14"));
    QCOMPARE(parsed1["RST"], QString("59"));

    // Zone first, RST second (non-traditional order)
    auto parsed2 = ssbContest.parseReceivedExchange("14 59");
    QCOMPARE(parsed2["Zone"], QString("14"));
    QCOMPARE(parsed2["RST"], QString("59"));

    // RST first, Zone second (traditional order)
    auto parsed3 = cwContest.parseReceivedExchange("599 14");
    QCOMPARE(parsed3["RST"], QString("599"));
    QCOMPARE(parsed3["Zone"], QString("14"));
}

// QSO points - CW mode

void TestCQWW::testCalculatePoints_CW_SameContinent() {
    CQWWContest contest(ModeType::CW);

    // Setup: Germany working France (both EU, different countries)
    StationInfo myStation;
    myStation.country = "Germany";
    myStation.continent = "EU";

    QSO qso;
    qso.dxccEntity = "France";
    qso.continent = "EU";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 1);  // Same continent, different country = 1 point
}

void TestCQWW::testCalculatePoints_CW_DifferentContinent() {
    CQWWContest contest(ModeType::CW);

    // Setup: US station working Japan (different continents)
    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    QSO qso;
    qso.dxccEntity = "Japan";
    qso.continent = "AS";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 3);  // Different continent (CW) = 3 points
}

void TestCQWW::testCalculatePoints_CW_WVE_Rule() {
    CQWWContest contest(ModeType::CW);

    // Setup: US station working Canada (W/VE special rule)
    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    QSO qso;
    qso.dxccEntity = "Canada";
    qso.continent = "NA";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 2);  // W/VE working each other = 2 points

    // Verify reverse: Canada working US
    myStation.country = "Canada";
    qso.dxccEntity = "United States";

    points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 2);
}

void TestCQWW::testCalculatePoints_CW_SameCountry() {
    CQWWContest contest(ModeType::CW);

    // Setup: Germany station working Germany (same country)
    StationInfo myStation;
    myStation.country = "Germany";
    myStation.continent = "EU";

    QSO qso;
    qso.dxccEntity = "Germany";
    qso.continent = "EU";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 0);  // Same country = 0 points
}

// QSO points - SSB mode

void TestCQWW::testCalculatePoints_SSB_SameContinent() {
    CQWWContest contest(ModeType::USB);

    // Setup: EU station working EU (same continent, different country)
    StationInfo myStation;
    myStation.country = "Germany";
    myStation.continent = "EU";

    QSO qso;
    qso.dxccEntity = "France";
    qso.continent = "EU";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 1);  // Same continent = 1 point (same for SSB)
}

void TestCQWW::testCalculatePoints_SSB_DifferentContinent() {
    CQWWContest contest(ModeType::USB);

    // Setup: US station working Japan (different continents)
    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    QSO qso;
    qso.dxccEntity = "Japan";
    qso.continent = "AS";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 2);  // Different continent (SSB) = 2 points (not 3 like CW)
}

void TestCQWW::testCalculatePoints_SSB_WVE_Rule() {
    CQWWContest contest(ModeType::USB);

    // Setup: US station working Canada (W/VE special rule applies to SSB too)
    StationInfo myStation;
    myStation.country = "United States";
    myStation.continent = "NA";

    QSO qso;
    qso.dxccEntity = "Canada";
    qso.continent = "NA";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 2);  // W/VE rule applies to both CW and SSB
}

// Total score calculation

void TestCQWW::testCalculateTotalScore_Formula() {
    CQWWContest contest(ModeType::CW);

    // Formula: QSO points × (Countries + Zones)
    QMap<MultiplierType, int> mults;
    mults[MultiplierType::Country] = 50;  // 50 countries
    mults[MultiplierType::CQZone] = 15;   // 15 zones

    int totalQSOPoints = 1000;
    int score = contest.calculateTotalScore(totalQSOPoints, mults);

    QCOMPARE(score, 1000 * (50 + 15));  // 1000 × 65 = 65,000
    QCOMPARE(score, 65000);
}

void TestCQWW::testCalculateTotalScore_NoMultipliers() {
    CQWWContest contest(ModeType::CW);

    QMap<MultiplierType, int> mults;
    // No multipliers

    int totalQSOPoints = 1000;
    int score = contest.calculateTotalScore(totalQSOPoints, mults);

    QCOMPARE(score, 0);  // 1000 × 0 = 0
}

// Multiplier tests

void TestCQWW::testGetMultiplierTypes() {
    CQWWContest contest(ModeType::CW);

    auto mults = contest.getMultiplierTypes();

    QCOMPARE(mults.size(), 2);  // Country and CQZone

    // Check for Country multiplier
    bool hasCountry = false;
    bool hasCQZone = false;

    for (const auto& mult : mults) {
        if (mult.type == MultiplierType::Country) {
            hasCountry = true;
            QCOMPARE(mult.scope, MultiplierScope::PerBand);
        }
        if (mult.type == MultiplierType::CQZone) {
            hasCQZone = true;
            QCOMPARE(mult.scope, MultiplierScope::PerBand);
        }
    }

    QVERIFY(hasCountry);
    QVERIFY(hasCQZone);
}

void TestCQWW::testGetMultiplierValue_Country_New() {
    CQWWContest contest(ModeType::CW);

    QSO qso;
    qso.dxccPrefix = "JA";  // Japan
    qso.cqZone = 25;

    QStringList alreadyWorked;  // Empty - first time working Japan

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::Country, alreadyWorked);
    QCOMPARE(multValue, QString("JA"));  // New multiplier
}

void TestCQWW::testGetMultiplierValue_Country_Duplicate() {
    CQWWContest contest(ModeType::CW);

    QSO qso;
    qso.dxccPrefix = "JA";  // Japan
    qso.cqZone = 25;

    QStringList alreadyWorked;
    alreadyWorked << "JA";  // Already worked Japan

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::Country, alreadyWorked);
    QVERIFY(multValue.isEmpty());  // Not a new multiplier
}

void TestCQWW::testGetMultiplierValue_Zone_New() {
    CQWWContest contest(ModeType::CW);

    QSO qso;
    qso.dxccPrefix = "JA";
    qso.cqZone = 25;

    QStringList alreadyWorked;  // Empty - first time working zone 25

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::CQZone, alreadyWorked);
    QCOMPARE(multValue, QString("25"));  // New multiplier
}

void TestCQWW::testGetMultiplierValue_Zone_Duplicate() {
    CQWWContest contest(ModeType::CW);

    QSO qso;
    qso.dxccPrefix = "JA";
    qso.cqZone = 25;

    QStringList alreadyWorked;
    alreadyWorked << "25";  // Already worked zone 25

    QString multValue = contest.getMultiplierValue(qso, MultiplierType::CQZone, alreadyWorked);
    QVERIFY(multValue.isEmpty());  // Not a new multiplier
}

// Metadata tests

void TestCQWW::testGetMetadata() {
    auto meta = CQWWContest::getMetadata();

    QCOMPARE(meta.id, QString("CQWW"));
    QVERIFY(meta.displayName.contains("CQ World Wide"));
    QVERIFY(meta.hasSeparateContests);
    QVERIFY(meta.supportedModes.contains(ModeType::CW));
    QVERIFY(meta.supportedModes.contains(ModeType::USB));
    QVERIFY(!meta.website.isEmpty());
}

QTEST_MAIN(TestCQWW)
#include "test_cqww.moc"
