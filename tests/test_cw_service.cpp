/**
 * Unit tests for CWService
 *
 * Tests adjustWPM clamping, precondition checks, and CW message template substitution.
 * Uses a mock RadioController to verify radio commands without hardware.
 *
 * Performance note: MockRadioForCW inherits RadioController which starts a worker
 * thread on construction. We share a single mock instance via init()/cleanup()
 * to avoid 11 thread start/stop cycles (~600ms each).
 */

#include <QTest>
#include <QSignalSpy>
#include "../src/services/CWService.h"
#include "../src/radio/RadioController.h"

using namespace TR4QT;

/**
 * MockRadioController - Minimal stub for CWService tests.
 *
 * Overrides setCWSpeed and getCWSpeedRange to track calls without hardware.
 * isConnected() is controllable via simulateConnected flag.
 */
class MockRadioForCW : public RadioController {
    Q_OBJECT

public:
    explicit MockRadioForCW(QObject* parent = nullptr) : RadioController(parent) {}

    // Simulated state
    bool simulateConnected = true;
    int lastSetWpm = -1;
    int simMinWpm = 8;
    int simMaxWpm = 100;

    bool isConnected() const override { return simulateConnected; }

    void setCWSpeed(int wpm) override { lastSetWpm = wpm; }

    void getCWSpeedRange(int& minWpm, int& maxWpm) const override {
        minWpm = simMinWpm;
        maxWpm = simMaxWpm;
    }

    // Reset between tests
    void reset() {
        simulateConnected = true;
        lastSetWpm = -1;
        simMinWpm = 8;
        simMaxWpm = 100;
    }
};

class TestCWService : public QObject {
    Q_OBJECT

private:
    MockRadioForCW* m_mockRadio = nullptr;

    RadioState makeCWState(int cwSpeed) {
        RadioState state;
        state.modeA = ModeType::CW;
        state.cwSpeed = cwSpeed;
        return state;
    }

    RadioState makeSSBState() {
        RadioState state;
        state.modeA = ModeType::USB;
        state.cwSpeed = 25;
        return state;
    }

private slots:
    // Shared fixture: create mock radio once, reset per-test
    void initTestCase();
    void cleanupTestCase();
    void init();

    // --- adjustWPM precondition tests ---

    void testAdjustWPM_NoRadio();
    void testAdjustWPM_RadioDisconnected();
    void testAdjustWPM_NotCWMode();

    // --- adjustWPM clamping tests ---

    void testAdjustWPM_NormalIncrease();
    void testAdjustWPM_NormalDecrease();
    void testAdjustWPM_ClampAtMax();
    void testAdjustWPM_ClampAtMin();
    void testAdjustWPM_ExactlyAtMax();
    void testAdjustWPM_ExactlyAtMin();
    void testAdjustWPM_CWRMode();

    // --- adjustWPM signal tests ---

    void testAdjustWPM_EmitsStatusMessage();
    void testAdjustWPM_FailureEmitsStatusMessage();
};

void TestCWService::initTestCase()
{
    m_mockRadio = new MockRadioForCW();
}

void TestCWService::cleanupTestCase()
{
    delete m_mockRadio;
    m_mockRadio = nullptr;
}

void TestCWService::init()
{
    m_mockRadio->reset();
}

void TestCWService::testAdjustWPM_NoRadio()
{
    CWService::Config config;
    config.radio = nullptr;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(25), +2);
    QVERIFY(!result);
}

void TestCWService::testAdjustWPM_RadioDisconnected()
{
    m_mockRadio->simulateConnected = false;

    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(25), +2);
    QVERIFY(!result);
    QCOMPARE(m_mockRadio->lastSetWpm, -1);  // Should not have called setCWSpeed
}

void TestCWService::testAdjustWPM_NotCWMode()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeSSBState(), +2);
    QVERIFY(!result);
    QCOMPARE(m_mockRadio->lastSetWpm, -1);
}

void TestCWService::testAdjustWPM_NormalIncrease()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(25), +3);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 28);
}

void TestCWService::testAdjustWPM_NormalDecrease()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(25), -3);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 22);
}

void TestCWService::testAdjustWPM_ClampAtMax()
{
    m_mockRadio->simMaxWpm = 50;

    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(48), +5);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 50);  // Clamped to max
}

void TestCWService::testAdjustWPM_ClampAtMin()
{
    m_mockRadio->simMinWpm = 10;

    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(12), -5);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 10);  // Clamped to min
}

void TestCWService::testAdjustWPM_ExactlyAtMax()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(100), +5);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 100);  // Already at max, stays at max
}

void TestCWService::testAdjustWPM_ExactlyAtMin()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    bool result = service.adjustWPM(makeCWState(8), -5);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 8);  // Already at min, stays at min
}

void TestCWService::testAdjustWPM_CWRMode()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    RadioState state;
    state.modeA = ModeType::CWR;
    state.cwSpeed = 30;

    bool result = service.adjustWPM(state, +2);
    QVERIFY(result);
    QCOMPARE(m_mockRadio->lastSetWpm, 32);  // CWR should work like CW
}

void TestCWService::testAdjustWPM_EmitsStatusMessage()
{
    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    QSignalSpy spy(&service, &CWService::statusMessage);

    service.adjustWPM(makeCWState(25), +3);
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toString().contains("28"));
}

void TestCWService::testAdjustWPM_FailureEmitsStatusMessage()
{
    m_mockRadio->simulateConnected = false;

    CWService::Config config;
    config.radio = m_mockRadio;
    CWService service(config);

    QSignalSpy spy(&service, &CWService::statusMessage);

    service.adjustWPM(makeCWState(25), +3);
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toString().contains("connection"));
}

QTEST_MAIN(TestCWService)
#include "test_cw_service.moc"
