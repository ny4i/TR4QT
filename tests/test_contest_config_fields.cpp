#include <QTest>
#include "../src/contests/ContestBase.h"
#include "../src/contests/NAQPSSBContest.h"
#include "../src/contests/NAQPCWContest.h"
#include "../src/contests/WinterFieldDayContest.h"
#include "../src/contests/ARRLSSCWContest.h"
#include "../src/contests/ARRLSSSSBContest.h"
#include "../src/contests/CQWWCWContest.h"
#include "../src/models/QSO.h"

using namespace TR4QT;

/**
 * Unit tests for Contest Config Fields (Contest Wizard)
 *
 * Tests the dynamic configuration field system that allows each contest
 * to declare what user input it needs at contest creation time.
 *
 * Key features tested:
 * - ContestConfigField struct constructors
 * - getConfigFields() returns correct fields for each contest
 * - Canonical exchange ordering via buildCanonicalExchange()
 * - getQSOFieldValue() field extraction helper
 */
class TestContestConfigFields : public QObject {
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
    // ContestConfigField struct tests
    void testConfigField_TextConstructor();
    void testConfigField_DropdownFactory();
    void testConfigField_DefaultValues();

    // getConfigFields() tests for each contest type
    void testGetConfigFields_NAQP();
    void testGetConfigFields_WinterFieldDay();
    void testGetConfigFields_ARRLSweepstakes();
    void testGetConfigFields_CQWW_ReturnsEmpty();

    // Canonical exchange ordering tests
    void testCanonicalExchange_NAQP();
    void testCanonicalExchange_WFD();
    void testCanonicalExchange_Sweepstakes();

    // getQSOFieldValue() helper tests
    void testGetQSOFieldValue_Name();
    void testGetQSOFieldValue_State();
    void testGetQSOFieldValue_Section();
    void testGetQSOFieldValue_Class();
    void testGetQSOFieldValue_Check();
    void testGetQSOFieldValue_Precedence();
    void testGetQSOFieldValue_Serial();
    void testGetQSOFieldValue_Zone();
    void testGetQSOFieldValue_Unknown();

    // Edge case tests
    void testCanonicalExchange_WithEmptyFields();
    void testCanonicalExchange_PartialFields_NAQP();
    void testCanonicalExchange_PartialFields_Sweepstakes();
    void testConfigFields_CWAndSSBVariantsMatch();
    void testConfigField_OptionalField();
    void testGetQSOFieldValue_EmptyStrings();
    void testGetQSOFieldValue_ZeroZone();
};

// ===== ContestConfigField Struct Tests =====

void TestContestConfigFields::testConfigField_TextConstructor() {
    ContestConfigField field("NAME", "Contest Name:", "First name (e.g., TOM)",
                             "Station/firstName", 20, true);

    QCOMPARE(field.id, QString("NAME"));
    QCOMPARE(field.label, QString("Contest Name:"));
    QCOMPARE(field.placeholder, QString("First name (e.g., TOM)"));
    QCOMPARE(field.settingsKey, QString("Station/firstName"));
    QCOMPARE(field.maxLength, 20);
    QCOMPARE(field.required, true);
    QCOMPARE(field.type, ContestConfigField::Type::Text);
    QVERIFY(field.options.isEmpty());
}

void TestContestConfigFields::testConfigField_DropdownFactory() {
    ContestConfigField field = ContestConfigField::dropdown(
        "PRECEDENCE", "Precedence:", {"Q", "A", "B", "U", "M", "S"}, true);

    QCOMPARE(field.id, QString("PRECEDENCE"));
    QCOMPARE(field.label, QString("Precedence:"));
    QCOMPARE(field.type, ContestConfigField::Type::DropDown);
    QCOMPARE(field.options.size(), 6);
    QCOMPARE(field.options[0], QString("Q"));
    QCOMPARE(field.options[5], QString("S"));
    QCOMPARE(field.required, true);
}

