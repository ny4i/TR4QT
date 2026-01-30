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

#include "HamRadioPrivileges.h"

namespace TR4QT {

// FCC Part 97 Phone Segment Start Frequencies (MHz)
// Source: ARRL Band Chart, FCC Part 97.301(e)
const HamRadioPrivileges::BandPrivileges HamRadioPrivileges::PRIVILEGES[] = {
    // HF Bands - Various restrictions by license class
    // Band        Tech      General   Extra
    {BandType::Band160M, 0.000,   1.800,    1.800},   // Tech: No privileges
    {BandType::Band80M,  0.000,   3.600,    3.600},   // Tech: No phone (CW only 3.525-3.600)
    {BandType::Band40M,  0.000,   7.175,    7.125},   // Tech: No phone (CW only 7.025-7.125)
    {BandType::Band20M,  0.000,   14.225,   14.150},  // Tech: No phone
    {BandType::Band15M,  21.300,  21.275,   21.200},  // Tech: Limited (21.300-21.450)
    {BandType::Band10M,  28.300,  28.300,   28.300},  // All classes same
    {BandType::Band6M,   50.100,  50.100,   50.100},  // All classes same

    // VHF/UHF Bands - Full privileges for all license classes (including Technician)
    {BandType::Band2M,   144.100, 144.100,  144.100}, // Phone starts 144.1 (CW only 144.0-144.1)
    {BandType::Band1_25M,222.000, 222.000,  222.000}, // All modes (222-225 MHz)
    {BandType::Band70CM, 420.000, 420.000,  420.000}, // All modes (420-450 MHz)
    {BandType::Band23CM, 1240.000,1240.000, 1240.000},// All modes (1240-1300 MHz)
};

const int HamRadioPrivileges::PRIVILEGE_COUNT = sizeof(PRIVILEGES) / sizeof(PRIVILEGES[0]);

HamRadioPrivileges::HamRadioPrivileges(LicenseClass license)
    : m_license(license)
{
}

QString HamRadioPrivileges::validatePhoneMode(freq_t freqHz, BandType band, ModeType mode) const {
    // Skip validation if license class is None (warnings disabled)
    if (m_license == LicenseClass::None) {
        return QString();
    }

    // Only validate phone modes (LSB, USB, FM, AM)
    if (!isPhoneMode(mode)) {
        return QString();  // Not phone mode, no warning
    }

    // Check if phone allowed at this frequency
    if (!isPhoneAllowed(freqHz, band)) {
        double phoneStartMHz = getPhoneStartMHz(band);

        if (phoneStartMHz == 0.0) {
            // No phone privileges at all on this band
            return QString("Phone mode not permitted on %1 for %2 class")
                .arg(bandToString(band))
                .arg(licenseClassToString(m_license));
        } else {
            // Phone not allowed below this frequency
            return QString("Phone not permitted below %1 MHz for %2 class")
                .arg(phoneStartMHz, 0, 'f', 3)
                .arg(licenseClassToString(m_license));
        }
    }

    return QString();  // No violation
}

double HamRadioPrivileges::getPhoneStartMHz(BandType band) const {
    const BandPrivileges* priv = getPrivileges(band);
    if (!priv) {
        return 0.0;  // Unknown band
    }

    switch (m_license) {
        case LicenseClass::None:
            return 0.0;  // Warnings disabled
        case LicenseClass::Technician:
            return priv->phoneStartTech;
        case LicenseClass::General:
            return priv->phoneStartGeneral;
        case LicenseClass::Extra:
            return priv->phoneStartExtra;
    }

    return 0.0;
}

bool HamRadioPrivileges::isPhoneAllowed(freq_t freqHz, BandType band) const {
    double phoneStartMHz = getPhoneStartMHz(band);

    if (phoneStartMHz == 0.0) {
        return false;  // No phone privileges on this band
    }

    double freqMHz = freqHz / 1000000.0;  // Convert Hz to MHz
    return freqMHz >= phoneStartMHz;
}

const HamRadioPrivileges::BandPrivileges* HamRadioPrivileges::getPrivileges(BandType band) const {
    for (int i = 0; i < PRIVILEGE_COUNT; i++) {
        if (PRIVILEGES[i].band == band) {
            return &PRIVILEGES[i];
        }
    }
    return nullptr;  // Band not in contest band list
}

QString HamRadioPrivileges::licenseClassToString(LicenseClass license) {
    switch (license) {
        case LicenseClass::None:
            return "None";
        case LicenseClass::Technician:
            return "Technician";
        case LicenseClass::General:
            return "General";
        case LicenseClass::Extra:
            return "Extra";
    }
    return "Unknown";
}

HamRadioPrivileges::LicenseClass HamRadioPrivileges::stringToLicenseClass(const QString& str, bool* ok) {
    if (ok) *ok = true;

    QString lower = str.toLower();
    if (lower == "none" || lower.isEmpty()) {
        return LicenseClass::None;
    } else if (lower == "technician" || lower == "tech") {
        return LicenseClass::Technician;
    } else if (lower == "general" || lower == "gen") {
        return LicenseClass::General;
    } else if (lower == "extra") {
        return LicenseClass::Extra;
    }

    if (ok) *ok = false;
    return LicenseClass::None;  // Default: warnings disabled
}

} // namespace TR4QT
