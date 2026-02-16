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

#include "HaliKeyMidiDevice.h"
#include "../logging/LogMacros.h"
#include "../3rdparty/rtmidi/RtMidi.h"

namespace TR4QT {

HaliKeyMidiDevice::HaliKeyMidiDevice(QObject* parent)
    : ICWKeyerDevice(parent)
{
}

HaliKeyMidiDevice::~HaliKeyMidiDevice() {
    close();
}

bool HaliKeyMidiDevice::open(const KeyerConfig& config) {
    close();

    m_ditNote = config.ditNoteNumber;
    m_dahNote = config.dahNoteNumber;

    LOG_INFO("HaliKeyMidi", QString("Opening MIDI device: %1 (dit=note %2, dah=note %3)")
             .arg(config.portName).arg(m_ditNote).arg(m_dahNote));

    try {
        m_midiIn = new RtMidiIn();
    } catch (RtMidiError& error) {
        LOG_ERROR("HaliKeyMidi", QString("Failed to create RtMidiIn: %1")
                  .arg(QString::fromStdString(error.getMessage())));
        emit errorOccurred(QString::fromStdString(error.getMessage()));
        return false;
    }

    // Find the requested port by name
    unsigned int portCount = m_midiIn->getPortCount();
    int targetPort = -1;

    for (unsigned int i = 0; i < portCount; i++) {
        QString portName = QString::fromStdString(m_midiIn->getPortName(i));
        LOG_DEBUG("HaliKeyMidi", QString("MIDI port %1: %2").arg(i).arg(portName));
        if (portName.contains(config.portName, Qt::CaseInsensitive)) {
            targetPort = static_cast<int>(i);
            break;
        }
    }

    if (targetPort < 0) {
        QString error = QString("MIDI device '%1' not found (%2 ports available)")
                        .arg(config.portName).arg(portCount);
        LOG_ERROR("HaliKeyMidi", error);
        emit errorOccurred(error);
        delete m_midiIn;
        m_midiIn = nullptr;
        return false;
    }

    try {
        // Set callback before opening port
        m_midiIn->setCallback(&HaliKeyMidiDevice::midiCallback, this);

        // Don't ignore any message types (we need Note On/Off)
        m_midiIn->ignoreTypes(true, true, true);  // Ignore sysex, timing, active sensing

        m_midiIn->openPort(static_cast<unsigned int>(targetPort));
    } catch (RtMidiError& error) {
        LOG_ERROR("HaliKeyMidi", QString("Failed to open MIDI port: %1")
                  .arg(QString::fromStdString(error.getMessage())));
        emit errorOccurred(QString::fromStdString(error.getMessage()));
        delete m_midiIn;
        m_midiIn = nullptr;
        return false;
    }

    m_connected = true;
    m_ditState = false;
    m_dahState = false;

    LOG_INFO("HaliKeyMidi", QString("Connected to MIDI port %1").arg(targetPort));
    emit connected();
    return true;
}

void HaliKeyMidiDevice::close() {
    if (m_midiIn) {
        try {
            m_midiIn->closePort();
        } catch (RtMidiError& error) {
            LOG_WARN("HaliKeyMidi", QString("Error closing MIDI port: %1")
                     .arg(QString::fromStdString(error.getMessage())));
        }
        delete m_midiIn;
        m_midiIn = nullptr;
    }

    if (m_connected) {
        m_connected = false;
        m_ditState = false;
        m_dahState = false;
        emit disconnected();
    }
}

bool HaliKeyMidiDevice::isConnected() const {
    return m_connected && m_midiIn != nullptr;
}

QStringList HaliKeyMidiDevice::availableMidiInputs() {
    QStringList devices;

    try {
        RtMidiIn midiIn;
        unsigned int portCount = midiIn.getPortCount();
        for (unsigned int i = 0; i < portCount; i++) {
            devices.append(QString::fromStdString(midiIn.getPortName(i)));
        }
    } catch (RtMidiError& error) {
        // Silently return empty list if MIDI subsystem not available
        Q_UNUSED(error);
    }

    return devices;
}

void HaliKeyMidiDevice::midiCallback(double deltaTime,
                                      std::vector<unsigned char>* message,
                                      void* userData) {
    if (!message || message->empty() || !userData) return;

    auto* device = static_cast<HaliKeyMidiDevice*>(userData);
    device->handleMidiMessage(*message, deltaTime);
}

void HaliKeyMidiDevice::handleMidiMessage(const std::vector<unsigned char>& message, double deltaTime) {
    if (message.size() < 2) return;

    unsigned char statusByte = message[0];
    unsigned char messageType = statusByte & 0xF0;

    // Only process Note On and Note Off
    if (messageType != NOTE_ON && messageType != NOTE_OFF) return;

    unsigned char note = message[1];
    unsigned char velocity = (message.size() >= 3) ? message[2] : 0;

    // Note On with velocity 0 is treated as Note Off (MIDI convention)
    bool isOn = (messageType == NOTE_ON && velocity > 0);

    // High-resolution timing: RtMidi deltaTime is seconds since last MIDI message
    if (!m_timingStarted) {
        m_timingTimer.start();
        m_timingStarted = true;
    }
    const qint64 elapsedUs = m_timingTimer.nsecsElapsed() / 1000;
    LOG_INFO("TIMING:MIDI", QString("note=%1 %2 deltaMs=%3 elapsed=%4us")
             .arg(note).arg(isOn ? "ON" : "OFF")
             .arg(deltaTime * 1000.0, 0, 'f', 3)
             .arg(elapsedUs));

    handleNoteEvent(static_cast<int>(note), isOn);
}

void HaliKeyMidiDevice::handleNoteEvent(int noteNumber, bool isOn) {
    bool stateChanged = false;

    if (noteNumber == m_ditNote) {
        if (m_ditState != isOn) {
            m_ditState = isOn;
            stateChanged = true;
            LOG_TRACE("HaliKeyMidi", QString("Dit paddle -> %1").arg(isOn ? "pressed" : "released"));
        }
    } else if (noteNumber == m_dahNote) {
        if (m_dahState != isOn) {
            m_dahState = isOn;
            stateChanged = true;
            LOG_TRACE("HaliKeyMidi", QString("Dah paddle -> %1").arg(isOn ? "pressed" : "released"));
        }
    }

    if (stateChanged) {
        // Emit from MIDI callback thread — Qt signal/slot handles cross-thread delivery
        emit paddleStateChanged(m_ditState, m_dahState);
    }
}

} // namespace TR4QT