void TestContestConfigFields::testConfigField_DefaultValues() {
    ContestConfigField field;

    QVERIFY(field.id.isEmpty());
    QVERIFY(field.label.isEmpty());
    QCOMPARE(field.maxLength, 0);
    QCOMPARE(field.required, true);
    QCOMPARE(field.type, ContestConfigField::Type::Text);
}

// ===== getConfigFields() Tests =====

void TestContestConfigFields::testGetConfigFields_NAQP() {
    NAQPSSBContest contest(testStation());
    QList<ContestConfigField> fields = contest.getConfigFields();

    QCOMPARE(fields.size(), 2);

    // First field: NAME
    QCOMPARE(fields[0].id, QString("NAME"));
    QCOMPARE(fields[0].type, ContestConfigField::Type::Text);
    QCOMPARE(fields[0].settingsKey, QString("Station/firstName"));

    // Second field: STATE
    QCOMPARE(fields[1].id, QString("STATE"));
    QCOMPARE(fields[1].type, ContestConfigField::Type::Text);
    QCOMPARE(fields[1].settingsKey, QString("Station/state"));
}

void TestContestConfigFields::testGetConfigFields_WinterFieldDay() {
    WinterFieldDayContest contest(testStation());
    QList<ContestConfigField> fields = contest.getConfigFields();

    QCOMPARE(fields.size(), 2);

    // First field: CLASS
    QCOMPARE(fields[0].id, QString("CLASS"));
    QCOMPARE(fields[0].type, ContestConfigField::Type::Text);
    QVERIFY(fields[0].settingsKey.isEmpty());  // No default for class

    // Second field: SECTION
    QCOMPARE(fields[1].id, QString("SECTION"));
    QCOMPARE(fields[1].type, ContestConfigField::Type::Text);
    QCOMPARE(fields[1].settingsKey, QString("Station/arrlSection"));
}

void TestContestConfigFields::testGetConfigFields_ARRLSweepstakes() {
    ARRLSSCWContest contest(testStation());
    QList<ContestConfigField> fields = contest.getConfigFields();

    QCOMPARE(fields.size(), 3);

    // First field: PRECEDENCE (dropdown)
    QCOMPARE(fields[0].id, QString("PRECEDENCE"));
    QCOMPARE(fields[0].type, ContestConfigField::Type::DropDown);
    QCOMPARE(fields[0].options.size(), 6);
    QVERIFY(fields[0].options.contains("Q"));
    QVERIFY(fields[0].options.contains("A"));

    // Second field: CHECK
    QCOMPARE(fields[1].id, QString("CHECK"));
    QCOMPARE(fields[1].type, ContestConfigField::Type::Text);
    QCOMPARE(fields[1].maxLength, 2);

    // Third field: SECTION
    QCOMPARE(fields[2].id, QString("SECTION"));
    QCOMPARE(fields[2].type, ContestConfigField::Type::Text);
    QCOMPARE(fields[2].settingsKey, QString("Station/arrlSection"));
}

void TestContestConfigFields::testGetConfigFields_CQWW_ReturnsEmpty() {
    // CQWW doesn't need any config fields - exchange is just RST + Zone
    CQWWCWContest contest(testStation());
    QList<ContestConfigField> fields = contest.getConfigFields();

    QVERIFY(fields.isEmpty());
}

// ===== Canonical Exchange Ordering Tests =====

void TestContestConfigFields::testCanonicalExchange_NAQP() {
    NAQPSSBContest contest(testStation());
    QSO qso;
    qso.operatorName = "TOM";
    qso.state = "FL";

    QString canonical = contest.buildCanonicalExchange(qso);

    // NAQP fields are: Name, State
    QCOMPARE(canonical, QString("TOM FL"));
}

void TestContestConfigFields::testCanonicalExchange_WFD() {
    WinterFieldDayContest contest(testStation());
    QSO qso;
    qso.contestClass = "1H";
    qso.arrlSection = "WCF";

    QString canonical = contest.buildCanonicalExchange(qso);

    // WFD fields are: Class, Section
    QCOMPARE(canonical, QString("1H WCF"));
}

