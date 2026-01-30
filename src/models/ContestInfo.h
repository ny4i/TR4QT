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

/**
 * ContestInfo - Contest metadata structure
 *
 * Contains all information about a contest needed for activation and logging.
 * Extracted from ContestChooserDialog.h to allow headless operation without
 * QtWidgets dependency.
 */

#ifndef CONTESTINFO_H
#define CONTESTINFO_H

#include <QString>
#include <QDateTime>

namespace TR4QT {

/**
 * Contest information structure
 *
 * Contains all metadata about a contest:
 * - Identification (id, name, type)
 * - Timing (start date)
 * - Configuration (mode, exchange, category, power)
 * - Storage (database path)
 */
struct ContestInfo {
    QString contestId;          // Unique ID (used for database filename)
    QString contestName;        // Display name (e.g., "CQ WW DX CW 2024")
    QString contestType;        // "CQWW_CW", "CQWW_SSB", "CQWPX_CW", "CQWPX_SSB", "WFD"
    QDateTime startDate;        // Contest start date/time
    QString mode;               // "CW", "SSB", "Mixed"
    bool isExisting;            // true if resuming existing contest
    QString databasePath;       // Full path to database file
    QString exchangeSent;       // Contest-specific sent exchange (e.g., "1H WCF" for WFD)

    // Contest configuration (for Cabrillo export)
    QString category;           // SINGLE-OP, MULTI-OP, MULTI-TWO, CHECKLOG
    QString powerClass;         // HIGH, LOW, QRP
    QString assisted;           // ASSISTED, NON-ASSISTED
    QString operatorName;       // Name for contests that use {NAME} (may differ from legal name)
};

} // namespace TR4QT

#endif // CONTESTINFO_H
