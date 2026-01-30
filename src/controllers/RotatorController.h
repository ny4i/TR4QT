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

#ifndef ROTATORCONTROLLER_H
#define ROTATORCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <atomic>
#include <optional>
#include "../rotator/IRotatorController.h"

namespace TR4QT {

/**
 * RotatorController - Thread-safe wrapper for IRotatorController
 *
 * Manages rotator devices in a dedicated worker thread to prevent
 * UI freezing from Hamlib I/O and polling timers.
 *
 * Pattern follows RadioController:
 * - Controller lives on main thread (safe to connect to UI)
 * - Actual rotator device lives on worker thread (all I/O isolated)
 * - Signals cross thread boundaries automatically (Qt handles queued connections)
 *
 * Why this matters:
 * - HamlibRotator polls every 500ms via Hamlib API
 * - On Windows, Hamlib I/O can block the main thread
 * - Moving to worker thread keeps UI responsive
 *
 * Issue #69: Windows UI Freezing
 */
class RotatorController : public QObject {
    Q_OBJECT

public:
    explicit RotatorController(QObject* parent = nullptr);
    ~RotatorController() override;

    // Thread-safe accessors (called from main thread)
    bool isConnected() const;
    RotatorState getCurrentState() const;
    std::optional<int> getAzimuth(int timeoutMs = 1000) const;

public slots:
    // Commands (executed in worker thread via signals)
    void connectToRotator(int rotatorType, const RotatorConfig& config);
    void disconnectFromRotator();
    bool setAzimuth(int degrees);
    bool setElevation(int degrees);
    void stop();

signals:
    // Status signals (emitted from worker thread, safe to connect to UI)
    void connectionStatusChanged(bool connected);
    void azimuthChanged(int degrees);
    void elevationChanged(int degrees);
    void stateUpdated(const RotatorState& state);
    void errorOccurred(const QString& error);

private:
    void createRotator(int rotatorType, const RotatorConfig& config);
    void connectRotatorSignals();

    QThread m_workerThread;
    IRotatorController* m_rotator{nullptr};  // Lives in worker thread
    mutable QMutex m_stateMutex;
    RotatorState m_lastState;
    bool m_connected{false};
    std::atomic<bool> m_shutdownRequested{false};
};

} // namespace TR4QT

#endif // ROTATORCONTROLLER_H
