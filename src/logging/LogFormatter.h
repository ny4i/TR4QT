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

#ifndef LOGFORMATTER_H
#define LOGFORMATTER_H

#include <QString>
#include <QDateTime>
#include <QThread>
#include "LogLevel.h"

namespace TR4QT {

/**
 * Formats log messages using TR4W format pattern
 * Pattern: "%d %r [%t] %p %c %x - %m%n"
 *
 * Placeholders:
 * %d - Date/time with milliseconds: "24 Dec 2025 14:23:45.123"
 * %r - Runtime/elapsed milliseconds since program start
 * [%t] - Thread ID in brackets
 * %p - Log level (trace, debug, info, warn, error, fatal)
 * %c - Logger category/name
 * %x - NDC (Nested Diagnostic Context) - reserved for future use
 * %m - Message text
 * %n - Newline
 */
class LogFormatter {
public:
    /**
     * Format a log message
     *
     * @param timestamp Current timestamp
     * @param elapsedMs Milliseconds since program start
     * @param threadId Thread ID
     * @param level Log level
     * @param category Logger category
     * @param message Message text
     * @return Formatted log line (with newline)
     */
    static QString format(const QDateTime& timestamp,
                         qint64 elapsedMs,
                         Qt::HANDLE threadId,
                         LogLevel level,
                         const QString& category,
                         const QString& message);

    // Format timestamp: "24 Dec 2025 14:23:45.123"
    static QString formatTimestamp(const QDateTime& timestamp);

    // Format thread ID: "[12345]"
    static QString formatThreadId(Qt::HANDLE threadId);
};

} // namespace TR4QT

#endif // LOGFORMATTER_H
