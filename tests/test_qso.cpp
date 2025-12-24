#include <QTest>
#include <QDateTime>
#include "../src/models/QSO.h"

using namespace TR4QT;

/**
 * Unit tests for QSO model
 * Tests data validation and key generation
 */
class TestQSO : public QObject {
    Q_OBJECT

private slots:
    // normalizeCallsign() tests
    void testNormalizeCallsign_Uppercase();
    void testNormalizeCallsign_Trim();
    void testNormalizeCallsign_Mixed();

    // isPersisted() tests
    void testIsPersisted_NotSaved();
    void testIsPersisted_Saved();

    // isValid() tests
    void testIsValid_Complete();
    void testIsValid_MissingCallsign();
    void testIsValid_MissingTimestamp();
    void testIsValid_MissingBand();
    void testIsValid_MissingMode();
    void testIsValid_AllMissing();

    // getDupeKey() tests
    void testGetDupeKey_Format();
    void testGetDupeKey_Uniqueness();
    void testGetDupeKey_SameContact();
    void testGetDupeKey_DifferentBand();
    void testGetDupeKey_DifferentMode();
};

// normalizeCallsign() tests

void TestQSO::testNormalizeCallsign_Uppercase() {
    QSO qso;
    qso.callsign = "w1aw";
    qso.normalizeCallsign();
    QCOMPARE(qso.callsign, QString("W1AW"));
}

void TestQSO::testNormalizeCallsign_Trim() {
    QSO qso;
    qso.callsign = "  W1AW  ";
    qso.normalizeCallsign();
    QCOMPARE(qso.callsign, QString("W1AW"));
}

void TestQSO::testNormalizeCallsign_Mixed() {
    QSO qso;
    qso.callsign = "  w1aw  ";
    qso.normalizeCallsign();
    QCOMPARE(qso.callsign, QString("W1AW"));

    // Test with tabs and other whitespace
    qso.callsign = "\tK3LR\n";
    qso.normalizeCallsign();
    QCOMPARE(qso.callsign, QString("K3LR"));
}

// isPersisted() tests

void TestQSO::testIsPersisted_NotSaved() {
    QSO qso;
    // Default id is -1
    QVERIFY(!qso.isPersisted());

    qso.id = -5;
    QVERIFY(!qso.isPersisted());
}

void TestQSO::testIsPersisted_Saved() {
    QSO qso;
    qso.id = 0;
    QVERIFY(qso.isPersisted());

    qso.id = 1;
    QVERIFY(qso.isPersisted());

    qso.id = 12345;
    QVERIFY(qso.isPersisted());
}

// isValid() tests

void TestQSO::testIsValid_Complete() {
    QSO qso;
    qso.callsign = "W1AW";
    qso.timestamp = QDateTime::currentDateTime();
    qso.band = BandType::Band20M;
    qso.mode = ModeType::CW;

    QVERIFY(qso.isValid());
}

void TestQSO::testIsValid_MissingCallsign() {
    QSO qso;
    qso.callsign = "";  // Missing
    qso.timestamp = QDateTime::currentDateTime();
    qso.band = BandType::Band20M;
    qso.mode = ModeType::CW;

    QVERIFY(!qso.isValid());
}

void TestQSO::testIsValid_MissingTimestamp() {
    QSO qso;
    qso.callsign = "W1AW";
    // timestamp not set (invalid by default)
    qso.band = BandType::Band20M;
    qso.mode = ModeType::CW;

    QVERIFY(!qso.isValid());
}

void TestQSO::testIsValid_MissingBand() {
    QSO qso;
    qso.callsign = "W1AW";
    qso.timestamp = QDateTime::currentDateTime();
    qso.band = BandType::None;  // Missing
    qso.mode = ModeType::CW;

    QVERIFY(!qso.isValid());
}

void TestQSO::testIsValid_MissingMode() {
    QSO qso;
    qso.callsign = "W1AW";
    qso.timestamp = QDateTime::currentDateTime();
    qso.band = BandType::Band20M;
    qso.mode = ModeType::None;  // Missing

    QVERIFY(!qso.isValid());
}

void TestQSO::testIsValid_AllMissing() {
    QSO qso;
    // Default constructed - all fields missing
    QVERIFY(!qso.isValid());
}

// getDupeKey() tests

void TestQSO::testGetDupeKey_Format() {
    QSO qso;
    qso.callsign = "W1AW";
    qso.band = BandType::Band20M;
    qso.mode = ModeType::CW;

    QString key = qso.getDupeKey();

    // Should contain callsign_band_mode
    QVERIFY(key.contains("W1AW"));
    QVERIFY(key.contains("_"));

    // Verify format: callsign_bandInt_modeInt
    int bandInt = static_cast<int>(BandType::Band20M);
    int modeInt = static_cast<int>(ModeType::CW);
    QString expected = QString("W1AW_%1_%2").arg(bandInt).arg(modeInt);
    QCOMPARE(key, expected);
}

void TestQSO::testGetDupeKey_Uniqueness() {
    QSO qso1;
    qso1.callsign = "W1AW";
    qso1.band = BandType::Band20M;
    qso1.mode = ModeType::CW;

    QSO qso2;
    qso2.callsign = "K3LR";
    qso2.band = BandType::Band20M;
    qso2.mode = ModeType::CW;

    // Different callsigns should have different keys
    QVERIFY(qso1.getDupeKey() != qso2.getDupeKey());
}

void TestQSO::testGetDupeKey_SameContact() {
    QSO qso1;
    qso1.callsign = "W1AW";
    qso1.band = BandType::Band20M;
    qso1.mode = ModeType::CW;

    QSO qso2;
    qso2.callsign = "W1AW";
    qso2.band = BandType::Band20M;
    qso2.mode = ModeType::CW;

    // Same callsign/band/mode should have same key
    QCOMPARE(qso1.getDupeKey(), qso2.getDupeKey());
}

void TestQSO::testGetDupeKey_DifferentBand() {
    QSO qso1;
    qso1.callsign = "W1AW";
    qso1.band = BandType::Band20M;
    qso1.mode = ModeType::CW;

    QSO qso2;
    qso2.callsign = "W1AW";
    qso2.band = BandType::Band40M;  // Different band
    qso2.mode = ModeType::CW;

    // Different band should have different key (not a dupe)
    QVERIFY(qso1.getDupeKey() != qso2.getDupeKey());
}

void TestQSO::testGetDupeKey_DifferentMode() {
    QSO qso1;
    qso1.callsign = "W1AW";
    qso1.band = BandType::Band20M;
    qso1.mode = ModeType::CW;

    QSO qso2;
    qso2.callsign = "W1AW";
    qso2.band = BandType::Band20M;
    qso2.mode = ModeType::USB;  // Different mode

    // Different mode should have different key (not a dupe)
    QVERIFY(qso1.getDupeKey() != qso2.getDupeKey());
}

QTEST_MAIN(TestQSO)
#include "test_qso.moc"
