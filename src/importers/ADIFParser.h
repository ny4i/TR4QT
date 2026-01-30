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

#ifndef ADIFPARSER_H
#define ADIFPARSER_H

#include <QString>
#include <QMap>
#include <QList>

namespace TR4QT {

/**
 * Low-level ADIF file parser
 *
 * Parses ADIF files into raw field/value maps without any business logic.
 * Handles the <FIELD:LENGTH>value format and record boundaries (<EOH>, <EOR>).
 */
class ADIFParser {
public:
    ADIFParser();
    ~ADIFParser() = default;

    /**
     * Parse an ADIF file from a string
     * @param adifContent Full ADIF file content
     * @return true if parsing succeeded
     */
    bool parse(const QString& adifContent);

    /**
     * Get the header fields (before <EOH>)
     */
    QMap<QString, QString> getHeader() const { return m_header; }

    /**
     * Get all QSO records (after <EOH>)
     * Each record is a map of field name -> value
     */
    QList<QMap<QString, QString>> getRecords() const { return m_records; }

    /**
     * Get the last error message
     */
    QString lastError() const { return m_lastError; }

    /**
     * Get number of records parsed
     */
    int recordCount() const { return m_records.size(); }

private:
    /**
     * Parse a single field in format <FIELD:LENGTH>value
     * @param text ADIF text to parse
     * @param pos Current position (updated after parsing)
     * @param fieldName Output: extracted field name
     * @param fieldValue Output: extracted field value
     * @return true if field was successfully parsed
     */
    bool parseField(const QString& text, int& pos, QString& fieldName, QString& fieldValue);

    /**
     * Skip whitespace at current position
     */
    void skipWhitespace(const QString& text, int& pos);

    /**
     * Check if we're at a tag (like <EOH> or <EOR>)
     * @return tag name if found, empty string otherwise
     */
    QString checkTag(const QString& text, int pos);

    QMap<QString, QString> m_header;
    QList<QMap<QString, QString>> m_records;
    QString m_lastError;
};

} // namespace TR4QT

#endif // ADIFPARSER_H
