#include "RadioPreflightHelper.h"
#include "../logging/LogMacros.h"
#include <QTcpSocket>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <QProcess>

namespace TR4QT {

bool RadioPreflightHelper::icmpPing(const QString& host, int timeoutMs)
{
    LOG_DEBUG("RadioPreflight", QString("ICMP ping: Testing %1 (timeout %2ms)")
        .arg(host).arg(timeoutMs));

    QProcess pingProcess;
    pingProcess.setProgram("ping");

    // Platform-specific ping arguments
#ifdef Q_OS_WIN
    // Windows: ping -n 1 -w timeout_ms host
    pingProcess.setArguments({"-n", "1", "-w", QString::number(timeoutMs), host});
#else
    // macOS/Linux: ping -c 1 -W timeout_seconds host
    int timeoutSec = (timeoutMs + 999) / 1000;  // Round up to nearest second
    pingProcess.setArguments({"-c", "1", "-W", QString::number(timeoutSec), host});
#endif

    QElapsedTimer timer;
    timer.start();
    pingProcess.start();

    // Wait for ping to complete (with timeout)
    bool finished = pingProcess.waitForFinished(timeoutMs + 500);  // Add 500ms margin
    qint64 elapsed = timer.elapsed();

    if (!finished) {
        pingProcess.kill();
        LOG_WARN("RadioPreflight", QString("ICMP ping: TIMEOUT - %1 did not respond within %2ms")
            .arg(host).arg(timeoutMs));
        return false;
    }

    int exitCode = pingProcess.exitCode();
    if (exitCode == 0) {
        LOG_DEBUG("RadioPreflight", QString("ICMP ping: SUCCESS - %1 is reachable (took %2ms)")
            .arg(host).arg(elapsed));
        return true;
    } else {
        QString output = QString::fromLocal8Bit(pingProcess.readAllStandardOutput());
        QString error = QString::fromLocal8Bit(pingProcess.readAllStandardError());
        LOG_WARN("RadioPreflight", QString("ICMP ping: FAILED - %1 unreachable (exit code %2, took %3ms)")
            .arg(host).arg(exitCode).arg(elapsed));
        if (!output.isEmpty()) {
            LOG_DEBUG("RadioPreflight", QString("Ping output: %1").arg(output.trimmed()));
        }
        if (!error.isEmpty()) {
            LOG_DEBUG("RadioPreflight", QString("Ping error: %1").arg(error.trimmed()));
        }
        return false;
    }
}

// TODO: Add ICMP ping as first preflight step for all radios (not just Icom)
// Enhancement: Before attempting TCP connection, perform ICMP ping test
// Rationale:
//   1. If host doesn't respond to ping, skip TCP connection attempt (faster failure)
//   2. Provides diagnostic info: "Radio unreachable (ping failed)" vs "Radio port not responding"
//   3. Helps distinguish network routing issues from radio configuration issues
// Implementation: Use QProcess to run system ping command or implement ICMP directly
// Note: ICMP ping doesn't use ports - this is a host-level reachability test

// TODO: Architecture Improvement - Make preflight strategy configurable per radio class
// Current implementation hardcodes preflight strategy in RadioPreflightHelper switch statement.
// Better design: Each radio class should specify its preflight requirements.
//
// Proposed approach:
//   1. Add PreflightStrategy enum to RadioInterface base class:
//      - PING_ONLY (Icom UDP radios)
//      - TCP_CONNECT (generic network connectivity)
//      - TCP_WITH_COMMAND (K4, verify with ID command)
//      - PING_THEN_TCP (belt-and-suspenders approach)
//      - PING_THEN_TCP_WITH_COMMAND (full verification)
//
//   2. Each radio class overrides getPreflightStrategy():
//      - IcomRadio::getPreflightStrategy() returns PING_ONLY
//      - K4Radio::getPreflightStrategy() returns TCP_WITH_COMMAND
//      - HamlibRadio::getPreflightStrategy() returns TCP_CONNECT
//
//   3. RadioPreflightHelper::radioSpecificPreflight() asks the radio class:
//      PreflightStrategy strategy = radioClass->getPreflightStrategy();
//      switch (strategy) { ... }
//
// Benefits:
//   - No hardcoded model ID lists (maintainability)
//   - Radio class encapsulates its own preflight requirements (cohesion)
//   - Easy to add new radios without touching RadioPreflightHelper (open/closed principle)
//   - Radio-specific commands/protocol details stay in radio class (separation of concerns)

bool RadioPreflightHelper::generalPreflight(const QString& host, quint16 port, int timeoutMs)
{
    QTcpSocket socket;

    LOG_DEBUG("RadioPreflight", QString("General pre-flight: Testing connectivity to %1:%2 (timeout %3ms)")
        .arg(host).arg(port).arg(timeoutMs));

    // Create event loop to wait for connection with timeout
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    bool connected = false;
    bool timedOut = false;
    QAbstractSocket::SocketError socketError = QAbstractSocket::UnknownSocketError;
    QString errorString;

    // Connect signals
    QObject::connect(&socket, &QTcpSocket::connected, [&]() {
        connected = true;
        loop.quit();
    });

    QObject::connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
                     [&](QAbstractSocket::SocketError error) {
        socketError = error;
        errorString = socket.errorString();
        loop.quit();
    });

    QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        timedOut = true;
        loop.quit();
    });

    // Start connection attempt and measure time
    QElapsedTimer timer;
    timer.start();
    socket.connectToHost(host, port);
    timeoutTimer.start(timeoutMs);

    // Wait for connection or timeout
    loop.exec();
    qint64 elapsed = timer.elapsed();

    // Clean up
    if (socket.state() == QAbstractSocket::ConnectedState) {
        socket.disconnectFromHost();
        if (socket.state() != QAbstractSocket::UnconnectedState) {
            socket.waitForDisconnected(100);
        }
    } else {
        socket.abort();
    }

    if (connected) {
        LOG_DEBUG("RadioPreflight", QString("General pre-flight: SUCCESS - %1:%2 is reachable (took %3ms)")
            .arg(host).arg(port).arg(elapsed));
        return true;
    } else if (timedOut) {
        LOG_WARN("RadioPreflight", QString("General pre-flight: TIMEOUT - %1:%2 not reachable within %3ms (elapsed: %4ms)")
            .arg(host).arg(port).arg(timeoutMs).arg(elapsed));
        return false;
    } else {
        LOG_WARN("RadioPreflight", QString("General pre-flight: FAILED - %1:%2 error after %3ms: %4 (error code: %5)")
            .arg(host).arg(port).arg(elapsed).arg(errorString).arg(socketError));
        return false;
    }
}

bool RadioPreflightHelper::radioSpecificPreflight(rig_model_t radioModel, const QString& host, quint16 port, int timeoutMs)
{
    LOG_DEBUG("RadioPreflight", QString("Radio-specific pre-flight: Testing radio model %1 at %2:%3")
        .arg(radioModel).arg(host).arg(port));

    // Dispatch to radio-specific verification based on model
    switch (radioModel) {
        case RIG_MODEL_K4:
            LOG_DEBUG("RadioPreflight", "Using K4-specific verification (ID command)");
            return verifyK4(host, port, timeoutMs);

        // Icom network radios use UDP protocol, so we can only verify host reachability via ICMP ping
        // Supported Icom network radios (model IDs 3000-3999)
        case 3090:  // IC-905
        case 3077:  // IC-9700
        case 3078:  // IC-7610
        case 3071:  // IC-7600
        case 3074:  // IC-7300MK2
        case 3087:  // IC-705
        case 3095:  // IC-R8600
        case 3075:  // IC-7850/7851
        case 3092:  // IC-7760
            LOG_DEBUG("RadioPreflight", QString("Using ICMP ping for Icom radio model %1 (UDP protocol)")
                .arg(radioModel));
            return icmpPing(host, timeoutMs);

        default:
            // No specific verification implemented for this radio model
            // Fall back to general connectivity test
            LOG_DEBUG("RadioPreflight", QString("No specific verification for model %1, using general pre-flight")
                .arg(radioModel));
            return generalPreflight(host, port, timeoutMs);
    }
}

