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

#ifndef HALIKEYMIDIDEVICE_H
#define HALIKEYMIDIDEVICE_H

#include "ICWKeyerDevice.h"
#include "../3rdparty/rtmidi/RtMidi.h"
#include <QStringList>
#include <QElapsedTimer>

namespace TR4QT {

/**
 * HaliKey MIDI paddle input device.
 *
 * Reads paddle state from MIDI Note On/Off messages via RtMidi.
 * Default note mapping (from NetKeyer/HaliKey):
 *   Note 20 = dit paddle (left)
 *   Note 21 = dah paddle (right)
 *
 * This device cannot send text — it only provides paddle input
 * for the software IambicKeyer.
 *
 * Based on NetKeyer's MidiPaddleInput implementation.
 */
class HaliKeyMidiDevice : public ICWKeyerDevice {
    Q_OBJECT

public:
    explicit HaliKeyMidiDevice(QObject* parent = nullptr);
    ~HaliKeyMidiDevice() override;

    // ICWKeyerDevice interface
    bool open(const KeyerConfig& config) override;
    void close() override;
    bool isConnected() const override;
    KeyerDeviceType deviceType() const override { return KeyerDeviceType::HaliKeyMidi; }
    QString deviceName() const override { return "HaliKey (MIDI)"; }

    bool canSendText() const override { return false; }
    void sendText(const QString& /*text*/) override {}
    void stopSending() override {}
    void setWpm(int /*wpm*/) override {}

    bool hasPaddleInput() const override { return true; }

    // Available MIDI input devices (static utility)
    static QStringList availableMidiInputs();

private:
    // RtMidi callback (static, called from MIDI thread)
    static void midiCallback(double deltaTime, std::vector<unsigned char>* message, void* userData);
    void handleMidiMessage(const std::vector<unsigned char>& message, double deltaTime);
    void handleNoteEvent(int noteNumber, bool isOn);

    RtMidiIn* m_midiIn = nullptr;
    bool m_connected = false;

    // Configurable note-to-paddle mapping
    int m_ditNote = 20;
    int m_dahNote = 21;

    // Current paddle states
    bool m_ditState = false;
    bool m_dahState = false;

    // Timing diagnostics
    QElapsedTimer m_timingTimer;
    bool m_timingStarted = false;

    // MIDI message constants
    static constexpr unsigned char NOTE_ON = 0x90;
    static constexpr unsigned char NOTE_OFF = 0x80;
};

} // namespace TR4QT

#endif // HALIKEYMIDIDEVICE_H
