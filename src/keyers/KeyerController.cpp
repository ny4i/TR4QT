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

#include "KeyerController.h"
#include "ICWKeyerDevice.h"
#include "KeyerFactory.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>

namespace TR4QT {

KeyerController::KeyerController(QObject* parent)
    : QObject(parent)
{
    m_workerThread.setObjectName("KeyerWorker");
    m_workerThread.start();

    m_reconnectTimer.setSingleShot(false);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &KeyerController::attemptReconnect);

    LOG_DEBUG("KeyerController", "Worker thread started");
}

KeyerController::~KeyerController() {
    LOG_DEBUG("KeyerController", "Destructor: cleaning up");
    stopReconnectTimer();
    cleanupDevice();

    m_workerThread.quit();
    if (!m_workerThread.wait(2000)) {
        LOG_WARN("KeyerController", "Worker thread didn't exit cleanly, terminating");
        m_workerThread.terminate();
        m_workerThread.wait(1000);
    }
}

bool KeyerController::isConnected() const {
    QMutexLocker locker(&m_mutex);
    return m_connected;
}

KeyerDeviceType KeyerController::connectedDeviceType() const {
    QMutexLocker locker(&m_mutex);
    return m_deviceType;
}

bool KeyerController::canSendText() const {
    QMutexLocker locker(&m_mutex);
    return m_connected && m_canSendText;
}

bool KeyerController::hasPaddleInput() const {
    QMutexLocker locker(&m_mutex);
    return m_connected && m_hasPaddleInput;
}

void KeyerController::connectKeyer(const KeyerConfig& config) {
    LOG_INFO("KeyerController", QString("Connecting keyer: type=%1 port=%2")
             .arg(KeyerFactory::deviceTypeName(config.type))
             .arg(config.portName));

    stopReconnectTimer();
    m_userDisconnected = false;
    m_lastConfig = config;

    // Clean up any existing device
    cleanupDevice();

    // Create device via factory
    ICWKeyerDevice* device = KeyerFactory::createKeyer(config.type);
    if (!device) {
        emit errorOccurred("Failed to create keyer device");
        return;
    }

    // Move device to worker thread
    device->moveToThread(&m_workerThread);

    // Connect device signals (cross-thread, Qt handles automatically)
    connect(device, &ICWKeyerDevice::connected, this, [this]() {
        {
            QMutexLocker locker(&m_mutex);
            m_connected = true;
        }
        LOG_INFO("KeyerController", "Keyer connected");
        emit connectionStatusChanged(true);
    });

    connect(device, &ICWKeyerDevice::disconnected, this, [this]() {
        {
            QMutexLocker locker(&m_mutex);
            m_connected = false;
        }
        LOG_INFO("KeyerController", "Keyer disconnected");
        emit connectionStatusChanged(false);

        // Auto-reconnect if this wasn't a user-initiated disconnect
        if (!m_userDisconnected) {
            startReconnectTimer();
        }
    });

    connect(device, &ICWKeyerDevice::errorOccurred,
            this, &KeyerController::errorOccurred);

    connect(device, &ICWKeyerDevice::paddleStateChanged,
            this, &KeyerController::paddleStateChanged);

    connect(device, &ICWKeyerDevice::echoText,
            this, &KeyerController::echoText);

    connect(device, &ICWKeyerDevice::wpmChanged,
            this, &KeyerController::wpmChanged);

    // Store device reference and capabilities
    {
        QMutexLocker locker(&m_mutex);
        m_device = device;
        m_deviceType = config.type;
        m_canSendText = false;     // Will be set after open() succeeds
        m_hasPaddleInput = false;
    }

    // Clean up device when worker thread finishes
    connect(&m_workerThread, &QThread::finished, device, &QObject::deleteLater);

    // Open device on worker thread
    QMetaObject::invokeMethod(device, [this, device, config]() {
        bool success = device->open(config);
        if (success) {
            QMutexLocker locker(&m_mutex);
            m_canSendText = device->canSendText();
            m_hasPaddleInput = device->hasPaddleInput();
            LOG_INFO("KeyerController", QString("Device opened: canSendText=%1 hasPaddleInput=%2")
                     .arg(m_canSendText).arg(m_hasPaddleInput));
        } else {
            LOG_WARN("KeyerController", "Failed to open keyer device");
        }
    }, Qt::QueuedConnection);
}

void KeyerController::disconnectKeyer() {
    LOG_INFO("KeyerController", "Disconnecting keyer (user-initiated)");
    m_userDisconnected = true;
    stopReconnectTimer();
    cleanupDevice();
    emit connectionStatusChanged(false);
}

void KeyerController::sendText(const QString& text) {
    QMutexLocker locker(&m_mutex);
    if (!m_connected || !m_canSendText || !m_device) {
        LOG_WARN("KeyerController", "Cannot send text: not connected or device doesn't support text sending");
        return;
    }

    ICWKeyerDevice* device = m_device;
    locker.unlock();

    QMetaObject::invokeMethod(device, [device, text]() {
        device->sendText(text);
    }, Qt::QueuedConnection);
}

void KeyerController::stopSending() {
    QMutexLocker locker(&m_mutex);
    if (!m_device) return;

    ICWKeyerDevice* device = m_device;
    locker.unlock();

    QMetaObject::invokeMethod(device, [device]() {
        device->stopSending();
    }, Qt::QueuedConnection);
}

