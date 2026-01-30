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
 * WebServerContext - Headless context for WebServer operation
 *
 * This class decouples WebServer from MainWindow by providing all the
 * context needed to handle web API requests without a GUI.
 *
 * Responsibilities:
 * - Hold contest state (active contest, database ID, serial number)
 * - Hold operating mode (CQ vs S&P)
 * - Own services needed for QSO logging (QSOLoggingService, ContestManager)
 * - Maintain QSO list for duplicate/multiplier checking
 * - Handle web server signals (logQSORequested, commandRequested, contest API)
 *
 * Design:
 * - Can be used standalone for headless server (tr4qt_server)
 * - MainWindow can also use this internally for consistency
 * - NO UI dependencies - pure business logic
 *
 * Architecture:
 * - tr4qt-server creates WebServerContext + WebServer
 * - Connects signals between them
 * - WebServerContext handles all business logic
 * - Returns results for web responses
 */

#ifndef WEBSERVERCONTEXT_H
#define WEBSERVERCONTEXT_H

#include <QObject>
#include <QList>
#include <memory>
#include "../models/QSO.h"
#include "../core/Types.h"
#include "../contests/ContestBase.h"
#include "../utils/CountryFile.h"
#include "../models/ContestInfo.h"  // ContestInfo struct (shared with GUI)
#include "../interfaces/IQSODataSource.h"  // Interface for QSO data access
#include "../interfaces/IRadioCommandHandler.h"  // Interface for radio commands

// Include service headers needed for unique_ptr members
#include "../controllers/QSOLogger.h"
#include "../controllers/ContestManager.h"
#include "../services/QSOLoggingService.h"
#include "../services/QSOPersistenceService.h"
#include "../services/ExchangeMemoryService.h"
#include "../services/QSOLoggingCoordinator.h"

namespace TR4QT {

// Forward declarations for WebServer request/response structs
struct LogQSOWebRequest;
struct LogQSOWebResponse;
struct CommandWebRequest;
struct CommandWebResponse;

/**
 * Request structure for POST /api/contest/create
 */
struct CreateContestRequest {
    QString contestType;          // "CQWW_CW", "CQWPX_SSB", "WFD", etc.
    QString callsign;             // Station callsign (e.g., "K1TEST")
    QString exchangeSent;         // Sent exchange (e.g., "599 05")
    QString mode;                 // "CW", "SSB", "Mixed"
    QString category;             // "SINGLE-OP", "MULTI-OP", etc.
    QString powerClass;           // "HIGH", "LOW", "QRP"
    QString operatorName;         // Operator name for {NAME} substitution
};

/**
 * Response structure for POST /api/contest/create
 */
struct CreateContestResponse {
    bool success = false;
    QString error;
    int contestDbId = -1;
    int serialNumber = 1;
    QString contestName;
    QString databasePath;
};

/**
 * Request structure for POST /api/contest/open
 */
struct OpenContestRequest {
    QString databasePath;         // Full path to contest database file
};

/**
 * Response structure for POST /api/contest/open
 */
struct OpenContestResponse {
    bool success = false;
    QString error;
    int contestDbId = -1;
    QString contestName;
    QString contestType;
    int qsoCount = 0;
    int serialNumber = 1;
};

/**
 * Response structure for GET /api/contest/status
 */
struct ContestStatusResponse {
    bool active = false;
    QString contestId;
    QString contestName;
    QString contestType;
    int qsoCount = 0;
    int serialNumber = 1;
    int totalPoints = 0;
    int totalMultipliers = 0;
};

/**
 * Band breakdown entry for score response
 */
struct BandBreakdown {
    QString band;           // "160M", "80M", etc.
    QString mode;           // "CW", "SSB", "RTTY"
    int qsos = 0;
    int points = 0;
    int multipliers = 0;
};

/**
 * Response structure for GET /api/contest/score
 */
struct ScoreResponse {
    bool active = false;
    QString contestName;
    int totalQsos = 0;
    int totalPoints = 0;
    int totalMultipliers = 0;
    int score = 0;          // totalPoints * totalMultipliers (or contest-specific)
    QList<BandBreakdown> bandBreakdown;
};

/**
 * WebServerContext - Headless context for web API handling
 *
 * Owns contest state and services, handles all web API requests
 * without requiring MainWindow or any UI components.
 */
class WebServerContext : public QObject, public IQSODataSource {
    Q_OBJECT

public:
    /**
     * Configuration for WebServerContext
     */
    struct Config {
        QString appDataDir;           // Application data directory
        IRadioCommandHandler* radioHandler = nullptr;  // Optional: for radio commands (can be nullptr)
    };

    /**
     * Construct a WebServerContext
     * @param config Configuration with dependencies
     * @param parent Parent QObject
     */
    explicit WebServerContext(const Config& config, QObject* parent = nullptr);

    /**
     * Destructor
     */
    ~WebServerContext() override;

