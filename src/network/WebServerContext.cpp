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
 * Implementation of the headless context that decouples WebServer from MainWindow.
 */

#include "WebServerContext.h"
#include "WebServer.h"  // For LogQSOWebRequest/Response, CommandWebRequest/Response structs

// Full includes for types used with unique_ptr and nested types
#include "../controllers/ContestManager.h"
#include "../controllers/QSOLogger.h"
#include "../services/QSOLoggingService.h"
#include "../services/QSOPersistenceService.h"
#include "../services/ExchangeMemoryService.h"
#include "../services/QSOLoggingCoordinator.h"
#include "../services/QSOPersistenceService.h"
#include "../services/ExchangeMemoryService.h"
#include "../services/QSOLoggingCoordinator.h"
#include "../data/QSORepository.h"
#include "../utils/AppSettings.h"
#include "../utils/PathManager.h"
#include "../utils/CabrilloExporter.h"
#include "../logging/LogMacros.h"
#include "../contests/ContestRegistry.h"

namespace TR4QT {

WebServerContext::WebServerContext(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    LOG_INFO("WebServerContext", "Initializing headless context");

    // Load country file - try filesystem first, then Qt resources
    QString ctyPath = PathManager::getCountryFilePath();
    if (m_countryFile.loadFromFile(ctyPath)) {
        LOG_INFO("WebServerContext", QString("Loaded country file from: %1 (version: %2)")
                 .arg(ctyPath).arg(m_countryFile.getVersion()));
    } else {
        // Fallback to Qt resources (embedded cty.dat)
        QString resourcePath = ":/data/cty.dat";
        if (m_countryFile.loadFromFile(resourcePath)) {
            LOG_INFO("WebServerContext", QString("Loaded country file from Qt resources (version: %1)")
                     .arg(m_countryFile.getVersion()));
        } else {
            LOG_ERROR("WebServerContext", "Failed to load country file from any source");
        }
    }

    // Initialize services
    initializeServices();

    LOG_INFO("WebServerContext", "Headless context initialized");
}

WebServerContext::~WebServerContext() {
    LOG_INFO("WebServerContext", "Shutting down headless context");

    // Close any active contest
    if (m_hasActiveContest) {
        closeContest();
    }
}

void WebServerContext::initializeServices() {
    // Create contest manager
    ContestManager::Config cmConfig;
    cmConfig.countryFile = &m_countryFile;
    m_contestManager = std::make_unique<ContestManager>(cmConfig);

    // Create persistence service
    QSOPersistenceService::Config psConfig;
    psConfig.appDataDir = m_config.appDataDir.isEmpty()
        ? PathManager::getAppDataDir()
        : m_config.appDataDir;
    psConfig.maxRetries = 3;
    m_persistenceService = std::make_unique<QSOPersistenceService>(psConfig);

    // Create exchange memory service
    m_exchangeMemoryService = std::make_unique<ExchangeMemoryService>();

    // Create logging coordinator (no managers for headless - skip UDP/backup/integrity)
    m_loggingCoordinator = std::make_unique<QSOLoggingCoordinator>(
        nullptr,  // No UDP manager
        nullptr,  // No backup manager
        nullptr   // No integrity manager
    );

    // QSOLogger and QSOLoggingService will be created when contest is activated
    // (they need contest context)
}

void WebServerContext::reconfigureQSOLogger() {
    if (!m_activeContest) {
        m_qsoLogger.reset();
        m_loggingService.reset();
        return;
    }

    // Create QSOLogger with current contest and station info
    QSOLogger::Config loggerConfig;
    loggerConfig.contest = m_activeContest.get();
    loggerConfig.countryFile = &m_countryFile;
    loggerConfig.myStation = m_myStation;
    loggerConfig.operatorName = m_operatorName;
    m_qsoLogger = std::make_unique<QSOLogger>(loggerConfig);

    // Create QSOLoggingService with all dependencies
    QSOLoggingService::Dependencies deps;
    deps.qsoLogger = m_qsoLogger.get();
    deps.persistenceService = m_persistenceService.get();
    deps.exchangeMemoryService = m_exchangeMemoryService.get();
    deps.coordinator = m_loggingCoordinator.get();
    m_loggingService = std::make_unique<QSOLoggingService>(deps);
}

StationInfo WebServerContext::buildStationInfo() const {
    StationInfo info;

    AppSettings& settings = AppSettings::instance();
    info.callsign = settings.getMyCallsign();

    // Look up our own location from country file
    if (!info.callsign.isEmpty()) {
        CountryData myCountry = m_countryFile.lookup(info.callsign);
        if (myCountry.isValid()) {
            info.country = myCountry.name;
            info.continent = continentToString(myCountry.continent);
            info.cqZone = myCountry.cqZone;
            info.ituZone = myCountry.ituZone;
            info.dxccEntity = myCountry.dxccEntity;
        }
    }

    return info;
}

void WebServerContext::calculateScore(int& totalPoints, int& totalMults) const {
    totalPoints = 0;
    totalMults = 0;

    for (const QSO& qso : m_qsoList) {
        totalPoints += qso.qsoPoints;
        if (qso.isMultiplier) {
            totalMults++;
        }
    }
}

// === Contest Management ===

CreateContestResponse WebServerContext::createContest(const CreateContestRequest& request) {
    CreateContestResponse response;

    LOG_INFO("WebServerContext", QString("Creating contest: type=%1 callsign=%2")
             .arg(request.contestType).arg(request.callsign));

    // Close any existing contest first
    if (m_hasActiveContest) {
        closeContest();
    }

    // Build ContestInfo from request
    ContestInfo contestInfo;
    contestInfo.contestType = request.contestType;
    contestInfo.contestId = QString("%1_%2")
        .arg(request.contestType)
        .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss"));
    contestInfo.contestName = QString("%1 %2")
        .arg(request.contestType)
        .arg(QDateTime::currentDateTimeUtc().toString("yyyy"));
    contestInfo.startDate = QDateTime::currentDateTimeUtc();
    contestInfo.mode = request.mode;
    contestInfo.exchangeSent = request.exchangeSent;
    contestInfo.category = request.category;
    contestInfo.powerClass = request.powerClass;
    contestInfo.operatorName = request.operatorName;
    contestInfo.isExisting = false;

    // Generate database path
    contestInfo.databasePath = PathManager::getLogsDir() + "/" + contestInfo.contestId + ".db";

    // Set callsign in settings if provided
    if (!request.callsign.isEmpty()) {
        AppSettings::instance().setMyCallsign(request.callsign);
    }

    // Activate contest via ContestManager
    ActivateContestResult result = m_contestManager->activateContest(contestInfo);

    if (!result.success) {
        response.success = false;
        response.error = result.errorMessage;
        LOG_ERROR("WebServerContext", QString("Failed to create contest: %1").arg(result.errorMessage));
        return response;
    }

    // Take ownership of contest instance
    m_activeContest.reset(result.contest);
    m_contestDbId = result.contestDbId;
    m_nextSerialNumber = result.nextSerialNumber;
    m_contestInfo = contestInfo;
    m_hasActiveContest = true;

    // Store station info
    m_myStation = result.myStation;
    m_operatorName = result.operatorName;

    // Load QSOs (should be empty for new contest)
    m_qsoList = result.loadedQSOs;

    // Reconfigure services with new contest
    reconfigureQSOLogger();

    // Build response
    response.success = true;
    response.contestDbId = m_contestDbId;
    response.serialNumber = m_nextSerialNumber;
    response.contestName = contestInfo.contestName;
    response.databasePath = contestInfo.databasePath;

    LOG_INFO("WebServerContext", QString("Contest created: %1 (dbId=%2)")
             .arg(contestInfo.contestName).arg(m_contestDbId));

    emit contestActivated(contestInfo.contestName);

    return response;
}

OpenContestResponse WebServerContext::openContest(const OpenContestRequest& request) {
    OpenContestResponse response;

    LOG_INFO("WebServerContext", QString("Opening contest: %1").arg(request.databasePath));

    // Close any existing contest first
    if (m_hasActiveContest) {
        closeContest();
    }

    // Build ContestInfo for opening existing database
    ContestInfo contestInfo;
    contestInfo.databasePath = request.databasePath;
    contestInfo.isExisting = true;

    // Activate contest via ContestManager
    ActivateContestResult result = m_contestManager->activateContest(contestInfo);

    if (!result.success) {
        response.success = false;
        response.error = result.errorMessage;
        LOG_ERROR("WebServerContext", QString("Failed to open contest: %1").arg(result.errorMessage));
        return response;
    }

    // Take ownership of contest instance
    m_activeContest.reset(result.contest);
    m_contestDbId = result.contestDbId;
    m_nextSerialNumber = result.nextSerialNumber;
    m_contestInfo = contestInfo;
    m_hasActiveContest = true;

    // Store station info
    m_myStation = result.myStation;
    m_operatorName = result.operatorName;

    // Load QSOs
    m_qsoList = result.loadedQSOs;

    // Reconfigure services with new contest
    reconfigureQSOLogger();

    // Build response
    response.success = true;
    response.contestDbId = m_contestDbId;
    response.contestName = m_activeContest ? m_activeContest->getContestName() : "";
    response.contestType = m_activeContest ? m_activeContest->getContestId() : "";
    response.qsoCount = m_qsoList.size();
    response.serialNumber = m_nextSerialNumber;

    LOG_INFO("WebServerContext", QString("Contest opened: %1 (dbId=%2, %3 QSOs)")
             .arg(response.contestName).arg(m_contestDbId).arg(response.qsoCount));

    emit contestActivated(response.contestName);

    return response;
}

void WebServerContext::closeContest() {
    if (!m_hasActiveContest) {
        return;
    }

    LOG_INFO("WebServerContext", "Closing contest");

    // Reset state
    m_activeContest.reset();
    m_contestDbId = -1;
    m_nextSerialNumber = 1;
    m_hasActiveContest = false;
    m_qsoList.clear();
    m_contestInfo = ContestInfo();

    // Reset services
    m_qsoLogger.reset();
    m_loggingService.reset();

    emit contestClosed();
}

ContestStatusResponse WebServerContext::getContestStatus() const {
    ContestStatusResponse status;

    status.active = m_hasActiveContest;

    if (m_hasActiveContest && m_activeContest) {
        status.contestId = m_activeContest->getContestId();
        status.contestName = m_activeContest->getContestName();
        status.contestType = m_activeContest->getContestId();
        status.qsoCount = m_qsoList.size();
        status.serialNumber = m_nextSerialNumber;

        // Calculate score
        int points = 0;
        int mults = 0;
        calculateScore(points, mults);
        status.totalPoints = points;
        status.totalMultipliers = mults;
    }

    return status;
}

ScoreResponse WebServerContext::getScore() const {
    ScoreResponse response;

    response.active = m_hasActiveContest;

    if (!m_hasActiveContest || !m_activeContest) {
        return response;
    }

    response.contestName = m_activeContest->getContestName();

    // Build band breakdown from QSO list
    QMap<QPair<BandType, ModeType>, BandBreakdown> breakdown;

    for (const QSO& qso : m_qsoList) {
        auto key = qMakePair(qso.band, qso.mode);

        if (!breakdown.contains(key)) {
            BandBreakdown entry;
            entry.band = bandToString(qso.band);
            entry.mode = modeToString(qso.mode);
            breakdown[key] = entry;
        }

        BandBreakdown& entry = breakdown[key];
        entry.qsos++;
        entry.points += qso.qsoPoints;

        // Count multipliers
        if (qso.isMultiplier) {
            entry.multipliers++;
        }
    }

    // Convert to list and calculate totals
    for (auto it = breakdown.begin(); it != breakdown.end(); ++it) {
        response.bandBreakdown.append(it.value());
        response.totalQsos += it.value().qsos;
        response.totalPoints += it.value().points;
        response.totalMultipliers += it.value().multipliers;
    }

    // Calculate score (points * multipliers for most contests)
    response.score = response.totalPoints * response.totalMultipliers;

    return response;
}

QString WebServerContext::generateCabrillo() const {
    if (!m_hasActiveContest || !m_activeContest) {
        return QString();
    }

    CabrilloExporter exporter;

    // Set station info from m_myStation (populated during contest creation)
    exporter.setStationInfo(
        m_myStation.callsign,
        m_myStation.grid,
        m_contestInfo.operatorName,
        QString(),  // address
        QString(),  // city
        QString(),  // state
        QString(),  // postal
        QString(),  // country
        QString()   // email
    );

    // Set category from contest info
    // m_contestInfo.mode is QString ("CW", "SSB", "Mixed")
    QString cabrilloMode = m_contestInfo.mode;
    if (cabrilloMode.isEmpty()) {
        cabrilloMode = "MIXED";
    }

    exporter.setCategory(
        "NON-ASSISTED",                           // assisted
        "ALL",                                    // band
        cabrilloMode,                             // mode
        m_contestInfo.category,                   // operator category
        m_contestInfo.powerClass,                 // power
        "FIXED",                                  // station
        QString(),                                // time
        "ONE",                                    // transmitter
        QString()                                 // overlay
    );

    // Calculate and set score
    int points = 0;
    int mults = 0;
    calculateScore(points, mults);
    exporter.setClaimedScore(points * mults);

    exporter.setOperators(m_myStation.callsign);

    return exporter.generateCabrillo(m_qsoList, m_activeContest.get());
}

// === Web Server Signal Handlers ===

void WebServerContext::onLogQSOFromWeb(const LogQSOWebRequest& request, LogQSOWebResponse* response) {
    LOG_DEBUG("WebServerContext", QString("Processing log QSO from web: %1").arg(request.callsign));

    // Check for active contest
    if (!m_loggingService || !m_hasActiveContest) {
        response->success = false;
        response->error = "No active contest - create or open a contest first";
        LOG_WARN("WebServerContext", "Web log QSO rejected: no active contest");
        return;
    }

    // Build the logging service request
    QSOLoggingService::LogQSORequest loggingRequest;

    // Basic QSO data
    loggingRequest.callsign = request.callsign;
    loggingRequest.exchange = request.exchange;

    // Build radio state from request or defaults
    RadioState radioState;
    radioState.frequencyA = request.frequency > 0 ? request.frequency : 14000000;  // Default 20m
    radioState.bandA = request.band != BandType::None ? request.band : frequencyToBand(radioState.frequencyA);

    // Default mode: use request mode, else contest mode, else CW
    if (request.mode != ModeType::None) {
        radioState.modeA = request.mode;
    } else if (!m_contestInfo.mode.isEmpty() && m_contestInfo.mode != "MIXED") {
        radioState.modeA = stringToMode(m_contestInfo.mode);
    } else {
        radioState.modeA = ModeType::CW;  // Fallback for MIXED or unknown
    }

    // If we have a radio handler, get actual radio state
    if (m_config.radioHandler && m_config.radioHandler->isRadioAvailable()) {
        RadioState actualState = m_config.radioHandler->getCurrentRadioState();
        if (request.frequency == 0) radioState.frequencyA = actualState.frequencyA;
        if (request.band == BandType::None) radioState.bandA = actualState.bandA;
        if (request.mode == ModeType::None) radioState.modeA = actualState.modeA;
    }

    loggingRequest.radioState = radioState;
    loggingRequest.operatorCallsign = AppSettings::instance().getCurrentOperator();
    loggingRequest.serialNumber = m_nextSerialNumber;
    loggingRequest.operatingMode = m_operatingMode;
    loggingRequest.radioNumber = 1;

    // Existing QSOs for duplicate/multiplier checking
    loggingRequest.existingQSOs = m_qsoList;

    // Exchange memory settings - don't save from web API
    loggingRequest.saveExchangeMemory = false;
    loggingRequest.autoPopulated = true;

    // Context for post-logging actions
    loggingRequest.stationCallsign = AppSettings::instance().getMyCallsign();
    loggingRequest.adifContestId = m_activeContest ? m_activeContest->getADIFContestId() : "";
    loggingRequest.wa7bnmContestId = m_activeContest ? m_activeContest->getWA7BNMContestId() : 0;
    loggingRequest.contestId = m_activeContest ? m_activeContest->getContestId() : "";
    loggingRequest.databasePath = m_contestInfo.databasePath;
    loggingRequest.totalQSOCount = m_qsoList.size() + 1;
    loggingRequest.qsosSinceLastCheck = 0;  // Skip integrity check in headless mode
    loggingRequest.contestDbId = m_contestDbId;
    loggingRequest.memoryQSOCount = m_qsoList.size() + 1;

    // Execute logging workflow
    QSOLoggingService::LogQSOResult result = m_loggingService->logQSO(loggingRequest);

    // Map result to web response
    if (!result.success) {
        response->success = false;
        response->error = result.errorMessage;

        // Map error field
        using ErrorField = QSOLoggingService::ErrorField;
        switch (result.errorField) {
            case ErrorField::Callsign:
                response->errorField = "callsign";
                break;
            case ErrorField::Exchange:
                response->errorField = "exchange";
                break;
            case ErrorField::Frequency:
                response->errorField = "frequency";
                break;
            case ErrorField::Mode:
                response->errorField = "mode";
                break;
            case ErrorField::Database:
                response->errorField = "database";
                break;
            default:
                break;
        }

        LOG_WARN("WebServerContext", QString("Web log QSO failed: %1").arg(result.errorMessage));
        return;
    }

    // Success - update internal state
    m_qsoList.append(result.qso);
    m_nextSerialNumber = result.updatedSerialNumber;

    // Populate response with QSO details
    response->success = true;
    response->qsoId = result.qso.id;
    response->callsign = result.qso.callsign;
    response->timestamp = result.qso.timestamp;
    response->frequency = result.qso.frequency;
    response->band = bandToString(result.qso.band);
    response->mode = modeToString(result.qso.mode);
    response->exchangeSent = result.qso.exchangeSent;
    response->exchangeReceived = result.qso.exchangeReceived;
    response->points = result.qso.qsoPoints;
    response->isMultiplier = result.isNewMultiplier;
    response->isDuplicate = result.isDuplicate;
    response->serialNumber = result.updatedSerialNumber;

    LOG_INFO("WebServerContext", QString("Web log QSO success: %1 on %2 %3")
             .arg(result.qso.callsign)
             .arg(bandToString(result.qso.band))
             .arg(modeToString(result.qso.mode)));

    emit qsoLogged(result.qso);
}

void WebServerContext::onCommandFromWeb(const CommandWebRequest& request, CommandWebResponse* response) {
    LOG_DEBUG("WebServerContext", QString("Processing command from web: %1").arg(request.command));

    response->command = request.command;

    // For headless mode, we support a limited set of commands
    // Radio commands require RadioManager to be configured

    if (request.command == "toggle-run-mode") {
        // Toggle between CQ and S&P modes
        if (m_operatingMode == OperatingMode::CQ) {
            m_operatingMode = OperatingMode::SP;
            response->success = true;
            response->message = "Switched to S&P mode";
        } else {
            m_operatingMode = OperatingMode::CQ;
            response->success = true;
            response->message = "Switched to CQ mode";
        }
        return;
    }

    // Radio commands - delegate to IRadioCommandHandler
    if (request.command == "set-frequency") {
        if (!m_config.radioHandler || !m_config.radioHandler->isRadioAvailable()) {
            response->success = false;
            response->error = "Radio not available";
            return;
        }

        QVariant freqParam = request.params.value("frequency");
        if (!freqParam.isValid()) {
            response->success = false;
            response->error = "Missing 'frequency' parameter";
            return;
        }

        freq_t frequency = static_cast<freq_t>(freqParam.toLongLong());
        if (frequency < 100000 || frequency > 500000000) {
            response->success = false;
            response->error = QString("Invalid frequency: %1 Hz").arg(frequency);
            return;
        }

        if (m_config.radioHandler->setFrequency(frequency)) {
            response->success = true;
            response->message = QString("Frequency set to %1 MHz")
                .arg(frequency / 1000000.0, 0, 'f', 3);
        } else {
            response->success = false;
            response->error = "Failed to set frequency";
        }
        return;
    }

    if (request.command == "set-band") {
        if (!m_config.radioHandler || !m_config.radioHandler->isRadioAvailable()) {
            response->success = false;
            response->error = "Radio not available";
            return;
        }

        QString bandStr = request.params.value("band").toString().toUpper();
        if (bandStr.isEmpty()) {
            response->success = false;
            response->error = "Missing 'band' parameter";
            return;
        }

        BandType band = stringToBand(bandStr);
        if (band == BandType::None) {
            response->success = false;
            response->error = QString("Invalid band: %1").arg(bandStr);
            return;
        }

        if (m_config.radioHandler->setBand(band)) {
            response->success = true;
            response->message = QString("Band set to %1").arg(bandStr);
        } else {
            response->success = false;
            response->error = "Failed to set band";
        }
        return;
    }

    if (request.command == "set-mode") {
        if (!m_config.radioHandler || !m_config.radioHandler->isRadioAvailable()) {
            response->success = false;
            response->error = "Radio not available";
            return;
        }

        QString modeStr = request.params.value("mode").toString().toUpper();
        if (modeStr.isEmpty()) {
            response->success = false;
            response->error = "Missing 'mode' parameter";
            return;
        }

        ModeType mode = stringToMode(modeStr);
        if (mode == ModeType::None) {
            response->success = false;
            response->error = QString("Invalid mode: %1").arg(modeStr);
            return;
        }

        if (m_config.radioHandler->setMode(mode)) {
            response->success = true;
            response->message = QString("Mode set to %1").arg(modeStr);
        } else {
            response->success = false;
            response->error = "Failed to set mode";
        }
        return;
    }

    // Unknown command
    response->success = false;
    response->error = QString("Unknown command: %1").arg(request.command);
}

} // namespace TR4QT
