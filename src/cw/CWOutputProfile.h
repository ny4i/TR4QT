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

#ifndef CWOUTPUTPROFILE_H
#define CWOUTPUTPROFILE_H

#include <QString>
#include "CWSenderFactory.h"
#include "DtrRtsCWSender.h"
#include "../keyers/KeyerConfig.h"

namespace TR4QT {

// CW output profile defaults
namespace CWProfileDefaults {
    constexpr const char* DEFAULT_PROFILE_NAME = "Default";
    constexpr int MIDI_DIT_NOTE = 20;
    constexpr int MIDI_DAH_NOTE = 21;
    constexpr int WINKEYER_WEIGHTING_NORMAL = 50;  // 10-90 range, 50=normal
    constexpr int WINKEYER_WEIGHTING_MIN = 10;
    constexpr int WINKEYER_WEIGHTING_MAX = 90;
    constexpr int WINKEYER_TIMING_MIN = 0;
    constexpr int WINKEYER_TIMING_MAX = 250;       // x10ms
    constexpr int MIDI_NOTE_MIN = 0;
    constexpr int MIDI_NOTE_MAX = 127;
}

/**
 * Named CW output configuration profile.
 *
 * Mirrors the RadioProfile pattern: users define named CW output configs
 * ("WinKeyer on COM5", "DTR on COM3", "Radio CAT"), then assign them to
 * radio slots via StationProfile. Switching radios automatically switches
 * CW output.
 *
 * The 'type' field determines which subset of settings is relevant:
 * - Hamlib (CAT): No extra config needed (uses RadioController)
 * - KeyerDevice: WinKeyer hardware settings (port, weighting, timing)
 * - DtrRts: DTR/RTS serial port settings
 *
 * NOTE: HaliKey (paddle input) is NOT an output device. Paddle input config
 * is stored separately in PaddleInputConfig (global, not per-radio).
 * KeyerDevice type here means WinKeyer exclusively.
 */
struct CWOutputProfile {
    QString name;                                          ///< Display name (e.g., "WinKeyer COM5")
    CWSenderFactory::Backend type = CWSenderFactory::Backend::Hamlib;  ///< Output backend type

    // --- WinKeyer settings (type == KeyerDevice) ---
    QString winKeyerPortName;                              ///< Serial port for WinKeyer
    int weighting = CWProfileDefaults::WINKEYER_WEIGHTING_NORMAL;  ///< WinKeyer weighting (10-90, 50=normal)
    int leadInTime = 0;                                    ///< WinKeyer lead-in (x10ms)
    int tailTime = 0;                                      ///< WinKeyer tail time (x10ms)

    // --- DTR/RTS settings (type == DtrRts) ---
    QString dtrRtsPortName;                                ///< Serial port for DTR/RTS keying
    DtrRtsCWSender::Pin dtrRtsPin = DtrRtsCWSender::Pin::DTR;

    /// Display string for combo boxes and lists
    QString displayString() const {
        switch (type) {
        case CWSenderFactory::Backend::Hamlib:
            return name.isEmpty() ? "Radio CAT" : QString("%1 (Radio CAT)").arg(name);
        case CWSenderFactory::Backend::KeyerDevice:
            return name.isEmpty()
                ? QString("WinKeyer on %1").arg(winKeyerPortName)
                : QString("%1 (WinKeyer on %2)").arg(name, winKeyerPortName);
        case CWSenderFactory::Backend::DtrRts: {
            QString pinStr = (dtrRtsPin == DtrRtsCWSender::Pin::DTR) ? "DTR" : "RTS";
            return name.isEmpty()
                ? QString("%1 on %2").arg(pinStr, dtrRtsPortName)
                : QString("%1 (%2 on %3)").arg(name, pinStr, dtrRtsPortName);
        }
        default:
            return name;
        }
    }

    /// Validate profile has minimum required fields
    bool isValid() const {
        if (name.isEmpty()) return false;
        switch (type) {
        case CWSenderFactory::Backend::Hamlib:
            return true;  // No extra config needed
        case CWSenderFactory::Backend::KeyerDevice:
            return !winKeyerPortName.isEmpty();
        case CWSenderFactory::Backend::DtrRts:
            return !dtrRtsPortName.isEmpty();
        default:
            return false;
        }
    }
};

/**
 * Global paddle input configuration (not per-radio).
 *
 * One HaliKey per station. Routes paddle presses to whichever CW output
 * is active (CAT, WinKeyer, or DTR/RTS). In SO2R, the same HaliKey keys
 * whichever radio is active.
 *
 * WinKeyer handles its own paddles in hardware — this config is only
 * for HaliKey (serial or MIDI) paddle input devices.
 */
struct PaddleInputConfig {
    enum class DeviceType {
        None,           ///< No paddle input device
        HaliKeySerial,  ///< HaliKey v1 paddle input via serial CTS/DSR
        HaliKeyMidi     ///< HaliKey MIDI paddle input via Note On/Off
    };

    DeviceType deviceType = DeviceType::None;
    QString portName;           ///< Serial port (HaliKeySerial) or MIDI device (HaliKeyMidi)
    bool paddleSwap = false;    ///< Swap dit/dah paddles
};

} // namespace TR4QT

#endif // CWOUTPUTPROFILE_H
