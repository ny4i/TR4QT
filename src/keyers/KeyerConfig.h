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

#ifndef KEYERCONFIG_H
#define KEYERCONFIG_H

#include <QString>

namespace TR4QT {

/**
 * Keyer device type enumeration
 */
enum class KeyerDeviceType {
    WinKeyer,        // K1EL WinKeyer (serial protocol, hardware Morse generation)
    HaliKeySerial,   // HaliKey v1 paddle input via serial CTS/DSR signals
    HaliKeyMidi      // HaliKey MIDI paddle input via Note On/Off messages
};

/**
 * Iambic keyer mode
 */
enum class IambicMode {
    IambicA,    // Release stops sending immediately
    IambicB     // Squeeze release completes one more alternate element
};

/**
 * Configuration for CW keyer hardware
 */
struct KeyerConfig {
    KeyerDeviceType type = KeyerDeviceType::WinKeyer;
    QString portName;           // Serial port or MIDI device name
    int baudRate = 1200;        // WinKeyer: 1200 baud, HaliKey serial: 9600 (irrelevant)
    int defaultWpm = 25;
    bool paddleSwap = false;

    // Iambic keyer settings
    IambicMode iambicMode = IambicMode::IambicB;

    // WinKeyer-specific
    int winKeyerMode = 2;       // 0=Single, 1=IambicA, 2=IambicB, 3=Ultimatic
    int weighting = 50;         // WinKeyer: 10-90, 50=normal
    int leadInTime = 0;         // WinKeyer: 0-250 x10ms
    int tailTime = 0;           // WinKeyer: 0-250 x10ms

    // MIDI-specific (default note assignments from NetKeyer/HaliKey)
    int ditNoteNumber = 20;     // MIDI note for dit paddle (left)
    int dahNoteNumber = 21;     // MIDI note for dah paddle (right)
};

} // namespace TR4QT

#endif // KEYERCONFIG_H