// Helper function to parse and log K4 Option Module Info response
// OM response format: "OM APXSHML14---;" where each position indicates an option module
static void parseAndLogK4OptionModules(const QString& omResponse)
{
    // Expected format: "OM APXSHML14---;"
    // Extract the option string (characters after "OM ")
    int omIndex = omResponse.indexOf("OM ");
    if (omIndex == -1) {
        LOG_WARN("RadioPreflight", QString("K4 OM parsing: Invalid response format: '%1'").arg(omResponse));
        return;
    }

    QString optionStr = omResponse.mid(omIndex + 3).trimmed();
    // Remove trailing semicolon if present
    if (optionStr.endsWith(';')) {
        optionStr.chop(1);
    }

    LOG_DEBUG("RadioPreflight", QString("K4 Option Module Info: Raw='%1'").arg(optionStr));

    // Parse each position
    QStringList detectedModules;
    QString radioModel = "K4";  // Base model

    // Position 0: A = ATU (KAT4)
    if (optionStr.length() > 0 && optionStr[0] == 'A') {
        detectedModules << "ATU (KAT4)";
    }

    // Position 1: P = PA (KPA4)
    if (optionStr.length() > 1 && optionStr[1] == 'P') {
        detectedModules << "PA (KPA4)";
    }

    // Position 2: X = XVTR (transverter)
    if (optionStr.length() > 2 && optionStr[2] == 'X') {
        detectedModules << "XVTR (Transverter)";
    }

    // Position 3: S = SUB RX (KRX4 + 2nd KDDC4, standard in K4D)
    if (optionStr.length() > 3 && optionStr[3] == 'S') {
        detectedModules << "SUB RX (KRX4 + KDDC4)";
    }

    // Position 4: H = HDR MODULE (KHDR4 + KDDC4-2, standard in K4HD)
    if (optionStr.length() > 4 && optionStr[4] == 'H') {
        detectedModules << "HDR MODULE (KHDR4 + KDDC4-2)";
    }

    // Position 5: M = K40 Mini
    if (optionStr.length() > 5 && optionStr[5] == 'M') {
        detectedModules << "K40 Mini";
    }

    // Position 6: L = Linear amp detected (generic, e.g. KPA500/KPA1500)
    // Position 7: 1 = KPA1500 amp detected (specific)
    // If KPA1500 is specifically detected, don't also report generic linear amp
    bool hasGenericLinearAmp = (optionStr.length() > 6 && optionStr[6] == 'L');
    bool hasKPA1500 = (optionStr.length() > 7 && optionStr[7] == '1');

    if (hasKPA1500) {
        detectedModules << "KPA1500 Amp";
    } else if (hasGenericLinearAmp) {
        detectedModules << "Linear Amp (Generic)";
    }

    // Position 8: 4 = K4 identifier
    // This also helps identify specific models when combined with S and H
    bool hasK4Identifier = (optionStr.length() > 8 && optionStr[8] == '4');
    bool hasSubRx = (optionStr.length() > 3 && optionStr[3] == 'S');
    bool hasHdr = (optionStr.length() > 4 && optionStr[4] == 'H');

    if (hasK4Identifier) {
        if (hasSubRx && hasHdr) {
            radioModel = "K4HD";  // K4 with both Sub RX and HDR
        } else if (hasSubRx) {
            radioModel = "K4D";   // K4 with Sub RX
        }
    }

    // Log the results
    LOG_DEBUG("RadioPreflight", QString("K4 Model: %1").arg(radioModel));

    if (detectedModules.isEmpty()) {
        LOG_DEBUG("RadioPreflight", "K4 Option Modules: None detected (base radio only)");
    } else {
        LOG_DEBUG("RadioPreflight", QString("K4 Option Modules: %1").arg(detectedModules.join(", ")));
    }

    // Log a summary for easy reading
    QString summary = QString("K4 Configuration: %1").arg(radioModel);
    if (!detectedModules.isEmpty()) {
        summary += QString(" with %1 option(s): %2").arg(detectedModules.size()).arg(detectedModules.join(", "));
    }
    LOG_DEBUG("RadioPreflight", summary);
}

