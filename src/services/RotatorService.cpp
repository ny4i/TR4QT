#include "RotatorService.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

RotatorService::RotatorService(IRotatorController* controller, QObject* parent)
    : QObject(parent)
    , m_rotator(controller)
{
    LOG_DEBUG("RotatorService", "Constructor");

    // Connect controller signals to service slots
    connect(m_rotator, &IRotatorController::connectionStatusChanged,
            this, &RotatorService::onRotatorConnected);
    connect(m_rotator, &IRotatorController::errorOccurred,
            this, &RotatorService::onRotatorError);
    connect(m_rotator, &IRotatorController::azimuthChanged,
            this, &RotatorService::onAzimuthChanged);
    connect(m_rotator, &IRotatorController::stateUpdated,
            this, &RotatorService::onStateUpdated);
}

RotatorService::~RotatorService()
{
    LOG_DEBUG("RotatorService", "Destructor");
}

bool RotatorService::isConnected() const
{
    return m_rotator->isConnected();
}

RotatorState RotatorService::currentState() const
{
    return m_currentState;
}

bool RotatorService::connectToRotator(const RotatorConfig& config)
{
    LOG_INFO("RotatorService", QString("Connecting to rotator at %1:%2")
        .arg(config.ipAddress).arg(config.port));

    bool success = m_rotator->connect(config);

    if (success) {
        QString msg = QString("Connected to rotator at %1:%2")
            .arg(config.ipAddress).arg(config.port);
        LOG_INFO("RotatorService", msg);
        emit statusMessage(msg);
    } else {
        QString msg = QString("Failed to connect to rotator at %1:%2")
            .arg(config.ipAddress).arg(config.port);
        LOG_ERROR("RotatorService", msg);
        emit errorOccurred(msg);
    }

    return success;
}

void RotatorService::disconnectFromRotator()
{
    LOG_INFO("RotatorService", "Disconnecting from rotator");
    m_rotator->disconnect();
    emit statusMessage("Disconnected from rotator");
}

bool RotatorService::setAzimuth(int degrees)
{
    if (!isConnected()) {
        QString error = "Cannot set azimuth: Not connected to rotator";
        LOG_WARN("RotatorService", error);
        emit errorOccurred(error);
        return false;
    }

    // Validation is handled by controller, but log here for visibility
    LOG_INFO("RotatorService", QString("Setting azimuth to %1°").arg(degrees));

    bool success = m_rotator->setAzimuth(degrees);

    if (success) {
        emit statusMessage(QString("Rotating to %1°").arg(degrees));
    }

    return success;
}

void RotatorService::stop()
{
    if (!isConnected()) {
        LOG_WARN("RotatorService", "Cannot stop: Not connected to rotator");
        return;
    }

    LOG_INFO("RotatorService", "Stopping rotator");
    m_rotator->stop();
    emit statusMessage("Rotator stopped");
}

std::optional<int> RotatorService::getCurrentAzimuth(int timeoutMs) const
{
    if (!isConnected()) {
        LOG_WARN("RotatorService", "Cannot query azimuth: Not connected to rotator");
        return std::nullopt;
    }

    LOG_DEBUG("RotatorService", "Querying current azimuth");
    return m_rotator->getAzimuth(timeoutMs);
}

// ==================== Private Slots ====================

void RotatorService::onRotatorConnected(bool connected)
{
    LOG_INFO("RotatorService", QString("Rotator connection status changed: %1")
        .arg(connected ? "connected" : "disconnected"));

    emit connectionStatusChanged(connected);

    if (!connected) {
        m_currentState.isConnected = false;
        m_currentState.isValid = false;
    }
}

void RotatorService::onRotatorError(const QString& error)
{
    LOG_ERROR("RotatorService", QString("Rotator error: %1").arg(error));
    emit errorOccurred(error);
}

void RotatorService::onAzimuthChanged(int degrees)
{
    LOG_DEBUG("RotatorService", QString("Azimuth changed to %1°").arg(degrees));
    m_currentState.azimuth = degrees;
    emit azimuthChanged(degrees);
}

void RotatorService::onStateUpdated(const RotatorState& state)
{
    LOG_DEBUG("RotatorService", QString("Rotator state updated: azimuth=%1°")
        .arg(state.azimuth));
    m_currentState = state;
    emit stateUpdated(state);
}

} // namespace TR4QT
