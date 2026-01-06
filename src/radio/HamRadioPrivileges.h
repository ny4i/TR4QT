#ifndef HAMRADIOPRIVILEGES_H
#define HAMRADIOPRIVILEGES_H

#include "../core/Types.h"
#include <QString>
#include <hamlib/rig.h>  // For freq_t type

namespace TR4QT {

/**
 * @brief US Amateur Radio Frequency/Mode Privilege Validator
 *
 * Validates whether phone mode is permitted on a given frequency
 * based on FCC Part 97 rules and operator license class.
 *
 * Covers contest bands: 160m, 80m, 40m, 20m, 15m, 10m, 6m
 *
 * Primary use case: Warn when attempting phone operation in
 * CW/RTTY-only segments to prevent accidental out-of-band transmission.
 */
class HamRadioPrivileges {
public:
    /**
     * US Amateur Radio License Classes
     */
    enum class LicenseClass {
        None,        // Warnings disabled (default)
        Technician,  // Entry level, limited HF privileges
        General,     // Most HF privileges with some restrictions
        Extra        // Full privileges on all bands
    };

    /**
     * Construct validator for given license class
     */
    explicit HamRadioPrivileges(LicenseClass license);

    /**
     * Validate phone mode operation on given frequency
     *
     * @param freqHz Frequency in Hertz
     * @param band Band (used for validation)
     * @param mode Operating mode
     * @return Empty string if OK, warning message if violation detected
     *
     * Example warnings:
     * - "Phone not permitted below 14.225 MHz for General class"
     * - "Phone not permitted below 7.175 MHz for General class"
     */
    QString validatePhoneMode(freq_t freqHz, BandType band, ModeType mode) const;

    /**
     * Get phone start frequency for current license class on given band
     *
     * @param band Band to check
     * @return Phone start frequency in MHz (0.0 = no phone privileges)
     */
    double getPhoneStartMHz(BandType band) const;

    /**
     * Check if phone mode is allowed at given frequency
     *
     * @param freqHz Frequency in Hertz
     * @param band Band
     * @return true if phone allowed, false otherwise
     */
    bool isPhoneAllowed(freq_t freqHz, BandType band) const;

    /**
     * Get current license class
     */
    LicenseClass licenseClass() const { return m_license; }

    /**
     * Convert license class enum to string
     */
    static QString licenseClassToString(LicenseClass license);

    /**
     * Convert string to license class enum
     */
    static LicenseClass stringToLicenseClass(const QString& str, bool* ok = nullptr);

private:
    LicenseClass m_license;

    /**
     * Band privilege data (phone start frequencies per license class)
     */
    struct BandPrivileges {
        BandType band;
        double phoneStartTech;    // MHz (0.0 = no phone privileges)
        double phoneStartGeneral; // MHz
        double phoneStartExtra;   // MHz
    };

    static const BandPrivileges PRIVILEGES[];
    static const int PRIVILEGE_COUNT;

    /**
     * Get privileges for a specific band
     */
    const BandPrivileges* getPrivileges(BandType band) const;
};

} // namespace TR4QT

#endif // HAMRADIOPRIVILEGES_H
