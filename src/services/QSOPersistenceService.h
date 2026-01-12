/**
 * QSOPersistenceService - Reliable QSO persistence with retry and emergency fallback
 *
 * Extracted from MainWindow::onLogQSO() as part of Phase 2 god class refactoring.
 *
 * Responsibility: Save QSOs to database with retry logic and emergency file fallback.
 *
 * Design: Service layer - no modal dialogs, returns result for UI to handle.
 * - Attempts database save with configurable retries
 * - Falls back to emergency ADIF file if database unavailable
 * - Returns detailed result for UI decision-making
 *
 * Why extracted:
 * - Database persistence is distinct from logging workflow
 * - Retry logic is complex (60+ lines)
 * - Emergency file I/O is distinct from database
 * - Testable independently (can mock QSORepository)
 * - Reusable for QSO edit, import operations
 */

#ifndef QSOPERSISTENCESERVICE_H
#define QSOPERSISTENCESERVICE_H

#include <QString>
#include "../models/QSO.h"

namespace TR4QT {

class QSORepository;

/**
 * Service for reliable QSO persistence with retry and emergency fallback
 *
 * Usage:
 *   QSOPersistenceService service(config);
 *   auto result = service.saveQSO(qso, contestDbId);
 *   if (result.status == SavedToDatabase) {
 *       // Success
 *   } else if (result.status == SavedToEmergencyFile) {
 *       // Show dialog: saved to emergency file
 *   } else {
 *       // Show error
 *   }
 */
class QSOPersistenceService {
public:
    /**
     * Configuration for persistence service
     */
    struct Config {
        QString appDataDir;     // Directory for emergency file (e.g., ~/.tr4qt)
        int maxRetries = 3;     // Max database save attempts before emergency fallback
    };

    /**
     * Result of save operation
     */
    struct SaveResult {
        enum Status {
            SavedToDatabase,        // Successfully saved to database
            SavedToEmergencyFile,   // Database failed, saved to emergency ADIF file
            Failed,                 // Both database and emergency file failed
            NeedsUserDecision       // Database failed, waiting for user choice (retry/emergency/stop)
        };

        Status status;
        QString errorMessage;           // Error details if failed
        QString emergencyFilePath;      // Path to emergency file if used
        int attemptCount;               // Number of save attempts made
        bool databaseAvailable;         // False if database is completely unavailable

        // Constructor for convenience
        SaveResult()
            : status(Failed), attemptCount(0), databaseAvailable(true) {}

        SaveResult(Status s, const QString& error = QString(), const QString& path = QString())
            : status(s), errorMessage(error), emergencyFilePath(path),
              attemptCount(0), databaseAvailable(true) {}
    };

    /**
     * Construct service with configuration
     * @param config Persistence configuration
     */
    explicit QSOPersistenceService(const Config& config);

    /**
     * Destructor
     */
    ~QSOPersistenceService();

    /**
     * Save QSO with retry and emergency fallback
     *
     * Attempts to save to database up to maxRetries times.
     * If all attempts fail, does NOT automatically save to emergency file.
     * Returns NeedsUserDecision status, allowing UI to show dialog and call
     * saveToEmergencyFile() if user chooses that option.
     *
     * NO MODAL DIALOGS - returns result for UI to handle
     *
     * @param qso QSO to save
     * @param contestDbId Database ID of active contest
     * @return SaveResult with status and details
     */
    SaveResult saveQSO(const QSO& qso, int contestDbId);

    /**
     * Save QSO to emergency ADIF file
     *
     * Called by UI when user chooses emergency file option after database failure.
     * Creates/appends to emergency_log.adi in appDataDir.
     *
     * @param qso QSO to save
     * @param filePath Output parameter - receives path where file was saved
     * @return true if saved successfully, false on error
     */
    bool saveToEmergencyFile(const QSO& qso, QString& filePath);

    /**
     * Get the last error message from repository
     * @return Error message
     */
    QString lastError() const;

private:
    Config m_config;
    QSORepository* m_repository;
    QString m_lastError;

    /**
     * Write ADIF header to emergency file
     * @param out Text stream to write to
     */
    void writeEmergencyFileHeader(QTextStream& out);

    /**
     * Write QSO record in ADIF format
     * @param out Text stream to write to
     * @param qso QSO to write
     */
    void writeADIFRecord(QTextStream& out, const QSO& qso);
};

} // namespace TR4QT

#endif // QSOPERSISTENCESERVICE_H
