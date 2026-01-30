/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "AmplifierController.h"
#include "../amplifiers/AmplifierFactory.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>

namespace TR4QT {

AmplifierController::AmplifierController(QObject* parent)
    : QObject(parent)
{
    LOG_DEBUG("AmplifierController", QString("Created (main thread: %1)")
              .arg(reinterpret_cast<quintptr>(QThread::currentThread())));

    // Start worker thread (amplifier will be moved to it when created)
    m_workerThread.start();

    LOG_DEBUG("AmplifierController", QString("Worker thread started: %1")
              .arg(reinterpret_cast<quintptr>(&m_workerThread)));
}

AmplifierController::~AmplifierController() {
    LOG_DEBUG("AmplifierController", "Destructor: Stopping worker thread");

    // Signal shutdown to prevent new operations
    m_shutdownRequested.store(true);

    // Disconnect amplifier in worker thread (use blocking to ensure it completes)
    if (m_amplifier) {
        bool disconnected = QMetaObject::invokeMethod(m_amplifier, [this]() {
            if (m_amplifier) {
                m_amplifier->disconnect();
                LOG_DEBUG("AmplifierController", "Amplifier disconnected in destructor");
            }
        }, Qt::BlockingQueuedConnection);

        if (!disconnected) {
            LOG_WARN("AmplifierController", "Failed to invoke disconnect on worker thread");
        }
    }

    // Stop worker thread
    m_workerThread.quit();
    if (!m_workerThread.wait(2000)) {
        LOG_WARN("AmplifierController", "Worker thread didn't exit cleanly, using terminate()");
        m_workerThread.terminate();
        m_workerThread.wait(1000);
    }

    LOG_DEBUG("AmplifierController", "Worker thread stopped");
}

bool AmplifierController::isConnected() const {
    QMutexLocker locker(&m_stateMutex);
    return m_connected;
}

AmplifierState AmplifierController::getState() const {
    QMutexLocker locker(&m_stateMutex);
    return m_lastState;
}

void AmplifierController::createAmplifier(int amplifierType, const AmplifierConfig& config) {
    // Delete old amplifier if exists
    if (m_amplifier) {
        // Disconnect signals first
        QObject::disconnect(m_amplifier, nullptr, this, nullptr);

        // Disconnect device in worker thread
        QMetaObject::invokeMethod(m_amplifier, [this]() {
            m_amplifier->disconnect();
        }, Qt::BlockingQueuedConnection);

        m_amplifier->deleteLater();
        m_amplifier = nullptr;
    }

    // Clear cached state
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastState = AmplifierState();
        m_connected = false;
    }

    // Create new amplifier via factory
    auto type = static_cast<AmplifierFactory::AmplifierType>(amplifierType);
    m_amplifier = AmplifierFactory::createAmplifier(type, config, nullptr);  // No parent - we manage lifetime

    if (!m_amplifier) {
        LOG_ERROR("AmplifierController", "Failed to create amplifier");
        emit errorOccurred("Failed to create amplifier");
        return;
    }

    // Move amplifier to worker thread (QTimer children move with it)
    m_amplifier->moveToThread(&m_workerThread);

    LOG_DEBUG("AmplifierController", QString("Amplifier moved to worker thread: %1")
              .arg(reinterpret_cast<quintptr>(&m_workerThread)));

    // Connect signals
    connectAmplifierSignals();

    // Clean up amplifier when thread finishes
    QObject::connect(&m_workerThread, &QThread::finished,
                     m_amplifier, &QObject::deleteLater);
}

void AmplifierController::connectAmplifierSignals() {
    if (!m_amplifier) return;

    // Forward all signals from amplifier to controller
    // Qt automatically uses QueuedConnection for cross-thread signals

    QObject::connect(m_amplifier, &IAmplifierController::connectionStatusChanged,
            this, [this](bool connected) {
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_connected = connected;
                }
                emit connectionStatusChanged(connected);
            });

    QObject::connect(m_amplifier, &IAmplifierController::stateUpdated,
            this, [this](const AmplifierState& state) {
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_lastState = state;
                }
                emit stateUpdated(state);
            });

    QObject::connect(m_amplifier, &IAmplifierController::forwardPowerChanged,
            this, &AmplifierController::forwardPowerChanged);

    QObject::connect(m_amplifier, &IAmplifierController::reflectedPowerChanged,
            this, &AmplifierController::reflectedPowerChanged);

    QObject::connect(m_amplifier, &IAmplifierController::swrChanged,
            this, &AmplifierController::swrChanged);

    QObject::connect(m_amplifier, &IAmplifierController::faultDetected,
            this, &AmplifierController::faultDetected);

    QObject::connect(m_amplifier, &IAmplifierController::operatingStatusChanged,
            this, &AmplifierController::operatingStatusChanged);

    QObject::connect(m_amplifier, &IAmplifierController::temperatureChanged,
            this, &AmplifierController::temperatureChanged);

    QObject::connect(m_amplifier, &IAmplifierController::errorOccurred,
            this, &AmplifierController::errorOccurred);

    LOG_DEBUG("AmplifierController", "Amplifier signals connected");
}

void AmplifierController::connectToAmplifier(int amplifierType, const AmplifierConfig& config) {
    LOG_DEBUG("AmplifierController", QString("connectToAmplifier: type=%1 port=%2")
              .arg(amplifierType).arg(config.port));

    // Reset shutdown flag for new connection
    m_shutdownRequested.store(false);

    // Create amplifier (creates device and moves to worker thread)
    createAmplifier(amplifierType, config);

    if (!m_amplifier) {
        return;  // Error already emitted in createAmplifier
    }

    // Connect in worker thread
    QMetaObject::invokeMethod(m_amplifier, [this, config]() {
        if (m_shutdownRequested.load()) {
            LOG_DEBUG("AmplifierController", "Shutdown requested, aborting connection");
            return;
        }

        bool success = m_amplifier->connect(config);
        LOG_DEBUG("AmplifierController", QString("connect() returned %1").arg(success));

        if (!success) {
            emit errorOccurred("Failed to connect to amplifier");
        }
    }, Qt::QueuedConnection);
}

void AmplifierController::disconnectFromAmplifier() {
    LOG_DEBUG("AmplifierController", "disconnectFromAmplifier called");

    m_shutdownRequested.store(true);

    if (!m_amplifier) {
        return;
    }

    // Disconnect in worker thread (non-blocking)
    QMetaObject::invokeMethod(m_amplifier, [this]() {
        if (m_amplifier) {
            m_amplifier->disconnect();
        }
    }, Qt::QueuedConnection);
}

void AmplifierController::setFrequency(freq_t freq) {
    if (!m_amplifier || m_shutdownRequested.load()) return;

    QMetaObject::invokeMethod(m_amplifier, "setFrequency", Qt::QueuedConnection,
                              Q_ARG(freq_t, freq));
}

void AmplifierController::sendRawCommand(const QString& command) {
    if (!m_amplifier || m_shutdownRequested.load()) return;

    QMetaObject::invokeMethod(m_amplifier, "sendRawCommand", Qt::QueuedConnection,
                              Q_ARG(QString, command));
}

void AmplifierController::queryStatus() {
    if (!m_amplifier || m_shutdownRequested.load()) return;

    QMetaObject::invokeMethod(m_amplifier, "queryStatus", Qt::QueuedConnection);
}

} // namespace TR4QT