    // === Contest Management ===

    /**
     * Create a new contest
     * @param request Contest creation parameters
     * @return Response with success status and contest info
     */
    CreateContestResponse createContest(const CreateContestRequest& request);

    /**
     * Open an existing contest from database file
     * @param request Open contest parameters
     * @return Response with success status and contest info
     */
    OpenContestResponse openContest(const OpenContestRequest& request);

    /**
     * Close the active contest
     */
    void closeContest();

    /**
     * Get current contest status
     * @return Status response with contest info
     */
    ContestStatusResponse getContestStatus() const;

    /**
     * Get detailed score with band breakdown
     * @return Score response with per-band statistics
     */
    ScoreResponse getScore() const;

    /**
     * Generate Cabrillo export
     * @return Cabrillo-formatted text, or empty string if no contest
     */
    QString generateCabrillo() const;

    // === State Accessors ===

    /**
     * Check if a contest is active
     * @return true if contest is active
     */
    bool hasActiveContest() const { return m_hasActiveContest; }

    /**
     * Get the active contest instance
     * @return Contest instance or nullptr
     */
    ContestBase* activeContest() const { return m_activeContest.get(); }

    /**
     * Get the contest database ID
     * @return Database ID or -1 if no contest
     */
    int contestDbId() const { return m_contestDbId; }

    /**
     * Get the next serial number
     * @return Next serial number
     */
    int nextSerialNumber() const { return m_nextSerialNumber; }

    /**
     * Get the operating mode (CQ vs S&P)
     * @return Operating mode
     */
    OperatingMode operatingMode() const { return m_operatingMode; }

    /**
     * Set the operating mode
     * @param mode New operating mode
     */
    void setOperatingMode(OperatingMode mode) { m_operatingMode = mode; }

    /**
     * Get all QSOs for duplicate/multiplier checking
     * @return List of all logged QSOs
     */
    QList<QSO> getAllQSOs() const { return m_qsoList; }

    // IQSODataSource interface implementation
    int qsoCount() const override { return m_qsoList.size(); }
    QSO qsoAt(int index) const override {
        return (index >= 0 && index < m_qsoList.size()) ? m_qsoList.at(index) : QSO();
    }
    QList<QSO> allQSOs() const override { return m_qsoList; }

    /**
     * Get the CountryFile for DXCC lookups
     * @return Pointer to CountryFile
     */
    CountryFile* countryFile() { return &m_countryFile; }

public slots:
    /**
     * Handle log QSO request from web server
     * @param request Parsed log QSO request
     * @param response Response to populate
     */
    void onLogQSOFromWeb(const LogQSOWebRequest& request, LogQSOWebResponse* response);

    /**
     * Handle command request from web server
     * @param request Parsed command request
     * @param response Response to populate
     */
    void onCommandFromWeb(const CommandWebRequest& request, CommandWebResponse* response);

signals:
    /**
     * Emitted when a QSO is logged
     * @param qso The logged QSO
     */
    void qsoLogged(const QSO& qso);

    /**
     * Emitted when contest is activated
     * @param contestName Name of the contest
     */
    void contestActivated(const QString& contestName);

    /**
     * Emitted when contest is closed
     */
    void contestClosed();

private:
    /**
     * Initialize services for QSO logging
     */
    void initializeServices();

    /**
     * Reconfigure QSOLogger with current contest and station info
     */
    void reconfigureQSOLogger();

    /**
     * Build station info from settings and country file
     * @return Station info
     */
    StationInfo buildStationInfo() const;

    /**
     * Calculate score from QSO list
     * @param totalPoints Output: total QSO points
     * @param totalMults Output: total multipliers
     */
    void calculateScore(int& totalPoints, int& totalMults) const;

    // Configuration
    Config m_config;

    // Country file (for DXCC lookups)
    CountryFile m_countryFile;

    // Contest state
    bool m_hasActiveContest = false;
    std::unique_ptr<ContestBase> m_activeContest;
    int m_contestDbId = -1;
    int m_nextSerialNumber = 1;
    OperatingMode m_operatingMode = OperatingMode::CQ;
    ContestInfo m_contestInfo;

    // Station info
    StationInfo m_myStation;
    QString m_operatorName;

    // QSO list (replaces QSOTableModel for headless operation)
    QList<QSO> m_qsoList;

    // Services
    std::unique_ptr<ContestManager> m_contestManager;
    std::unique_ptr<QSOLogger> m_qsoLogger;
    std::unique_ptr<QSOPersistenceService> m_persistenceService;
    std::unique_ptr<ExchangeMemoryService> m_exchangeMemoryService;
    std::unique_ptr<QSOLoggingCoordinator> m_loggingCoordinator;
    std::unique_ptr<QSOLoggingService> m_loggingService;
};

} // namespace TR4QT

#endif // WEBSERVERCONTEXT_H
