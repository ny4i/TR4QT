/**
 * Unit tests for SpotCollectorService
 *
 * Tests the public API surface of SpotCollectorService.
 *
 * LIMITATION: The core acceptance state machine (onPathfinderCallsign) is a
 * private slot driven by the DDE bridge. It cannot be exercised through the
 * public API on non-Windows platforms since DXLabPathfinder is a no-op stub.
 * Full state machine coverage requires a Windows environment with DDE, or a
 * mock DXLabPathfinder injected via constructor (future improvement).
 *
 * What IS tested here:
 * - Initial state correctness
 * - Public API contracts (setFieldsEmpty, resetUserEngagement, setFrequencyLookup)
 * - Non-Windows loadSettings() is a safe no-op
 * - No signal emissions on non-Windows (state machine never fires)
 */

#include <QtTest/QtTest>
#include "../src/services/SpotCollectorService.h"

using namespace TR4QT;

class TestSpotCollectorService : public QObject {
    Q_OBJECT

private slots:
    /**
     * Test: Service starts with fields empty and no DDE callsign
     */
    void testInitialState() {
        SpotCollectorService service;
        QVERIFY(!service.isCallsignFromDDE());
        QVERIFY(!service.isRunning());  // Non-Windows stub never starts
    }

    /**
     * Test: No signals emitted on non-Windows (DDE bridge is a no-op)
     */
    void testNoSignalsOnNonWindows() {
        SpotCollectorService service;
        QSignalSpy callsignSpy(&service, &SpotCollectorService::callsignReceived);
        QSignalSpy qsySpy(&service, &SpotCollectorService::qsyRequested);

        // Set up all preconditions that would allow emission
        service.setFieldsEmpty(true);
        service.setFrequencyLookup([](const QString&) -> double { return 14200000.0; });
        service.loadSettings();

        // On non-Windows, no signals should ever fire
        QCOMPARE(callsignSpy.count(), 0);
        QCOMPARE(qsySpy.count(), 0);
        QVERIFY(!service.isCallsignFromDDE());
    }

    /**
     * Test: resetUserEngagement clears the DDE flag
     */
    void testResetUserEngagement() {
        SpotCollectorService service;
        service.resetUserEngagement();
        QVERIFY(!service.isCallsignFromDDE());
    }

    /**
     * Test: setFieldsEmpty transitions correctly without crash
     */
    void testFieldsEmptyTransitions() {
        SpotCollectorService service;
        // Rapid state transitions should not crash
        service.setFieldsEmpty(false);  // User typed something
        service.setFieldsEmpty(true);   // User cleared fields
        service.setFieldsEmpty(false);  // User typed again
        service.setFieldsEmpty(true);   // User cleared again
        QVERIFY(!service.isCallsignFromDDE());
    }

    /**
     * Test: Frequency lookup callback is stored (not invoked until DDE fires)
     */
    void testFrequencyLookupStored() {
        SpotCollectorService service;
        bool callbackCalled = false;
        service.setFrequencyLookup([&callbackCalled](const QString&) -> double {
            callbackCalled = true;
            return 14200000.0;
        });
        // Callback should NOT be called — only invoked when a DDE callsign arrives
        QVERIFY(!callbackCalled);
    }

    /**
     * Test: loadSettings on non-Windows is a safe no-op
     */
    void testLoadSettingsNonWindows() {
        SpotCollectorService service;
        service.loadSettings();  // Should early-return on non-Windows
        QVERIFY(!service.isRunning());
    }

    /**
     * Test: Multiple resetUserEngagement calls are idempotent
     */
    void testMultipleResetsIdempotent() {
        SpotCollectorService service;
        service.resetUserEngagement();
        service.resetUserEngagement();
        service.resetUserEngagement();
        QVERIFY(!service.isCallsignFromDDE());
    }
};

QTEST_GUILESS_MAIN(TestSpotCollectorService)
#include "test_spot_collector_service.moc"
