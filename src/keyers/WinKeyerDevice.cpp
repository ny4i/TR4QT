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

#include "WinKeyerDevice.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>
#include <QThread>
#include <QCoreApplication>

namespace TR4QT {

WinKeyerDevice::WinKeyerDevice(QObject* parent)
    : ICWKeyerDevice(parent)
{
}

WinKeyerDevice::~WinKeyerDevice() {
    closeInternal();
}

bool WinKeyerDevice::open(const KeyerConfig& config) {
    QMutexLocker locker(&m_commandMutex);

    closeInternal();

    m_defaultWpm = config.defaultWpm;
    m_paddleSwap = config.paddleSwap;
    m_keyerMode = config.winKeyerMode;
    m_potMinWpm = config.potMinWpm;
    m_potRangeWpm = config.potMaxWpm - config.potMinWpm;

    // Configure serial port: 1200 baud, 8N2 (WinKeyer protocol requirement)
    m_serial.setPortName(config.portName);
    m_serial.setBaudRate(1200);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::TwoStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    LOG_INFO("WinKeyer", QString("Opening port %1 at 1200 baud 8N2").arg(config.portName));

    if (!m_serial.open(QIODevice::ReadWrite)) {
        QString error = m_serial.errorString();
        LOG_ERROR("WinKeyer", QString("Failed to open port: %1").arg(error));
        emit errorOccurred(error);
        return false;
    }

    // Set read buffer size to 1 byte (CH340 chip compatibility, see qlog reference)
    m_serial.setReadBufferSize(1);
    m_serial.setDataTerminalReady(true);
    m_serial.setRequestToSend(false);

    QThread::msleep(POST_OPEN_DELAY_MS);

    // Echo test: verify device is a WinKeyer
    LOG_DEBUG("WinKeyer", "Performing echo test");
    QByteArray cmd(3, '\0');
    cmd[0] = 0x00;  // Admin command prefix
    cmd[1] = 0x04;  // Echo test sub-command
    cmd[2] = static_cast<char>(0xF1);  // Test byte

    QByteArray response;
    if (!sendAndReceive(cmd, response) || response.isEmpty()) {
        LOG_ERROR("WinKeyer", "Echo test failed - no response");
        closeInternal();
        return false;
    }

    if (static_cast<unsigned char>(response.at(0)) != 0xF1) {
        LOG_ERROR("WinKeyer", "Echo test failed - device is not a WinKeyer");
        emit errorOccurred("Connected device is not a WinKeyer");
        closeInternal();
        return false;
    }
    LOG_DEBUG("WinKeyer", "Echo test passed");

    // Enable host mode
    LOG_DEBUG("WinKeyer", "Enabling host mode");
    cmd.resize(2);
    cmd[0] = 0x00;
    cmd[1] = 0x02;

    if (!sendAndReceive(cmd, response) || response.isEmpty()) {
        LOG_ERROR("WinKeyer", "Failed to enable host mode");
        closeInternal();
        return false;
    }

    m_version = static_cast<unsigned char>(response.at(0));
    LOG_INFO("WinKeyer", QString("Host mode enabled, version=%1").arg(m_version));

    // Set keyer mode byte
    LOG_DEBUG("WinKeyer", "Setting keyer mode");
    cmd.resize(2);
    cmd[0] = 0x0E;
    cmd[1] = static_cast<char>(buildModeByte());

    if (!sendCommand(cmd)) {
        LOG_ERROR("WinKeyer", "Failed to set keyer mode");
        closeInternal();
        return false;
    }

    // WK2+ push-button mode
    if (m_version >= 20) {
        LOG_DEBUG("WinKeyer", "Setting WK2 push-button mode");
        cmd.resize(2);
        cmd[0] = 0x00;
        cmd[1] = 0x0B;
        sendCommand(cmd);
    }

    QThread::msleep(POST_OPEN_DELAY_MS);

    // Switch to async mode for all further communication
    connect(&m_serial, &QSerialPort::readyRead, this, &WinKeyerDevice::handleReadyRead);
    connect(&m_serial, &QSerialPort::bytesWritten, this, &WinKeyerDevice::handleBytesWritten);
    connect(&m_serial, &QSerialPort::errorOccurred, this, &WinKeyerDevice::handleError);

    m_inHostMode = true;

    // Request current status
    cmd.resize(1);
    cmd[0] = 0x15;
    m_serial.write(cmd);

    // Set POT range: <05><MIN><RANGE><don't care>
    // Full swing: MIN to MIN+RANGE WPM
    cmd.resize(4);
    cmd[0] = 0x05;
    cmd[1] = static_cast<char>(m_potMinWpm);
    cmd[2] = static_cast<char>(m_potRangeWpm);
    cmd[3] = 0x00;  // Don't care per spec, zero recommended
    m_serial.write(cmd);
    LOG_INFO("WinKeyer", QString("POT range: %1-%2 WPM (range=%3)")
             .arg(m_potMinWpm).arg(m_potMinWpm + m_potRangeWpm).arg(m_potRangeWpm));

    // Set default WPM
    setWpm(m_defaultWpm);

    emit connected();
    return true;
}

void WinKeyerDevice::close() {
    QMutexLocker locker(&m_commandMutex);
    closeInternal();
    emit disconnected();
}

bool WinKeyerDevice::isConnected() const {
    return m_inHostMode && m_serial.isOpen();
}

void WinKeyerDevice::sendText(const QString& text) {
    QMutexLocker locker(&m_commandMutex);

    if (!m_inHostMode) {
        LOG_WARN("WinKeyer", "Cannot send text: not in host mode");
        emit errorOccurred("Keyer is not connected");
        return;
    }

    // Append text to write buffer (strip newlines)
    QString cleanText = text;
    cleanText.replace('\n', "");

    {
        QMutexLocker bufLock(&m_writeBufferMutex);
        m_writeBuffer.append(cleanText.toLocal8Bit());
    }

    tryAsyncWrite();
}

void WinKeyerDevice::stopSending() {
    QMutexLocker locker(&m_commandMutex);

    if (!m_inHostMode) return;

    // Clear local buffer
    {
        QMutexLocker bufLock(&m_writeBufferMutex);
        m_writeBuffer.clear();
    }

    // Send stop + clear to WinKeyer
    QByteArray cmd(3, '\0');
    cmd[0] = 0x06;  // Pause/stop
    cmd[1] = 0x01;  // Resume (clears pause state)
    cmd[2] = 0x0A;  // Clear buffer
    m_serial.write(cmd);
}

void WinKeyerDevice::setWpm(int wpm) {
    if (!m_inHostMode) return;

    QByteArray cmd(2, '\0');
    cmd[0] = 0x02;  // Speed command
    cmd[1] = static_cast<char>(wpm);
    m_serial.write(cmd);

    emit wpmChanged(wpm);
}

void WinKeyerDevice::setWeighting(int weight) {
    if (!m_inHostMode) return;
    weight = qBound(10, weight, 90);

    QByteArray cmd(2, '\0');
    cmd[0] = 0x03;  // Weighting command
    cmd[1] = static_cast<char>(weight);
    m_serial.write(cmd);
    LOG_DEBUG("WinKeyer", QString("Set weighting: %1").arg(weight));
}

void WinKeyerDevice::setLeadInTime(int time) {
    if (!m_inHostMode) return;
    time = qBound(0, time, 250);

    QByteArray cmd(2, '\0');
    cmd[0] = 0x04;  // Lead-in time command
    cmd[1] = static_cast<char>(time);
    m_serial.write(cmd);
    LOG_DEBUG("WinKeyer", QString("Set lead-in time: %1 (x10ms)").arg(time));
}

void WinKeyerDevice::setTailTime(int time) {
    if (!m_inHostMode) return;
    time = qBound(0, time, 250);

    QByteArray cmd(2, '\0');
    cmd[0] = 0x07;  // Tail time command (first extension)
    cmd[1] = static_cast<char>(time);
    m_serial.write(cmd);
    LOG_DEBUG("WinKeyer", QString("Set tail time: %1 (x10ms)").arg(time));
}

void WinKeyerDevice::handleReadyRead() {
    // Drain ALL available bytes, but only emit the LAST speed pot value.
    // This prevents flooding the radio with dozens of KS commands when the
    // user turns the speed pot (many intermediate values arrive in one burst).
    int lastPotWpm = -1;

    while (m_serial.bytesAvailable() > 0) {
        unsigned char rcvByte;
        m_serial.read(reinterpret_cast<char*>(&rcvByte), 1);

        LOG_TRACE("WinKeyer", QString("RCV async: 0x%1").arg(rcvByte, 2, 16, QChar('0')));

        if ((rcvByte & 0xC0) == 0xC0) {
            // Status byte
            m_xoff = false;

            if (rcvByte == 0xC0) {
                LOG_TRACE("WinKeyer", "Status: Idle");
            } else if (m_version >= 20 && (rcvByte & 0x08)) {
                // Push-button status (WK2+)
                LOG_TRACE("WinKeyer", "Push-button event");
            } else {
                // Regular status byte
                if (rcvByte & 0x01) {
                    LOG_TRACE("WinKeyer", "Status: Buffer 2/3 full (XOFF)");
                    m_xoff = true;
                }
                if (rcvByte & 0x04) {
                    LOG_TRACE("WinKeyer", "Status: Key busy");
                }
            }
        } else if ((rcvByte & 0xC0) == 0x80) {
            // Speed pot value — accumulate, emit only the last one after draining
            // Pot reports 6-bit value (0-63), scale across configured range
            int potValue = rcvByte & 0x3F;
            lastPotWpm = m_potMinWpm + (potValue * m_potRangeWpm + POT_MAX_VALUE / 2) / POT_MAX_VALUE;
            LOG_DEBUG("WinKeyer", QString("Speed pot: raw=0x%1 potValue=%2 WPM=%3")
                      .arg(rcvByte, 2, 16, QChar('0')).arg(potValue).arg(lastPotWpm));
        } else {
            // Echo character
            LOG_TRACE("WinKeyer", QString("Echo: '%1'").arg(QChar(rcvByte)));
            emit echoText(QString(QChar(rcvByte)));
        }
    }

    // Emit only the final speed pot value (if any were received in this burst)
    if (lastPotWpm >= 0) {
        emit wpmChanged(lastPotWpm);
    }

    tryAsyncWrite();
}

void WinKeyerDevice::handleBytesWritten(qint64 /*bytes*/) {
    tryAsyncWrite();
}

void WinKeyerDevice::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) return;

    QString detail = m_serial.errorString();
    LOG_ERROR("WinKeyer", QString("Serial error %1: %2").arg(error).arg(detail));
    emit errorOccurred(detail);

    // Fatal errors (USB unplug, device removed) — tear down the connection
    if (error == QSerialPort::ResourceError) {
        LOG_WARN("WinKeyer", "Fatal serial error (device removed?) — closing connection");
        closeInternal();
        emit disconnected();
    }
}

