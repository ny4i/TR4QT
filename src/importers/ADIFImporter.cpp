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

#include "ADIFImporter.h"
#include "ADIFParser.h"
#include "ADIFFieldMapper.h"
#include "../logging/LogMacros.h"
#include <QFile>
#include <QFileInfo>

namespace TR4QT {

ADIFImporter::ADIFImporter() {
}

bool ADIFImporter::importFile(const QString& filePath, QList<QSO>& qsos) {
    m_lastError.clear();
    m_warnings.clear();
    m_importedCount = 0;
    m_failedCount = 0;

    // Check file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        m_lastError = QString("File does not exist: %1").arg(filePath);
        LOG_ERROR("ADIFImporter", m_lastError);
        return false;
    }

    if (!fileInfo.isReadable()) {
        m_lastError = QString("File is not readable: %1").arg(filePath);
        LOG_ERROR("ADIFImporter", m_lastError);
        return false;
    }

    // Read file content
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QString("Failed to open file: %1").arg(filePath);
        LOG_ERROR("ADIFImporter", m_lastError);
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    LOG_INFO("ADIFImporter", QString("Importing ADIF file: %1 (%2 bytes)")
        .arg(filePath).arg(content.length()));

    return importFromString(content, qsos);
}

bool ADIFImporter::importFromString(const QString& adifContent, QList<QSO>& qsos) {
    m_lastError.clear();
    m_warnings.clear();
    m_validationErrors.clear();
    m_importedCount = 0;
    m_failedCount = 0;

    qsos.clear();

    // Step 1: Parse ADIF file
    ADIFParser parser;
    if (!parser.parse(adifContent)) {
        m_lastError = QString("Failed to parse ADIF: %1").arg(parser.lastError());
        LOG_ERROR("ADIFImporter", m_lastError);
        return false;
    }

    LOG_INFO("ADIFImporter", QString("Parsed %1 records from ADIF").arg(parser.recordCount()));

    // Step 2: Map records to QSOs
    ADIFFieldMapper mapper;

    // Enable validation if CountryFile is available
    if (m_countryFile) {
        mapper.setCountryFile(m_countryFile);
    }

    QList<QMap<QString, QString>> records = parser.getRecords();

    for (int i = 0; i < records.size(); i++) {
        const QMap<QString, QString>& record = records[i];
        QSO qso;
        int recordNumber = i + 1;  // 1-based record numbering

        // Clear mapper's validation errors before each record
        mapper.clearValidationErrors();

        if (mapper.mapToQSO(record, qso, recordNumber)) {
            qsos.append(qso);
            m_importedCount++;

            // Collect validation errors/warnings even for successful mappings
            QList<ADIFValidationError> recordErrors = mapper.validationErrors();
            if (!recordErrors.isEmpty()) {
                m_validationErrors.append(recordErrors);

                // Log validation issues
                for (const auto& error : recordErrors) {
                    if (error.severity == ADIFValidationError::Error) {
                        LOG_WARN("ADIFImporter", error.toString());
                    } else {
                        LOG_INFO("ADIFImporter", error.toString());
                    }
                }
            }
        } else {
            m_failedCount++;
            QString warning = QString("Record %1 failed: %2 (Call: %3)")
                .arg(recordNumber)
                .arg(mapper.lastError())
                .arg(record.value("CALL", "unknown"));
            m_warnings.append(warning);
            LOG_WARN("ADIFImporter", warning);
        }
    }

    if (m_importedCount == 0) {
        m_lastError = "No QSOs were successfully imported";
        LOG_ERROR("ADIFImporter", m_lastError);
        return false;
    }

    LOG_INFO("ADIFImporter", QString("Import complete: %1 succeeded, %2 failed, %3 validation issues")
        .arg(m_importedCount).arg(m_failedCount).arg(m_validationErrors.size()));

    return true;
}

} // namespace TR4QT
