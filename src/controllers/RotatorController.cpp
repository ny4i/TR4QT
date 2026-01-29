#include "RotatorController.h"
#include "../rotator/RotatorFactory.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>

namespace TR4QT {

RotatorController::RotatorController(QObject* parent)
    : QObject(parent)
{
    LOG_DEBUG("RotatorController", QString("Created (main thread: %1)")
              .arg(reinterpret_cast<quintptr>(QThread::currentThread())));

    // Start worker thread (rotator will be moved to it when created)
    m_workerThread.start();

    LOG_DEBUG("RotatorController", QString("Worker thread started: %1")
              .arg(reinterpret_cast<quintptr>(&m_workerThread)));
}

RotatorController::~RotatorController() {
    LOG_DEBUG("RotatorController", "Destructor: Stopping worker thread");

    // Signal shutdown to prevent new operations
    m_shutdownRequested.store(true);

    // Disconnect rotator in worker thread (use blocking to ensure it completes)
    if (m_rotator) {
        bool disconnected = QMetaObject::invokeMethod(m_rotator, [this]() {
            if (m_rotator) {
                m_rotator->disconnect();
                LOG_DEBUG("RotatorController", "Rotator disconnected in destructor");
            }
        }, Qt::BlockingQueuedConnection);

        if (!disconnected) {
            LOG_WARN("RotatorController", "Failed to invoke disconnect on worker thread");
        }
    }

    // Stop worker thread
    m_workerThread.quit();
    if (!m_workerThread.wait(2000)) {
        LOG_WARN("RotatorController", "Worker thread didn't exit cleanly, using terminate()");
        m_workerThread.terminate();
        m_workerThread.wait(1000);
    }

    LOG_DEBUG("RotatorController", "Worker thread stopped");
}

bool RotatorController::isConnected() const {
    QMutexLocker locker(&m_stateMutex);
    return m_connected;
}

RotatorState RotatorController::getCurrentState() const {
    QMutexLocker locker(&m_stateMutex);
    return m_lastState;
}

std::optional<int> RotatorController::getAzimuth(int timeoutMs) const {
    if (!m_rotator || m_shutdownRequested.load()) {
        return std::nullopt;
    }

    // Query azimuth in worker thread with timeout
    std::optional<int> result;
    QMetaObject::invokeMethod(m_rotator, [this, &result, timeoutMs]() {
        result = m_rotator->getAzimuth(timeoutMs);
    }, Qt::BlockingQueuedConnection);

    return result;
}

void RotatorController::createRotator(int rotatorType, const RotatorConfig& config) {
    // Delete old rotator if exists
    if (m_rotator) {
        // Disconnect signals first
        QObject::disconnect(m_rotator, nullptr, this, nullptr);

        // Disconnect device in worker thread
        QMetaObject::invokeMethod(m_rotator, [this]() {
            m_rotator->disconnect();
        }, Qt::BlockingQueuedConnection);

        m_rotator->deleteLater();
        m_rotator = nullptr;
    }

    // Clear cached state
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastState = RotatorState();
        m_connected = false;
    }

    // Create new rotator via factory
    auto type = static_cast<RotatorFactory::RotatorType>(rotatorType);
    m_rotator = RotatorFactory::createRotator(type, config, nullptr);  // No parent - we manage lifetime

    if (!m_rotator) {
        LOG_ERROR("RotatorController", "Failed to create rotator");
        emit errorOccurred("Failed to create rotator");
        return;
    }

    // Move rotator to worker thread (QTimer children move with it)
    m_rotator->moveToThread(&m_workerThread);

    LOG_DEBUG("RotatorController", QString("Rotator moved to worker thread: %1")
              .arg(reinterpret_cast<quintptr>(&m_workerThread)));

    // Connect signals
    connectRotatorSignals();

    // Clean up rotator when thread finishes
    QObject::connect(&m_workerThread, &QThread::finished,
                     m_rotator, &QObject::deleteLater);
}

void RotatorController::connectRotatorSignals() {
    if (!m_rotator) return;

    // Forward all signals from rotator to controller
    // Qt automatically uses QueuedConnection for cross-thread signals

    QObject::connect(m_rotator, &IRotatorController::connectionStatusChanged,
            this, [this](bool connected) {
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_connected = connected;
                }
                emit connectionStatusChanged(connected);
            });

    QObject::connect(m_rotator, &IRotatorController::azimuthChanged,
            this, &RotatorController::azimuthChanged);

    QObject::connect(m_rotator, &IRotatorController::elevationChanged,
            this, &RotatorController::elevationChanged);

    QObject::connect(m_rotator, &IRotatorController::stateUpdated,
            this, [this](const RotatorState& state) {
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_lastState = state;
                }
                emit stateUpdated(state);
            });

    QObject::connect(m_rotator, &IRotatorController::errorOccurred,
            this, &RotatorController::errorOccurred);

    LOG_DEBUG("RotatorController", "Rotator signals connected");
}

void RotatorController::connectToRotator(int rotatorType, const RotatorConfig& config) {
    LOG_DEBUG("RotatorController", QString("connectToRotator: type=%1 ip=%2 port=%3")
              .arg(rotatorType).arg(config.ipAddress).arg(config.port));

    // Reset shutdown flag for new connection
    m_shutdownRequested.store(false);

    // Create rotator (creates device and moves to worker thread)
    createRotator(rotatorType, config);

    if (!m_rotator) {
        return;  // Error already emitted in createRotator
    }

    // Connect in worker thread
    QMetaObject::invokeMethod(m_rotator, [this, config]() {
        if (m_shutdownRequested.load()) {
            LOG_DEBUG("RotatorController", "Shutdown requested, aborting connection");
            return;
        }

        bool success = m_rotator->connect(config);
        LOG_DEBUG("RotatorController", QString("connect() returned %1").arg(success));

        if (!success) {
            emit errorOccurred("Failed to connect to rotator");
        }
    }, Qt::QueuedConnection);
}

void RotatorController::disconnectFromRotator() {
    LOG_DEBUG("RotatorController", "disconnectFromRotator called");

    m_shutdownRequested.store(true);

    if (!m_rotator) {
        return;
    }

    // Disconnect in worker thread (non-blocking)
    QMetaObject::invokeMethod(m_rotator, [this]() {
        if (m_rotator) {
            m_rotator->disconnect();
        }
    }, Qt::QueuedConnection);
}

bool RotatorController::setAzimuth(int degrees) {
    if (!m_rotator || m_shutdownRequested.load()) return false;

    // Validate azimuth range
    if (degrees < 0 || degrees > 360) {
        LOG_WARN("RotatorController", QString("Invalid azimuth: %1 (must be 0-360)").arg(degrees));
        return false;
    }

    QMetaObject::invokeMethod(m_rotator, "setAzimuth", Qt::QueuedConnection,
                              Q_ARG(int, degrees));
    return true;
}

bool RotatorController::setElevation(int degrees) {
    if (!m_rotator || m_shutdownRequested.load()) return false;

    // Validate elevation range
    if (degrees < -90 || degrees > 90) {
        LOG_WARN("RotatorController", QString("Invalid elevation: %1 (must be -90 to 90)").arg(degrees));
        return false;
    }

    QMetaObject::invokeMethod(m_rotator, "setElevation", Qt::QueuedConnection,
                              Q_ARG(int, degrees));
    return true;
}

void RotatorController::stop() {
    if (!m_rotator || m_shutdownRequested.load()) return;

    QMetaObject::invokeMethod(m_rotator, "stop", Qt::QueuedConnection);
}

} // namespace TR4QT
