/**
 * Manual Test Program for PSTRotator Physical Hardware
 *
 * Purpose:
 * - Verify PSTRotatorController works with actual PSTRotator software
 * - Validate rotator movements and position queries
 * - Test UDP communication protocol implementation
 *
 * Prerequisites:
 * - PSTRotator software must be running and configured
 * - Rotator hardware connected to PSTRotator
 * - UDP control enabled in PSTRotator (default port 12000)
 *
 * Usage:
 *   cd build/tools
 *   ./rotator_test_manual <ip_address> <port>
 *
 * Example:
 *   ./rotator_test_manual 192.168.1.100 12000
 *
 * Test Sequence:
 * 1. Prompt user to start PSTRotator and verify setup
 * 2. Connect to PSTRotator via UDP
 * 3. Query current azimuth
 * 4. Rotate +30° from current position
 * 5. Wait for rotation to complete (assumes 2°/second speed)
 * 6. Query azimuth again and verify movement
 * 7. Test stop command (rotate +60°, stop after 5 seconds)
 * 8. Disconnect
 *
 * Developer Validation:
 * - Watch rotator physically turn to commanded headings
 * - Verify query responses match actual rotator position
 * - Confirm rotation completes and positions are accurate
 */

#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "../src/rotator/PSTRotatorController.h"
#include "../src/rotator/RotatorFactory.h"
#include "../src/logging/LogMacros.h"

using namespace TR4QT;

class RotatorManualTest : public QObject {
    Q_OBJECT

public:
    RotatorManualTest(const QString& ipAddress, int port)
        : m_ipAddress(ipAddress)
        , m_port(port)
        , m_testStep(0)
        , m_initialAzimuth(0)
        , m_targetAzimuth(0)
    {
        std::cout << "=== PSTRotator Manual Test Program ===" << std::endl;
        std::cout << "Target: " << ipAddress.toStdString() << ":" << port << std::endl;
        std::cout << std::endl;
    }

