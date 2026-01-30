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

#ifndef DATABASETRANSACTION_H
#define DATABASETRANSACTION_H

#include <QString>

namespace TR4QT {

/**
 * RAII wrapper for database transactions.
 * Automatically rolls back on destruction if not committed.
 *
 * Template parameter DbType must have:
 * - bool beginTransaction()
 * - bool commitTransaction()
 * - bool rollbackTransaction()
 * - QString lastError() const
 *
 * Usage:
 *   DatabaseTransaction txn(db);
 *   if (!txn.begin()) return false;
 *   // ... do work ...
 *   return txn.commit();  // Auto-rollback if exception or early return
 */
template<typename DbType>
class DatabaseTransaction {
public:
    explicit DatabaseTransaction(DbType& db)
        : m_db(db)
        , m_active(false)
        , m_committed(false)
    {
    }

    ~DatabaseTransaction() {
        // RAII: Auto-rollback if transaction was begun but not committed
        if (m_active && !m_committed) {
            m_db.rollbackTransaction();
        }
    }

    // Delete copy/move to prevent multiple transactions on same instance
    DatabaseTransaction(const DatabaseTransaction&) = delete;
    DatabaseTransaction& operator=(const DatabaseTransaction&) = delete;

    /**
     * Begin the transaction
     * @return true if successful, false on error
     */
    bool begin() {
        if (m_active) {
            m_lastError = "Transaction already active";
            return false;
        }

        if (!m_db.beginTransaction()) {
            m_lastError = "Failed to begin transaction: " + m_db.lastError();
            return false;
        }

        m_active = true;
        return true;
    }

    /**
     * Commit the transaction
     * @return true if successful, false on error (auto-rollback on failure)
     */
    bool commit() {
        if (!m_active) {
            m_lastError = "No active transaction to commit";
            return false;
        }

        if (m_committed) {
            m_lastError = "Transaction already committed";
            return false;
        }

        if (!m_db.commitTransaction()) {
            m_lastError = "Failed to commit transaction: " + m_db.lastError();
            // Auto-rollback on commit failure
            m_db.rollbackTransaction();
            m_active = false;
            return false;
        }

        m_committed = true;
        m_active = false;
        return true;
    }

    /**
     * Explicitly rollback (usually not needed due to auto-rollback in destructor)
     */
    void rollback() {
        if (m_active && !m_committed) {
            m_db.rollbackTransaction();
            m_active = false;
        }
    }

    /**
     * Get the last error message
     */
    QString lastError() const { return m_lastError; }

private:
    DbType& m_db;
    bool m_active;      // Transaction was begun
    bool m_committed;   // Transaction was committed
    QString m_lastError;
};

} // namespace TR4QT

#endif // DATABASETRANSACTION_H