void TestContestConfigFields::testCanonicalExchange_Sweepstakes() {
    ARRLSSCWContest contest(testStation());
    QSO qso;
    qso.serialNumberReceived = 123;
    qso.precedence = "A";
    qso.check = "95";
    qso.arrlSection = "WMA";

    QString canonical = contest.buildCanonicalExchange(qso);

    // SS fields are: Serial, Precedence, Check, Section
    QCOMPARE(canonical, QString("123 A 95 WMA"));
}

// ===== getQSOFieldValue() Helper Tests =====

void TestContestConfigFields::testGetQSOFieldValue_Name() {
    NAQPSSBContest contest(testStation());
    QSO qso;
    qso.operatorName = "JOHN";

    QString value = contest.getQSOFieldValue(qso, "Name");
    QCOMPARE(value, QString("JOHN"));
}

void TestContestConfigFields::testGetQSOFieldValue_State() {
    NAQPSSBContest contest(testStation());
    QSO qso;
    qso.state = "CA";

    QString value = contest.getQSOFieldValue(qso, "State");
    QCOMPARE(value, QString("CA"));
}

void TestContestConfigFields::testGetQSOFieldValue_Section() {
    WinterFieldDayContest contest(testStation());
    QSO qso;
    qso.arrlSection = "WCF";

    QString value = contest.getQSOFieldValue(qso, "Section");
    QCOMPARE(value, QString("WCF"));
}

void TestContestConfigFields::testGetQSOFieldValue_Class() {
    WinterFieldDayContest contest(testStation());
    QSO qso;
    qso.contestClass = "2O";

    QString value = contest.getQSOFieldValue(qso, "Class");
    QCOMPARE(value, QString("2O"));
}

void TestContestConfigFields::testGetQSOFieldValue_Check() {
    ARRLSSCWContest contest(testStation());
    QSO qso;
    qso.check = "82";

    QString value = contest.getQSOFieldValue(qso, "Check");
    QCOMPARE(value, QString("82"));
}

void TestContestConfigFields::testGetQSOFieldValue_Precedence() {
    ARRLSSCWContest contest(testStation());
    QSO qso;
    qso.precedence = "Q";

    QString value = contest.getQSOFieldValue(qso, "Precedence");
    QCOMPARE(value, QString("Q"));
}

void TestContestConfigFields::testGetQSOFieldValue_Serial() {
    ARRLSSCWContest contest(testStation());
    QSO qso;
    qso.serialNumberReceived = 456;

    QString value = contest.getQSOFieldValue(qso, "Serial");
    QCOMPARE(value, QString("456"));

    // Test zero returns empty
    QSO qso2;
    qso2.serialNumberReceived = 0;
    QString value2 = contest.getQSOFieldValue(qso2, "Serial");
    QVERIFY(value2.isEmpty());
}

void TestContestConfigFields::testGetQSOFieldValue_Zone() {
    CQWWCWContest contest(testStation());
    QSO qso;
    qso.cqZone = 5;

    QString value = contest.getQSOFieldValue(qso, "Zone");
    QCOMPARE(value, QString("5"));

    // Also test "CQ Zone" alias
    QString value2 = contest.getQSOFieldValue(qso, "CQ Zone");
    QCOMPARE(value2, QString("5"));
}

void TestContestConfigFields::testGetQSOFieldValue_Unknown() {
    NAQPSSBContest contest(testStation());
    QSO qso;
    qso.operatorName = "TOM";

    // Unknown field should return empty string
    QString value = contest.getQSOFieldValue(qso, "UnknownField");
    QVERIFY(value.isEmpty());
}

// ===== Edge Case Tests =====

void TestContestConfigFields::testCanonicalExchange_WithEmptyFields() {
    // Test canonical exchange when all fields are empty
    NAQPSSBContest contest(testStation());
    QSO qso;
    // Leave operatorName and state empty

    QString canonical = contest.buildCanonicalExchange(qso);

    // Should return empty string when all fields are empty
    QVERIFY(canonical.trimmed().isEmpty());
}

