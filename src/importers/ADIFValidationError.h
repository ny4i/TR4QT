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
