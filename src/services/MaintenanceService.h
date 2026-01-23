/**
 * MaintenanceService - Log maintenance operations
 *
 * Extracted from MainWindow::onClearLog() as part of god class refactoring.
 *
 * Responsibility: Handle log maintenance workflows including:
 * - Clear log with optional backup
 * - Backup creation
 * - Exchange memory clearing
 *
 * Design: Use-case pattern - receives request, performs workflow, returns result
 * - NO UI interactions (caller handles dialogs based on result)
 * - Returns structured result for UI to handle
 * - Testable without UI
 */

#ifndef MAINTENANCESERVICE_H
#define MAINTENANCESERVICE_H

#include <QString>
#include <QWidget>

namespace TR4QT {

// Forward declarations
class ContestBase;

/**
 * Request parameters for clear log operation
 */
struct ClearLogRequest {
    int contestDbId = -1;           // Contest to clear
    QString contestType;            // Contest type for exchange memory (e.g., "CQWW")
    QString databasePath;           // Database path for backup
    int qsoCount = 0;               // Number of QSOs to clear (for confirmation message)
};

/**
 * Result from clear log operation
 */
struct ClearLogResult {
    enum class Status {
        Success,                    // Log cleared successfully
        UserCancelled,              // User cancelled the operation
        BackupFailed,               // Backup creation failed (user may continue)
        BackupFailedUserAborted,    // Backup failed and user chose not to continue
        ClearFailed,                // Database clear failed
        AlreadyEmpty                // Log was already empty
    };

    Status status = Status::UserCancelled;
    bool backupCreated = false;     // True if backup was created
    QString backupPath;             // Path to backup file (if created)
    QString errorMessage;           // Error details (if failed)
};

/**
 * Service for log maintenance operations
 */
class MaintenanceService {
public:
    /**
     * Construct service
     * @param parentWidget Parent for dialogs (can be nullptr for headless testing)
     */
    explicit MaintenanceService(QWidget* parentWidget = nullptr);

    /**
     * Clear log with optional backup
     *
     * Workflow:
     * 1. Check if log is empty (return AlreadyEmpty if so)
     * 2. Ask user about backup (return UserCancelled if cancelled)
     * 3. Create backup if requested (may return BackupFailed*)
     * 4. Confirm clear with user (return UserCancelled if declined)
     * 5. Clear QSOs from database
     * 6. Clear exchange memory
     *
     * @param request Clear log parameters
     * @return Result with status, backup info, and any error details
     *
     * NOTE: This method shows dialogs. For headless operation, use clearLogHeadless().
     */
    ClearLogResult clearLogWithBackup(const ClearLogRequest& request);

    /**
     * Clear log without dialogs (for testing/batch operations)
     *
     * @param request Clear log parameters
     * @param createBackup If true, create backup before clearing
     * @return Result with status and any error details
     */
    ClearLogResult clearLogHeadless(const ClearLogRequest& request, bool createBackup);

    /**
     * Create backup only (without clearing)
     *
     * @param databasePath Path to database to backup
     * @param backupPath Output: path to created backup
     * @return true if backup created successfully
     */
    bool createBackup(const QString& databasePath, QString& backupPath);

private:
    QWidget* m_parentWidget;

    /**
     * Clear QSOs from database
     * @return true if successful
     */
    bool clearDatabase(int contestDbId, QString& errorMessage);

    /**
     * Clear exchange memory for contest
     */
    void clearExchangeMemory(const QString& contestType);
};

} // namespace TR4QT

#endif // MAINTENANCESERVICE_H
