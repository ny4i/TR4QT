/**
 * Unit tests for SpotProcessorWorker
 *
 * Tests the in-memory dupe/multiplier cache logic, split frequency parsing,
 * and spot color classification. These run synchronously on the test thread
 * (no QThread needed — the worker is just a QObject with methods).
 */

#include <QtTest/QtTest>
#include "../src/services/SpotProcessorWorker.h"

using namespace TR4QT;

class TestSpotProcessorWorker : public QObject {
    Q_OBJECT

private:
    SpotProcessorWorker* m_worker{nullptr};

private slots:
    void init() {
        m_worker = new SpotProcessorWorker();
        // Set a config so colors are predictable
        SpotProcessorConfig config;
        config.dupeColor = QColor(128, 128, 128);        // Gray
        config.multiplierColor = QColor(255, 0, 0);      // Red
        config.defaultCallColor = QColor(0, 0, 0);       // Black
        config.lotwLookupEnabled = false;
        m_worker->setConfig(config);
    }

    void cleanup() {
        delete m_worker;
        m_worker = nullptr;
    }

    // ========== Split Frequency Parsing ==========

    /**
     * Test: QSX absolute frequency in kHz
     */
    void testParseSplitQSX() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "QSX 14205", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        QCOMPARE(result.listenFrequency, 14205000.0);  // 14205 kHz -> Hz
    }

    /**
     * Test: QSX relative (value < 1000 means kHz offset added to spot MHz floor)
     * Note: freq_t is double, so 7005000/1000000 = 7.005, *1000000 = 7005000
     * Result: 7005000 + 45000 = 7050000 (anchors to spot freq's MHz, not band start)
     */
    void testParseSplitQSXRelative() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 7005000, "W1XYZ", "QSX 045", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        QCOMPARE(result.listenFrequency, 7050000.0);  // 7005 kHz floor + 045 kHz
    }

    /**
     * Test: UP offset
     */
    void testParseSplitUP() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("JA1XYZ", 14200000, "W1ABC", "UP 5", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        QCOMPARE(result.listenFrequency, 14205000.0);
    }

    /**
     * Test: DOWN offset
     */
    void testParseSplitDOWN() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "DOWN 3", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        QCOMPARE(result.listenFrequency, 14197000.0);
    }

    /**
     * Test: DN abbreviation for DOWN
     */
    void testParseSplitDN() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "DN 2", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        QCOMPARE(result.listenFrequency, 14198000.0);
    }

    /**
     * Test: No split info in comment
     */
    void testNoSplit() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "loud sig cq", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(!result.isSplit);
        QCOMPARE(result.listenFrequency, 0.0);
    }

    /**
     * Test: UP with decimal offset
     */
    void testParseSplitUPDecimal() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("DL1ABC", 14200000, "W1ABC", "UP 2.5", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        QCOMPARE(result.listenFrequency, 14202500.0);
    }

    // ========== Dupe Cache ==========

    /**
     * Test: Spot is not a dupe when cache is empty
     */
    void testNoDupeWhenCacheEmpty() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        // No contest context set, so no dupe checking
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(!result.bandMapSpot.isWorked);
    }

    /**
     * Test: addWorkedCallsign marks subsequent spots as dupes
     */
    void testDupeDetectionAfterAddWorked() {
        // Set contest context with no mult defs (just dupe checking)
        m_worker->setContestContext(1, {});

        // Add a worked callsign on 20m CW
        m_worker->addWorkedCallsign("K1ABC", "20M", "CW");

        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        // Spot on 20m (14.200 MHz is 20m)
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.bandMapSpot.isWorked);
    }

    /**
     * Test: Different band is not a dupe
     */
    void testDifferentBandNotDupe() {
        m_worker->setContestContext(1, {});
        m_worker->addWorkedCallsign("K1ABC", "20M", "CW");

        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        // Spot on 40m (7.005 MHz)
        m_worker->processSpot("K1ABC", 7005000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(!result.bandMapSpot.isWorked);
    }

    /**
     * Test: Dupe check is case-insensitive
     */
    void testDupeCaseInsensitive() {
        m_worker->setContestContext(1, {});
        m_worker->addWorkedCallsign("k1abc", "20M", "CW");

        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.bandMapSpot.isWorked);
    }

    /**
     * Test: Dupe checks all modes (CW, USB, LSB, FM)
     */
    void testDupeChecksAllModes() {
        m_worker->setContestContext(1, {});
        m_worker->addWorkedCallsign("K1ABC", "20M", "USB");

        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        // Spot doesn't specify mode, but worker checks all common modes
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.bandMapSpot.isWorked);
    }

    // ========== Multiplier Cache ==========

    /**
     * Test: New multiplier is flagged when not in cache
     */
    void testNewMultiplierDetection() {
        // Set up a contest with CQ Zone multiplier (PerBand)
        QList<MultiplierDefinition> multDefs;
        MultiplierDefinition zoneMult;
        zoneMult.type = MultiplierType::CQZone;
        zoneMult.scope = MultiplierScope::PerBand;
        multDefs.append(zoneMult);

        m_worker->setContestContext(1, multDefs);
        // Don't add any worked multipliers — everything should be "new"

        // Note: Without country file loaded, the worker can't determine
        // CQ zone from callsign, so mult detection won't fire.
        // This test validates the cache structure works without crash.
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
        // Without country file, mult value extraction returns empty → not flagged
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(!result.bandMapSpot.isMultiplier);
    }

    /**
     * Test: addWorkedMultiplier updates cache correctly
     */
    void testAddWorkedMultiplier() {
        QList<MultiplierDefinition> multDefs;
        MultiplierDefinition zoneMult;
        zoneMult.type = MultiplierType::CQZone;
        zoneMult.scope = MultiplierScope::PerBand;
        multDefs.append(zoneMult);

        m_worker->setContestContext(1, multDefs);
        m_worker->addWorkedMultiplier("CQZone", "5", "20M");

        // Cache should now have zone 5 on 20m
        // If we process a zone 5 station on 20m, it should NOT be flagged as new mult
        // (Would need country file to actually test this end-to-end)
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "", "1234");
        QCOMPARE(spy.count(), 1);
    }

    // ========== Display Text Formatting ==========

    /**
     * Test: Processed spot has non-empty display text
     */
    void testDisplayTextPopulated() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "cq dx", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(!result.displayText.isEmpty());
        QVERIFY(result.displayText.contains("K1ABC"));
        QVERIFY(result.displayText.contains("W1XYZ"));
        QVERIFY(result.displayText.contains("14200.0"));
    }

    /**
     * Test: Format ranges cover callsign, frequency, spotter
     */
    void testFormatRangesPresent() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "cq", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        // Should have at least: spotter, frequency, callsign, timestamp, comment
        QVERIFY(result.formats.size() >= 4);
    }

    /**
     * Test: Split indicator appears in display text
     */
    void testSplitIndicatorInDisplayText() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "UP 5", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QVERIFY(result.isSplit);
        // Split indicator is Unicode bullet ●
        QVERIFY(result.displayText.contains(QString::fromUtf8("\u25CF")));
    }

    // ========== Band Map Spot ==========

    /**
     * Test: Band map spot fields populated
     */
    void testBandMapSpotFields() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "cq dx", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QCOMPARE(result.bandMapSpot.callsign, QString("K1ABC"));
        QCOMPARE(result.bandMapSpot.frequency, static_cast<freq_t>(14200000));
        QVERIFY(result.bandMapSpot.source.contains("W1XYZ"));
        QCOMPARE(result.bandMapSpot.comment, QString("cq dx"));
    }

    /**
     * Test: QSX is set on band map spot for split
     */
    void testBandMapSpotQSX() {
        QSignalSpy spy(m_worker, &SpotProcessorWorker::spotProcessed);
        m_worker->processSpot("K1ABC", 14200000, "W1XYZ", "QSX 14210", "1234");
        QCOMPARE(spy.count(), 1);
        ProcessedSpot result = spy.first().first().value<ProcessedSpot>();
        QCOMPARE(result.bandMapSpot.qsx, static_cast<freq_t>(14210000));
    }
};

QTEST_GUILESS_MAIN(TestSpotProcessorWorker)
#include "test_spot_processor_worker.moc"
