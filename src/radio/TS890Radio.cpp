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

#include "TS890Radio.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

TS890Radio::TS890Radio(QObject* parent)
    : KenwoodRadio(parent)
{
    LOG_DEBUG("TS890Radio", "TS-890S radio instance created");
}

// ============================================================================
// CW Speed Range
// ============================================================================

void TS890Radio::getCWSpeedRange(int& minWpm, int& maxWpm) const
{
    minWpm = MIN_CW_SPEED_WPM;
    maxWpm = MAX_CW_SPEED_WPM;
}

// ============================================================================
// Mode Mapping: TS-890S OM Command P2 Values
// ============================================================================
//
// The TS-890 uses single-character mode identifiers in the OM command.
// Standard modes use digits 1-9, data modes use hex characters A-F.
//
// P2 | TS-890 Mode | TR4QT ModeType
// ---|-------------|---------------
//  1 | LSB         | LSB
//  2 | USB         | USB
//  3 | CW          | CW
//  4 | FM          | FM
//  5 | AM          | AM
//  6 | FSK         | RTTY
//  7 | CW-R        | CW (reverse)
//  9 | FSK-R        | RTTY (reverse)
//  A | PSK         | PSK
//  B | PSK-R       | PSK (reverse)
//  C | LSB-DATA    | DATA
//  D | USB-DATA    | DATA
//  E | FM-DATA     | DATA
//  F | AM-DATA     | DATA

ModeType TS890Radio::kenwoodModeToMode(const QString& modeChar) const
{
    if (modeChar.length() != 1) return ModeType::None;

    QChar ch = modeChar.at(0);
    switch (ch.toLatin1()) {
        case '1': return ModeType::LSB;
        case '2': return ModeType::USB;
        case '3': return ModeType::CW;
        case '4': return ModeType::FM;
        case '5': return ModeType::AM;
        case '6': return ModeType::RTTY;
        case '7': return ModeType::CW;       // CW-R → CW (TR4QT doesn't distinguish)
        case '9': return ModeType::RTTY;     // FSK-R → RTTY
        case 'A': return ModeType::PSK;
        case 'B': return ModeType::PSK;      // PSK-R → PSK
        case 'C': return ModeType::DATA;     // LSB-DATA
        case 'D': return ModeType::DATA;     // USB-DATA
        case 'E': return ModeType::DATA;     // FM-DATA
        case 'F': return ModeType::DATA;     // AM-DATA
        default:
            LOG_WARN("TS890Radio", QString("Unknown mode char: '%1'").arg(modeChar));
            return ModeType::None;
    }
}

QString TS890Radio::modeToKenwoodMode(ModeType mode) const
{
    switch (mode) {
        case ModeType::LSB:   return "1";
        case ModeType::USB:   return "2";
        case ModeType::CW:    return "3";
        case ModeType::CWR:   return "7";
        case ModeType::FM:    return "4";
        case ModeType::AM:    return "5";
        case ModeType::RTTY:  return "6";
        case ModeType::RTTYR: return "9";
        case ModeType::PSK:   return "A";
        case ModeType::PSKR:  return "B";
        case ModeType::DATA:  return "D";    // Default to USB-DATA
        case ModeType::DATAR: return "C";    // LSB-DATA
        case ModeType::FT8:   return "D";    // FT8 → USB-DATA
        case ModeType::FT4:   return "D";    // FT4 → USB-DATA
        default:
            LOG_WARN("TS890Radio", QString("Unsupported mode for TS-890: %1").arg(static_cast<int>(mode)));
            return QString();
    }
}

// ============================================================================
// Post-Authentication Initialization
// ============================================================================

void TS890Radio::onConnectedInitialize()
{
    LOG_INFO("TS890Radio", "Initializing TS-890S radio state");

    // Enable Auto-Information mode 2 (push updates for freq, mode, split, etc.)
    sendCommand("AI2");

    // Query initial state
    sendCommand("FA");     // VFO A frequency
    sendCommand("FB");     // VFO B frequency
    sendCommand("OM0");    // VFO A mode query (OM + VFO=0, no mode = query)
    sendCommand("OM1");    // VFO B mode query (OM + VFO=1, no mode = query)
    sendCommand("KS");     // CW speed
    sendCommand("TB");     // Split status
    sendCommand("FT");     // TX VFO select
    sendCommand("RT");     // RIT on/off
    sendCommand("XT");     // XIT on/off
    sendCommand("ID");     // Radio identification

    // Set radio model in state
    {
        QMutexLocker lock(&m_stateMutex);
        m_state.radioModel = "TS-890S";
        m_state.isValid = true;
    }

    emit connectionStatusChanged(true);
    emit stateUpdated(getCurrentState());

    LOG_INFO("TS890Radio", "TS-890S initialization complete");
}

} // namespace TR4QT
