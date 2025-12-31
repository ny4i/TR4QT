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

QTEST_MAIN(TestSmartExchangeParser)
#include "test_smartexchangeparser.moc"
