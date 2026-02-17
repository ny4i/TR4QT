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

class QSerialPort;

namespace TR4QT {

class MorseEncoder;

/**
 * CW sender implementation using DTR/RTS serial port line toggling.
 *
 * Uses a SEPARATE serial port (not the radio's CAT port). Owns a QSerialPort
 * and toggles DTR or RTS directly. Connect a USB-serial adapter to the radio's
 * key jack.
 *
 * Hamlib does NOT support DTR/RTS CW keying, so the radio's CAT port cannot
 * be shared for this purpose. A dedicated serial port is always required.
 *
 * For text-based CW (F-key messages), uses MorseEncoder to convert text
 * into timed key events. For paddle keying (IambicKeyer), provides
 * direct keyDown()/keyUp() slots.
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

private slots:
    void onEncoderKeyDown();
    void onEncoderKeyUp();
    void onEncoderFinished();

private:
    void setKeyLine(bool active);
    bool openPort();
    void closePort();

    Config m_config;
    MorseEncoder* m_encoder;              // Owned, for text-to-morse conversion
    QSerialPort* m_serialPort = nullptr;  // Owned, separate keying port
    State m_state = State::Idle;
    int m_wpm = 25;
    bool m_portOpen = false;
};

} // namespace TR4QT

#endif // DTRRTS_CWSENDER_H
