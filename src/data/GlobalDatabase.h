#ifndef GLOBALDATABASE_H
#define GLOBALDATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace TR4QT {

/**
 * Global database manager for TR4QT
 * Handles application-wide data that should be shared across all contests
 *
 * This database stores:
 * - LOTW user list (shared across all contest logs)
 * - Future: Global settings, cached lookups, etc.
 *
 * Unlike the contest-specific Database class, GlobalDatabase:
 * - Opens once at app startup and stays open
 * - Uses a separate connection name ("tr4qt_global")
 * - Stores data in ~/.tr4qt/tr4qt_global.db
 *
 * Usage:
 *   GlobalDatabase& db = GlobalDatabase::instance();
 *   if (!db.open()) {
 *       // handle error
 *   }
 */
class GlobalDatabase {
public:
    /**
     * Get singleton instance
     */
    static GlobalDatabase& instance();

    /**
     * Open global database connection
     * Creates database file if it doesn't exist
     * Runs schema if new database
     *
     * Opens database at default location: ~/.tr4qt/tr4qt_global.db
     *
     * @return true if successful
     */
    bool open();

    /**
     * Open global database at specific path (for testing)
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
     * Get the Qt SQL database connection
     * Use this when you need direct access to the QSqlDatabase
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
     * Get default database path
     * Returns: ~/.tr4qt/tr4qt_global.db
     */
    static QString defaultDatabasePath();

    /**
     * Check if QSQLITE driver is available
     * @return true if QSQLITE driver can be loaded
     */
    static bool isSqliteDriverAvailable();

private:
    GlobalDatabase();
    ~GlobalDatabase();

    // Prevent copying
    GlobalDatabase(const GlobalDatabase&) = delete;
    GlobalDatabase& operator=(const GlobalDatabase&) = delete;

    /**
     * Initialize database schema
     * Called on first open of a new database
     */
    bool initSchema();

    /**
     * Migrate existing database schema
     * Adds missing columns/tables for backwards compatibility
     */
    bool migrateSchema();

    /**
     * Load schema SQL from embedded resource or file
     */
    QString loadSchemaSql();

    QSqlDatabase m_db;
    QString m_lastError;
    int m_lastInsertId{-1};
};

} // namespace TR4QT

#endif // GLOBALDATABASE_H
