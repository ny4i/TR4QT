/**
 * ContestService - Business logic for contest operations
 *
 * Extracted from MainWindow as part of Phase 1 CLAUDE.md compliance refactoring.
 * Orchestrates contest-level operations using repository pattern.
 *
 * Design: Service layer - contains business logic but no SQL queries or UI code.
 */

#ifndef CONTESTSERVICE_H
#define CONTESTSERVICE_H

#include <QString>

namespace TR4QT {

class ContestBase;
class QSOTableModel;
class ContestRepository;
class QSORepository;

/**
 * Service for contest-level business operations
 *
 * Handles workflows that span multiple repositories or require business logic.
 * Keeps MainWindow thin by extracting complex contest operations.
 */
class ContestService {
public:
    /**
     * Configuration for ContestService
     * Uses dependency injection pattern - caller provides dependencies
     */
    struct Config {
        ContestBase* activeContest{nullptr};   // Current active contest instance
        QSOTableModel* qsoTableModel{nullptr}; // Model for updating UI (optional)
        int currentContestDbId{-1};            // Database ID of active contest
    };

    /**
     * Result of updateContestExchange operation
     */
    struct UpdateExchangeResult {
        bool success{false};
        QString errorMessage;
        QString statusMessage;
        int qsosUpdated{0};
    };

    /**
     * Construct with initial configuration
     * @param config Dependencies and current state
     */
    explicit ContestService(const Config& config);
    ~ContestService();

    /**
     * Update configuration (e.g., when contest changes)
     * @param config New configuration
     */
    void updateConfig(const Config& config);

    /**
     * Update contest exchange and all QSO records
     *
     * Extracted from MainWindow::onEditContestSettings()
     *
     * @param newExchange New exchange string
     * @return Result with success/error status and count of updated QSOs
     */
    UpdateExchangeResult updateContestExchange(const QString& newExchange);

private:
    Config m_config;
    ContestRepository* m_contestRepository;
    QSORepository* m_qsoRepository;
};

} // namespace TR4QT

#endif // CONTESTSERVICE_H
