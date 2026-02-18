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

#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace TR4QT {

/**
 * Database manager for TR4QT
 * Handles SQLite database connection and schema management
 *
 * Schema Versioning:
 * - Uses PRAGMA user_version to track schema version (currently v7)
 * - Uses PRAGMA application_id to identify TR4QT databases (0x54523451 = "TR4Q")
 * - Prevents opening databases from newer TR4QT versions
 * - Automatically migrates older databases to current schema
 *
 * Usage:
 *   Database& db = Database::instance();
 *   if (!db.open("path/to/database.db")) {
 *       // handle error
 *   }
 */
class Database {
public:
    // Schema version constants
    static constexpr int CURRENT_SCHEMA_VERSION = 9;  // Increment when adding migrations
    static constexpr uint32_t TR4QT_APP_ID = 0x54523451;  // "TR4Q" in hex (ASCII)
public:
    /**
     * Get singleton instance
     */
    static Database& instance();

    /**
     * Open database connection
     * Creates database file if it doesn't exist
     * Runs schema if new database
     *
     * @param dbPath Path to SQLite database file
     * @return true if successful
     */
    bool open(const QString& dbPath);

    /**
     * Close database connection
     */
    void close();

    /**
     * Check if database is open
     */
    bool isOpen() const;

    /**
     * Get the SQLite engine version string (e.g. "3.45.1")
     */
    QString sqliteVersion() const;

    /**
     * Get the Qt SQL database connection
     */
    QSqlDatabase& connection();

    /**
     * Execute a query
     * @param query SQL query string
     * @param values Optional bind values
     * @return QSqlQuery result (check isActive() and lastError())
     */
    QSqlQuery execute(const QString& query, const QVariantList& values = QVariantList());

    /**
     * Begin transaction
     */
    bool beginTransaction();

    /**
     * Commit transaction
     */
    bool commitTransaction();

    /**
     * Rollback transaction
     */
    bool rollbackTransaction();

    /**
     * Get last error
     */
    QString lastError() const;

    /**
     * Get last insert ID
     */
    int lastInsertId() const;

    /**
     * Get database schema version from PRAGMA user_version
     * @return Schema version (0 if not set)
     */
    int getUserVersion() const;

    /**
     * Get database application ID from PRAGMA application_id
     * @return Application ID (0 if not set)
     */
    uint32_t getApplicationId() const;

private:
    Database();
    ~Database();

    // Prevent copying
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * Initialize database schema
     * Called on first open of a new database
     */
    bool initSchema();

    /**
     * Load schema SQL from embedded resource or file
     */
    QString loadSchemaSql();

    /**
     * Migrate database schema to current version
     * Handles adding new columns and tables to existing databases
     */
    bool migrateSchema();

    /**
     * Set database schema version using PRAGMA user_version
     * @param version Schema version number
     */
    void setUserVersion(int version);

    /**
     * Set database application ID using PRAGMA application_id
     * @param appId Application identifier (TR4QT_APP_ID)
     */
    void setApplicationId(uint32_t appId);

    QSqlDatabase m_db;
    QString m_lastError;
    int m_lastInsertId{-1};
};

} // namespace TR4QT

#endif // DATABASE_H
