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

#include "DtrRtsCWSender.h"
#include "MorseEncoder.h"
#include "../logging/LogMacros.h"
#include <QSerialPort>

namespace TR4QT {

DtrRtsCWSender::DtrRtsCWSender(const Config& config, QObject* parent)
    : CWSender(parent)
    , m_config(config)
    , m_encoder(new MorseEncoder(this))
{
    // Connect encoder signals to our key line control
    connect(m_encoder, &MorseEncoder::keyDown, this, &DtrRtsCWSender::onEncoderKeyDown);
    connect(m_encoder, &MorseEncoder::keyUp, this, &DtrRtsCWSender::onEncoderKeyUp);
    connect(m_encoder, &MorseEncoder::finished, this, &DtrRtsCWSender::onEncoderFinished);
    connect(m_encoder, &MorseEncoder::stopped, this, &DtrRtsCWSender::onEncoderFinished);

    // Open the keying port
    if (!m_config.portName.isEmpty()) {
        openPort();
    }

    LOG_DEBUG("DtrRtsCWSender", QString("Created: pin=%1, port=%2")
              .arg(m_config.pin == Pin::DTR ? "DTR" : "RTS")
              .arg(m_config.portName));
}

DtrRtsCWSender::~DtrRtsCWSender()
{
    stop();
    closePort();
}

CWSender::State DtrRtsCWSender::state() const
{
    return m_state;
}

bool DtrRtsCWSender::isAvailable() const
{
    return m_portOpen;
}

int DtrRtsCWSender::wpm() const
{
    return m_wpm;
}

void DtrRtsCWSender::setWpm(int wpm)
{
    if (m_wpm != wpm) {
        m_wpm = wpm;
        emit wpmChanged(wpm);
    }
}

void DtrRtsCWSender::send(const QString& text)
{
    if (text.isEmpty()) {
        return;
    }

    if (!isAvailable()) {
        emit error("DTR/RTS port not available");
        return;
    }

    // Stop any in-progress sending
    if (m_state == State::Sending) {
        m_encoder->stop();
    }

    m_state = State::Sending;
    emit stateChanged(m_state);
    emit transmissionStarted(text);

    // MorseEncoder converts text to timed keyDown/keyUp events
    m_encoder->send(text, m_wpm);
}

void DtrRtsCWSender::stop()
{
    if (m_state == State::Idle) {
        return;
    }

    m_encoder->stop();
    setKeyLine(false);  // Ensure key is released

    m_state = State::Idle;
    emit stateChanged(m_state);
    emit transmissionStopped();
}

void DtrRtsCWSender::keyDown()
{
    setKeyLine(true);
}

void DtrRtsCWSender::keyUp()
{
    setKeyLine(false);
}

void DtrRtsCWSender::onEncoderKeyDown()
{
    setKeyLine(true);
}

void DtrRtsCWSender::onEncoderKeyUp()
{
    setKeyLine(false);
}

void DtrRtsCWSender::onEncoderFinished()
{
    setKeyLine(false);  // Safety: ensure key released
    m_state = State::Idle;
    emit stateChanged(m_state);
    emit transmissionComplete();
}

void DtrRtsCWSender::setKeyLine(bool active)
{
    if (!m_serialPort || !m_portOpen) return;

    if (m_config.pin == Pin::DTR) {
        m_serialPort->setDataTerminalReady(active);
    } else {
        m_serialPort->setRequestToSend(active);
    }
}

bool DtrRtsCWSender::openPort()
{
    if (m_config.portName.isEmpty()) {
        LOG_WARN("DtrRtsCWSender", "Cannot open port: no port name configured");
        return false;
    }

    closePort();

    m_serialPort = new QSerialPort(m_config.portName, this);

    // We only need DTR/RTS line control, not data transfer.
    // Open with minimal settings - baud rate doesn't matter for line toggling.
    m_serialPort->setBaudRate(QSerialPort::Baud9600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        LOG_ERROR("DtrRtsCWSender", QString("Failed to open port %1: %2")
                  .arg(m_config.portName).arg(m_serialPort->errorString()));
        delete m_serialPort;
        m_serialPort = nullptr;
        return false;
    }

    // Start with key released
    if (m_config.pin == Pin::DTR) {
        m_serialPort->setDataTerminalReady(false);
    } else {
        m_serialPort->setRequestToSend(false);
    }

    m_portOpen = true;
    LOG_INFO("DtrRtsCWSender", QString("Opened port %1 for %2 keying")
             .arg(m_config.portName)
             .arg(m_config.pin == Pin::DTR ? "DTR" : "RTS"));
    return true;
}

void DtrRtsCWSender::closePort()
{
    if (m_serialPort) {
        // Release key before closing
        if (m_portOpen) {
            if (m_config.pin == Pin::DTR) {
                m_serialPort->setDataTerminalReady(false);
            } else {
                m_serialPort->setRequestToSend(false);
            }
        }
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = nullptr;
    }
    m_portOpen = false;
}

} // namespace TR4QT
