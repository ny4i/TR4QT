#include <QTest>
#include "../src/ui/widgets/SMeterWidget.h"

using namespace TR4QT;

/**
 * Unit tests for S-meter dBm to S-unit conversion
 * Validates against standard HF S-meter table (Table 3 - ITU standard)
 */
class TestSMeter : public QObject {
    Q_OBJECT

private slots:
    void testDbmToSUnit_StandardTable();
    void testDbmToSUnit_BelowS1();
    void testDbmToSUnit_AboveS9Plus60();
    void testDbmToSUnit_EdgeCases();
};

/**
 * Test dBm to S-unit conversion against standard table
 *
 * Standard HF S-meter values (ITU):
 * S1 = -121 dBm
 * S2 = -115 dBm
 * S3 = -109 dBm
 * S4 = -103 dBm
 * S5 = -97 dBm
 * S6 = -91 dBm
 * S7 = -85 dBm
 * S8 = -79 dBm
 * S9 = -73 dBm
 * S9 + 10 dB = -63 dBm
 * S9 + 20 dB = -53 dBm
 * S9 + 30 dB = -43 dBm
 * S9 + 40 dB = -33 dBm
 */
void TestSMeter::testDbmToSUnit_StandardTable() {
    SMeterWidget widget;

    // Test S1 through S9 (6 dB per S-unit)
    // S1 = -121 dBm ± 3 dB tolerance
    int s1 = widget.rawToSUnit(-121);
    QVERIFY2(s1 >= 1 && s1 <= 2,
             QString("S1 (-121 dBm): expected 1-2, got %1").arg(s1).toLatin1());

    // S2 = -115 dBm
    int s2 = widget.rawToSUnit(-115);
    QVERIFY2(s2 >= 2 && s2 <= 3,
             QString("S2 (-115 dBm): expected 2-3, got %1").arg(s2).toLatin1());

    // S3 = -109 dBm
    int s3 = widget.rawToSUnit(-109);
    QVERIFY2(s3 >= 3 && s3 <= 4,
             QString("S3 (-109 dBm): expected 3-4, got %1").arg(s3).toLatin1());

    // S4 = -103 dBm
    int s4 = widget.rawToSUnit(-103);
    QVERIFY2(s4 >= 4 && s4 <= 5,
             QString("S4 (-103 dBm): expected 4-5, got %1").arg(s4).toLatin1());

    // S5 = -97 dBm
    int s5 = widget.rawToSUnit(-97);
    QVERIFY2(s5 >= 5 && s5 <= 6,
             QString("S5 (-97 dBm): expected 5-6, got %1").arg(s5).toLatin1());

    // S6 = -91 dBm
    int s6 = widget.rawToSUnit(-91);
    QVERIFY2(s6 >= 6 && s6 <= 7,
             QString("S6 (-91 dBm): expected 6-7, got %1").arg(s6).toLatin1());

    // S7 = -85 dBm
    int s7 = widget.rawToSUnit(-85);
    QVERIFY2(s7 >= 7 && s7 <= 8,
             QString("S7 (-85 dBm): expected 7-8, got %1").arg(s7).toLatin1());

    // S8 = -79 dBm
    int s8 = widget.rawToSUnit(-79);
    QVERIFY2(s8 >= 8 && s8 <= 9,
             QString("S8 (-79 dBm): expected 8-9, got %1").arg(s8).toLatin1());

    // S9 = -73 dBm (exact)
    int s9 = widget.rawToSUnit(-73);
    QCOMPARE(s9, 9);

    // S9 + 10 dB = -63 dBm (should still be S9, not yet +20)
    int s9p10 = widget.rawToSUnit(-63);
    QCOMPARE(s9p10, 9);

    // S9 + 20 dB = -53 dBm
    int s9p20 = widget.rawToSUnit(-53);
    QCOMPARE(s9p20, 10);

    // S9 + 30 dB = -43 dBm (should still be +20, not yet +40)
    int s9p30 = widget.rawToSUnit(-43);
    QCOMPARE(s9p30, 10);

    // S9 + 40 dB = -33 dBm
    int s9p40 = widget.rawToSUnit(-33);
    QCOMPARE(s9p40, 11);
}

/**
 * Test signals below S1 (weaker than -121 dBm)
 */
void TestSMeter::testDbmToSUnit_BelowS1() {
    SMeterWidget widget;

    // S0 = -127 dBm or weaker
    QCOMPARE(widget.rawToSUnit(-127), 0);
    QCOMPARE(widget.rawToSUnit(-130), 0);
    QCOMPARE(widget.rawToSUnit(-200), 0);
}

/**
 * Test very strong signals (above S9 + 40 dB)
 */
void TestSMeter::testDbmToSUnit_AboveS9Plus60() {
    SMeterWidget widget;

    // S9 + 60 dB = -13 dBm (max on most displays)
    QCOMPARE(widget.rawToSUnit(-13), 12);

    // Even stronger signals should still show as max (12 = +60)
    QCOMPARE(widget.rawToSUnit(-10), 12);
    QCOMPARE(widget.rawToSUnit(-5), 12);
    QCOMPARE(widget.rawToSUnit(0), 12);
}

/**
 * Test edge cases and boundary conditions
 */
void TestSMeter::testDbmToSUnit_EdgeCases() {
    SMeterWidget widget;

    // Test exact S9 boundary
    QCOMPARE(widget.rawToSUnit(-73), 9);

    // Test just below S9 threshold (should be S8 or S9 depending on rounding)
    int justBelowS9 = widget.rawToSUnit(-74);
    QVERIFY2(justBelowS9 >= 8 && justBelowS9 <= 9,
             QString("Just below S9 (-74 dBm): expected 8-9, got %1").arg(justBelowS9).toLatin1());

    // Test just above S9 threshold
    int justAboveS9 = widget.rawToSUnit(-72);
    QCOMPARE(justAboveS9, 9);

    // Test S9+20 boundary
    QCOMPARE(widget.rawToSUnit(-53), 10);

    // Test just below S9+20 threshold
    int justBelowS9p20 = widget.rawToSUnit(-54);
    QCOMPARE(justBelowS9p20, 9);

    // Test S9+40 boundary
    QCOMPARE(widget.rawToSUnit(-33), 11);

    // Test just below S9+40 threshold
    int justBelowS9p40 = widget.rawToSUnit(-34);
    QCOMPARE(justBelowS9p40, 10);
}

QTEST_MAIN(TestSMeter)
#include "test_smeter.moc"
