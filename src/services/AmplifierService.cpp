#include "AmplifierService.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

AmplifierService::AmplifierService(AmplifierController* controller, QObject* parent)
    : QObject(parent)
    , m_amplifier(controller)
{
    if (!m_amplifier) {
        LOG_ERROR("AmplifierService", "Null amplifier controller provided");
        return;
    }

    // Connect amplifier controller signals to service slots
    // AmplifierController runs the actual device in a worker thread to prevent UI freezing (Issue #69)
    connect(m_amplifier, &AmplifierController::connectionStatusChanged,
            this, &AmplifierService::onAmplifierConnected);
    connect(m_amplifier, &AmplifierController::stateUpdated,
            this, &AmplifierService::onStateUpdated);
    connect(m_amplifier, &AmplifierController::forwardPowerChanged,
            this, &AmplifierService::onForwardPowerChanged);
    connect(m_amplifier, &AmplifierController::swrChanged,
            this, &AmplifierService::onSwrChanged);
    connect(m_amplifier, &AmplifierController::faultDetected,
            this, &AmplifierService::onFaultDetected);
    connect(m_amplifier, &AmplifierController::operatingStatusChanged,
            this, &AmplifierService::onOperatingStatusChanged);
    connect(m_amplifier, &AmplifierController::temperatureChanged,
            this, &AmplifierService::onTemperatureChanged);
    connect(m_amplifier, &AmplifierController::errorOccurred,
            this, &AmplifierService::onAmplifierError);

    // Get initial state if connected
    if (m_amplifier->isConnected()) {
        m_currentState = m_amplifier->getState();
    }
}

AmplifierService::~AmplifierService() = default;

bool AmplifierService::isConnected() const {
    return m_amplifier ? m_amplifier->isConnected() : false;
}

AmplifierState AmplifierService::currentState() const {
    return m_currentState;
}

void AmplifierService::connectToAmplifier(int amplifierType, const AmplifierConfig& config) {
    if (!m_amplifier) {
        LOG_ERROR("AmplifierService", "Cannot connect: null amplifier controller");
        emit errorOccurred("Cannot connect: null amplifier controller");
        return;
    }

    LOG_INFO("AmplifierService", QString("Connecting to amplifier at %1 (async)").arg(config.port));

    // Connection is async - success/failure will be reported via connectionStatusChanged signal
    m_amplifier->connectToAmplifier(amplifierType, config);

    emit statusMessage(QString("Connecting to amplifier at %1...").arg(config.port));
}

void AmplifierService::disconnectFromAmplifier() {
    if (!m_amplifier) return;

    LOG_INFO("AmplifierService", "Disconnecting from amplifier");
    m_amplifier->disconnectFromAmplifier();
    m_currentState = AmplifierState{};  // Reset state
    emit statusMessage("Disconnected from amplifier");
}

void AmplifierService::sendCommand(const QString& command) {
    if (!m_amplifier) {
        LOG_ERROR("AmplifierService", "Cannot send command: null amplifier controller");
        return;
    }

    if (!m_amplifier->isConnected()) {
        LOG_WARN("AmplifierService", "Cannot send command: not connected");
        emit errorOccurred("Cannot send command: amplifier not connected");
        return;
    }

    LOG_TRACE("AmplifierService", QString("Sending command: %1").arg(command));

    // Send command through the controller (runs in worker thread)
    // The controller implementation handles logging and feedback
    m_amplifier->sendRawCommand(command);
}

void AmplifierService::queryStatus() {
    if (!m_amplifier) return;

    m_amplifier->queryStatus();
}

void AmplifierService::onAmplifierConnected(bool connected) {
    LOG_INFO("AmplifierService", QString("Amplifier connection status changed: %1")
        .arg(connected ? "connected" : "disconnected"));

    if (!connected) {
        m_currentState = AmplifierState{};  // Reset state on disconnect
    }

    emit connectionStatusChanged(connected);
}

void AmplifierService::onAmplifierError(const QString& error) {
    LOG_ERROR("AmplifierService", QString("Amplifier error: %1").arg(error));
    emit errorOccurred(error);
}

void AmplifierService::onStateUpdated(const AmplifierState& state) {
    m_currentState = state;
    emit stateUpdated(state);
}

void AmplifierService::onForwardPowerChanged(int watts) {
    m_currentState.forwardPowerWatts = watts;
    emit forwardPowerChanged(watts);
}

void AmplifierService::onSwrChanged(float swr) {
    m_currentState.swr = swr;
    emit swrChanged(swr);
}

void AmplifierService::onFaultDetected(const QString& faultCode) {
    m_currentState.faultDetected = true;
    m_currentState.faultCode = faultCode;
    emit faultDetected(faultCode);
}

void AmplifierService::onOperatingStatusChanged(bool operateMode) {
    m_currentState.operateMode = operateMode;
    emit operatingStatusChanged(operateMode);
}

void AmplifierService::onTemperatureChanged(int celsius) {
    m_currentState.temperature = celsius;
    emit temperatureChanged(celsius);
}

} // namespace TR4QT