void WinKeyerDevice::tryAsyncWrite() {
    QMutexLocker bufLock(&m_writeBufferMutex);

    if (m_writeBuffer.isEmpty() || m_xoff) return;

    // Send one byte at a time (WinKeyer protocol)
    qint64 written = m_serial.write(m_writeBuffer.constData(), 1);
    if (written == 1) {
        m_writeBuffer.remove(0, 1);
    } else {
        LOG_WARN("WinKeyer", "Failed to write byte to serial port");
    }
}

unsigned char WinKeyerDevice::buildModeByte() const {
    /*
       Bit 7: Disable paddle watchdog (1=disabled)
       Bit 6: Paddle echoback (0=disabled)
       Bits 5,4: Key mode: 00=IambicB, 01=IambicA, 10=Ultimatic, 11=Bug
       Bit 3: Paddle swap (1=swap)
       Bit 2: Serial echoback (1=enabled)
       Bit 1: Autospace (0=disabled)
       Bit 0: CT spacing (0=normal wordspace)
    */
    unsigned char mode = 0;

    mode |= (1 << 7);  // Disable paddle watchdog

    // Key mode (bits 5,4)
    switch (m_keyerMode) {
        case 0:  // Single paddle (Bug mode)
            mode |= (1 << 5) | (1 << 4);
            break;
        case 1:  // Iambic A
            mode |= (1 << 4);
            break;
        case 2:  // Iambic B (default, bits 00)
            break;
        case 3:  // Ultimatic
            mode |= (1 << 5);
            break;
    }

    if (m_paddleSwap) {
        mode |= (1 << 3);
    }

    mode |= (1 << 2);  // Serial echoback enabled (required)

    return mode;
}

