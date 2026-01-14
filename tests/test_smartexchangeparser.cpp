#include <QTest>
#include "../src/exchanges/SmartExchangeParser.h"
#include "../src/contests/WinterFieldDayContest.h"
#include "../src/contests/ARRLSweepstakesContest.h"

using namespace TR4QT;

/**
 * Unit tests for Smart Exchange Parser
 * Tests field detection, reordering, and intelligent parsing
 */
class TestSmartExchangeParser : public QObject {
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
    // Winter Field Day parsing
    void testParse_WFD_ClassFirst();
    void testParse_WFD_SectionFirst();
    void testParse_WFD_AllCategories();
    void testParse_WFD_TwoDigitTransmitter();
    void testParse_WFD_MaxTransmitter();
    void testParse_WFD_UppercaseConversion();

    // ARRL Sweepstakes parsing
    void testParse_SS_TraditionalOrder();
    void testParse_SS_PrecedenceFirst();
    void testParse_SS_SectionFirst();
    void testParse_SS_MixedOrder();
    void testParse_SS_PartialExchange();

    // Section detection
    void testSectionDetection_CommonSections();
    void testSectionDetection_Canadian();
    void testSectionDetection_ThreeLetterSections();

    // Class detection
    void testClassDetection_WFD_AllCategories();
    void testClassDetection_WFD_Ranges();
    void testClassDetection_InvalidFormats();

    // Precedence detection
    void testPrecedenceDetection_AllValues();
    void testPrecedenceDetection_Lowercase();

    // Check detection
    void testCheckDetection_Valid();
    void testCheckDetection_Invalid();

    // Serial number detection
    void testSerialDetection_Valid();
    void testSerialDetection_Ambiguous();

    // Edge cases
    void testParse_EmptyExchange();
    void testParse_SingleField();
    void testParse_ExtraSpaces();

    // State detection (for NAQP)
    void testStateDetection_USStates();
    void testStateDetection_CanadianProvinces();
    void testStateDetection_Sections_NotStates();
    void testStateDetection_Names_NotStates();
    void testStateDetection_Numbers_NotStates();
    void testStateDetection_CaseSensitivity();

    // Power detection (for ARRL DX)
    void testPowerDetection_PlainWatts();
    void testPowerDetection_WithWSuffix();
    void testPowerDetection_WithKSuffix();
    void testPowerDetection_InvalidValues();
    void testPowerDetection_RSTNotPower();

    // CQ Zone detection (for CQ WW)
    void testCQZoneDetection_ValidRange();
    void testCQZoneDetection_Boundaries();
    void testCQZoneDetection_Invalid();

    // ITU Zone detection (for IARU HF)
    void testITUZoneDetection_ValidRange();
    void testITUZoneDetection_Boundaries();
    void testITUZoneDetection_Invalid();

    // County detection (for QSO Parties)
    void testCountyDetection_ValidFormats();
    void testCountyDetection_Invalid();
};

// ===== Winter Field Day Parsing Tests =====

