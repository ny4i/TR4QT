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
    ModeType parseMode(const QString& modeStr);

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
};

} // namespace TR4QT

#endif // ADIFFIELDMAPPER_H
