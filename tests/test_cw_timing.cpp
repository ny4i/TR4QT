#include <QtTest/QtTest>
#include "../src/radio/CWTiming.h"

using namespace TR4QT;

class TestCWTiming : public QObject {
    Q_OBJECT

private slots:
    void testPARISStandard();
    void testIndividualCharacters();
    void testNumbers();
    void testCallsignExamples();
    void testSpacing();
};

void TestCWTiming::testPARISStandard() {
    // The word "PARIS" is the standard for CW speed measurement
    // It should be exactly 50 timing units

    // Calculate individual characters
    int p = CWTiming::getCharacterUnits('P');  // .--.   = 11
    int a = CWTiming::getCharacterUnits('A');  // .-     = 5
    int r = CWTiming::getCharacterUnits('R');  // .-.    = 7
    int i = CWTiming::getCharacterUnits('I');  // ..     = 3
    int s = CWTiming::getCharacterUnits('S');  // ...    = 5

    QCOMPARE(p, 11);
    QCOMPARE(a, 5);
    QCOMPARE(r, 7);
    QCOMPARE(i, 3);
    QCOMPARE(s, 5);

    // Character units: 11+5+7+3+5 = 31
    // Inter-character spacing: 4 gaps × 3 = 12
    // Trailing word space: 7
    // Total: 31 + 12 + 7 = 50

    // Test at 1 WPM (should take 60 seconds = 60000ms)
    int duration1wpm = CWTiming::calculateDuration("PARIS", 1);
    QCOMPARE(duration1wpm, 60000);  // 50 units × 1200ms/unit = 60000ms

    // Test at 20 WPM
    int duration20wpm = CWTiming::calculateDuration("PARIS", 20);
    QCOMPARE(duration20wpm, 3000);  // 50 units × 60ms/unit = 3000ms

    // Test at 25 WPM
    int duration25wpm = CWTiming::calculateDuration("PARIS", 25);
    QCOMPARE(duration25wpm, 2400);  // 50 units × 48ms/unit = 2400ms
}

void TestCWTiming::testIndividualCharacters() {
    // Test letters with known patterns
    QCOMPARE(CWTiming::getCharacterUnits('E'), 1);   // .      = 1
    QCOMPARE(CWTiming::getCharacterUnits('T'), 3);   // -      = 3
    QCOMPARE(CWTiming::getCharacterUnits('M'), 7);   // --     = 7
    QCOMPARE(CWTiming::getCharacterUnits('O'), 11);  // ---    = 11

    // Test case insensitivity
    QCOMPARE(CWTiming::getCharacterUnits('e'), 1);
    QCOMPARE(CWTiming::getCharacterUnits('t'), 3);
}

void TestCWTiming::testNumbers() {
    // All numbers are 5 symbols
    QCOMPARE(CWTiming::getCharacterUnits('0'), 19);  // -----
    QCOMPARE(CWTiming::getCharacterUnits('5'), 9);   // .....
    QCOMPARE(CWTiming::getCharacterUnits('9'), 17);  // ----.
}

void TestCWTiming::testCallsignExamples() {
    // Test "CQ" at 20 WPM
    // C (-.-.): 11 units
    // Inter-char: 3 units
    // Q (--.-): 13 units
    // Total: 11+3+13 = 27 units
    // At 20 WPM: 27 × 60ms = 1620ms
    int cqDuration = CWTiming::calculateDuration("CQ", 20);
    QCOMPARE(cqDuration, 1620);

    // Test "TEST" at 30 WPM
    // T(-): 3, space: 3, E(.): 1, space: 3, S(...): 5, space: 3, T(-): 3
    // Total: 3+3+1+3+5+3+3 = 21 units
    // At 30 WPM: 21 × 40ms = 840ms
    int testDuration = CWTiming::calculateDuration("TEST", 30);
    QCOMPARE(testDuration, 840);

    // Test "W1AW" at 25 WPM
    // W(9) + space(3) + 1(17) + space(3) + A(5) + space(3) + W(9) = 49 units
    // At 25 WPM: 49 × 48ms = 2352ms
    int w1awDuration = CWTiming::calculateDuration("W1AW", 25);
    QCOMPARE(w1awDuration, 2352);
}

void TestCWTiming::testSpacing() {
    // Test word spacing
    // "CQ TEST" has word gap (7 units total, 3 already counted in inter-char = 4 extra)
    // C(11) + space(3) + Q(13) + word(4) + T(3) + space(3) + E(1) + space(3) + S(5) + space(3) + T(3)
    // = 11+3+13+4+3+3+1+3+5+3+3 = 52 units
    // At 35 WPM: 52 × 34.286ms ≈ 1783ms
    int cqTestDuration = CWTiming::calculateDuration("CQ TEST", 35);
    QCOMPARE(cqTestDuration, 1783);

    // Test multiple spaces (should be treated as single word gap)
    int singleSpace = CWTiming::calculateDuration("CQ TEST", 20);
    int doubleSpace = CWTiming::calculateDuration("CQ  TEST", 20);
    QCOMPARE(singleSpace, doubleSpace);  // Multiple spaces = one word gap
}

QTEST_MAIN(TestCWTiming)
#include "test_cw_timing.moc"
