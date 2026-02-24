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

#ifndef TS890RADIO_H
#define TS890RADIO_H

#include "KenwoodRadio.h"

namespace TR4QT {

/**
 * @brief Kenwood TS-890S specific implementation
 *
 * Model-specific features:
 * - Mode encoding uses hex chars for data modes (A-F)
 * - CW speed range: 4-60 WPM
 * - Radio ID: 024
 * - 100W max power
 * - AI2 auto-information mode
 * - Default LAN port: 60000
 */
class TS890Radio : public KenwoodRadio {
    Q_OBJECT

public:
    // TS-890 CW speed limits
    static constexpr int MIN_CW_SPEED_WPM = 4;
    static constexpr int MAX_CW_SPEED_WPM = 60;

    explicit TS890Radio(QObject* parent = nullptr);
    ~TS890Radio() override = default;

    // Radio capabilities
    Q_INVOKABLE int maxPowerWatts() const override { return 100; }
    void getCWSpeedRange(int& minWpm, int& maxWpm) const override;

protected:
    // KenwoodRadio subclass hooks
    ModeType kenwoodModeToMode(const QString& modeChar) const override;
    QString modeToKenwoodMode(ModeType mode) const override;
    QString radioIdString() const override { return "024"; }
    void onConnectedInitialize() override;
};

} // namespace TR4QT

#endif // TS890RADIO_H
