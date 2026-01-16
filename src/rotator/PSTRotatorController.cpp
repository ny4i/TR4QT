#include "PSTRotatorController.h"
#include "../logging/LogMacros.h"
#include <QHostAddress>
#include <QRegularExpression>

namespace TR4QT {

PSTRotatorController::PSTRotatorController(QObject* parent)
    : IRotatorController(parent)
{
    LOG_DEBUG("PSTRotatorController", "Constructor");
}

PSTRotatorController::~PSTRotatorController()
{
    LOG_DEBUG("PSTRotatorController", "Destructor");
    disconnect();
}

bool PSTRotatorController::connect(const RotatorConfig& config)
{
    LOG_INFO("PSTRotatorController", QString("Connecting to %1:%2")
        .arg(config.ipAddress).arg(config.port));

    QMutexLocker locker(&m_stateMutex);

    if (m_connected) {
        LOG_WARN("PSTRotatorController", "Already connected");
        return true;
    }

    // Validate configuration
    if (config.ipAddress.isEmpty()) {
        QString error = "IP address cannot be empty";
        LOG_ERROR("PSTRotatorController", error);
        emit errorOccurred(error);
        return false;
    }

    if (config.port <= 0 || config.port > 65535) {
        QString error = QString("Invalid port: %1 (must be 1-65535)").arg(config.port);
        LOG_ERROR("PSTRotatorController", error);
        emit errorOccurred(error);
        return false;
    }

    // Store configuration
    m_config = config;

    // Start worker thread
    m_workerThread = new WorkerThread(this);
    m_workerThread->start();

    // Mark as connected
    m_connected = true;
    m_currentState.isConnected = true;
    m_currentState.isValid = false;  // No state data yet

    emit connectionStatusChanged(true);
    LOG_INFO("PSTRotatorController", "Connected successfully");

    return true;
}

void PSTRotatorController::disconnect()
{
    LOG_INFO("PSTRotatorController", "Disconnecting");

    QMutexLocker locker(&m_stateMutex);

    if (!m_connected) {
        return;
    }

    // Stop worker thread
    if (m_workerThread) {
        m_workerThread->requestStop();
        m_queueCondition.wakeOne();  // Wake thread if waiting
        m_workerThread->wait(3000);  // Wait up to 3 seconds
        if (m_workerThread->isRunning()) {
            LOG_WARN("PSTRotatorController", "Worker thread did not stop, terminating");
            m_workerThread->terminate();
            m_workerThread->wait(1000);
        }
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    // Clear state
    m_connected = false;
    m_currentState.isConnected = false;
    m_currentState.isValid = false;

    // Clear command queue
    m_commandQueue.clear();

    emit connectionStatusChanged(false);
    LOG_INFO("PSTRotatorController", "Disconnected");
}

bool PSTRotatorController::setAzimuth(int degrees)
{
    // Validate azimuth range
    if (!isValidAzimuth(degrees)) {
        QString error = QString("Invalid azimuth: %1 (must be 0-360)").arg(degrees);
        LOG_ERROR("PSTRotatorController", error);
        emit errorOccurred(error);
        return false;
    }

    if (!m_connected) {
        QString error = "Not connected to rotator";
        LOG_WARN("PSTRotatorController", error);
        emit errorOccurred(error);
        return false;
    }

    LOG_DEBUG("PSTRotatorController", QString("Queueing setAzimuth(%1)").arg(degrees));

    // Enqueue command for background execution
    Command cmd;
    cmd.type = Command::SetAzimuth;
    cmd.value = degrees;
    enqueueCommand(cmd);

    return true;
}

void PSTRotatorController::stop()
{
    if (!m_connected) {
        LOG_WARN("PSTRotatorController", "Not connected, ignoring stop command");
        return;
    }

    LOG_DEBUG("PSTRotatorController", "Queueing stop command");

    // Enqueue command for background execution
    Command cmd;
    cmd.type = Command::Stop;
    enqueueCommand(cmd);
}

bool PSTRotatorController::setElevation(int degrees)
{
    Q_UNUSED(degrees);
    // Not implemented for PSTRotator (future feature)
    LOG_WARN("PSTRotatorController", "setElevation not implemented for PSTRotator");
    return false;
}

bool PSTRotatorController::isConnected() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_connected;
}

std::optional<int> PSTRotatorController::getAzimuth(int timeoutMs) const
{
    if (!m_connected) {
        LOG_WARN("PSTRotatorController", "Not connected, cannot query azimuth");
        return std::nullopt;
    }

    LOG_DEBUG("PSTRotatorController", QString("Querying azimuth (timeout=%1ms)").arg(timeoutMs));

    // Create socket for query
    QUdpSocket querySocket;

    // CRITICAL: Bind BEFORE sending (otherwise socket state prevents binding)
    int responsePort = m_config.port + 1;
    if (!querySocket.bind(QHostAddress::Any, responsePort)) {
        QString error = QString("Failed to bind to response port %1: %2")
            .arg(responsePort).arg(querySocket.errorString());
        LOG_ERROR("PSTRotatorController", error);
        // Note: Cannot emit signal from const method - error is logged
        return std::nullopt;
    }

    // Send query command
    QString command = buildQueryAzimuthCommand();
    QByteArray datagram = command.toUtf8();

    LOG_INFO("PSTRotatorController", QString("UDP SEND -> %1:%2 : '%3'")
        .arg(m_config.ipAddress).arg(m_config.port).arg(command));

    qint64 sent = querySocket.writeDatagram(
        datagram,
        QHostAddress(m_config.ipAddress),
        m_config.port
    );

    if (sent != datagram.size()) {
        QString error = QString("Failed to send UDP query: %1").arg(querySocket.errorString());
        LOG_ERROR("PSTRotatorController", error);
        // Note: Cannot emit signal from const method - error is logged
        return std::nullopt;
    }

    LOG_INFO("PSTRotatorController", QString("UDP SEND -> %1 bytes sent").arg(sent));

    // Wait for response
    if (!querySocket.waitForReadyRead(timeoutMs)) {
        QString error = QString("No response from rotator on UDP port %1 (timeout after %2ms)")
            .arg(responsePort).arg(timeoutMs);
        LOG_ERROR("PSTRotatorController", error);
        // Note: Cannot emit signal from const method - error is logged
        return std::nullopt;
    }

    // Read response
    QByteArray responseData;
    responseData.resize(querySocket.pendingDatagramSize());
    QHostAddress senderAddress;
    quint16 senderPort;
    querySocket.readDatagram(responseData.data(), responseData.size(), &senderAddress, &senderPort);

    QString response = QString::fromUtf8(responseData).trimmed();
    LOG_INFO("PSTRotatorController", QString("UDP RECV <- %1:%2 : '%3' (%4 bytes)")
        .arg(senderAddress.toString()).arg(senderPort).arg(response).arg(responseData.size()));

    // Parse response
    return parseAzimuthResponse(response);
}

RotatorState PSTRotatorController::getCurrentState() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_currentState;
}

