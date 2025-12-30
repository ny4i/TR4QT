#ifndef ARRLSECTIONHELPER_H
#define ARRLSECTIONHELPER_H

#include <QString>

namespace TR4QT {
namespace Arrl {

/**
 * @brief Map a US state and county to the corresponding ARRL section abbreviation.
 *
 * This helper implements the authoritative ARRL section-by-county mappings
 * as defined by the ARRL (http://www.arrl.org/section-boundaries) and
 * mirrored in Ham::Reference::QRZ's get_arrl_section() method.
 *
 * Most US states map 1:1 to a single ARRL section (e.g. UT, CO, AZ).
 * Some states are subdivided by county (FL, CA, TX, NY, NJ, MA, PA, WA).
 *
 * @param stateAbbrev Two-letter US state code (case-insensitive, e.g. "FL", "CA", "UT")
 * @param countyName County name (case-insensitive, e.g. "Pinellas", "Los Angeles")
 *                   The word "County" is optional and will be stripped if present.
 *
 * @return ARRL section abbreviation (e.g. "WCF", "LAX", "UT"), or empty QString if not found
 *
 * @examples
 * sectionForStateCounty("FL", "Pinellas") -> "WCF"
 * sectionForStateCounty("FL", "Duval") -> "NFL"
 * sectionForStateCounty("FL", "Broward") -> "SFL"
 * sectionForStateCounty("CA", "Los Angeles") -> "LAX"
 * sectionForStateCounty("CA", "San Diego") -> "SDG"
 * sectionForStateCounty("UT", "Salt Lake") -> "UT"
 * sectionForStateCounty("TX", "Harris") -> "STX"
 * sectionForStateCounty("NY", "Queens") -> "NLI"
 * sectionForStateCounty("MA", "Suffolk") -> "EMA"
 * sectionForStateCounty("PA", "Philadelphia") -> "EPA"
 * sectionForStateCounty("WA", "King") -> "WWA"
 */
QString sectionForStateCounty(const QString& stateAbbrev, const QString& countyName);

/**
 * @brief Get ARRL section for a state that has no county subdivision.
 *
 * This is a convenience function for states with 1:1 state→section mapping.
 *
 * @param stateAbbrev Two-letter US state code (case-insensitive)
 * @return ARRL section abbreviation, or empty QString if state is subdivided or not found
 */
QString sectionForState(const QString& stateAbbrev);

/**
 * @brief Check if a section code is a valid ARRL or RAC section.
 *
 * Validates against the complete list of ARRL sections (US), RAC sections (Canada),
 * and special sections (DX, HI, PR, VI).
 *
 * @param section Section code to validate (case-insensitive)
 * @return true if the section is valid, false otherwise
 *
 * @examples
 * isValidSection("WMA") -> true
 * isValidSection("STX") -> true
 * isValidSection("BC") -> true (Canadian)
 * isValidSection("DX") -> true
 * isValidSection("ZZZ") -> false
 */
bool isValidSection(const QString& section);

/**
 * @brief Get list of all valid ARRL and RAC sections.
 *
 * Returns the complete list of valid section codes for use in
 * contest exchange validation.
 *
 * @return QStringList containing all valid section codes
 */
QStringList getAllSections();

/**
 * @brief Get list of US state abbreviations only (2-letter codes).
 *
 * Returns the 50 US states as 2-letter postal codes.
 * Does NOT include Canadian provinces or ARRL section subdivisions.
 *
 * @return QStringList containing US state codes (AL, AK, AZ, ..., WY)
 */
QStringList getUSStates();

/**
 * @brief Get list of Canadian province/territory abbreviations.
 *
 * Returns Canadian provinces and territories as 2-letter codes,
 * plus Ontario subdivisions (GH, ONE, ONN, ONS).
 *
 * @return QStringList containing Canadian codes (AB, BC, MB, ..., YT, GH, ONE, ONN, ONS)
 */
QStringList getCanadianProvinces();

/**
 * @brief Get combined list of US states and Canadian provinces.
 *
 * Convenience function for contests that use states/provinces as multipliers
 * (like ARRL RTTY Roundup, NAQP, etc.).
 *
 * @return QStringList containing all US states + Canadian provinces
 */
QStringList getStatesAndProvinces();

} // namespace Arrl
} // namespace TR4QT

#endif // ARRLSECTIONHELPER_H
