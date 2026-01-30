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
 * ContestRepository - Database access layer for contests table
 *
 * Handles all SQL queries for contest records. Part of Phase 1 CLAUDE.md compliance
 * refactoring to remove SQL from MainWindow.
 *
 * Design: Repository pattern - encapsulates database access with clean interface.
 */

#ifndef CONTESTREPOSITORY_H
#define CONTESTREPOSITORY_H

#include <QString>
#include <QDateTime>
#include <QList>

namespace TR4QT {

/**
 * Simple contest record structure for database operations
 */
struct ContestRecord {
    int id{-1};
    QString contestId;
    QString contestName;
    QDateTime startTime;
    QString contestType;
    QString myCall;
    QString myGrid;
    QString myContinent;
    int myCqZone{0};
    int myItuZone{0};
    int currentSerial{1};
    QString exchangeSent;
    QDateTime createdAt;

    bool isValid() const { return id > 0; }
};

/**
 * Repository for contest database operations
 *
 * Provides CRUD operations for contests table without exposing SQL to callers.
 */
class ContestRepository {
public:
    ContestRepository() = default;

    /**
     * Find contest by database ID
     * @param contestDbId Database ID (primary key)
     * @return Contest record, isValid() == false if not found
     */
    ContestRecord findById(int contestDbId);

    /**
     * Find contest by string contest ID (e.g., "CQWW-SSB-2024")
     * @param contestId String contest identifier
     * @return Contest record, isValid() == false if not found
     */
    ContestRecord findByContestId(const QString& contestId);

    /**
     * Find first (and usually only) contest in database
     * @return Contest record, isValid() == false if none found
     */
    ContestRecord findFirst();

    /**
     * Find all contests in a database file
     * @param databasePath Path to database file
     * @return List of contest records (empty if none found or error)
     */
    QList<ContestRecord> findAll(const QString& databasePath);

    /**
     * Update exchange sent for a contest
     * @param contestId Database ID
     * @param exchange New exchange string
     * @return true on success, false on error
     */
    bool updateExchange(int contestId, const QString& exchange);

    /**
     * Get last error message
     * @return Error description from last failed operation
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
};

} // namespace TR4QT

#endif // CONTESTREPOSITORY_H