bool WinKeyerDevice::sendCommand(const QByteArray& cmd) {
    qint64 written = m_serial.write(cmd);
    if (written != cmd.size()) {
        LOG_ERROR("WinKeyer", QString("Write failed: expected %1 bytes, wrote %2")
                  .arg(cmd.size()).arg(written));
        return false;
    }
    m_serial.waitForBytesWritten(SERIAL_TIMEOUT_MS);
    return true;
}

bool WinKeyerDevice::sendAndReceive(const QByteArray& cmd, QByteArray& response) {
    if (!sendCommand(cmd)) return false;

    if (!m_serial.waitForReadyRead(SERIAL_TIMEOUT_MS)) {
        LOG_ERROR("WinKeyer", "Timeout waiting for response");
        return false;
    }

    response = m_serial.readAll();
    return !response.isEmpty();
}

void WinKeyerDevice::closeInternal() {
    // Clear write buffer
    {
        QMutexLocker bufLock(&m_writeBufferMutex);
        m_writeBuffer.clear();
    }

    if (!m_serial.isOpen()) return;

    // Disconnect async signals
    disconnect(&m_serial, &QSerialPort::readyRead, this, &WinKeyerDevice::handleReadyRead);
    disconnect(&m_serial, &QSerialPort::bytesWritten, this, &WinKeyerDevice::handleBytesWritten);
    disconnect(&m_serial, &QSerialPort::errorOccurred, this, &WinKeyerDevice::handleError);

    if (m_inHostMode) {
        // Clear buffer
        QByteArray cmd(3, '\0');
        cmd[0] = 0x06;
        cmd[1] = 0x01;
        cmd[2] = 0x0A;
        sendCommand(cmd);

        // Disable host mode
        cmd.resize(2);
        cmd[0] = 0x00;
        cmd[1] = 0x03;
        sendCommand(cmd);

        LOG_INFO("WinKeyer", "Host mode disabled");
    }

    QThread::msleep(POST_OPEN_DELAY_MS);
    m_serial.setDataTerminalReady(false);
    m_serial.close();

    m_inHostMode = false;
    m_xoff = false;
    m_version = 0;
}

} // namespace TR4QT