void TestContestConfigFields::testCanonicalExchange_PartialFields_NAQP() {
    // Test canonical exchange when only some fields are populated
    NAQPSSBContest contest(testStation());

    // Only name, no state
    QSO qso1;
    qso1.operatorName = "TOM";
    QString canonical1 = contest.buildCanonicalExchange(qso1);
    QCOMPARE(canonical1, QString("TOM"));

    // Only state, no name
    QSO qso2;
    qso2.state = "FL";
    QString canonical2 = contest.buildCanonicalExchange(qso2);
    QCOMPARE(canonical2, QString("FL"));
}

void TestContestConfigFields::testCanonicalExchange_PartialFields_Sweepstakes() {
    // Test canonical exchange when only some fields are populated
    ARRLSSCWContest contest(testStation());

    // Only serial and section
    QSO qso;
    qso.serialNumberReceived = 123;
    qso.arrlSection = "WMA";
    // precedence and check are empty

    QString canonical = contest.buildCanonicalExchange(qso);

    // Should include only the populated fields
    QCOMPARE(canonical, QString("123 WMA"));
}

void TestContestConfigFields::testConfigFields_CWAndSSBVariantsMatch() {
    // CW and SSB variants of same contest should return identical config fields
    NAQPCWContest cwContest(testStation());
    NAQPSSBContest ssbContest(testStation());

    QList<ContestConfigField> cwFields = cwContest.getConfigFields();
    QList<ContestConfigField> ssbFields = ssbContest.getConfigFields();

    QCOMPARE(cwFields.size(), ssbFields.size());

    for (int i = 0; i < cwFields.size(); ++i) {
        QCOMPARE(cwFields[i].id, ssbFields[i].id);
        QCOMPARE(cwFields[i].label, ssbFields[i].label);
        QCOMPARE(cwFields[i].type, ssbFields[i].type);
        QCOMPARE(cwFields[i].settingsKey, ssbFields[i].settingsKey);
    }

    // Also verify SS CW/SSB match
    ARRLSSCWContest ssCWContest(testStation());
    ARRLSSSSBContest ssSSBContest(testStation());

    QList<ContestConfigField> ssCWFields = ssCWContest.getConfigFields();
    QList<ContestConfigField> ssSSBFields = ssSSBContest.getConfigFields();

    QCOMPARE(ssCWFields.size(), ssSSBFields.size());

    for (int i = 0; i < ssCWFields.size(); ++i) {
        QCOMPARE(ssCWFields[i].id, ssSSBFields[i].id);
    }
}

void TestContestConfigFields::testConfigField_OptionalField() {
    // Test creating an optional field (required = false)
    ContestConfigField field("OPTIONAL", "Optional Field:", "Optional value",
                             "", 10, false);  // required = false

    QCOMPARE(field.required, false);
    QCOMPARE(field.id, QString("OPTIONAL"));
}

void TestContestConfigFields::testGetQSOFieldValue_EmptyStrings() {
    // Test that empty string fields return empty (not crash)
    NAQPSSBContest contest(testStation());
    QSO qso;
    qso.operatorName = "";  // Explicitly empty
    qso.state = "";

    QString name = contest.getQSOFieldValue(qso, "Name");
    QString state = contest.getQSOFieldValue(qso, "State");

    QVERIFY(name.isEmpty());
    QVERIFY(state.isEmpty());
}

void TestContestConfigFields::testGetQSOFieldValue_ZeroZone() {
    // Test that zone = 0 returns empty string (invalid zone)
    CQWWCWContest contest(testStation());
    QSO qso;
    qso.cqZone = 0;

    QString value = contest.getQSOFieldValue(qso, "Zone");

    // Zone 0 should return empty (not "0") since 0 is invalid
    QVERIFY(value.isEmpty());
}

QTEST_MAIN(TestContestConfigFields)
#include "test_contest_config_fields.moc"
