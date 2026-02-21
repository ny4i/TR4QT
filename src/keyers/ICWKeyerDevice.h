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

#ifndef ICWKEYERDEVICE_H
#define ICWKEYERDEVICE_H

#include <QObject>
#include <QString>
#include "KeyerConfig.h"

namespace TR4QT {

/**
 * Abstract interface for all CW keyer hardware devices.
 *
 * Implementations:
 * - WinKeyerDevice: K1EL WinKeyer serial protocol (sends Morse in hardware)
 * - HaliKeySerialDevice: HaliKey paddle input via serial CTS/DSR signals
 * - HaliKeyMidiDevice: HaliKey paddle input via MIDI Note On/Off
 *
 * Devices that support text sending (WinKeyer) generate Morse in hardware.
 * Devices that provide paddle input (HaliKey) emit paddleStateChanged signals
 * which feed into the software IambicKeyer for timing and CAT key-down/key-up.
 */
class ICWKeyerDevice : public QObject {
    Q_OBJECT

public:
    explicit ICWKeyerDevice(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ICWKeyerDevice() = default;

    // Connection management
    virtual bool open(const KeyerConfig& config) = 0;
    virtual void close() = 0;
    virtual bool isConnected() const = 0;
    virtual KeyerDeviceType deviceType() const = 0;
    virtual QString deviceName() const = 0;

    // Text sending capability (WinKeyer handles Morse generation in hardware)
    virtual bool canSendText() const = 0;
    virtual void sendText(const QString& text) = 0;
    virtual void stopSending() = 0;
    virtual void setWpm(int wpm) = 0;

    // Paddle input capability (HaliKey provides raw paddle state)
    virtual bool hasPaddleInput() const = 0;

    // Extended WinKeyer commands (no-op defaults for non-WinKeyer devices)
    virtual void setWeighting(int /*weight*/) {}      // 10-90, 50=normal
    virtual void setLeadInTime(int /*time*/) {}       // 0-250, x10ms
    virtual void setTailTime(int /*time*/) {}         // 0-250, x10ms

signals:
    // Connection status
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

    // Paddle state (emitted by HaliKey devices)
    void paddleStateChanged(bool dit, bool dah);

    // WinKeyer echo-back (each character as it's sent)
    void echoText(const QString& text);

    // WinKeyer speed pot change
    void wpmChanged(int wpm);

    // WinKeyer transitioned from busy to idle (finished sending or paddle break-in)
    void keyerIdle();
};

} // namespace TR4QT

#endif // ICWKEYERDEVICE_H
