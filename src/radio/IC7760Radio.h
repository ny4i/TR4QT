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

#ifndef IC7760RADIO_H
#define IC7760RADIO_H

#include "IcomRadio.h"

namespace TR4QT {

/**
 * @brief IC-7760 specific implementation
 *
 * Model-specific features:
 * - VFO B commands use sub-command byte 0x01 (dual receiver)
 * - Transceive parameter: 0x01 0x31
 * - Shared RIT/XIT offset (like K4)
 * - CI-V address typically 0xB2
 */
class IC7760Radio : public IcomRadio
{
    Q_OBJECT

public:
    explicit IC7760Radio(QObject* parent = nullptr);
    ~IC7760Radio() override = default;

    // Model identification
    QString modelName() const override { return "IC-7760"; }

    // VFO B format: IC-7760 uses sub-command byte
    bool vfoBUsesSubCommand() const override { return true; }

    // Transceive command parameter (0x1A 0x05 0x01 <param2> 0x01/00)
    quint8 transceiveParameter2() const override { return 0x31; }

    // Model capabilities
    bool supportsTransceive() const override { return true; }
    bool supportsVfoB() const override { return true; }
    bool supportsScope() const override { return true; }
    int maxPowerWatts() const override { return 200; }  // IC-7760 is 200W radio
};

} // namespace TR4QT

#endif // IC7760RADIO_H