void TestSmartExchangeParser::testParse_WFD_ClassFirst() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result = SmartExchangeParser::parse("1O WMA", fields, &contest);

    QCOMPARE(result["Class"], QString("1O"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_WFD_SectionFirst() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Parser should detect Section first, Class second
    QMap<QString, QString> result = SmartExchangeParser::parse("WMA 1O", fields, &contest);

    QCOMPARE(result["Class"], QString("1O"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_WFD_AllCategories() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Test all 4 WFD categories
    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("SCV 1I", fields, &contest);
    QCOMPARE(result["Class"], QString("1I"));
    QCOMPARE(result["Section"], QString("SCV"));

    result = SmartExchangeParser::parse("STX 2O", fields, &contest);
    QCOMPARE(result["Class"], QString("2O"));
    QCOMPARE(result["Section"], QString("STX"));

    result = SmartExchangeParser::parse("WCF 3H", fields, &contest);
    QCOMPARE(result["Class"], QString("3H"));
    QCOMPARE(result["Section"], QString("WCF"));

    result = SmartExchangeParser::parse("UT 4M", fields, &contest);
    QCOMPARE(result["Class"], QString("4M"));
    QCOMPARE(result["Section"], QString("UT"));
}

void TestSmartExchangeParser::testParse_WFD_TwoDigitTransmitter() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // User's test case: 22O
    QMap<QString, QString> result = SmartExchangeParser::parse("STX 22O", fields, &contest);

    QCOMPARE(result["Class"], QString("22O"));
    QCOMPARE(result["Section"], QString("STX"));
}

void TestSmartExchangeParser::testParse_WFD_MaxTransmitter() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Maximum 99 transmitters
    QMap<QString, QString> result = SmartExchangeParser::parse("WMA 99I", fields, &contest);

    QCOMPARE(result["Class"], QString("99I"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_WFD_UppercaseConversion() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Lowercase input
    QMap<QString, QString> result = SmartExchangeParser::parse("wma 1h", fields, &contest);

    QCOMPARE(result["Class"], QString("1H"));
    QCOMPARE(result["Section"], QString("WMA"));
}

// ===== ARRL Sweepstakes Parsing Tests =====

void TestSmartExchangeParser::testParse_SS_TraditionalOrder() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Traditional: Serial Precedence Check Section
    QMap<QString, QString> result = SmartExchangeParser::parse("123 A 95 WMA", fields, &contest);

    QCOMPARE(result["Serial"], QString("123"));
    QCOMPARE(result["Precedence"], QString("A"));
    QCOMPARE(result["Check"], QString("95"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_SS_PrecedenceFirst() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Precedence first
    QMap<QString, QString> result = SmartExchangeParser::parse("A 95 WMA 123", fields, &contest);

    QCOMPARE(result["Serial"], QString("123"));
    QCOMPARE(result["Precedence"], QString("A"));
    QCOMPARE(result["Check"], QString("95"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_SS_SectionFirst() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Section first
    QMap<QString, QString> result = SmartExchangeParser::parse("WMA 123 A 95", fields, &contest);

    QCOMPARE(result["Serial"], QString("123"));
    QCOMPARE(result["Precedence"], QString("A"));
    QCOMPARE(result["Check"], QString("95"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_SS_MixedOrder() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Random order
    QMap<QString, QString> result = SmartExchangeParser::parse("95 WMA 123 A", fields, &contest);

    QCOMPARE(result["Serial"], QString("123"));
    QCOMPARE(result["Precedence"], QString("A"));
    QCOMPARE(result["Check"], QString("95"));
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_SS_PartialExchange() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Just serial and precedence
    QMap<QString, QString> result = SmartExchangeParser::parse("1 M", fields, &contest);

    QCOMPARE(result["Serial"], QString("1"));
    QCOMPARE(result["Precedence"], QString("M"));
}

// ===== Section Detection Tests =====

void TestSmartExchangeParser::testSectionDetection_CommonSections() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("WMA 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("WMA"));

    result = SmartExchangeParser::parse("STX 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("STX"));

    result = SmartExchangeParser::parse("SCV 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("SCV"));
}

void TestSmartExchangeParser::testSectionDetection_Canadian() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("BC 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("BC"));

    result = SmartExchangeParser::parse("ON 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("ON"));

    result = SmartExchangeParser::parse("QC 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("QC"));
}

void TestSmartExchangeParser::testSectionDetection_ThreeLetterSections() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("WCF 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("WCF"));

    result = SmartExchangeParser::parse("NFL 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("NFL"));

    result = SmartExchangeParser::parse("SFL 1O", fields, &contest);
    QCOMPARE(result["Section"], QString("SFL"));
}

// ===== Class Detection Tests =====

void TestSmartExchangeParser::testClassDetection_WFD_AllCategories() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("WMA 1I", fields, &contest);
    QCOMPARE(result["Class"], QString("1I"));

    result = SmartExchangeParser::parse("WMA 1O", fields, &contest);
    QCOMPARE(result["Class"], QString("1O"));

    result = SmartExchangeParser::parse("WMA 1H", fields, &contest);
    QCOMPARE(result["Class"], QString("1H"));

    result = SmartExchangeParser::parse("WMA 1M", fields, &contest);
    QCOMPARE(result["Class"], QString("1M"));
}

void TestSmartExchangeParser::testClassDetection_WFD_Ranges() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    // Single digit
    result = SmartExchangeParser::parse("WMA 5O", fields, &contest);
    QCOMPARE(result["Class"], QString("5O"));

    // Two digits
    result = SmartExchangeParser::parse("WMA 22O", fields, &contest);
    QCOMPARE(result["Class"], QString("22O"));

    // Maximum
    result = SmartExchangeParser::parse("WMA 99O", fields, &contest);
    QCOMPARE(result["Class"], QString("99O"));
}

void TestSmartExchangeParser::testClassDetection_InvalidFormats() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    // 100 transmitters exceeds limit - parser extracts it, validation rejects it
    result = SmartExchangeParser::parse("WMA 100O", fields, &contest);
    // Parser should extract both fields (permissive parsing)
    QVERIFY(result.contains("Class"));
    QVERIFY(result.contains("Section"));
    QCOMPARE(result["Class"], QString("100O"));
    QCOMPARE(result["Section"], QString("WMA"));

    // But validation should reject it
    QString errorMsg;
    QVERIFY(!contest.validateReceivedExchange("WMA 100O", errorMsg));
    QVERIFY(errorMsg.contains("Invalid class"));
}

// ===== Precedence Detection Tests =====

void TestSmartExchangeParser::testPrecedenceDetection_AllValues() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QStringList precedences = {"Q", "A", "B", "M", "U", "S"};

    for (const QString& prec : precedences) {
        QMap<QString, QString> result = SmartExchangeParser::parse(
            QString("%1 95 WMA 123").arg(prec), fields, &contest);
        QCOMPARE(result["Precedence"], prec);
    }
}

void TestSmartExchangeParser::testPrecedenceDetection_Lowercase() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result = SmartExchangeParser::parse("m 95 WMA 123", fields, &contest);
    QCOMPARE(result["Precedence"], QString("M"));
}

// ===== Check Detection Tests =====

void TestSmartExchangeParser::testCheckDetection_Valid() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("A 95 WMA 123", fields, &contest);
    QCOMPARE(result["Check"], QString("95"));

    result = SmartExchangeParser::parse("A 00 WMA 123", fields, &contest);
    QCOMPARE(result["Check"], QString("00"));

    result = SmartExchangeParser::parse("A 50 WMA 123", fields, &contest);
    QCOMPARE(result["Check"], QString("50"));
}

void TestSmartExchangeParser::testCheckDetection_Invalid() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // Single digit should not be detected as check
    QMap<QString, QString> result = SmartExchangeParser::parse("A 5 WMA 123", fields, &contest);
    QVERIFY(result["Check"] != "5");  // Should be detected as something else
}

// ===== Serial Number Detection Tests =====

void TestSmartExchangeParser::testSerialDetection_Valid() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result;

    result = SmartExchangeParser::parse("A 95 WMA 1", fields, &contest);
    QCOMPARE(result["Serial"], QString("1"));

    result = SmartExchangeParser::parse("A 95 WMA 9999", fields, &contest);
    QCOMPARE(result["Serial"], QString("9999"));
}

void TestSmartExchangeParser::testSerialDetection_Ambiguous() {
    ARRLSweepstakesContest contest(ModeType::CW, testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    // "599" looks like RST but Sweepstakes doesn't use RST in exchange
    // So it should be treated as a serial number
    QMap<QString, QString> result = SmartExchangeParser::parse("A 95 WMA 599", fields, &contest);
    QVERIFY(result.contains("Serial"));
    QCOMPARE(result["Serial"], QString("599"));  // Should be treated as serial
}

// ===== Edge Cases =====

void TestSmartExchangeParser::testParse_EmptyExchange() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result = SmartExchangeParser::parse("", fields, &contest);
    QVERIFY(result.isEmpty());
}

void TestSmartExchangeParser::testParse_SingleField() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result = SmartExchangeParser::parse("WMA", fields, &contest);
    // Should detect as section
    QCOMPARE(result["Section"], QString("WMA"));
}

void TestSmartExchangeParser::testParse_ExtraSpaces() {
    WinterFieldDayContest contest(testStation());
    QList<ExchangeField> fields = contest.getReceivedExchangeFields();

    QMap<QString, QString> result = SmartExchangeParser::parse("  WMA   1O  ", fields, &contest);

    QCOMPARE(result["Class"], QString("1O"));
    QCOMPARE(result["Section"], QString("WMA"));
}

// ===== State Detection Tests (for NAQP) =====

void TestSmartExchangeParser::testStateDetection_USStates() {
    // All US states should be recognized
    QStringList usStates = {
        "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA",
        "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD",
        "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ",
        "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC",
        "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY"
    };

    for (const QString& state : usStates) {
        QVERIFY2(SmartExchangeParser::looksLikeState(state),
                 qPrintable(QString("US state %1 should be recognized").arg(state)));
    }

    // Also verify DC
    QVERIFY(SmartExchangeParser::looksLikeState("DC"));
}

void TestSmartExchangeParser::testStateDetection_CanadianProvinces() {
    // Canadian provinces (Ontario subdivisions are sections, not states)
    QStringList provinces = {
        "AB", "BC", "MB", "NB", "NL", "NS", "NT", "NU", "ON", "PE", "QC", "SK", "YT"
    };

    for (const QString& province : provinces) {
        QVERIFY2(SmartExchangeParser::looksLikeState(province),
                 qPrintable(QString("Canadian province %1 should be recognized").arg(province)));
    }
}

void TestSmartExchangeParser::testStateDetection_Sections_NotStates() {
    // ARRL sections are NOT states
    // These are geographic divisions for ARRL contests but are NOT valid for NAQP
    QStringList sections = {
        "WMA", "EMA",  // Massachusetts sections
        "NFL", "WCF", "SFL",  // Florida sections
        "SCV", "EB", "SF", "LAX", "SJV", "SDG", "ORG", "SB", "PAC",  // CA sections
        "ENY", "NLI", "WNY", "NNY",  // NY sections
        "STX", "NTX", "WTX",  // TX sections
        "EPA", "WPA",  // PA sections
        "OH", "MI", "IL", "WI"  // These happen to be both sections and states
    };

    // WMA, EMA, etc. should NOT be states
    QVERIFY(!SmartExchangeParser::looksLikeState("WMA"));
    QVERIFY(!SmartExchangeParser::looksLikeState("EMA"));
    QVERIFY(!SmartExchangeParser::looksLikeState("NFL"));
    QVERIFY(!SmartExchangeParser::looksLikeState("WCF"));
    QVERIFY(!SmartExchangeParser::looksLikeState("SFL"));
    QVERIFY(!SmartExchangeParser::looksLikeState("SCV"));
    QVERIFY(!SmartExchangeParser::looksLikeState("STX"));
    QVERIFY(!SmartExchangeParser::looksLikeState("NTX"));
    QVERIFY(!SmartExchangeParser::looksLikeState("WTX"));

    // But OH, MI, IL, WI ARE states (also sections)
    QVERIFY(SmartExchangeParser::looksLikeState("OH"));
    QVERIFY(SmartExchangeParser::looksLikeState("MI"));
    QVERIFY(SmartExchangeParser::looksLikeState("IL"));
    QVERIFY(SmartExchangeParser::looksLikeState("WI"));
}

void TestSmartExchangeParser::testStateDetection_Names_NotStates() {
    // Common names should NOT be detected as states
    QVERIFY(!SmartExchangeParser::looksLikeState("JOHN"));
    QVERIFY(!SmartExchangeParser::looksLikeState("TOM"));
    QVERIFY(!SmartExchangeParser::looksLikeState("BOB"));
    QVERIFY(!SmartExchangeParser::looksLikeState("DAVE"));
    QVERIFY(!SmartExchangeParser::looksLikeState("JOE"));
    QVERIFY(!SmartExchangeParser::looksLikeState("BILL"));
    QVERIFY(!SmartExchangeParser::looksLikeState("MARY"));
    QVERIFY(!SmartExchangeParser::looksLikeState("SUE"));
}

void TestSmartExchangeParser::testStateDetection_Numbers_NotStates() {
    // Numbers should NOT be detected as states
    QVERIFY(!SmartExchangeParser::looksLikeState("1"));
    QVERIFY(!SmartExchangeParser::looksLikeState("12"));
    QVERIFY(!SmartExchangeParser::looksLikeState("123"));
    QVERIFY(!SmartExchangeParser::looksLikeState("59"));
    QVERIFY(!SmartExchangeParser::looksLikeState("599"));
}

void TestSmartExchangeParser::testStateDetection_CaseSensitivity() {
    // Should work regardless of case
    QVERIFY(SmartExchangeParser::looksLikeState("fl"));
    QVERIFY(SmartExchangeParser::looksLikeState("Fl"));
    QVERIFY(SmartExchangeParser::looksLikeState("fL"));
    QVERIFY(SmartExchangeParser::looksLikeState("FL"));

    QVERIFY(SmartExchangeParser::looksLikeState("ca"));
    QVERIFY(SmartExchangeParser::looksLikeState("Ca"));
    QVERIFY(SmartExchangeParser::looksLikeState("CA"));
}

// ===== Power Detection Tests (for ARRL DX) =====

void TestSmartExchangeParser::testPowerDetection_PlainWatts() {
    // Plain numeric power values (1-2000 watts)
    QVERIFY(SmartExchangeParser::looksLikePower("5"));      // QRP
    QVERIFY(SmartExchangeParser::looksLikePower("10"));
    QVERIFY(SmartExchangeParser::looksLikePower("50"));
    QVERIFY(SmartExchangeParser::looksLikePower("100"));
    QVERIFY(SmartExchangeParser::looksLikePower("500"));
    QVERIFY(SmartExchangeParser::looksLikePower("1000"));
    QVERIFY(SmartExchangeParser::looksLikePower("1500"));
    QVERIFY(SmartExchangeParser::looksLikePower("2000"));
}

void TestSmartExchangeParser::testPowerDetection_WithWSuffix() {
    // Power values with W suffix
    QVERIFY(SmartExchangeParser::looksLikePower("5W"));
    QVERIFY(SmartExchangeParser::looksLikePower("100W"));
    QVERIFY(SmartExchangeParser::looksLikePower("1500W"));
    QVERIFY(SmartExchangeParser::looksLikePower("100w"));  // Lowercase
}

void TestSmartExchangeParser::testPowerDetection_WithKSuffix() {
    // Power values with K suffix (kilowatts)
    QVERIFY(SmartExchangeParser::looksLikePower("1K"));    // 1000W
    QVERIFY(SmartExchangeParser::looksLikePower("1.5K")); // 1500W
    QVERIFY(SmartExchangeParser::looksLikePower("2K"));    // 2000W
    QVERIFY(SmartExchangeParser::looksLikePower("0.5K")); // 500W
    QVERIFY(SmartExchangeParser::looksLikePower("1k"));    // Lowercase
}

void TestSmartExchangeParser::testPowerDetection_InvalidValues() {
    // Invalid power values
    QVERIFY(!SmartExchangeParser::looksLikePower("0"));     // Too low
    QVERIFY(!SmartExchangeParser::looksLikePower("2001")); // Too high
    QVERIFY(!SmartExchangeParser::looksLikePower("3K"));    // 3000W - too high
    QVERIFY(!SmartExchangeParser::looksLikePower("ABC"));  // Not numeric
    QVERIFY(!SmartExchangeParser::looksLikePower(""));      // Empty
}

void TestSmartExchangeParser::testPowerDetection_RSTNotPower() {
    // RST values should NOT be detected as power (special case)
    // These are handled by looksLikeRST() which takes priority
    QVERIFY(!SmartExchangeParser::looksLikePower("599"));  // RST, not 599W
    QVERIFY(!SmartExchangeParser::looksLikePower("59"));   // RST, not 59W
    QVERIFY(!SmartExchangeParser::looksLikePower("579"));  // RST
}

// ===== CQ Zone Detection Tests =====

void TestSmartExchangeParser::testCQZoneDetection_ValidRange() {
    // CQ zones are 1-40
    QVERIFY(SmartExchangeParser::looksLikeCQZone("1"));
    QVERIFY(SmartExchangeParser::looksLikeCQZone("5"));
    QVERIFY(SmartExchangeParser::looksLikeCQZone("14"));
    QVERIFY(SmartExchangeParser::looksLikeCQZone("25"));
    QVERIFY(SmartExchangeParser::looksLikeCQZone("40"));
}

void TestSmartExchangeParser::testCQZoneDetection_Boundaries() {
    // Boundary testing
    QVERIFY(SmartExchangeParser::looksLikeCQZone("1"));    // Minimum
    QVERIFY(SmartExchangeParser::looksLikeCQZone("40"));  // Maximum
    QVERIFY(!SmartExchangeParser::looksLikeCQZone("0"));   // Below minimum
    QVERIFY(!SmartExchangeParser::looksLikeCQZone("41")); // Above maximum
}

void TestSmartExchangeParser::testCQZoneDetection_Invalid() {
    // Invalid CQ zone values
    QVERIFY(!SmartExchangeParser::looksLikeCQZone(""));    // Empty
    QVERIFY(!SmartExchangeParser::looksLikeCQZone("ABC")); // Non-numeric
    QVERIFY(!SmartExchangeParser::looksLikeCQZone("-1"));  // Negative
    QVERIFY(!SmartExchangeParser::looksLikeCQZone("100")); // Way too high
}

// ===== ITU Zone Detection Tests =====

void TestSmartExchangeParser::testITUZoneDetection_ValidRange() {
    // ITU zones are 1-90
    QVERIFY(SmartExchangeParser::looksLikeITUZone("1"));
    QVERIFY(SmartExchangeParser::looksLikeITUZone("8"));
    QVERIFY(SmartExchangeParser::looksLikeITUZone("46"));
    QVERIFY(SmartExchangeParser::looksLikeITUZone("75"));
    QVERIFY(SmartExchangeParser::looksLikeITUZone("90"));
}

void TestSmartExchangeParser::testITUZoneDetection_Boundaries() {
    // Boundary testing
    QVERIFY(SmartExchangeParser::looksLikeITUZone("1"));    // Minimum
    QVERIFY(SmartExchangeParser::looksLikeITUZone("90"));  // Maximum
    QVERIFY(!SmartExchangeParser::looksLikeITUZone("0"));   // Below minimum
    QVERIFY(!SmartExchangeParser::looksLikeITUZone("91")); // Above maximum
}

void TestSmartExchangeParser::testITUZoneDetection_Invalid() {
    // Invalid ITU zone values
    QVERIFY(!SmartExchangeParser::looksLikeITUZone(""));    // Empty
    QVERIFY(!SmartExchangeParser::looksLikeITUZone("ABC")); // Non-numeric
    QVERIFY(!SmartExchangeParser::looksLikeITUZone("-1"));  // Negative
    QVERIFY(!SmartExchangeParser::looksLikeITUZone("100")); // Too high
}

// ===== County Detection Tests =====

void TestSmartExchangeParser::testCountyDetection_ValidFormats() {
    // Counties are typically 3-letter codes (QSO Parties)
    // Note: without a contest context, detection is heuristic
    QVERIFY(SmartExchangeParser::looksLikeCounty("PAL", nullptr));  // Florida county
    QVERIFY(SmartExchangeParser::looksLikeCounty("DUV", nullptr));  // Florida county
    QVERIFY(SmartExchangeParser::looksLikeCounty("ORA", nullptr));  // Florida county
}

void TestSmartExchangeParser::testCountyDetection_Invalid() {
    // Invalid county values
    QVERIFY(!SmartExchangeParser::looksLikeCounty("", nullptr));      // Empty
    QVERIFY(!SmartExchangeParser::looksLikeCounty("A", nullptr));     // Too short
    QVERIFY(!SmartExchangeParser::looksLikeCounty("ABCDEF", nullptr)); // Too long
    QVERIFY(!SmartExchangeParser::looksLikeCounty("123", nullptr));   // Numeric
    QVERIFY(!SmartExchangeParser::looksLikeCounty("FL", nullptr));    // State, not county

    // Known sections should NOT be detected as counties
    QVERIFY(!SmartExchangeParser::looksLikeCounty("WMA", nullptr));  // ARRL section
    QVERIFY(!SmartExchangeParser::looksLikeCounty("NFL", nullptr));  // ARRL section
}

QTEST_MAIN(TestSmartExchangeParser)
#include "test_smartexchangeparser.moc"
