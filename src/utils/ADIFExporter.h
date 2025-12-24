#ifndef ADIFEXPORTER_H
#define ADIFEXPORTER_H

#include <QString>
#include <QList>
#include "../models/QSO.h"

namespace TR4QT {

/**
 * ADIF (Amateur Data Interchange Format) file exporter
 *
 * Exports QSO log to ADIF format for import into other logging software.
 * ADIF is the standard interchange format used by ham radio logging programs.
 *
 * Format specification: https://adif.org/
 *
 * Example usage:
 *   ADIFExporter exporter;
 *   bool success = exporter.exportToFile(qsos, "/path/to/file.adi");
 */
class ADIFExporter {
public:
    ADIFExporter() = default;
    ~ADIFExporter() = default;

    /**
     * Export QSO list to ADIF file
     *
     * @param qsos List of QSOs to export
     * @param filePath Path to output file (typically .adi or .adif extension)
     * @param contestName Optional contest name for CONTEST_ID field
     * @param operatorCall Optional operator callsign for OPERATOR field
     * @return true if export succeeded, false on error
     */
    bool exportToFile(const QList<QSO>& qsos,
                     const QString& filePath,
                     const QString& contestName = QString(),
                     const QString& operatorCall = QString());

    /**
     * Generate ADIF text from QSO list
     *
     * @param qsos List of QSOs to export
     * @param contestName Optional contest name
     * @param operatorCall Optional operator callsign
     * @return ADIF-formatted text
     */
    QString generateADIF(const QList<QSO>& qsos,
                        const QString& contestName = QString(),
                        const QString& operatorCall = QString());

    /**
     * Get the last error message
     *
     * @return Error message from last export operation
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;

    /**
     * Format a single QSO as ADIF record
     *
     * @param qso QSO to format
     * @return ADIF-formatted record
     */
    QString formatQSO(const QSO& qso);

    /**
     * Format ADIF field with proper encoding
     *
     * @param fieldName ADIF field name (e.g., "CALL", "QSO_DATE")
     * @param value Field value
     * @return Formatted ADIF field (e.g., "<CALL:5>W1AW")
     */
    QString formatField(const QString& fieldName, const QString& value);

    /**
     * Generate ADIF header
     *
     * @return ADIF header text
     */
    QString generateHeader();
};

} // namespace TR4QT

#endif // ADIFEXPORTER_H
