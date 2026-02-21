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

#ifndef KEYERCONTROLLER_H
#define KEYERCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include "KeyerConfig.h"

namespace TR4QT {

class ICWKeyerDevice;

/**
 * Thread-safe controller for CW keyer hardware.
 *
 * Follows the RadioController pattern: owns a QThread and moves the
 * ICWKeyerDevice to the worker thread. All serial/MIDI I/O runs on the
 * worker thread, keeping the UI responsive.
 *
 * Signals from the device (paddleStateChanged, echoText, wpmChanged)
 * are forwarded to the main thread via Qt's cross-thread signal/slot mechanism.
 */
class KeyerController : public QObject {
    Q_OBJECT

public:
    explicit KeyerController(QObject* parent = nullptr);
    ~KeyerController() override;

    bool isConnected() const;
    KeyerDeviceType connectedDeviceType() const;

    // Query capabilities
    bool canSendText() const;
    bool hasPaddleInput() const;

public slots:
    /**
     * Connect to a keyer device with the given configuration.
     * Creates the device via KeyerFactory and moves it to the worker thread.
     */
    void connectKeyer(const KeyerConfig& config);

    /**
     * Disconnect from the current keyer device.
     */
    void disconnectKeyer();

    /**
     * Send text via the keyer (only works for devices that canSendText, e.g. WinKeyer).
     */
    void sendText(const QString& text);

    /**
     * Stop any in-progress text sending.
     */
    void stopSending();

    /**
     * Set CW speed in WPM.
     */
    void setWpm(int wpm);

    // Extended WinKeyer commands (forwarded to device)
    void setWeighting(int weight);
    void setLeadInTime(int time);
    void setTailTime(int time);

signals:
    // Connection status
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& error);

    // Paddle state (forwarded from HaliKey devices)
    void paddleStateChanged(bool dit, bool dah);

    // WinKeyer echo-back
    void echoText(const QString& text);

    // WinKeyer speed pot change
    void wpmChanged(int wpm);

    // WinKeyer finished sending (busy→idle transition, paddle break-in)
    void keyerIdle();

private:
    void cleanupDevice();
    void attemptReconnect();
    void startReconnectTimer();
    void stopReconnectTimer();

    QThread m_workerThread;
    ICWKeyerDevice* m_device = nullptr;
    mutable QMutex m_mutex;
    bool m_connected = false;
    KeyerDeviceType m_deviceType = KeyerDeviceType::WinKeyer;
    bool m_canSendText = false;
    bool m_hasPaddleInput = false;

    // Auto-reconnect state
    QTimer m_reconnectTimer;
    KeyerConfig m_lastConfig;           ///< Config to use for reconnect attempts
    bool m_userDisconnected = false;    ///< true when user explicitly called disconnectKeyer()

    static constexpr int RECONNECT_INTERVAL_MS = 3000;  ///< Retry every 3 seconds
};

} // namespace TR4QT

#endif // KEYERCONTROLLER_H
