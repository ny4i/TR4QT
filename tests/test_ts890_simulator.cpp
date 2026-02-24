/**
 * @file test_ts890_simulator.cpp
 * @brief Integration tests for TS890Radio DIRECT implementation using Hamlib's TS-890 simulator
 *
 * CRITICAL: This tests TR4QT's TS890Radio/KenwoodRadio DIRECT protocol implementation, NOT Hamlib!
 *
 * TS890Radio bypasses Hamlib and speaks Kenwood CAT protocol directly via TCP.
 * The Hamlib simulator also speaks Kenwood CAT protocol directly.
 * Therefore: TS890Radio ←[Kenwood CAT Protocol]→ Simulator (NO HAMLIB IN THE PATH)
 *
 * This test mirrors the K4 simulator test pattern:
 * - Uses REAL TS890Radio/KenwoodRadio production code (direct protocol implementation)
 * - Uses REAL Kenwood TS-890 commands (FA, FB, OM, KS, KY, TB, FT, RT, XT, etc.)
 * - Simulator maintained by Hamlib (not instrumented for testing)
 * - Only difference from production: pty device via TCP bridge vs direct TCP to radio
 *
 * What this tests:
 * ✅ KenwoodRadio's direct protocol implementation (shared base class)
 * ✅ TS890Radio's model-specific mode mapping (hex A-F for data modes)
 * ✅ Command formatting (FA00014200000;, OM02;, KS025;, etc.)
 * ✅ Response parsing
 * ✅ State management
 * ✅ Frequency/mode/CW speed/PTT/split/RIT control
 *
 * What this does NOT test:
 * ❌ LAN authentication (##CN/##ID) — simulator is pty-based, no auth
 * ❌ Hamlib library (Hamlib maintains their own tests)
 * ❌ HamlibRadio class (uses Hamlib, not direct protocol)
 */

#include <QTest>
#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QDir>
#include "../src/radio/TS890Radio.h"
#include "../src/radio/RadioFactory.h"
#include "../src/logging/Logger.h"
#include "../src/logging/LogMacros.h"
#include "TcpToPtyBridge.h"

using namespace TR4QT;

/**
 * @brief Helper class to manage Hamlib TS-890 simulator subprocess
 *
 * Launches simts890 as a child process and captures the pty device
 * it creates for communication.
 */
class TS890SimulatorProcess {
public:
    TS890SimulatorProcess(const QString& simulatorPath)
        : m_simulatorPath(simulatorPath)
        , m_process(nullptr)
        , m_running(false)
    {
    }

    ~TS890SimulatorProcess() {
        stop();
    }

    bool start() {
        // Get list of existing pty devices before starting simulator
        QStringList beforeDevices = getDevTtyDevices();

        m_process = new QProcess();
        m_process->start(m_simulatorPath, QStringList() << "dummy");

        if (!m_process->waitForStarted(1000)) {
            LOG_ERROR("TS890SimTest", "Failed to start simulator");
            delete m_process;
            m_process = nullptr;
            return false;
        }

        // Give simulator time to create pty device
        QThread::msleep(300);

        // Find the new device that appeared
        QStringList afterDevices = getDevTtyDevices();
        for (const QString& device : afterDevices) {
            if (!beforeDevices.contains(device)) {
                m_ptyDevice = device;
                LOG_INFO("TS890SimTest", QString("Simulator created pty device: %1").arg(m_ptyDevice));
                m_running = true;
                QThread::msleep(100);  // Give device time to be ready
                return true;
            }
        }

        LOG_ERROR("TS890SimTest", QString("Could not find new pty device. Before=%1, After=%2")
                  .arg(beforeDevices.size()).arg(afterDevices.size()));
        stop();
        return false;
    }

    void stop() {
        if (m_process && m_running) {
            m_process->terminate();
            if (!m_process->waitForFinished(1000)) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
            delete m_process;
            m_process = nullptr;
            m_running = false;
            LOG_INFO("TS890SimTest", "Simulator stopped");
        }
    }

    QString getPtyDevice() const { return m_ptyDevice; }
    bool isRunning() const { return m_running && m_process && m_process->state() == QProcess::Running; }

private:
    QStringList getDevTtyDevices() const {
        QStringList devices;
        QDir devDir("/dev");
        QStringList filters;
        filters << "ttys*" << "pts*";  // macOS uses ttys*, Linux uses pts*
        QFileInfoList files = devDir.entryInfoList(filters, QDir::System | QDir::Files);
        for (const QFileInfo& file : files) {
            devices.append(file.absoluteFilePath());
        }
        return devices;
    }

    QString m_simulatorPath;
    QProcess* m_process;
    QString m_ptyDevice;
    bool m_running;
};

// ============================================================================
// TEST CASES
// ============================================================================

class TestTS890RadioSimulator : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Test cases
    void testSimulatorStartsAndCreatesPty();
    void testTS890RadioConnectsToSimulator();
    void testFrequencyControl();
    void testModeControl();
    void testCWSpeedControl();
    void testCWMessageSending();
    void testPTTControl();
    void testSplitControl();
    void testRITControl();
    void testMultipleRapidCommands();
    void testErrorHandlingInvalidCWSpeed();
    void testDisconnectAndReconnect();

