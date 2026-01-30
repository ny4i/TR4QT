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

#include "HamlibRotator.h"
#include "../logging/LogMacros.h"
#include <cstring>
#include <cmath>

namespace TR4QT {

HamlibRotator::HamlibRotator(QObject* parent)
    : IRotatorController(parent)
    , m_pollTimer(new QTimer(this))
{
    QObject::connect(m_pollTimer, &QTimer::timeout, this, &HamlibRotator::pollRotator);
}

HamlibRotator::~HamlibRotator() {
    disconnect();
}

bool HamlibRotator::connect(const RotatorConfig& config) {
    QMutexLocker locker(&m_rotMutex);

    // Disconnect if already connected
    if (m_rot) {
        rot_close(m_rot);
        rot_cleanup(m_rot);
        m_rot = nullptr;
    }

    m_config = config;

    // Determine Hamlib rotator model ID
    // For PSTRotator, we would use direct implementation (not Hamlib)
    // For Hamlib rotators, use the rotatorType from config
    // This is populated from PreferencesDialog rotator model selection
    int hamlibModelId = config.rotatorType;  // Hamlib ROT_MODEL_* constant

    // Initialize rotator
    m_rot = rot_init(hamlibModelId);
    if (!m_rot) {
        LOG_ERROR("HamlibRotator", QString("Failed to initialize rotator (invalid model ID: %1)").arg(hamlibModelId));
        emit errorOccurred(QString("Failed to initialize rotator (invalid model ID: %1)").arg(hamlibModelId));
        return false;
    }

    // Configure port (serial or network)
    if (!config.serialPort.isEmpty()) {
        // Serial connection
        strncpy(m_rot->state.rotport.pathname,
                config.serialPort.toStdString().c_str(),
                HAMLIB_FILPATHLEN - 1);

        // Configure serial parameters
        m_rot->state.rotport.parm.serial.rate = config.baudRate;
        m_rot->state.rotport.parm.serial.data_bits = 8;
        m_rot->state.rotport.parm.serial.stop_bits = 1;
        m_rot->state.rotport.parm.serial.parity = RIG_PARITY_NONE;
        m_rot->state.rotport.parm.serial.handshake = RIG_HANDSHAKE_NONE;
    } else {
        // Network connection (TCP/IP)
        QString networkPath = QString("%1:%2").arg(config.ipAddress).arg(config.port);
        strncpy(m_rot->state.rotport.pathname,
                networkPath.toStdString().c_str(),
                HAMLIB_FILPATHLEN - 1);
    }

    // Set network timeout
    m_rot->state.rotport.timeout = config.responseTimeoutMs;

    // Open connection
    int retcode = rot_open(m_rot);
    if (retcode != RIG_OK) {
        logHamlibError("rot_open", retcode);
        rot_cleanup(m_rot);
        m_rot = nullptr;
        emit connectionStatusChanged(false);
        return false;
    }

    m_connected = true;

    // Get rotator model (mutex already locked, access directly)
    QString model = m_rot->caps->model_name;
    LOG_INFO("HamlibRotator", QString("Connected to rotator: %1").arg(model));

    // Start polling timer (500ms for rotator status monitoring)
    m_pollTimer->start(m_pollIntervalMs);
    LOG_DEBUG("HamlibRotator", QString("Poll timer started with interval %1 ms").arg(m_pollIntervalMs));

    // Reset error counter on successful connection
    m_consecutiveErrors = 0;

    emit connectionStatusChanged(true);
    LOG_DEBUG("HamlibRotator", "Connection successful");

    return true;
}

void HamlibRotator::disconnect() {
    QMutexLocker locker(&m_rotMutex);

    m_pollTimer->stop();

    if (m_rot) {
        rot_close(m_rot);
        rot_cleanup(m_rot);
        m_rot = nullptr;
    }

    m_connected = false;
    m_currentState = RotatorState{};
    emit connectionStatusChanged(false);
}

bool HamlibRotator::isConnected() const {
    return m_connected;
}

RotatorState HamlibRotator::getCurrentState() const {
    QMutexLocker locker(&m_rotMutex);
    return m_currentState;
}

std::optional<int> HamlibRotator::getAzimuth(int timeoutMs) const {
    QMutexLocker locker(&m_rotMutex);
    if (!checkRotPointer("getAzimuth")) return std::nullopt;

    azimuth_t az = 0;
    elevation_t el = 0;
    int retcode = rot_get_position(m_rot, &az, &el);

    if (retcode != RIG_OK) {
        logHamlibError("rot_get_position", retcode);
        return std::nullopt;
    }

    return static_cast<int>(std::round(az));
}

bool HamlibRotator::setAzimuth(int degrees) {
    QMutexLocker locker(&m_rotMutex);
    if (!checkRotPointer("setAzimuth")) return false;

    // Validate azimuth range (0-360 degrees)
    if (degrees < 0 || degrees > 360) {
        LOG_ERROR("HamlibRotator", QString("Invalid azimuth: %1 (must be 0-360)").arg(degrees));
        emit errorOccurred(QString("Invalid azimuth: %1 (must be 0-360)").arg(degrees));
        return false;
    }

    // Set azimuth (leave elevation unchanged at current value)
    azimuth_t az = static_cast<azimuth_t>(degrees);
    elevation_t el = m_currentState.elevation;  // Keep current elevation

    int retcode = rot_set_position(m_rot, az, el);
    if (retcode == RIG_OK) {
        LOG_DEBUG("HamlibRotator", QString("Azimuth set to %1 degrees").arg(degrees));
        return true;
    } else {
        logHamlibError("rot_set_position", retcode);
        return false;
    }
}

bool HamlibRotator::setElevation(int degrees) {
    QMutexLocker locker(&m_rotMutex);
    if (!checkRotPointer("setElevation")) return false;

    // Validate elevation range (-90 to +90 degrees)
    if (degrees < -90 || degrees > 90) {
        LOG_ERROR("HamlibRotator", QString("Invalid elevation: %1 (must be -90 to +90)").arg(degrees));
        emit errorOccurred(QString("Invalid elevation: %1 (must be -90 to +90)").arg(degrees));
        return false;
    }

    // Set elevation (leave azimuth unchanged at current value)
    azimuth_t az = m_currentState.azimuth;  // Keep current azimuth
    elevation_t el = static_cast<elevation_t>(degrees);

    int retcode = rot_set_position(m_rot, az, el);
    if (retcode == RIG_OK) {
        LOG_DEBUG("HamlibRotator", QString("Elevation set to %1 degrees").arg(degrees));
        return true;
    } else {
        logHamlibError("rot_set_position", retcode);
        return false;
    }
}

void HamlibRotator::stop() {
    QMutexLocker locker(&m_rotMutex);
    if (!checkRotPointer("stop")) return;

    int retcode = rot_stop(m_rot);
    if (retcode == RIG_OK) {
        LOG_DEBUG("HamlibRotator", "Rotator stopped");
    } else {
        logHamlibError("rot_stop", retcode);
    }
}

void HamlibRotator::pollRotator() {
    QMutexLocker locker(&m_rotMutex);
    if (!checkRotPointer("pollRotator")) return;

    updateState();
}

void HamlibRotator::updateState() {
    // NOTE: Mutex must already be locked by caller!

    if (!m_rot) return;

    bool stateChanged = false;
    RotatorState newState = m_currentState;
    newState.isConnected = true;
    newState.isValid = true;

    // Query azimuth and elevation
    azimuth_t az = 0;
    elevation_t el = 0;
    int retcode = rot_get_position(m_rot, &az, &el);

    if (retcode == RIG_OK) {
        int azInt = static_cast<int>(std::round(az));
        int elInt = static_cast<int>(std::round(el));

        if (azInt != newState.azimuth) {
            newState.azimuth = azInt;
            stateChanged = true;
            emit azimuthChanged(azInt);
        }

        if (elInt != newState.elevation) {
            newState.elevation = elInt;
            stateChanged = true;
            emit elevationChanged(elInt);
        }

        m_consecutiveErrors = 0;  // Reset error counter on successful query
    } else {
        logHamlibError("rot_get_position", retcode);
        m_consecutiveErrors++;
    }

    // TODO: Query rotator status (moving, etc.) if supported by rotator
    // Some rotators provide status via rot_get_status() or rot_get_info()

    // Update cached state
    if (stateChanged) {
        m_currentState = newState;
        emit stateUpdated(m_currentState);
    }

    // Auto-disconnect on persistent errors
    if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
        LOG_ERROR("HamlibRotator", QString("Too many consecutive errors (%1), disconnecting rotator").arg(m_consecutiveErrors));
        emit errorOccurred(QString("Lost connection to rotator (too many errors)"));
        m_connected = false;
        m_pollTimer->stop();
        emit connectionStatusChanged(false);
    }
}

void HamlibRotator::logHamlibError(const char* operation, int retcode) const {
    QString errorStr = QString::fromUtf8(rigerror(retcode));
    LOG_ERROR("HamlibRotator", QString("%1 failed: %2 (code: %3)")
        .arg(operation)
        .arg(errorStr)
        .arg(retcode));
}

bool HamlibRotator::checkRotPointer(const char* context) const {
    if (!m_rot) {
        LOG_ERROR("HamlibRotator", QString("%1: Rotator not connected").arg(context));
        return false;
    }
    return true;
}

} // namespace TR4QT
