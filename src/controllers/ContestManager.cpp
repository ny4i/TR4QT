#include "ContestManager.h"
#include "../data/Database.h"
#include "../data/QSORepository.h"
#include "../contests/ContestRegistry.h"
#include "../utils/AppSettings.h"
#include "../utils/CountryFile.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QDateTime>

using namespace TR4QT;

ContestManager::ContestManager(const Config& config)
    : m_countryFile(config.countryFile)
{
    LOG_DEBUG("ContestManager", "ContestManager created");
}

ContestManager::~ContestManager()
{
    LOG_DEBUG("ContestManager", "ContestManager destroyed");
}

ActivateContestResult ContestManager::activateContest(const ContestInfo& contestInfo)
{
    ActivateContestResult result;

    // Open database
    Database& db = Database::instance();
    if (!db.open(contestInfo.databasePath)) {
        result.success = false;
        result.errorMessage = QString("Failed to open database:\n%1").arg(db.lastError());
        return result;
    }

    LOG_DEBUG("ContestManager", QString("Database opened: %1").arg(contestInfo.databasePath));

    // Find or create contest record, load existing QSOs, populate config fields
    if (!findOrCreateContestRecord(contestInfo, result)) {
        result.success = false;
        result.errorMessage = QString("Failed to create/load contest record:\n%1").arg(db.lastError());
        return result;
    }

    // Build station info from settings and cty.dat
    result.myStation = buildStationInfo();

    // Determine contest mode
    ModeType contestMode = determineModeType(contestInfo);

    // Create contest instance via registry
    result.contest = ContestRegistry::instance().createContest(
        contestInfo.contestType,
        contestMode,
        result.myStation);

    if (!result.contest) {
        result.success = false;
        result.errorMessage = QString("Failed to create contest: %1\nCheck that the contest type is registered.").arg(contestInfo.contestType);
        LOG_WARN("ContestManager", QString("Failed to create contest: %1").arg(contestInfo.contestType));
        return result;
    }

    // Configure contest with sent exchange from database
    result.contest->setExchangeSent(result.exchangeSent);

    // Extract exchange field definitions
    result.receivedFields = result.contest->getReceivedExchangeFields();
    result.sentFields = result.contest->getSentExchangeFields();
    result.tableColumns = result.contest->getTableColumns();

    // Extract contest capabilities for UI configuration
    result.usesMultipliers = result.contest->usesMultipliers();
    result.usesModeGroupBreakdown = result.contest->usesModeGroupBreakdown();
    result.allowedBands = result.contest->getAllowedBands();
    result.multiplierTypes = result.contest->getMultiplierTypes();
    result.usesZones = contestUsesZones(result.contest);

    result.success = true;
    LOG_DEBUG("ContestManager", QString("Contest activated: %1 (DB ID: %2, %3 QSOs loaded)")
        .arg(contestInfo.contestName)
        .arg(result.contestDbId)
        .arg(result.loadedQSOs.size()));

    return result;
}