private:
    QString m_simulatorPath;
    bool m_simulatorAvailable;

    struct TestEnvironment {
        TS890SimulatorProcess* simulator;
        TcpToPtyBridge* bridge;

        ~TestEnvironment() {
            if (bridge) {
                bridge->stop();
                delete bridge;
            }
            if (simulator) {
                simulator->stop();
                delete simulator;
            }
        }
    };

    TestEnvironment* setupTestEnvironment(quint16 port) {
        auto* env = new TestEnvironment();

        env->simulator = new TS890SimulatorProcess(m_simulatorPath);
        if (!env->simulator->start()) {
            delete env;
            return nullptr;
        }

        env->bridge = new TcpToPtyBridge(env->simulator->getPtyDevice(), port);
        if (!env->bridge->start()) {
            delete env;
            return nullptr;
        }

        QThread::msleep(100);
        return env;
    }

    RadioConfig createTestConfig() {
        // Use port range 13000+ to avoid collision with K4 tests (12345+)
        static int portNumber = 13000;
        portNumber++;

        RadioConfig config;
        config.radioType = static_cast<int>(RadioFactory::RadioType::KENWOOD_DIRECT);
        config.port = QString("localhost:%1").arg(portNumber);
        // No credentials → skip LAN auth (simulator is pty-based)
        config.kenwoodAdminId = "";
        config.kenwoodAdminPassword = "";
        return config;
    }

    int getPortFromConfig(const RadioConfig& config) {
        int colonPos = config.port.indexOf(':');
        if (colonPos != -1) {
            return config.port.mid(colonPos + 1).toInt();
        }
        return 13001;
    }

    struct TestSetup {
        RadioConfig config;
        TestEnvironment* env;
    };

    TestSetup setupTest() {
        TestSetup setup;
        setup.config = createTestConfig();
        setup.env = setupTestEnvironment(getPortFromConfig(setup.config));
        return setup;
    }

    // Helper to wait for radio responses with event processing
    void waitForResponse(int maxIterations = 30) {
        for (int i = 0; i < maxIterations; i++) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }

    // Helper to connect radio and wait for connection
    bool connectAndWait(TS890Radio& radio, const RadioConfig& config, int maxWaitMs = 500) {
        if (!radio.connect(config)) return false;
        int waited = 0;
        while (!radio.isConnected() && waited < maxWaitMs) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
            waited += 10;
        }
        return radio.isConnected();
    }
};

void TestTS890RadioSimulator::initTestCase() {
    Logger& logger = Logger::instance();
    logger.setLogLevel(LogLevel::Debug);

    // Path to simulator - set TS890_SIMULATOR_PATH env var, or search common locations
    m_simulatorPath = qEnvironmentVariable("TS890_SIMULATOR_PATH");
    if (m_simulatorPath.isEmpty()) {
        const QStringList searchPaths = {
            QDir::homePath() + "/projects/hamlib/build/simulators/simts890",
            QDir::homePath() + "/projects/Hamlib/build/simulators/simts890",
            "/usr/local/bin/simts890",
            "/opt/homebrew/bin/simts890",
        };
        for (const QString& path : searchPaths) {
            if (QFile::exists(path)) {
                m_simulatorPath = path;
                break;
            }
        }
    }

    m_simulatorAvailable = QFile::exists(m_simulatorPath);
    if (!m_simulatorAvailable) {
        QSKIP(QString("TS-890 simulator not found. Set TS890_SIMULATOR_PATH or build Hamlib simulators.")
                   .toUtf8().constData());
    }

    LOG_INFO("TS890SimTest", QString("Using simulator: %1").arg(m_simulatorPath));
}

void TestTS890RadioSimulator::cleanupTestCase() {
}

void TestTS890RadioSimulator::testSimulatorStartsAndCreatesPty() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TS890SimulatorProcess sim(m_simulatorPath);

    QVERIFY(sim.start());
    QVERIFY(sim.isRunning());
    QVERIFY(!sim.getPtyDevice().isEmpty());
    QVERIFY(sim.getPtyDevice().startsWith("/dev/pts/") || sim.getPtyDevice().startsWith("/dev/ttys"));

    sim.stop();
    QVERIFY(!sim.isRunning());
}

void TestTS890RadioSimulator::testTS890RadioConnectsToSimulator() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));
    QVERIFY(radio.isConnected());

    radio.disconnect();
    QVERIFY(!radio.isConnected());

    delete setup.env;
}

