#include "RadioPreflightHelper.h"
#include "../logging/LogMacros.h"
#include <QTcpSocket>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>

namespace TR4QT {

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

        // Add other radio-specific verifications here in the future
        // case RIG_MODEL_IC7300:
        //     return verifyIcom7300(host, port, timeoutMs);

        default:
            // No specific verification implemented for this radio model
            // Fall back to general connectivity test
            LOG_DEBUG("RadioPreflight", QString("No specific verification for model %1, using general pre-flight")
                .arg(radioModel));
            return generalPreflight(host, port, timeoutMs);
    }
}

bool RadioPreflightHelper::verifyK4(const QString& host, quint16 port, int timeoutMs)
{
    LOG_DEBUG("RadioPreflight", QString("K4 verification: Connecting to %1:%2").arg(host).arg(port));

    QTcpSocket socket;
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    bool connected = false;
    bool responseReceived = false;
    bool timedOut = false;
    QString response;
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
        if (response.contains("ID017;")) {
            responseReceived = true;
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
    if (responseReceived && response.contains("ID017;")) {
        LOG_DEBUG("RadioPreflight", QString("K4 verification: SUCCESS - K4 confirmed with ID017 response (took %1ms)")
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