// ==================== Command Building ====================

QString PSTRotatorController::buildSetAzimuthCommand(int degrees) const
{
    return QString("<PST><AZIMUTH>%1</AZIMUTH></PST>").arg(degrees);
}

QString PSTRotatorController::buildStopCommand() const
{
    return QString("<PST><STOP>1</STOP></PST>");
}

QString PSTRotatorController::buildQueryAzimuthCommand() const
{
    return QString("<PST>AZ?</PST>");
}

// ==================== Response Parsing ====================

std::optional<int> PSTRotatorController::parseAzimuthResponse(const QString& response) const
{
    // Expected format: AZ:nnn or AZ:nnn.n
    // Regex: AZ:(\d{1,3})(?:\.\d+)?
    QRegularExpression regex(R"(AZ:(\d{1,3})(?:\.\d+)?)");
    QRegularExpressionMatch match = regex.match(response);

    if (!match.hasMatch()) {
        QString error = QString("Malformed azimuth response: '%1'").arg(response);
        LOG_ERROR("PSTRotatorController", error);
        // Note: Cannot emit signal from const method - error is logged
        return std::nullopt;
    }

    bool ok;
    int azimuth = match.captured(1).toInt(&ok);
    if (!ok || !isValidAzimuth(azimuth)) {
        QString error = QString("Invalid azimuth value in response: '%1'").arg(response);
        LOG_ERROR("PSTRotatorController", error);
        // Note: Cannot emit signal from const method - error is logged
        return std::nullopt;
    }

    LOG_DEBUG("PSTRotatorController", QString("Parsed azimuth: %1°").arg(azimuth));
    return azimuth;
}

