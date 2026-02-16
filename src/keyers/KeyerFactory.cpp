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

#include "KeyerFactory.h"
#include "ICWKeyerDevice.h"
#include "WinKeyerDevice.h"
#include "HaliKeySerialDevice.h"
#include "HaliKeyMidiDevice.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

ICWKeyerDevice* KeyerFactory::createKeyer(KeyerDeviceType type, QObject* parent) {
    switch (type) {
        case KeyerDeviceType::WinKeyer:
            LOG_INFO("KeyerFactory", "Creating WinKeyer device");
            return new WinKeyerDevice(parent);

        case KeyerDeviceType::HaliKeySerial:
            LOG_INFO("KeyerFactory", "Creating HaliKey Serial device");
            return new HaliKeySerialDevice(parent);

        case KeyerDeviceType::HaliKeyMidi:
            LOG_INFO("KeyerFactory", "Creating HaliKey MIDI device");
            return new HaliKeyMidiDevice(parent);

        default:
            LOG_ERROR("KeyerFactory", "Unknown keyer device type");
            return nullptr;
    }
}

QString KeyerFactory::deviceTypeName(KeyerDeviceType type) {
    switch (type) {
        case KeyerDeviceType::WinKeyer:      return "WinKeyer";
        case KeyerDeviceType::HaliKeySerial: return "HaliKey (Serial)";
        case KeyerDeviceType::HaliKeyMidi:   return "HaliKey (MIDI)";
        default:                              return "Unknown";
    }
}

QStringList KeyerFactory::availableDeviceTypes() {
    return {
        deviceTypeName(KeyerDeviceType::WinKeyer),
        deviceTypeName(KeyerDeviceType::HaliKeySerial),
        deviceTypeName(KeyerDeviceType::HaliKeyMidi)
    };
}

} // namespace TR4QT
