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

#include "HaliKeySerialDevice.h"
#include "../logging/LogMacros.h"
#include <QSerialPortInfo>

namespace TR4QT {

HaliKeySerialDevice::HaliKeySerialDevice(QObject* parent)
    : ICWKeyerDevice(parent)
{
    m_pollTimer.setInterval(POLL_INTERVAL_MS);
    connect(&m_pollTimer, &QTimer::timeout, this, &HaliKeySerialDevice::pollSignals);
}

HaliKeySerialDevice::~HaliKeySerialDevice() {
    close();
}

bool HaliKeySerialDevice::open(const KeyerConfig& config) {
    if (m_serial.isOpen()) {
        close();
    }

    m_serial.setPortName(config.portName);
    // Baud rate is irrelevant for HaliKey (only reads flow control signals)
    // but we need to open the port with some valid settings
    m_serial.setBaudRate(QSerialPort::Baud9600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    LOG_INFO("HaliKeySerial", QString("Opening port %1").arg(config.portName));

    if (!m_serial.open(QIODevice::ReadOnly)) {
        QString error = m_serial.errorString();
        LOG_ERROR("HaliKeySerial", QString("Failed to open port: %1").arg(error));
        emit errorOccurred(error);
        return false;
    }

    // Reset paddle states and debounce counters
    m_lastDitState = false;
    m_lastDahState = false;
    m_rawDitState = false;
    m_rawDahState = false;
    m_ditDebounceCounter = 0;
    m_dahDebounceCounter = 0;

    // Start polling flow control signals
    m_pollTimer.start();

    LOG_INFO("HaliKeySerial", "Connected, polling started");
    emit connected();
    return true;
}

void HaliKeySerialDevice::close() {
    m_pollTimer.stop();

    if (m_serial.isOpen()) {
        m_serial.close();
        LOG_INFO("HaliKeySerial", "Port closed");
        emit disconnected();
    }

    m_lastDitState = false;
    m_lastDahState = false;
}

bool HaliKeySerialDevice::isConnected() const {
    return m_serial.isOpen();
}

QStringList HaliKeySerialDevice::availablePorts() {
    QStringList ports;
    const auto portInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : portInfos) {
        ports.append(info.portName());
    }
    return ports;
}

void HaliKeySerialDevice::pollSignals() {
    if (!m_serial.isOpen()) return;

    QSerialPort::PinoutSignals pinState = m_serial.pinoutSignals();

    // Check for read error
    if (pinState == QSerialPort::NoSignal && m_serial.error() != QSerialPort::NoError) {
        QString error = m_serial.errorString();
        LOG_WARN("HaliKeySerial", QString("Error reading signals: %1").arg(error));
        close();
        emit errorOccurred(error);
        return;
    }

    // CTS = dit paddle (tip contact)
    bool ditState = (pinState & QSerialPort::ClearToSendSignal) != 0;
    // DSR = dah paddle (ring contact)
    bool dahState = (pinState & QSerialPort::DataSetReadySignal) != 0;

    // Debounce dit — require stable state for DEBOUNCE_COUNT consecutive reads
    if (ditState == m_rawDitState) {
        if (m_ditDebounceCounter < DEBOUNCE_COUNT) {
            m_ditDebounceCounter++;
        }
        if (m_ditDebounceCounter >= DEBOUNCE_COUNT && ditState != m_lastDitState) {
            m_lastDitState = ditState;
            emit paddleStateChanged(m_lastDitState, m_lastDahState);
        }
    } else {
        m_rawDitState = ditState;
        m_ditDebounceCounter = 1;
    }

    // Debounce dah — same logic
    if (dahState == m_rawDahState) {
        if (m_dahDebounceCounter < DEBOUNCE_COUNT) {
            m_dahDebounceCounter++;
        }
        if (m_dahDebounceCounter >= DEBOUNCE_COUNT && dahState != m_lastDahState) {
            m_lastDahState = dahState;
            emit paddleStateChanged(m_lastDitState, m_lastDahState);
        }
    } else {
        m_rawDahState = dahState;
        m_dahDebounceCounter = 1;
    }
}

} // namespace TR4QT
