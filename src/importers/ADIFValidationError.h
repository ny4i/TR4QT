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

#ifndef ADIFVALIDATIONERROR_H
#define ADIFVALIDATIONERROR_H

#include <QString>

namespace TR4QT {

/**
 * Represents a validation error found during ADIF import
 *
 * Used to report data quality issues like:
 * - US callsign with Canadian state
 * - Invalid mode/band combinations
 * - Missing required contest fields
 */
struct ADIFValidationError {
    enum Severity {
        Warning,    // Data inconsistency but might be intentional
        Error       // Clear data error (e.g., US call with VE state)
    };

    int recordNumber{0};           // Which record (1-based)
    QString callsign;              // Callsign of problematic QSO
    Severity severity{Warning};    // How serious is this issue
    QString field;                 // Which field has the issue
    QString value;                 // The problematic value
    QString message;               // Human-readable description

    /**
     * Format error for display
     */
    QString toString() const {
        QString severityStr = (severity == Error) ? "ERROR" : "WARNING";
        return QString("[%1] Record %2 (%3): %4 - %5=\"%6\"")
            .arg(severityStr)
            .arg(recordNumber)
            .arg(callsign)
            .arg(message)
            .arg(field)
            .arg(value);
    }
};

} // namespace TR4QT

#endif // ADIFVALIDATIONERROR_H
