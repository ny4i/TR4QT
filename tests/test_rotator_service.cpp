/**
 * Unit tests for RotatorService and PSTRotatorController
 *
 * Tests rotator control system extracted for Issue #60
 *
 * Coverage:
 * - Mock rotator for testing service layer
 * - RotatorService connection management
 * - Azimuth validation
 * - Error signal propagation
 * - Command execution (setAzimuth, stop)
 *
 * Note: Physical rotator tests require manual validation
 * (see tools/rotator_test_manual.cpp)
 */

#include <QtTest/QtTest>
#include "../src/services/RotatorService.h"
#include "../src/rotator/IRotatorController.h"
#include "../src/rotator/RotatorFactory.h"

using namespace TR4QT;

/**
 * MockRotatorController - Test double for IRotatorController
 *
 * Simulates rotator behavior without network communication
 * Tracks commands for verification in tests
 */
class MockRotatorController : public IRotatorController {
    Q_OBJECT

public:
    explicit MockRotatorController(QObject* parent = nullptr)
        : IRotatorController(parent) {}

    // Command tracking
    struct CommandLog {
        QString command;
        int value{0};
        QDateTime timestamp;
    };
    QVector<CommandLog> commandHistory;

    // Simulated state
    bool simulateConnectSuccess{true};
    bool simulateAzimuthSuccess{true};
    int simulatedAzimuth{0};
    int simulatedResponseDelay{0};  // ms

public slots:
    bool connect(const RotatorConfig& config) override {
        m_config = config;
        m_connected = simulateConnectSuccess;

        if (m_connected) {
            m_state.isConnected = true;
            m_state.isValid = true;
            emit connectionStatusChanged(true);
        }

        return m_connected;
    }

    void disconnect() override {
        m_connected = false;
        m_state.isConnected = false;
        emit connectionStatusChanged(false);
    }

    bool setAzimuth(int degrees) override {
        if (!m_connected) {
            emit errorOccurred("Not connected");
            return false;
        }

        if (degrees < 0 || degrees > 360) {
            emit errorOccurred(QString("Invalid azimuth: %1").arg(degrees));
            return false;
        }

        CommandLog log;
        log.command = "setAzimuth";
        log.value = degrees;
        log.timestamp = QDateTime::currentDateTime();
        commandHistory.append(log);

        if (simulateAzimuthSuccess) {
            simulatedAzimuth = degrees;
            m_state.azimuth = degrees;
            emit azimuthChanged(degrees);
        }

        return simulateAzimuthSuccess;
    }

    void stop() override {
        if (!m_connected) {
            return;
        }

        CommandLog log;
        log.command = "stop";
        log.timestamp = QDateTime::currentDateTime();
        commandHistory.append(log);
    }

    bool setElevation(int degrees) override {
        Q_UNUSED(degrees);
        return false;  // Not implemented
    }

public:
    bool isConnected() const override {
        return m_connected;
    }

    std::optional<int> getAzimuth(int timeoutMs = 1000) const override {
        Q_UNUSED(timeoutMs);

        if (!m_connected) {
            return std::nullopt;
        }

        if (simulatedResponseDelay > 0) {
            QThread::msleep(simulatedResponseDelay);
        }

        return simulatedAzimuth;
    }

    RotatorState getCurrentState() const override {
        return m_state;
    }

    // Test helpers
    void clearCommandHistory() {
        commandHistory.clear();
    }

    int getCommandCount(const QString& command) const {
        return std::count_if(commandHistory.begin(), commandHistory.end(),
            [&command](const CommandLog& log) {
                return log.command == command;
            });
    }

private:
    RotatorConfig m_config;
    RotatorState m_state;
    bool m_connected{false};
};

/**
 * TestRotatorService - Test suite for rotator service layer
 */
class TestRotatorService : public QObject {
    Q_OBJECT

private slots:
    /**
     * Test: Service connects to rotator successfully
     */
    void testConnectSuccess() {
        MockRotatorController* mockRotator = new MockRotatorController();
        RotatorService service(mockRotator, this);

        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;

        // Setup signal spy
        QSignalSpy connectedSpy(&service, &RotatorService::connectionStatusChanged);

        bool result = service.connectToRotator(config);

        QVERIFY(result);
        QVERIFY(service.isConnected());
        QCOMPARE(connectedSpy.count(), 1);
        QCOMPARE(connectedSpy.at(0).at(0).toBool(), true);

        delete mockRotator;
    }

    /**
     * Test: Service handles connection failure
     */
    void testConnectFailure() {
        MockRotatorController* mockRotator = new MockRotatorController();
        mockRotator->simulateConnectSuccess = false;

        RotatorService service(mockRotator, this);

        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;

        // Setup signal spy for errors
        QSignalSpy errorSpy(&service, &RotatorService::errorOccurred);

        bool result = service.connectToRotator(config);

        QVERIFY(!result);
        QVERIFY(!service.isConnected());
        QCOMPARE(errorSpy.count(), 1);

        delete mockRotator;
    }

