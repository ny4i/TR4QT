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

#include "LogFormatter.h"

namespace TR4QT {

QString LogFormatter::format(const QDateTime& timestamp,
                             qint64 elapsedMs,
                             Qt::HANDLE threadId,
                             LogLevel level,
                             const QString& category,
                             const QString& message) {
    // TR4W pattern: "%d %r [%t] %p %c %x - %m%n"
    return QString("%1 %2 %3 %4 %5 - %6\n")
        .arg(formatTimestamp(timestamp))       // %d
        .arg(elapsedMs)                        // %r
        .arg(formatThreadId(threadId))         // [%t]
        .arg(logLevelToShortString(level))     // %p
        .arg(category)                         // %c
        // %x (NDC) omitted - reserved for future use
        .arg(message);                         // %m
    // %n (newline) included at end
}

QString LogFormatter::formatTimestamp(const QDateTime& timestamp) {
    // Format: "24 Dec 2025 14:23:45.123"
    // Base timestamp without milliseconds
    QString base = timestamp.toString("dd MMM yyyy HH:mm:ss");

    // Add milliseconds with zero padding
    int ms = timestamp.time().msec();
    return QString("%1.%2").arg(base).arg(ms, 3, 10, QChar('0'));
}

QString LogFormatter::formatThreadId(Qt::HANDLE threadId) {
    // Convert thread handle to numeric ID (portable across platforms)
    quintptr numericId = reinterpret_cast<quintptr>(threadId);
    return QString("[%1]").arg(numericId);
}

} // namespace TR4QT
