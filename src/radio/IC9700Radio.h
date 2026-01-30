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

#ifndef IC9700RADIO_H
#define IC9700RADIO_H

#include "IcomRadio.h"

namespace TR4QT {

/**
 * @brief IC-9700 specific implementation
 *
 * Model-specific features:
 * - VFO B commands use standard format (no sub-command byte)
 * - Transceive parameter: 0x01 0x27
 * - Supports dual VFO
 * - CI-V address typically 0xA2
 */
class IC9700Radio : public IcomRadio
{
    Q_OBJECT

public:
    explicit IC9700Radio(QObject* parent = nullptr);
    ~IC9700Radio() override = default;

    // Model identification
    QString modelName() const override { return "IC-9700"; }

    // VFO B format: IC-9700 uses standard format (no sub-command)
    bool vfoBUsesSubCommand() const override { return false; }

    // Transceive command parameter (0x1A 0x05 0x01 <param2> 0x01/00)
    quint8 transceiveParameter2() const override { return 0x27; }

    // Model capabilities
    bool supportsTransceive() const override { return true; }
    bool supportsVfoB() const override { return true; }
    bool supportsScope() const override { return true; }
    int maxPowerWatts() const override { return 100; }  // IC-9700 is 100W radio
};

} // namespace TR4QT

#endif // IC9700RADIO_H