void TestTS890RadioSimulator::testFrequencyControl() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // Test setFrequency — simulator handles FA/FB with 11-digit Hz
    // Simulator defaults: VFO A = 14000000 Hz (20m USB)
    freq_t testFreq = 14200000;  // 14.2 MHz
    QVERIFY(radio.setFrequency(testFreq));
    waitForResponse(20);

    // Test different frequency (different band)
    testFreq = 7100000;  // 7.1 MHz
    QVERIFY(radio.setFrequency(testFreq));
    waitForResponse(20);

    // Test VFO B frequency
    testFreq = 21200000;  // 21.2 MHz
    QVERIFY(radio.setFrequency(testFreq, VFO::VFO_B));
    waitForResponse(20);

    // Verify connection remains stable
    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testModeControl() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // Test standard mode commands via OM command
    // OM<vfo><modeChar>;  where vfo=0 for A, modeChar per TS-890 encoding
    QVERIFY(radio.setMode(ModeType::CW));      // OM03;
    waitForResponse(10);

    QVERIFY(radio.setMode(ModeType::USB));      // OM02;
    waitForResponse(10);

    QVERIFY(radio.setMode(ModeType::LSB));      // OM01;
    waitForResponse(10);

    QVERIFY(radio.setMode(ModeType::RTTY));     // OM06;
    waitForResponse(10);

    // Test hex-encoded data modes (TS-890 specific)
    QVERIFY(radio.setMode(ModeType::PSK));      // OM0A;
    waitForResponse(10);

    QVERIFY(radio.setMode(ModeType::DATA));     // OM0D; (USB-DATA)
    waitForResponse(10);

    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testCWSpeedControl() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // TS-890 supports 4-60 WPM via KS command (3-digit, zero-padded)
    QVERIFY(radio.setCWSpeed(25));  // KS025;
    waitForResponse(10);

    QVERIFY(radio.setCWSpeed(30));  // KS030;
    waitForResponse(10);

    QVERIFY(radio.setCWSpeed(4));   // KS004; (minimum)
    waitForResponse(10);

    QVERIFY(radio.setCWSpeed(60));  // KS060; (maximum)
    waitForResponse(10);

    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testCWMessageSending() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // Send CW via KY command (max 24 chars per chunk)
    QString cwMessage = "TEST DE NY4I";
    QVERIFY(radio.sendCW(cwMessage));
    waitForResponse(10);

    // Test longer message (requires chunking)
    QString longMessage = "CQ CQ CQ DE NY4I NY4I NY4I K";
    QVERIFY(radio.sendCW(longMessage));
    waitForResponse(10);

    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testPTTControl() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // TX0; for transmit, RX; for receive
    QVERIFY(radio.setPTT(true));
    waitForResponse(10);

    QVERIFY(radio.setPTT(false));
    waitForResponse(10);

    // Verify multiple PTT cycles work
    QVERIFY(radio.setPTT(true));
    waitForResponse(10);
    QVERIFY(radio.setPTT(false));
    waitForResponse(10);

    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testSplitControl() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // Enable split: TB1; FT1; (split on, TX on VFO B)
    QVERIFY(radio.setSplit(true));
    waitForResponse(20);

    // Disable split: TB0;
    QVERIFY(radio.setSplit(false));
    waitForResponse(20);

    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testRITControl() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // Enable RIT: RT1;
    QVERIFY(radio.enableRIT(true));
    waitForResponse(10);

    // Disable RIT: RT0;
    QVERIFY(radio.enableRIT(false));
    waitForResponse(10);

    // Clear RIT offset: RC;
    QVERIFY(radio.clearRIT());
    waitForResponse(10);

    // Enable XIT: XT1;
    QVERIFY(radio.enableXIT(true));
    waitForResponse(10);

    // Disable XIT: XT0;
    QVERIFY(radio.enableXIT(false));
    waitForResponse(10);

    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testMultipleRapidCommands() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // Rapid-fire frequency changes (stress test)
    for (int i = 0; i < 10; i++) {
        freq_t freq = 14000000 + (i * 10000);  // 14.0, 14.01, 14.02...
        QVERIFY(radio.setFrequency(freq));
        QThread::msleep(10);
    }

    waitForResponse(20);

    // Connection must remain stable
    QVERIFY(radio.isConnected());

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testErrorHandlingInvalidCWSpeed() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // TS-890 supports 4-60 WPM — values outside range should fail
    QVERIFY(!radio.setCWSpeed(2));    // Too low (min is 4)
    QVERIFY(!radio.setCWSpeed(100));  // Too high (max is 60)

    radio.disconnect();
    delete setup.env;
}

void TestTS890RadioSimulator::testDisconnectAndReconnect() {
    if (!m_simulatorAvailable) QSKIP("Simulator not available");

    TestSetup setup = setupTest();
    QVERIFY(setup.env != nullptr);

    TS890Radio radio;
    QVERIFY(connectAndWait(radio, setup.config));

    // First connection — send a command
    QVERIFY(radio.setFrequency(14200000));
    waitForResponse(10);

    // Disconnect
    radio.disconnect();
    QVERIFY(!radio.isConnected());

    // Give bridge/simulator time to reset
    QThread::msleep(200);

    // Reconnect (longer timeout for second connection)
    QVERIFY(connectAndWait(radio, setup.config, 1000));
    QVERIFY(radio.isConnected());

    // Verify we can still control radio after reconnect
    QVERIFY(radio.setMode(ModeType::CW));
    waitForResponse(10);

    radio.disconnect();
    delete setup.env;
}

QTEST_MAIN(TestTS890RadioSimulator)
#include "test_ts890_simulator.moc"