    /**
     * Test: Service sets azimuth successfully
     */
    void testSetAzimuthSuccess() {
        MockRotatorController* mockRotator = new MockRotatorController();
        RotatorService service(mockRotator, this);

        // Connect first
        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;
        service.connectToRotator(config);

        mockRotator->clearCommandHistory();

        // Set azimuth
        bool result = service.setAzimuth(90);

        QVERIFY(result);
        QCOMPARE(mockRotator->getCommandCount("setAzimuth"), 1);
        QCOMPARE(mockRotator->commandHistory.last().value, 90);

        delete mockRotator;
    }

    /**
     * Test: Service rejects invalid azimuth values
     */
    void testSetAzimuthValidation() {
        MockRotatorController* mockRotator = new MockRotatorController();
        RotatorService service(mockRotator, this);

        // Connect first
        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;
        service.connectToRotator(config);

        // Setup signal spy for errors
        QSignalSpy errorSpy(&service, &RotatorService::errorOccurred);

        // Test invalid values
        mockRotator->setAzimuth(-1);    // Below min
        mockRotator->setAzimuth(361);   // Above max

        QCOMPARE(errorSpy.count(), 2);

        delete mockRotator;
    }

    /**
     * Test: Service stops rotator
     */
    void testStop() {
        MockRotatorController* mockRotator = new MockRotatorController();
        RotatorService service(mockRotator, this);

        // Connect first
        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;
        service.connectToRotator(config);

        mockRotator->clearCommandHistory();

        // Stop rotator
        service.stop();

        QCOMPARE(mockRotator->getCommandCount("stop"), 1);

        delete mockRotator;
    }

    /**
     * Test: Service queries current azimuth
     */
    void testGetAzimuth() {
        MockRotatorController* mockRotator = new MockRotatorController();
        mockRotator->simulatedAzimuth = 180;

        RotatorService service(mockRotator, this);

        // Connect first
        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;
        service.connectToRotator(config);

        // Query azimuth
        auto azimuth = service.getCurrentAzimuth();

        QVERIFY(azimuth.has_value());
        QCOMPARE(azimuth.value(), 180);

        delete mockRotator;
    }

    /**
     * Test: Service cannot set azimuth when disconnected
     */
    void testSetAzimuthWhenDisconnected() {
        MockRotatorController* mockRotator = new MockRotatorController();
        RotatorService service(mockRotator, this);

        // Don't connect

        // Setup signal spy for errors
        QSignalSpy errorSpy(&service, &RotatorService::errorOccurred);

        bool result = service.setAzimuth(90);

        QVERIFY(!result);
        QCOMPARE(errorSpy.count(), 1);

        delete mockRotator;
    }

    /**
     * Test: Disconnect clears connection state
     */
    void testDisconnect() {
        MockRotatorController* mockRotator = new MockRotatorController();
        RotatorService service(mockRotator, this);

        // Connect first
        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;
        service.connectToRotator(config);

        QVERIFY(service.isConnected());

        // Setup signal spy
        QSignalSpy connectedSpy(&service, &RotatorService::connectionStatusChanged);

        // Disconnect
        service.disconnectFromRotator();

        QVERIFY(!service.isConnected());
        QCOMPARE(connectedSpy.count(), 1);
        QCOMPARE(connectedSpy.at(0).at(0).toBool(), false);

        delete mockRotator;
    }

    /**
     * Test: RotatorFactory creates PSTRotator instance
     */
    void testFactoryCreatesPSTRotator() {
        RotatorConfig config;
        config.ipAddress = "192.168.1.100";
        config.port = 12000;

        // Note: Factory connects during creation
        // UDP binding can succeed locally even without physical rotator
        IRotatorController* rotator = RotatorFactory::createRotator(
            RotatorFactory::RotatorType::PSTROTATOR,
            config,
            this
        );

        // Factory should succeed in creating controller and binding UDP socket
        // Actual rotator commands will fail if no physical hardware present
        QVERIFY(rotator != nullptr);
        QVERIFY(rotator->isConnected());

        // Cleanup
        delete rotator;

        // Real PSTRotator communication would require physical hardware or mock UDP server
        // See tools/rotator_test_manual.cpp for manual testing with actual rotator
    }

    /**
     * Test: Factory returns correct type name
     */
    void testFactoryTypeName() {
        QString name = RotatorFactory::rotatorTypeName(
            RotatorFactory::RotatorType::PSTROTATOR
        );

        QCOMPARE(name, QString("PSTRotator"));
    }
};

// Include MOC-generated code
#include "test_rotator_service.moc"

// Run tests
QTEST_MAIN(TestRotatorService)