bool ContestManager::findOrCreateContestRecord(
    const ContestInfo& contestInfo,
    ActivateContestResult& result)
{
    Database& db = Database::instance();

    // Try to find existing contest record
    QSqlQuery query = db.execute(
        "SELECT id, current_serial, exchange_sent, category, power_class, assisted, operator_name "
        "FROM contests WHERE contest_id = ?",
        {contestInfo.contestId});

    if (query.next()) {
        // Existing contest - load data
        result.contestDbId = query.value(0).toInt();
        result.nextSerialNumber = query.value(1).toInt();
        result.exchangeSent = query.value(2).toString();
        result.category = query.value(3).toString();
        result.powerClass = query.value(4).toString();
        result.assisted = query.value(5).toString();
        result.operatorName = query.value(6).toString();

        LOG_DEBUG("ContestManager", QString("Resumed contest with DB ID: %1 next serial: %2 category: %3")
            .arg(result.contestDbId)
            .arg(result.nextSerialNumber)
            .arg(result.category));

        // Load existing QSOs from database
        QSORepository repo;
        result.loadedQSOs = repo.findByContest(result.contestDbId);
        LOG_DEBUG("ContestManager", QString("Loaded %1 existing QSOs").arg(result.loadedQSOs.size()));

        // Calculate next serial number from loaded QSOs
        // This ensures we don't reuse serial numbers even if current_serial in DB is out of sync
        if (!result.loadedQSOs.isEmpty()) {
            result.nextSerialNumber = calculateNextSerialNumber(result.loadedQSOs);
            LOG_DEBUG("ContestManager", QString("Calculated next serial number from QSOs: %1")
                .arg(result.nextSerialNumber));
        }

    } else {
        // New contest - create record
        AppSettings& settings = AppSettings::instance();
        QDateTime now = QDateTime::currentDateTimeUtc();

        // Use values from contestInfo (from ContestChooserDialog)
        result.exchangeSent = contestInfo.exchangeSent;
        result.category = contestInfo.category;
        result.powerClass = contestInfo.powerClass;
        result.assisted = contestInfo.assisted;
        result.operatorName = contestInfo.operatorName;

        query = db.execute(
            "INSERT INTO contests (contest_id, contest_name, start_time, contest_type, my_call, "
            "my_grid, my_continent, my_cq_zone, my_itu_zone, "
            "current_serial, exchange_sent, category, power_class, assisted, operator_name, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            {contestInfo.contestId, contestInfo.contestName,
             contestInfo.startDate.toSecsSinceEpoch(),
             contestInfo.contestType,  // Store contest type (registry ID)
             settings.getMyCallsign(), settings.getMyGridSquare(),
             settings.getMyContinent(),
             settings.getMyCQZone(), settings.getMyITUZone(),
             1, result.exchangeSent,
             result.category, result.powerClass, result.assisted, result.operatorName,
             now.toSecsSinceEpoch()});

        if (!query.isActive()) {
            LOG_ERROR("ContestManager", QString("Failed to create contest record: %1").arg(db.lastError()));
            return false;
        }

        result.contestDbId = db.lastInsertId();
        result.nextSerialNumber = 1;
        result.loadedQSOs.clear();  // No QSOs for new contest
        LOG_DEBUG("ContestManager", QString("Created new contest with DB ID: %1 exchange: '%2' category: '%3'")
            .arg(result.contestDbId).arg(result.exchangeSent).arg(result.category));
    }

    return true;
}

int ContestManager::calculateNextSerialNumber(const QList<QSO>& qsos) const
{
    int maxSerial = 0;
    for (const QSO& qso : qsos) {
        // exchangeSent contains the sent serial number as a string (e.g., "1", "2", "3")
        bool ok;
        int serial = qso.exchangeSent.toInt(&ok);
        if (ok && serial > maxSerial) {
            maxSerial = serial;
        }
    }
    return maxSerial + 1;
}

StationInfo ContestManager::buildStationInfo() const
{
    StationInfo myStation;
    myStation.callsign = AppSettings::instance().getMyCallsign();
    myStation.continent = AppSettings::instance().getMyContinent();
    myStation.cqZone = AppSettings::instance().getMyCQZone();
    myStation.ituZone = AppSettings::instance().getMyITUZone();
    myStation.state = AppSettings::instance().getMyState();
    myStation.county = AppSettings::instance().getMyCounty();

    // Lookup country and other geographic data from cty.dat
    if (m_countryFile) {
        CountryData myCountryData = m_countryFile->lookup(myStation.callsign);
        if (myCountryData.isValid()) {
            myStation.country = myCountryData.name;
            myStation.dxccPrefix = myCountryData.primaryPrefix;
            myStation.dxccEntity = myCountryData.dxccEntity;
        }
    }

    return myStation;
}

ModeType ContestManager::determineModeType(const ContestInfo& contestInfo) const
{
    if (contestInfo.mode == "CW") {
        return ModeType::CW;
    } else if (contestInfo.mode == "SSB") {
        return ModeType::USB;  // SSB mode is represented as USB
    } else {
        return ModeType::None;  // Mixed mode
    }
}

bool ContestManager::contestUsesZones(ContestBase* contest) const
{
    if (!contest) {
        return false;
    }

    for (const MultiplierDefinition& multDef : contest->getMultiplierTypes()) {
        if (multDef.type == MultiplierType::CQZone || multDef.type == MultiplierType::ITUZone) {
            return true;
        }
    }
    return false;
}