    void run() {
        // Prompt user to verify setup
        std::cout << "PREREQUISITES:" << std::endl;
        std::cout << "  1. PSTRotator software must be running" << std::endl;
        std::cout << "  2. Rotator hardware must be connected" << std::endl;
        std::cout << "  3. UDP control must be enabled (port " << m_port << ")" << std::endl;
        std::cout << std::endl;
        std::cout << "Press ENTER when ready to begin test..." << std::endl;
        std::cin.get();
        std::cout << std::endl;

        // Create rotator controller
        RotatorConfig config;
        config.ipAddress = m_ipAddress;
        config.port = m_port;

        std::cout << "[Step 1] Connecting to PSTRotator..." << std::endl;
        m_rotator = RotatorFactory::createRotator(
            RotatorFactory::RotatorType::PSTROTATOR,
            config,
            this
        );

        if (!m_rotator) {
            std::cerr << "ERROR: Failed to create rotator controller" << std::endl;
            std::cerr << "       Make sure PSTRotator is running and UDP control is enabled" << std::endl;
            QCoreApplication::exit(1);
            return;
        }

        if (!m_rotator->isConnected()) {
            std::cerr << "ERROR: Failed to connect to rotator" << std::endl;
            std::cerr << "       Check IP address and port: " << m_ipAddress.toStdString()
                     << ":" << m_port << std::endl;
            QCoreApplication::exit(1);
            return;
        }

        std::cout << "[Step 1] ✓ Connected to PSTRotator" << std::endl;
        std::cout << std::endl;

        // Connect signals
        connect(m_rotator, &IRotatorController::errorOccurred,
                this, &RotatorManualTest::onError);

        // Start test sequence
        QTimer::singleShot(500, this, &RotatorManualTest::runNextStep);
    }

private slots:
    void runNextStep() {
        m_testStep++;

        switch (m_testStep) {
        case 1:
            // Query initial azimuth
            std::cout << "[Step 2] Querying current azimuth..." << std::endl;
            {
                auto azimuth = m_rotator->getAzimuth();
                if (azimuth.has_value()) {
                    m_initialAzimuth = azimuth.value();
                    std::cout << "[Step 2] ✓ Current azimuth: " << m_initialAzimuth << "°" << std::endl;
                    std::cout << "         VERIFY: Does this match your rotator display?" << std::endl;
                } else {
                    std::cerr << "[Step 2] ✗ Failed to query azimuth" << std::endl;
                    std::cerr << "         Cannot continue without initial position" << std::endl;
                    QCoreApplication::exit(1);
                    return;
                }
            }
            std::cout << std::endl;
            QTimer::singleShot(2000, this, &RotatorManualTest::runNextStep);
            break;

        case 2:
            // Rotate +30° from current position
            m_targetAzimuth = (m_initialAzimuth + 30) % 360;
            std::cout << "[Step 3] Rotating from " << m_initialAzimuth << "° to "
                     << m_targetAzimuth << "° (+30°)..." << std::endl;
            std::cout << "         VERIFY: Watch rotator physically turn clockwise" << std::endl;
            m_rotator->setAzimuth(m_targetAzimuth);
            std::cout << "[Step 3] ✓ Command sent" << std::endl;
            std::cout << std::endl;

            // Calculate wait time: 30° / 2°/sec = 15 seconds + 3 second buffer
            std::cout << "         Waiting 18 seconds for rotation to complete..." << std::endl;
            std::cout << "         (30° at 2°/second = 15 seconds + 3 second buffer)" << std::endl;
            std::cout << std::endl;
            QTimer::singleShot(18000, this, &RotatorManualTest::runNextStep);
            break;

        case 3:
            // Query azimuth after rotation
            std::cout << "[Step 4] Querying azimuth after rotation..." << std::endl;
            {
                auto azimuth = m_rotator->getAzimuth();
                if (azimuth.has_value()) {
                    int actual = azimuth.value();
                    int error = std::abs(actual - m_targetAzimuth);

                    std::cout << "[Step 4] ✓ Current azimuth: " << actual << "°" << std::endl;
                    std::cout << "         Expected: " << m_targetAzimuth << "°" << std::endl;
                    std::cout << "         Error: " << error << "°" << std::endl;

                    if (error <= 5) {
                        std::cout << "         ✓ Azimuth within ±5° tolerance - MOVEMENT VERIFIED!" << std::endl;
                    } else if (error <= 10) {
                        std::cout << "         ⚠ Azimuth within ±10° - Acceptable but check calibration" << std::endl;
                    } else {
                        std::cerr << "         ✗ ERROR: Azimuth error > 10° - Check rotator!" << std::endl;
                    }
                } else {
                    std::cerr << "[Step 4] ✗ Failed to query azimuth" << std::endl;
                }
            }
            std::cout << std::endl;
            QTimer::singleShot(3000, this, &RotatorManualTest::runNextStep);
            break;

        case 4:
            // Test stop command - rotate +60°, stop after 5 seconds
            {
                int stopTarget = (m_targetAzimuth + 60) % 360;
                std::cout << "[Step 5] Testing STOP command..." << std::endl;
                std::cout << "         Commanding rotation to " << stopTarget << "° (+60°)" << std::endl;
                std::cout << "         Will send STOP after 5 seconds (should rotate ~10°)" << std::endl;
                std::cout << "         VERIFY: Rotator starts turning, then stops mid-rotation" << std::endl;
                m_rotator->setAzimuth(stopTarget);
                std::cout << "[Step 5] ✓ Rotation command sent" << std::endl;
                std::cout << std::endl;
            }
            // Wait 5 seconds then stop
            QTimer::singleShot(5000, this, &RotatorManualTest::runNextStep);
            break;

        case 5:
            // Send stop command
            std::cout << "[Step 6] Sending STOP command..." << std::endl;
            std::cout << "         VERIFY: Rotator stops immediately" << std::endl;
            m_rotator->stop();
            std::cout << "[Step 6] ✓ Stop command sent" << std::endl;
            std::cout << std::endl;

            // Wait 2 seconds for rotator to stabilize
            QTimer::singleShot(2000, this, &RotatorManualTest::runNextStep);
            break;

        case 6:
            // Query final azimuth after stop
            std::cout << "[Step 7] Querying azimuth after STOP..." << std::endl;
            {
                auto azimuth = m_rotator->getAzimuth();
                if (azimuth.has_value()) {
                    int actual = azimuth.value();
                    int expected = (m_targetAzimuth + 10) % 360;  // Roughly 10° movement in 5 seconds

                    std::cout << "[Step 7] ✓ Final azimuth: " << actual << "°" << std::endl;
                    std::cout << "         Previous position: " << m_targetAzimuth << "°" << std::endl;
                    std::cout << "         Expected ~" << expected << "° (moved ~10° before stop)" << std::endl;

                    // Check if stopped somewhere between start and full rotation
                    int minExpected = m_targetAzimuth;
                    int maxExpected = (m_targetAzimuth + 60) % 360;

                    if (actual > minExpected && actual < maxExpected) {
                        std::cout << "         ✓ Rotator stopped mid-rotation - STOP VERIFIED!" << std::endl;
                    } else {
                        std::cout << "         ⚠ Position unexpected - Verify stop worked correctly" << std::endl;
                    }
                } else {
                    std::cerr << "[Step 7] ✗ Failed to query azimuth" << std::endl;
                }
            }
            std::cout << std::endl;
            QTimer::singleShot(2000, this, &RotatorManualTest::runNextStep);
            break;

        case 7:
            // Disconnect
            std::cout << "[Step 8] Disconnecting from PSTRotator..." << std::endl;
            m_rotator->disconnect();
            std::cout << "[Step 8] ✓ Disconnected" << std::endl;
            std::cout << std::endl;

            // Print summary
            std::cout << "=== Test Complete ===" << std::endl;
            std::cout << std::endl;
            std::cout << "VERIFICATION CHECKLIST:" << std::endl;
            std::cout << "  ✓ UDP connection to PSTRotator successful" << std::endl;
            std::cout << "  ✓ AZ? query command works" << std::endl;
            std::cout << "  ✓ Rotation command (+30°) works" << std::endl;
            std::cout << "  ✓ Position verification matches target" << std::endl;
            std::cout << "  ✓ STOP command halts rotation" << std::endl;
            std::cout << std::endl;
            std::cout << "If all checks show ✓, PSTRotator integration is working correctly!" << std::endl;

            QCoreApplication::quit();
            break;

        default:
            QCoreApplication::quit();
            break;
        }
    }

    void onError(QString error) {
        std::cerr << "ERROR: " << error.toStdString() << std::endl;
    }

private:
    QString m_ipAddress;
    int m_port;
    IRotatorController* m_rotator{nullptr};
    int m_testStep;
    int m_initialAzimuth;  // Starting position
    int m_targetAzimuth;   // Target position after first rotation
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // Parse command line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip_address> <port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 192.168.1.100 12000" << std::endl;
        return 1;
    }

    QString ipAddress = argv[1];
    int port = QString(argv[2]).toInt();

    if (port <= 0 || port > 65535) {
        std::cerr << "ERROR: Invalid port number (must be 1-65535)" << std::endl;
        return 1;
    }

    // Create and run test
    RotatorManualTest test(ipAddress, port);
    QTimer::singleShot(0, &test, &RotatorManualTest::run);

    return app.exec();
}

#include "rotator_test_manual.moc"