// ==================== Command Queue Management ====================

void PSTRotatorController::enqueueCommand(const Command& cmd)
{
    QMutexLocker locker(&m_queueMutex);
    m_commandQueue.enqueue(cmd);
    m_queueCondition.wakeOne();  // Wake worker thread
}

bool PSTRotatorController::dequeueCommand(Command& cmd)
{
    QMutexLocker locker(&m_queueMutex);
    if (m_commandQueue.isEmpty()) {
        return false;
    }
    cmd = m_commandQueue.dequeue();
    return true;
}

void PSTRotatorController::processCommandQueue()
{
    Command cmd;
    while (dequeueCommand(cmd)) {
        QString command;

        switch (cmd.type) {
        case Command::SetAzimuth:
            command = buildSetAzimuthCommand(cmd.value);
            LOG_DEBUG("PSTRotatorController", QString("Executing setAzimuth(%1)").arg(cmd.value));
            break;

        case Command::Stop:
            command = buildStopCommand();
            LOG_DEBUG("PSTRotatorController", "Executing stop");
            break;

        case Command::QueryAzimuth:
            command = buildQueryAzimuthCommand();
            LOG_DEBUG("PSTRotatorController", "Executing queryAzimuth");
            break;
        }

        // Send UDP command
        if (!sendUdpCommand(command)) {
            QString error = "Failed to send UDP command";
            LOG_ERROR("PSTRotatorController", error);
            emit errorOccurred(error);
        }

        // For set commands, we could optionally wait for response
        // For now, fire-and-forget (no response validation)
    }
}

// ==================== UDP Communication ====================

bool PSTRotatorController::sendUdpCommand(const QString& command)
{
    if (!m_sendSocket) {
        LOG_ERROR("PSTRotatorController", "Send socket not initialized");
        return false;
    }

    QByteArray datagram = command.toUtf8();

    LOG_INFO("PSTRotatorController", QString("UDP SEND -> %1:%2 : '%3'")
        .arg(m_config.ipAddress).arg(m_config.port).arg(command));

    qint64 sent = m_sendSocket->writeDatagram(
        datagram,
        QHostAddress(m_config.ipAddress),
        m_config.port
    );

    if (sent != datagram.size()) {
        LOG_ERROR("PSTRotatorController", QString("Failed to send UDP: %1")
            .arg(m_sendSocket->errorString()));
        return false;
    }

    LOG_INFO("PSTRotatorController", QString("UDP SEND -> %1 bytes sent").arg(sent));
    return true;
}

// receiveUdpResponse() removed - not used (fire-and-forget commands don't wait for responses)
// getAzimuth() creates its own temporary socket for queries

// ==================== Validation ====================

bool PSTRotatorController::isValidAzimuth(int degrees) const
{
    return degrees >= MIN_AZIMUTH && degrees <= MAX_AZIMUTH;
}

// ==================== Worker Thread ====================

void PSTRotatorController::WorkerThread::run()
{
    LOG_DEBUG("PSTRotatorController::WorkerThread", "Starting");

    // Create UDP socket for sending commands
    m_controller->m_sendSocket = new QUdpSocket();
    // Note: We don't create m_receiveSocket here because:
    // 1. Fire-and-forget commands don't need responses
    // 2. getAzimuth() creates its own socket to avoid port conflicts

    LOG_INFO("PSTRotatorController::WorkerThread", "Worker thread ready");

    // Main loop: process commands from queue
    while (!m_stopRequested.loadAcquire()) {
        // Wait for commands (with timeout to check stop flag)
        QMutexLocker locker(&m_controller->m_queueMutex);

        if (m_controller->m_commandQueue.isEmpty()) {
            // Wait for command or stop signal (100ms timeout)
            m_controller->m_queueCondition.wait(&m_controller->m_queueMutex, 100);
            continue;
        }

        locker.unlock();

        // Process all pending commands
        m_controller->processCommandQueue();
    }

    // Cleanup
    delete m_controller->m_sendSocket;
    m_controller->m_sendSocket = nullptr;

    LOG_DEBUG("PSTRotatorController::WorkerThread", "Stopped");
}

void PSTRotatorController::WorkerThread::requestStop()
{
    m_stopRequested.storeRelease(1);
}

} // namespace TR4QT
