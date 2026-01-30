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

#ifndef HAMLIBROTATOR_H
#define HAMLIBROTATOR_H

#include "IRotatorController.h"
#include <hamlib/rotator.h>
#include <QMutex>
#include <QTimer>

namespace TR4QT {

/**
 * Hamlib Rotator Controller
 *
 * Concrete implementation of IRotatorController using Hamlib rotator library.
 * Supports all rotators through Hamlib, including:
 * - EASYCOMM (ROT_MODEL_EASYCOMM1, ROT_MODEL_EASYCOMM2)
 * - ROTOREZ (ROT_MODEL_ROTOREZ)
 * - GS-232A/B (ROT_MODEL_GS232A, ROT_MODEL_GS232B)
 * - NETROTCTL (ROT_MODEL_NETROTCTL) - Network control via rotctld
 * - Many other rotators supported by Hamlib
 *
 * Threading:
 * - Uses QMutex for thread-safe access to ROT* handle
 * - Polling timer runs in same thread as controller
 * - All Hamlib calls protected by mutex
 */
class HamlibRotator : public IRotatorController {
    Q_OBJECT

public:
    explicit HamlibRotator(QObject* parent = nullptr);
    ~HamlibRotator() override;

public slots:
    // IRotatorController slot overrides (must be in slots section for MOC)
    bool connect(const RotatorConfig& config) override;
    void disconnect() override;
    bool setAzimuth(int degrees) override;
    void stop() override;
    bool setElevation(int degrees) override;

public:
    // Query methods (const, not slots)
    bool isConnected() const override;
    std::optional<int> getAzimuth(int timeoutMs = 1000) const override;
    RotatorState getCurrentState() const override;

private slots:
    void pollRotator();

private:
    // Hamlib error logging helper
    void logHamlibError(const char* operation, int retcode) const;

    // Pointer check helper (for const methods)
    bool checkRotPointer(const char* context) const;

    // Update state from Hamlib queries
    void updateState();

    // Hamlib handle
    ROT* m_rot{nullptr};

    // Thread safety
    mutable QMutex m_rotMutex;

    // Polling timer
    QTimer* m_pollTimer{nullptr};
    int m_pollIntervalMs{500};  // Default: 500ms polling interval (rotators are slower than radios)

    // Connection state
    bool m_connected{false};
    RotatorConfig m_config;

    // Current state (cached from last successful query)
    RotatorState m_currentState;

    // Error tracking (for auto-disconnect on persistent errors)
    int m_consecutiveErrors{0};
    static constexpr int MAX_CONSECUTIVE_ERRORS = 10;
};

} // namespace TR4QT

#endif // HAMLIBROTATOR_H
