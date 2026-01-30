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

#ifndef ADIFFIELDMAPPER_H
#define ADIFFIELDMAPPER_H

#include <QString>
#include <QMap>
#include <QList>
#include "../models/QSO.h"
#include "ADIFValidationError.h"

namespace TR4QT {

class CountryFile;

/**
 * Maps ADIF fields to TR4QT QSO struct
 *
 * Handles type conversions, date/time parsing, and custom APP_ fields.
 * Separate from parser to keep concerns separated.
 */
class ADIFFieldMapper {
public:
    ADIFFieldMapper();
    ~ADIFFieldMapper() = default;

    /**
     * Enable validation using CountryFile lookup
     * @param countryFile Pointer to CountryFile for validation (optional)
     */
    void setCountryFile(CountryFile* countryFile) { m_countryFile = countryFile; }

    /**
     * Enable/disable auto-correction of validation errors
     * @param autoCorrect If true, automatically fix errors (e.g., clear invalid STATE)
     *                    If false, report errors without fixing
     */
    void setAutoCorrect(bool autoCorrect) { m_autoCorrect = autoCorrect; }

    /**
     * Map ADIF record fields to a QSO struct
     * @param adifFields Raw field map from ADIFParser
     * @param qso Output QSO struct
     * @param recordNumber Record number for validation errors (1-based)
     * @return true if mapping succeeded (minimum required fields present)
     */
    bool mapToQSO(const QMap<QString, QString>& adifFields, QSO& qso, int recordNumber = 0);

    /**
     * Get the last error message
     */
    QString lastError() const { return m_lastError; }

    /**
     * Get validation errors/warnings from last mapping
     */
    QList<ADIFValidationError> validationErrors() const { return m_validationErrors; }

    /**
     * Clear validation errors
     */
    void clearValidationErrors() { m_validationErrors.clear(); }

private:
    /**
     * Parse ADIF date (YYYYMMDD) and time (HHMMSS or HHMM) to QDateTime
     */
    QDateTime parseDateTime(const QString& date, const QString& time);

    /**
     * Convert ADIF band string to BandType enum
     * ADIF uses: "160M", "80M", "40M", etc.
     */
    BandType parseBand(const QString& bandStr);

    /**
     * Convert ADIF mode string to ModeType enum
     * ADIF uses: "CW", "SSB", "RTTY", "FT8", etc.
     */
    ModeType parseMode(const QString& modeStr, BandType band = BandType::None, const QString& submode = QString());

    /**
     * Convert ADIF frequency (MHz) to Hz
     */
    freq_t parseFrequency(const QString& freqStr);

    /**
     * Convert ADIF continent code to TR4QT format
     * ADIF uses: "NA", "SA", "EU", "AF", "AS", "OC"
     */
    QString parseContinent(const QString& contStr);

    /**
     * Validate QSO data for common errors
     * @param qso QSO to validate
     * @param recordNumber Record number for error reporting
     */
    void validateQSO(const QSO& qso, int recordNumber);

    QString m_lastError;
    QList<ADIFValidationError> m_validationErrors;
    CountryFile* m_countryFile{nullptr};  // Optional - for validation
    bool m_autoCorrect{true};              // Auto-correct validation errors
};

} // namespace TR4QT

#endif // ADIFFIELDMAPPER_H
