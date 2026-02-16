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

#ifndef HALIKEYSERIALDEVICE_H
#define HALIKEYSERIALDEVICE_H

#include "ICWKeyerDevice.h"
#include <QSerialPort>
#include <QTimer>

namespace TR4QT {

/**
 * HaliKey serial paddle input device.
 *
 * Reads paddle state from serial port flow-control signals:
 *   CTS = dit paddle (tip contact)
 *   DSR = dah paddle (ring contact)
 *
 * Polls pinoutSignals() at 1ms interval with 3-read debounce.
 * This device cannot send text — it only provides paddle input
 * for the software IambicKeyer.
 *
 * Based on QK4's HalikeyDevice implementation.
 */
class HaliKeySerialDevice : public ICWKeyerDevice {
    Q_OBJECT

public:
    explicit HaliKeySerialDevice(QObject* parent = nullptr);
    ~HaliKeySerialDevice() override;

    // ICWKeyerDevice interface
    bool open(const KeyerConfig& config) override;
    void close() override;
    bool isConnected() const override;
    KeyerDeviceType deviceType() const override { return KeyerDeviceType::HaliKeySerial; }
    QString deviceName() const override { return "HaliKey (Serial)"; }

    bool canSendText() const override { return false; }
    void sendText(const QString& /*text*/) override {}
    void stopSending() override {}
    void setWpm(int /*wpm*/) override {}

    bool hasPaddleInput() const override { return true; }

    // Available serial ports (static utility)
    static QStringList availablePorts();

private slots:
    void pollSignals();

private:
    QSerialPort m_serial;
    QTimer m_pollTimer;

    // Polling interval — fast enough for responsive keying
    static constexpr int POLL_INTERVAL_MS = 1;

    // Debounce — require stable state for N consecutive reads
    static constexpr int DEBOUNCE_COUNT = 3;  // 3ms at 1ms polling

    // Last known paddle states (debounced, what we report)
    bool m_lastDitState = false;
    bool m_lastDahState = false;

    // Raw states and debounce counters
    bool m_rawDitState = false;
    bool m_rawDahState = false;
    int m_ditDebounceCounter = 0;
    int m_dahDebounceCounter = 0;
};

} // namespace TR4QT

#endif // HALIKEYSERIALDEVICE_H
