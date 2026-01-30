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

#include "ADIFParser.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

ADIFParser::ADIFParser() {
}

bool ADIFParser::parse(const QString& adifContent) {
    m_header.clear();
    m_records.clear();
    m_lastError.clear();

    if (adifContent.isEmpty()) {
        m_lastError = "ADIF content is empty";
        return false;
    }

    int pos = 0;
    bool inHeader = true;
    QMap<QString, QString> currentRecord;

    while (pos < adifContent.length()) {
        skipWhitespace(adifContent, pos);

        if (pos >= adifContent.length()) {
            break;
        }

        // Check for special tags
        QString tag = checkTag(adifContent, pos);
        if (!tag.isEmpty()) {
            if (tag == "EOH") {
                inHeader = false;
                pos += 5; // Skip <EOH>
                LOG_DEBUG("ADIFParser", QString("Header parsed with %1 fields").arg(m_header.size()));
                continue;
            } else if (tag == "EOR") {
                // End of record - save current record
                if (!currentRecord.isEmpty()) {
                    m_records.append(currentRecord);
                    currentRecord.clear();
                }
                pos += 5; // Skip <EOR>
                continue;
            }
        }

        // Parse field
        QString fieldName, fieldValue;
        if (parseField(adifContent, pos, fieldName, fieldValue)) {
            if (inHeader) {
                m_header[fieldName] = fieldValue;
            } else {
                currentRecord[fieldName] = fieldValue;
            }
        } else {
            // If we can't parse a field, skip to next '<'
            int nextAngleBracket = adifContent.indexOf('<', pos + 1);
            if (nextAngleBracket == -1) {
                break;
            }
            pos = nextAngleBracket;
        }
    }

    // Save last record if not empty
    if (!currentRecord.isEmpty()) {
        m_records.append(currentRecord);
    }

    LOG_INFO("ADIFParser", QString("Parsed %1 QSO records from ADIF file").arg(m_records.size()));
    return true;
}

bool ADIFParser::parseField(const QString& text, int& pos, QString& fieldName, QString& fieldValue) {
    // Expected format: <FIELD:LENGTH>value or <FIELD:LENGTH:TYPE>value

    if (pos >= text.length() || text[pos] != '<') {
        return false;
    }

    // Find closing >
    int closePos = text.indexOf('>', pos);
    if (closePos == -1) {
        return false;
    }

    // Extract field spec: FIELD:LENGTH or FIELD:LENGTH:TYPE
    QString fieldSpec = text.mid(pos + 1, closePos - pos - 1);
    QStringList parts = fieldSpec.split(':');

    if (parts.size() < 2) {
        // Not a valid field (might be <EOH> or <EOR>)
        return false;
    }

    fieldName = parts[0].trimmed().toUpper();
    bool ok;
    int length = parts[1].toInt(&ok);

    if (!ok || length < 0) {
        m_lastError = QString("Invalid field length for %1").arg(fieldName);
        return false;
    }

    // Extract field value (after >)
    int valueStart = closePos + 1;
    if (valueStart + length > text.length()) {
        m_lastError = QString("Field %1 declares length %2 but not enough data remains")
            .arg(fieldName).arg(length);
        return false;
    }

    fieldValue = text.mid(valueStart, length);

    // Update position to after this field
    pos = valueStart + length;

    return true;
}

void ADIFParser::skipWhitespace(const QString& text, int& pos) {
    while (pos < text.length() && text[pos].isSpace()) {
        pos++;
    }
}

QString ADIFParser::checkTag(const QString& text, int pos) {
    // Check for <EOH> or <EOR> tags
    if (pos + 5 <= text.length()) {
        QString potential = text.mid(pos, 5).toUpper();
        if (potential == "<EOH>" || potential == "<EOR>") {
            return potential.mid(1, 3); // Return "EOH" or "EOR"
        }
    }
    return QString();
}

} // namespace TR4QT
