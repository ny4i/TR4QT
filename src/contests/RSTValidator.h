#ifndef RSTVALIDATOR_H
#define RSTVALIDATOR_H

#include <QString>
#include <QRegularExpression>
#include "../core/Types.h"

namespace TR4QT {

/**
 * RSTValidator - Centralized RST (Readability-Strength-Tone) validation
 *
 * Provides consistent RST validation and default value generation across
 * all contest implementations. Eliminates duplicate validation logic.
 *
 * RST Format:
 * - First digit: 1-5 (Readability)
 * - Second digit: 1-9 (Signal Strength)
 * - Third digit: 1-9 (Tone quality) - required for CW, optional for phone
 *
 * Examples:
 * - CW modes: "599", "579", "339" (must be 3 digits)
 * - Phone modes: "59", "599", "33" (2 or 3 digits)
 */
class RSTValidator {
public:
    /**
     * Validate RST format based on mode
     *
     * @param rst The RST string to validate (e.g., "599", "59")
     * @param mode The operating mode (CW requires 3 digits, phone allows 2 or 3)
     * @return true if RST is valid for the given mode, false otherwise
     *
     * Examples:
     *   isValid("599", ModeType::CW)  -> true
     *   isValid("59", ModeType::CW)   -> false (CW requires 3 digits)
     *   isValid("59", ModeType::USB)  -> true
     *   isValid("599", ModeType::USB) -> true (3 digits also valid for phone)
     */
    static bool isValid(const QString& rst, ModeType mode) {
        if (mode == ModeType::CW || mode == ModeType::CWR) {
            // CW: Must be exactly 3 digits [1-5][1-9][1-9]
            QRegularExpression re("^[1-5][1-9][1-9]$");
            return re.match(rst).hasMatch();
        } else {
            // Phone: Can be 2 or 3 digits [1-5][1-9][1-9]?
            QRegularExpression re("^[1-5][1-9][1-9]?$");
            return re.match(rst).hasMatch();
        }
    }

    /**
     * Get default RST value for a given mode
     *
     * @param mode The operating mode
     * @return "599" for CW/CWR modes, "59" for all other modes
     *
     * Usage:
     *   QString rst = RSTValidator::getDefault(ModeType::CW);   // Returns "599"
     *   QString rst = RSTValidator::getDefault(ModeType::USB);  // Returns "59"
     */
    static QString getDefault(ModeType mode) {
        return (mode == ModeType::CW || mode == ModeType::CWR) ? "599" : "59";
    }
};

} // namespace TR4QT

#endif // RSTVALIDATOR_H
