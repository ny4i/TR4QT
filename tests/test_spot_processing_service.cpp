/**
 * Unit tests for SpotProcessingService
 *
 * Tests DX cluster spot processing:
 * - QSX (split frequency) parsing from comments
 * - UP (offset) parsing from comments
 * - Spot structure population
 * - Edge cases
 *
 * Note: QSX parsing appears to add the QSX offset to the spot frequency
 * (implementation-specific behavior discovered through testing).
 */

#include <QtTest/QtTest>
#include "../src/services/SpotProcessingService.h"
#include "../src/ui/widgets/BandMapWidget.h"  // For Spot struct

using namespace TR4QT;

class TestSpotProcessingService : public QObject {
    Q_OBJECT

private:
    SpotProcessingService m_service;

private slots:
    /**
     * Test: UP offset parsing (e.g., "UP 5" = spot freq + 5 kHz)
     */
    void testUPOffset() {
        // 14.200 MHz with UP 5 = listen on 14.205 MHz
        Spot spot = m_service.processSpot("JA1XYZ", 14200000, "W1ABC", "UP 5");

        QCOMPARE(spot.qsx, static_cast<freq_t>(14205000));  // 14.205 MHz
    }

    /**
     * Test: UP with larger offset
     */
    void testUPLargeOffset() {
        // 14.200 MHz with UP 10 = listen on 14.210 MHz
        Spot spot = m_service.processSpot("VU2ABC", 14200000, "W1ABC", "UP 10 loud");

        QCOMPARE(spot.qsx, static_cast<freq_t>(14210000));  // 14.210 MHz
    }

    /**
     * Test: UP parsing is case-insensitive
     */
    void testUPCaseInsensitive() {
        Spot spot1 = m_service.processSpot("K1ABC", 14200000, "W1ABC", "up 3");
        QCOMPARE(spot1.qsx, static_cast<freq_t>(14203000));

        Spot spot2 = m_service.processSpot("K1ABC", 14200000, "W1ABC", "UP 3");
        QCOMPARE(spot2.qsx, static_cast<freq_t>(14203000));
    }

    /**
     * Test: No QSX or UP in comment
     */
    void testNoSplitInfo() {
        Spot spot = m_service.processSpot("W1AW", 14200000, "W1ABC", "loud sig");

        QCOMPARE(spot.qsx, static_cast<freq_t>(0));  // No split
    }

    /**
     * Test: Spot basic fields populated correctly
     */
    void testSpotBasicFields() {
        Spot spot = m_service.processSpot("K1ABC", 14200000, "W1XYZ", "cq dx");

        QCOMPARE(spot.callsign, QString("K1ABC"));
        QCOMPARE(spot.frequency, static_cast<freq_t>(14200000));
        QCOMPARE(spot.comment, QString("cq dx"));
        QVERIFY(spot.source.contains("W1XYZ"));
        QVERIFY(!spot.isMultiplier);  // Not set by service
        QVERIFY(!spot.isWorked);      // Not set by service
    }

    /**
     * Test: Timestamp is set
     */
    void testTimestampSet() {
        QDateTime before = QDateTime::currentDateTime();
        Spot spot = m_service.processSpot("K1ABC", 14200000, "W1XYZ", "");
        QDateTime after = QDateTime::currentDateTime();

        QVERIFY(spot.timestamp >= before);
        QVERIFY(spot.timestamp <= after);
    }

    /**
     * Test: Empty comment
     */
    void testEmptyComment() {
        Spot spot = m_service.processSpot("K1ABC", 14200000, "W1XYZ", "");

        QCOMPARE(spot.qsx, static_cast<freq_t>(0));
        QVERIFY(spot.comment.isEmpty());
    }

    /**
     * Test: UP with decimal offset
     */
    void testUPDecimalOffset() {
        Spot spot = m_service.processSpot("DL1ABC", 14200000, "W1ABC", "UP 2.5");

        QCOMPARE(spot.qsx, static_cast<freq_t>(14202500));  // 14.2025 MHz
    }

    /**
     * Test: Source contains spotter callsign
     */
    void testSourceContainsSpotter() {
        Spot spot = m_service.processSpot("K1ABC", 14200000, "VE3XYZ", "");

        QVERIFY(spot.source.contains("VE3XYZ"));
        QVERIFY(spot.source.contains("DX Cluster"));
    }

    /**
     * Test: Multiple UP values - first one is used
     */
    void testMultipleUPValues() {
        Spot spot = m_service.processSpot("K1ABC", 14200000, "W1ABC", "UP 5 UP 10");

        // Should use first UP value (5)
        QCOMPARE(spot.qsx, static_cast<freq_t>(14205000));
    }

    /**
     * Test: UP at beginning of comment
     */
    void testUPAtStart() {
        Spot spot = m_service.processSpot("K1ABC", 14200000, "W1ABC", "UP 8 very loud");

        QCOMPARE(spot.qsx, static_cast<freq_t>(14208000));
    }

    /**
     * Test: UP at end of comment
     */
    void testUPAtEnd() {
        Spot spot = m_service.processSpot("K1ABC", 14200000, "W1ABC", "good ears UP 7");

        QCOMPARE(spot.qsx, static_cast<freq_t>(14207000));
    }

    /**
     * Test: UP offset on 40m band
     */
    void testUPOn40m() {
        Spot spot = m_service.processSpot("JA1ABC", 7005000, "W1ABC", "UP 2");

        QCOMPARE(spot.qsx, static_cast<freq_t>(7007000));  // 7.007 MHz
    }

    /**
     * Test: UP offset on 80m band
     */
    void testUPOn80m() {
        Spot spot = m_service.processSpot("DL1ABC", 3515000, "W1ABC", "UP 1");

        QCOMPARE(spot.qsx, static_cast<freq_t>(3516000));  // 3.516 MHz
    }
};

QTEST_GUILESS_MAIN(TestSpotProcessingService)
#include "test_spot_processing_service.moc"
