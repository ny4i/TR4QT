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

#ifndef ADIFIMPORTER_H
#define ADIFIMPORTER_H

#include <QString>
#include <QList>
#include "../models/QSO.h"
#include "ADIFValidationError.h"

namespace TR4QT {

class CountryFile;

/**
 * High-level ADIF import coordinator
 *
 * Provides a clean API for importing ADIF files into TR4QT.
 * Uses ADIFParser for parsing and ADIFFieldMapper for field mapping.
 */
class ADIFImporter {
public:
    ADIFImporter();
    ~ADIFImporter() = default;

    /**
     * Set CountryFile for validation and auto-population
     * @param countryFile Pointer to CountryFile (optional)
     */
    void setCountryFile(CountryFile* countryFile) { m_countryFile = countryFile; }

    /**
     * Import QSOs from an ADIF file
     * @param filePath Path to ADIF file
     * @param qsos Output: list of imported QSOs
     * @return true if import succeeded
     */
    bool importFile(const QString& filePath, QList<QSO>& qsos);

    /**
     * Import QSOs from ADIF content string
     * @param adifContent ADIF file content
     * @param qsos Output: list of imported QSOs
     * @return true if import succeeded
     */
    bool importFromString(const QString& adifContent, QList<QSO>& qsos);

    /**
     * Get the last error message
     */
    QString lastError() const { return m_lastError; }

    /**
     * Get number of records successfully imported
     */
    int importedCount() const { return m_importedCount; }

    /**
     * Get number of records that failed to import
     */
    int failedCount() const { return m_failedCount; }

    /**
     * Get list of warnings (non-fatal issues during import)
     */
    QStringList warnings() const { return m_warnings; }

    /**
     * Get validation errors/warnings from import
     */
    QList<ADIFValidationError> validationErrors() const { return m_validationErrors; }

    /**
     * Check if any validation errors occurred
     */
    bool hasValidationErrors() const {
        for (const auto& error : m_validationErrors) {
            if (error.severity == ADIFValidationError::Error) {
                return true;
            }
        }
        return false;
    }

    /**
     * Check if any validation warnings occurred
     */
    bool hasValidationWarnings() const {
        for (const auto& error : m_validationErrors) {
            if (error.severity == ADIFValidationError::Warning) {
                return true;
            }
        }
        return false;
    }

private:
    QString m_lastError;
    int m_importedCount{0};
    int m_failedCount{0};
    QStringList m_warnings;
    QList<ADIFValidationError> m_validationErrors;
    CountryFile* m_countryFile{nullptr};
};

} // namespace TR4QT

#endif // ADIFIMPORTER_H