bool RadioPreflightHelper::verifyK4(const QString& host, quint16 port, int timeoutMs)
{
    LOG_DEBUG("RadioPreflight", QString("K4 verification: Connecting to %1:%2").arg(host).arg(port));

    QTcpSocket socket;
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    bool connected = false;
    bool idVerified = false;
    bool omReceived = false;
    bool timedOut = false;
    QString response;
    QString omResponse;
    QAbstractSocket::SocketError socketError = QAbstractSocket::UnknownSocketError;
    QString errorString;

    // Connect signals
    QObject::connect(&socket, &QTcpSocket::connected, [&]() {
        connected = true;
        LOG_DEBUG("RadioPreflight", "K4 verification: Connected, sending ID command");

        // Send ID command to K4
        const QByteArray idCommand = "ID;";
        qint64 written = socket.write(idCommand);
        if (written != idCommand.size()) {
            LOG_WARN("RadioPreflight", QString("K4 verification: Failed to write complete ID command (wrote %1/%2 bytes)")
                .arg(written).arg(idCommand.size()));
        }
        socket.flush();
    });

    QObject::connect(&socket, &QTcpSocket::readyRead, [&]() {
        // Read response from K4
        QByteArray data = socket.readAll();
        response.append(QString::fromLatin1(data));

        LOG_DEBUG("RadioPreflight", QString("K4 verification: Received data: '%1'").arg(response));

        // Check if we received the expected K4 ID response
        if (!idVerified && response.contains("ID017;")) {
            idVerified = true;
            LOG_DEBUG("RadioPreflight", "K4 verification: ID017 confirmed, sending OM command");

            // Send OM (Option Module Info) command
            const QByteArray omCommand = "OM;";
            qint64 written = socket.write(omCommand);
            if (written != omCommand.size()) {
                LOG_WARN("RadioPreflight", QString("K4 verification: Failed to write complete OM command (wrote %1/%2 bytes)")
                    .arg(written).arg(omCommand.size()));
            }
            socket.flush();

            // Clear response buffer for OM response
            response.clear();
        }
        // Check if we received OM response (after ID verification)
        else if (idVerified && response.contains("OM ")) {
            omResponse = response;
            omReceived = true;
            loop.quit();
        }
    });

    QObject::connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
                     [&](QAbstractSocket::SocketError error) {
        socketError = error;
        errorString = socket.errorString();
        loop.quit();
    });

    QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        timedOut = true;
        loop.quit();
    });

    // Start connection attempt
    QElapsedTimer timer;
    timer.start();
    socket.connectToHost(host, port);
    timeoutTimer.start(timeoutMs);

    // Wait for response or timeout
    loop.exec();
    qint64 elapsed = timer.elapsed();

    // Clean up
    if (socket.state() == QAbstractSocket::ConnectedState) {
        socket.disconnectFromHost();
        if (socket.state() != QAbstractSocket::UnconnectedState) {
            socket.waitForDisconnected(100);
        }
    } else {
        socket.abort();
    }

    // Evaluate result
    if (idVerified && omReceived) {
        // Parse and log K4 option module info
        parseAndLogK4OptionModules(omResponse);

        LOG_DEBUG("RadioPreflight", QString("K4 verification: SUCCESS - K4 confirmed with ID017 and OM info (took %1ms)")
            .arg(elapsed));
        return true;
    } else if (idVerified && !omReceived) {
        // ID verified but OM command failed/timed out - still consider this a success
        LOG_WARN("RadioPreflight", QString("K4 verification: ID017 confirmed but OM command failed (took %1ms)")
            .arg(elapsed));
        return true;
    } else if (timedOut) {
        LOG_WARN("RadioPreflight", QString("K4 verification: TIMEOUT - No ID response within %1ms (elapsed: %2ms, connected: %3, response: '%4')")
            .arg(timeoutMs).arg(elapsed).arg(connected ? "yes" : "no").arg(response));
        return false;
    } else if (!connected) {
        LOG_WARN("RadioPreflight", QString("K4 verification: CONNECTION FAILED - %1:%2 after %3ms: %4 (error code: %5)")
            .arg(host).arg(port).arg(elapsed).arg(errorString).arg(socketError));
        return false;
    } else {
        // Connected but wrong/no response
        LOG_WARN("RadioPreflight", QString("K4 verification: INVALID RESPONSE - Expected 'ID017;' but got '%1' (took %2ms)")
            .arg(response).arg(elapsed));
        return false;
    }
}

} // namespace TR4QT
