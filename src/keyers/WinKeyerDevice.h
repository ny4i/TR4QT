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

#ifndef WINKEYERDEVICE_H
#define WINKEYERDEVICE_H

#include "ICWKeyerDevice.h"
#include <QSerialPort>
#include <QMutex>

namespace TR4QT {

/**
 * K1EL WinKeyer serial protocol implementation.
 *
 * WinKeyer generates Morse code in hardware from text sent over serial.
 * Protocol: 1200 baud, 8N2.
 *
 * Features:
 * - Echo test to verify device identity
 * - Host mode enable/disable
 * - Async text sending with XOFF flow control
 * - Speed pot tracking (WPM changes from hardware knob)
 * - Serial echo-back of sent characters
 * - Immediate stop/clear
 *
 * Based on qlog's CWWinKey implementation and K1EL WinKey specification.
 */
class WinKeyerDevice : public ICWKeyerDevice {
    Q_OBJECT

public:
    explicit WinKeyerDevice(QObject* parent = nullptr);
    ~WinKeyerDevice() override;

    // ICWKeyerDevice interface
    bool open(const KeyerConfig& config) override;
    void close() override;
    bool isConnected() const override;
    KeyerDeviceType deviceType() const override { return KeyerDeviceType::WinKeyer; }
    QString deviceName() const override { return "WinKeyer"; }

    bool canSendText() const override { return true; }
    void sendText(const QString& text) override;
    void stopSending() override;
    void setWpm(int wpm) override;

    bool hasPaddleInput() const override { return false; }

    // Extended WinKeyer commands
    void setWeighting(int weight) override;
    void setLeadInTime(int time) override;
    void setTailTime(int time) override;

private slots:
    void handleReadyRead();
    void handleBytesWritten(qint64 bytes);
    void handleError(QSerialPort::SerialPortError error);

private:
    void tryAsyncWrite();
    unsigned char buildModeByte() const;
    bool sendCommand(const QByteArray& cmd);
    bool sendAndReceive(const QByteArray& cmd, QByteArray& response);
    void closeInternal();

    QSerialPort m_serial;
    QMutex m_commandMutex;
    QMutex m_writeBufferMutex;

    QByteArray m_writeBuffer;
    bool m_inHostMode = false;
    bool m_xoff = false;
    unsigned char m_version = 0;
    int m_potMinWpm = 10;
    int m_potRangeWpm = 35;    ///< potMaxWpm - potMinWpm
    int m_defaultWpm = 25;
    bool m_paddleSwap = false;
    int m_keyerMode = 2;  // IambicB default

    // Serial I/O timeout (ms)
    static constexpr int SERIAL_TIMEOUT_MS = 5000;
    // Delay after opening port (ms)
    static constexpr int POST_OPEN_DELAY_MS = 200;
    // WinKeyer speed pot status byte has 6-bit value (0-63)
    static constexpr int POT_MAX_VALUE = 63;
};

} // namespace TR4QT

#endif // WINKEYERDEVICE_H
