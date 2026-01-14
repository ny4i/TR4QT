#include <QTest>
#include "../src/contests/NAQPSSBContest.h"
#include "../src/exchanges/SmartExchangeParser.h"
#include "../src/models/QSO.h"

using namespace TR4QT;

/**
 * Unit tests for NAQP Contest Exchange Parsing
 * Tests order-agnostic parsing of Name + State exchanges
 *
 * Issue #59: Users should be able to enter "TOM FL" or "FL TOM"
 * and both should parse correctly.
 */
class TestNAQP : public QObject {
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
    // Order-agnostic parsing tests
    void testParse_StandardOrder();
    void testParse_ReversedOrder();
    void testParse_MultiWordName();
    void testParse_ReversedMultiWordName();
    void testParse_CanadianProvince();
    void testParse_AmbiguousBothStates();
    void testParse_LowercaseInput();
    void testParse_MixedCaseInput();

    // Validation tests
    void testValidate_ValidExchange();
    void testValidate_MissingName();
    void testValidate_MissingState();
    void testValidate_InvalidState();
    void testValidate_SingleWord();

    // Edge cases
    void testParse_ExtraSpaces();
    void testParse_AllUSStates();
    void testParse_AllCanadianProvinces();
};

// ===== Order-Agnostic Parsing Tests =====

void TestNAQP::testParse_StandardOrder() {
    // Traditional order: Name State
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("TOM FL", qso);

    QCOMPARE(qso.operatorName, QString("TOM"));
    QCOMPARE(qso.state, QString("FL"));
}

void TestNAQP::testParse_ReversedOrder() {
    // Reversed order: State Name (should still parse correctly)
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("FL TOM", qso);

    QCOMPARE(qso.operatorName, QString("TOM"));
    QCOMPARE(qso.state, QString("FL"));
}

void TestNAQP::testParse_MultiWordName() {
    // Multi-word name: "JOHN QUINCY FL"
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("JOHN QUINCY FL", qso);

    QCOMPARE(qso.operatorName, QString("JOHN QUINCY"));
    QCOMPARE(qso.state, QString("FL"));
}

void TestNAQP::testParse_ReversedMultiWordName() {
    // Reversed multi-word: "FL JOHN QUINCY"
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("FL JOHN QUINCY", qso);

    QCOMPARE(qso.operatorName, QString("JOHN QUINCY"));
    QCOMPARE(qso.state, QString("FL"));
}

void TestNAQP::testParse_CanadianProvince() {
    // Canadian province (BC = British Columbia)
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("DAVE BC", qso);

    QCOMPARE(qso.operatorName, QString("DAVE"));
    QCOMPARE(qso.state, QString("BC"));

    // Also test reversed
    QSO qso2;
    contest.parseReceivedExchange("BC DAVE", qso2);

    QCOMPARE(qso2.operatorName, QString("DAVE"));
    QCOMPARE(qso2.state, QString("BC"));
}

void TestNAQP::testParse_AmbiguousBothStates() {
    // Ambiguous case: Both parts are valid states
    // "AL MA" = Al from Massachusetts (last wins for state)
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("AL MA", qso);

    QCOMPARE(qso.operatorName, QString("AL"));
    QCOMPARE(qso.state, QString("MA"));
}

void TestNAQP::testParse_LowercaseInput() {
    // Lowercase input should be normalized to uppercase
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("tom fl", qso);

    // Name is preserved from input (may be lowercase)
    // State should be uppercase
    QCOMPARE(qso.state, QString("FL"));
    QVERIFY(!qso.operatorName.isEmpty());
}

void TestNAQP::testParse_MixedCaseInput() {
    // Mixed case input
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("Tom Fl", qso);

    QCOMPARE(qso.state, QString("FL"));
    QVERIFY(!qso.operatorName.isEmpty());
}

// ===== Validation Tests =====

void TestNAQP::testValidate_ValidExchange() {
    NAQPSSBContest contest(testStation());
    QString errorMsg;

    QVERIFY(contest.validateReceivedExchange("TOM FL", errorMsg));
    QVERIFY(contest.validateReceivedExchange("FL TOM", errorMsg));
    QVERIFY(contest.validateReceivedExchange("JOHN QUINCY MA", errorMsg));
    QVERIFY(contest.validateReceivedExchange("DAVE BC", errorMsg));
}

void TestNAQP::testValidate_MissingName() {
    // Just a state with no name
    NAQPSSBContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("FL", errorMsg));
    QVERIFY(errorMsg.contains("Name") || errorMsg.contains("name") ||
            errorMsg.contains("Exchange"));
}

void TestNAQP::testValidate_MissingState() {
    // Just a name with no valid state
    NAQPSSBContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("TOM", errorMsg));
    QVERIFY(errorMsg.contains("State") || errorMsg.contains("state") ||
            errorMsg.contains("province") || errorMsg.contains("Exchange"));
}

void TestNAQP::testValidate_InvalidState() {
    // Invalid state code
    NAQPSSBContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("TOM ZZ", errorMsg));
    QVERIFY(errorMsg.contains("Invalid") || errorMsg.contains("invalid") ||
            errorMsg.contains("state") || errorMsg.contains("province"));
}

void TestNAQP::testValidate_SingleWord() {
    // Single word should fail validation
    NAQPSSBContest contest(testStation());
    QString errorMsg;

    QVERIFY(!contest.validateReceivedExchange("TOM", errorMsg));
    QVERIFY(!contest.validateReceivedExchange("FL", errorMsg));
}

// ===== Edge Cases =====

void TestNAQP::testParse_ExtraSpaces() {
    // Extra whitespace should be handled
    NAQPSSBContest contest(testStation());
    QSO qso;

    contest.parseReceivedExchange("  TOM   FL  ", qso);

    QCOMPARE(qso.operatorName, QString("TOM"));
    QCOMPARE(qso.state, QString("FL"));
}

void TestNAQP::testParse_AllUSStates() {
    // Verify all 50 US states work
    NAQPSSBContest contest(testStation());

    QStringList states = {
        "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA",
        "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD",
        "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ",
        "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC",
        "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY",
        "DC"  // Plus DC
    };

    for (const QString& state : states) {
        QSO qso;
        QString exchange = QString("BOB %1").arg(state);
        contest.parseReceivedExchange(exchange, qso);

        QVERIFY2(qso.state == state,
                 qPrintable(QString("Failed for state %1: got %2")
                            .arg(state).arg(qso.state)));
    }
}

void TestNAQP::testParse_AllCanadianProvinces() {
    // Verify Canadian provinces work
    NAQPSSBContest contest(testStation());

    QStringList provinces = {
        "AB", "BC", "MB", "NB", "NL", "NS", "NT", "NU", "ON", "PE", "QC", "SK", "YT"
    };

    for (const QString& prov : provinces) {
        QSO qso;
        QString exchange = QString("DAVE %1").arg(prov);
        contest.parseReceivedExchange(exchange, qso);

        QVERIFY2(qso.state == prov,
                 qPrintable(QString("Failed for province %1: got %2")
                            .arg(prov).arg(qso.state)));
    }
}

QTEST_MAIN(TestNAQP)
#include "test_naqp.moc"