void KeyerController::setWpm(int wpm) {
    QMutexLocker locker(&m_mutex);
    if (!m_device) return;

    ICWKeyerDevice* device = m_device;
    locker.unlock();

    QMetaObject::invokeMethod(device, [device, wpm]() {
        device->setWpm(wpm);
    }, Qt::QueuedConnection);
}

void KeyerController::setWeighting(int weight) {
    QMutexLocker locker(&m_mutex);
    if (!m_device) return;

    ICWKeyerDevice* device = m_device;
    locker.unlock();

    QMetaObject::invokeMethod(device, [device, weight]() {
        device->setWeighting(weight);
    }, Qt::QueuedConnection);
}

void KeyerController::setLeadInTime(int time) {
    QMutexLocker locker(&m_mutex);
    if (!m_device) return;

    ICWKeyerDevice* device = m_device;
    locker.unlock();

    QMetaObject::invokeMethod(device, [device, time]() {
        device->setLeadInTime(time);
    }, Qt::QueuedConnection);
}

void KeyerController::setTailTime(int time) {
    QMutexLocker locker(&m_mutex);
    if (!m_device) return;

    ICWKeyerDevice* device = m_device;
    locker.unlock();

    QMetaObject::invokeMethod(device, [device, time]() {
        device->setTailTime(time);
    }, Qt::QueuedConnection);
}

void KeyerController::cleanupDevice() {
    QMutexLocker locker(&m_mutex);
    if (!m_device) return;

    ICWKeyerDevice* device = m_device;
    m_device = nullptr;
    m_connected = false;
    m_canSendText = false;
    m_hasPaddleInput = false;
    locker.unlock();

    // Close device on worker thread (blocking to ensure cleanup completes)
    QMetaObject::invokeMethod(device, [device]() {
        device->close();
    }, Qt::BlockingQueuedConnection);

    device->deleteLater();
}

void KeyerController::startReconnectTimer() {
    if (m_reconnectTimer.isActive()) return;
    LOG_INFO("KeyerController", QString("Starting auto-reconnect (every %1ms)")
             .arg(RECONNECT_INTERVAL_MS));
    m_reconnectTimer.start(RECONNECT_INTERVAL_MS);
}

void KeyerController::stopReconnectTimer() {
    if (m_reconnectTimer.isActive()) {
        LOG_INFO("KeyerController", "Stopping auto-reconnect timer");
        m_reconnectTimer.stop();
    }
}

void KeyerController::attemptReconnect() {
    if (m_lastConfig.portName.isEmpty()) {
        LOG_WARN("KeyerController", "No saved config for reconnect");
        stopReconnectTimer();
        return;
    }

    LOG_DEBUG("KeyerController", QString("Reconnect attempt: port=%1").arg(m_lastConfig.portName));

    // connectKeyer stops the timer on entry, so save/restore reconnect state
    KeyerConfig config = m_lastConfig;
    stopReconnectTimer();
    m_userDisconnected = false;

    // Try to connect — if it fails, the disconnected signal won't fire
    // (open() returns false), so we restart the timer manually
    cleanupDevice();

    ICWKeyerDevice* device = KeyerFactory::createKeyer(config.type);
    if (!device) {
        startReconnectTimer();
        return;
    }

    device->moveToThread(&m_workerThread);

    // Wire signals (same as connectKeyer)
    connect(device, &ICWKeyerDevice::connected, this, [this]() {
        {
            QMutexLocker locker(&m_mutex);
            m_connected = true;
        }
        LOG_INFO("KeyerController", "Keyer reconnected");
        emit connectionStatusChanged(true);
    });

    connect(device, &ICWKeyerDevice::disconnected, this, [this]() {
        {
            QMutexLocker locker(&m_mutex);
            m_connected = false;
        }
        LOG_INFO("KeyerController", "Keyer disconnected");
        emit connectionStatusChanged(false);
        if (!m_userDisconnected) {
            startReconnectTimer();
        }
    });

    connect(device, &ICWKeyerDevice::errorOccurred,
            this, &KeyerController::errorOccurred);
    connect(device, &ICWKeyerDevice::paddleStateChanged,
            this, &KeyerController::paddleStateChanged);
    connect(device, &ICWKeyerDevice::echoText,
            this, &KeyerController::echoText);
    connect(device, &ICWKeyerDevice::wpmChanged,
            this, &KeyerController::wpmChanged);

    {
        QMutexLocker locker(&m_mutex);
        m_device = device;
        m_deviceType = config.type;
        m_canSendText = false;
        m_hasPaddleInput = false;
    }

    connect(&m_workerThread, &QThread::finished, device, &QObject::deleteLater);

    m_lastConfig = config;

    QMetaObject::invokeMethod(device, [this, device, config]() {
        bool success = device->open(config);
        if (success) {
            QMutexLocker locker(&m_mutex);
            m_canSendText = device->canSendText();
            m_hasPaddleInput = device->hasPaddleInput();
            LOG_INFO("KeyerController", "Reconnect succeeded");
        } else {
            LOG_DEBUG("KeyerController", "Reconnect failed, will retry");
            // open() failed — device won't emit disconnected, so restart timer from main thread
            QMetaObject::invokeMethod(this, [this]() {
                startReconnectTimer();
            }, Qt::QueuedConnection);
        }
    }, Qt::QueuedConnection);
}

} // namespace TR4QT
