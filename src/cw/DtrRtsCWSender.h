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

#ifndef DTRRTS_CWSENDER_H
#define DTRRTS_CWSENDER_H

#include "CWSender.h"
#include <QThread>

namespace TR4QT {

class MorseEncoder;

// Internal worker that owns MorseEncoder + serial port handle in a dedicated thread.
// Uses native API (CreateFile/EscapeCommFunction on Windows) to avoid QSerialPort's
// DTR glitch on open/close.
class DtrRtsWorker : public QObject {
    Q_OBJECT
public:
    enum class Pin { DTR = 0, RTS = 1 };

    explicit DtrRtsWorker(const QString& portName, Pin pin);
    ~DtrRtsWorker() override;

    bool isPortOpen() const { return m_portOpen; }

public slots:
    void init();  // Must be called AFTER moveToThread (creates timers on correct thread)
    void doSend(const QString& text, int wpm);
    void doStop();
    void doKeyDown();
    void doKeyUp();

signals:
    void initialized(bool portOpen);
    void sendingStarted(const QString& text);
    void sendingComplete();
    void sendingStopped();
    void portError(const QString& error);

private slots:
    void onEncoderKeyDown();
    void onEncoderKeyUp();
    void onEncoderFinished();

private:
    void setKeyLine(bool active);
    bool openPort();
    void closePort();

    QString m_portName;
    Pin m_pin;
    MorseEncoder* m_encoder = nullptr;
    void* m_portHandle = nullptr;  // Native serial port handle (HANDLE on Windows)
    bool m_portOpen = false;
};

/**
 * CW sender implementation using DTR/RTS serial port line toggling.
 *
 * Uses a SEPARATE serial port (not the radio's CAT port). Connect a USB-serial
 * adapter to the radio's key jack.
 *
 * On Windows, uses native CreateFile/EscapeCommFunction to open the port
 * WITHOUT raising DTR (avoids QSerialPort's DTR glitch on open/close).
 * On other platforms, falls back to QSerialPort.
 *
 * Future: for K4 Direct over serial (shared CAT + keying port), the worker
 * can borrow an already-open QSerialPort from the radio controller instead
 * of opening its own.
 *
 * All timing-critical operations run in a dedicated high-priority worker
 * thread for jitter-free CW timing.
 *
 * Pin selection (DTR or RTS) is configurable.
 */
class DtrRtsCWSender : public CWSender {
    Q_OBJECT

public:
    enum class Pin { DTR = 0, RTS = 1 };
    Q_ENUM(Pin)

    /**
     * Configuration for the DTR/RTS sender.
     */
    struct Config {
        QString portName;              // Serial port name (e.g., "/dev/ttyUSB0", "COM3")
        Pin pin = Pin::DTR;            // Which serial line to toggle for keying
    };

    explicit DtrRtsCWSender(const Config& config, QObject* parent = nullptr);
    ~DtrRtsCWSender() override;

    // CWSender interface
    State state() const override;
    bool isAvailable() const override;
    QString backendName() const override { return "DTR/RTS"; }
    int wpm() const override;
    void setWpm(int wpm) override;

public slots:
    void send(const QString& text) override;
    void stop() override;

    /**
     * Direct key control for paddle keying (IambicKeyer).
     * Toggles DTR/RTS immediately without MorseEncoder.
     */
    void keyDown();
    void keyUp();

signals:
    // Internal signals to dispatch to worker thread
    void requestSend(const QString& text, int wpm);
    void requestStop();
    void requestKeyDown();
    void requestKeyUp();

private slots:
    void onWorkerSendingStarted(const QString& text);
    void onWorkerSendingComplete();
    void onWorkerSendingStopped();
    void onWorkerPortError(const QString& error);

private:
    QThread m_workerThread;
    DtrRtsWorker* m_worker = nullptr;
    State m_state = State::Idle;
    int m_wpm = 25;
    bool m_portOpen = false;
};

} // namespace TR4QT

#endif // DTRRTS_CWSENDER_H
