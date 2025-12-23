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
 * Usage:
 *   Database& db = Database::instance();
 *   if (!db.open("path/to/database.db")) {
 *       // handle error
 *   }
 */
class Database {
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

    QSqlDatabase m_db;
    QString m_lastError;
    int m_lastInsertId{-1};
};

} // namespace TR4QT

#endif // DATABASE_H
