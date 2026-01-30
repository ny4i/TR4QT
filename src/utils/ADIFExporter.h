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

#ifndef ADIFEXPORTER_H
#define ADIFEXPORTER_H

#include <QString>
#include <QList>
#include "../models/QSO.h"
#include "../contests/ContestBase.h"

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
     * @param contest Contest instance (for official ADIF CONTEST_ID)
     * @param operatorCall Optional operator callsign for OPERATOR field
     * @return true if export succeeded, false on error
     */
    bool exportToFile(const QList<QSO>& qsos,
                     const QString& filePath,
                     ContestBase* contest = nullptr,
                     const QString& operatorCall = QString());

    /**
     * Generate ADIF text from QSO list
     *
     * @param qsos List of QSOs to export
     * @param contest Contest instance (for official ADIF CONTEST_ID)
     * @param operatorCall Optional operator callsign
     * @return ADIF-formatted text
     */
    QString generateADIF(const QList<QSO>& qsos,
                        ContestBase* contest = nullptr,
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
     * @param contestId Official ADIF contest ID (e.g., "CQ-WW-CW")
     * @return ADIF-formatted record
     */
    QString formatQSO(const QSO& qso, const QString& contestId = QString());

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
