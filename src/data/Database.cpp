#include "Database.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVariant>
#include <QUuid>

namespace TR4QT {

// TODO: THREADING ISSUE - Fix before implementing networked TR4QT (TCP-based multi-operator)
//
// ISSUE DISCOVERED: 2025-12-27 via test_qso_load_performance (concurrent access tests)
//
// PROBLEM:
//   The current Database singleton pattern is NOT thread-safe for concurrent writes.
//   Multiple threads calling saveQSO() simultaneously will:
//   1. Call beginTransaction() on the same QSqlDatabase instance
//   2. Cause "cannot start a transaction within a transaction" errors
//   3. Result in memory corruption in SQLite (SIGBUS crash at sqlite3DbMallocRawNNTyped)
//   4. Crash address 0x0000004e49474542 ("BEGIN" string corruption)
//
// CURRENT BEHAVIOR:
//   - Single-threaded performance: EXCELLENT (6,800+ QSOs/second, <1ms avg transaction time)
//   - Multi-threaded access: CRASHES (see tests/test_qso_load_performance.cpp concurrent tests)
//   - Single-operator use: SAFE (99% of users, no threading)
//   - Networked/multi-op: UNSAFE (will crash on concurrent QSO logging)
//
// SOLUTIONS (choose one when implementing TCP networking):
//
//   Option 1: CONNECTION POOL (Recommended for networked TR4QT)
//     - Create QSqlDatabase connection per thread
//     - Use thread_local or QThreadStorage<QSqlDatabase>
//     - Each thread gets its own connection to same database file
//     - WAL mode allows concurrent writes from multiple connections
//     - Example: thread_local QSqlDatabase getThreadConnection() { ... }
//
//   Option 2: SERIALIZED ACCESS (Simpler, lower performance)
//     - Add QMutex to Database class
//     - Lock mutex before all database operations
//     - Serialize all access (single writer at a time)
//     - Easier to implement but limits concurrency
//
//   Option 3: MESSAGE QUEUE (Best for networked architecture)
//     - Create dedicated database writer thread
//     - Other threads queue QSO save requests
//     - Writer thread processes queue sequentially
//     - Clean separation, no lock contention
//     - Best for TCP server receiving QSOs from network
//
// RECOMMENDATION:
//   For TCP-based networked TR4QT, use Option 3 (message queue):
//   - TCP server thread receives QSOs from remote stations
//   - Queues them to database writer thread
//   - Writer thread batches writes for efficiency
//   - UI thread reads via separate connection
//   - Clean, scalable architecture
//
// TEST COVERAGE:
//   tests/test_qso_load_performance.cpp has concurrent access tests
//   Currently disabled due to crash, re-enable after fix to verify
//
// REFERENCES:
//   - SQLite threading: https://www.sqlite.org/threadsafe.html
//   - Qt SQL threading: https://doc.qt.io/qt-6/threads-modules.html#threads-and-the-sql-module
//   - Load test crash report: Incident 1B246FB5-29BB-4A8E-A6B1-34A0DACDB488 (2025-12-27)

Database& Database::instance() {
    static Database instance;
    return instance;
}

Database::Database() {
    // Create database connection with unique name
    m_db = QSqlDatabase::addDatabase("QSQLITE", "tr4qt_main");
}

Database::~Database() {
    close();
}

bool Database::open(const QString& dbPath) {
    if (m_db.isOpen()) {
        close();
    }

    // Create directory if it doesn't exist
    QFileInfo fileInfo(dbPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = QString("Failed to create directory: %1").arg(dir.path());
            LOG_WARN("Database", m_lastError);
            return false;
        }
    }

    // Check if this is a new database
    bool isNewDatabase = !QFile::exists(dbPath);

    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("Database", QString("Failed to open database: %1").arg(m_lastError));
        return false;
    }

    LOG_DEBUG("Database", QString("Database opened: %1").arg(dbPath));

    // Enable foreign keys
    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON");

    // Enable WAL mode for better concurrency and performance
    // WAL allows multiple readers while writing, and is faster for most workloads
    if (!query.exec("PRAGMA journal_mode = WAL")) {
        LOG_WARN("Database", QString("Failed to enable WAL mode: %1").arg(query.lastError().text()));
        // Not fatal - continue with default journaling
    } else {
        LOG_DEBUG("Database", "WAL mode enabled for improved performance");
    }

    // Initialize schema if new database
    if (isNewDatabase) {
        if (!initSchema()) {
            close();
            return false;
        }
    } else {
        // Verify application ID for existing database
        uint32_t appId = getApplicationId();
        if (appId != 0 && appId != TR4QT_APP_ID) {
            m_lastError = QString("Database file has incorrect application ID (0x%1).\n"
                                "This is not a TR4QT contest database.\n"
                                "Expected: 0x%2 (TR4QT)")
                .arg(appId, 8, 16, QChar('0'))
                .arg(TR4QT_APP_ID, 8, 16, QChar('0'));
            LOG_WARN("Database", m_lastError);
            close();
            return false;
        }

        // Check schema version
        int dbVersion = getUserVersion();
        LOG_DEBUG("Database", QString("Database schema version: %1 (current: %2)")
            .arg(dbVersion).arg(CURRENT_SCHEMA_VERSION));

        if (dbVersion > CURRENT_SCHEMA_VERSION) {
            // Database is from a NEWER version of TR4QT - cannot open!
            m_lastError = QString("Database schema version (%1) is newer than this TR4QT version supports (%2).\n\n"
                                "This database was created with a newer version of TR4QT.\n"
                                "Please upgrade TR4QT to open this database.")
                .arg(dbVersion).arg(CURRENT_SCHEMA_VERSION);
            LOG_WARN("Database", m_lastError);
            LOG_WARN("Database", QString("Database path: %1").arg(dbPath));
            close();
            return false;
        }

        if (dbVersion < CURRENT_SCHEMA_VERSION) {
            LOG_INFO("Database", QString("Database needs migration from v%1 to v%2")
                .arg(dbVersion).arg(CURRENT_SCHEMA_VERSION));
        }
        // Check if existing database has schema (graceful error handling)
        query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='contests'");
        if (!query.next()) {
            m_lastError = "Database file exists but schema is missing or incompatible.\n"
                         "This database was likely created with an older version of TR4QT.\n"
                         "Please create a new contest or delete the old database file.";
            LOG_WARN("Database", m_lastError);
            LOG_WARN("Database", QString("Database path: %1").arg(dbPath));
            close();
            return false;
        }

        // Migrate schema for existing databases
        if (!migrateSchema()) {
            LOG_WARN("Database", "Schema migration failed");
            close();
            return false;
        }
    }

    return true;
}

void Database::close() {
    if (m_db.isOpen()) {
        QString dbName = m_db.databaseName();
        m_db.close();
        LOG_DEBUG("Database", QString("Database closed: %1").arg(dbName));
    }
}

bool Database::isOpen() const {
    return m_db.isOpen();
}

QSqlDatabase& Database::connection() {
    return m_db;
}

QSqlQuery Database::execute(const QString& query, const QVariantList& values) {
    QSqlQuery q(m_db);

    if (!q.prepare(query)) {
        m_lastError = q.lastError().text();
        LOG_WARN("Database", QString("Failed to prepare query: %1").arg(m_lastError));
        LOG_WARN("Database", QString("Query: %1").arg(query));
        return q;
    }

    // Bind values
    for (const QVariant& value : values) {
        q.addBindValue(value);
    }

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        LOG_WARN("Database", QString("Failed to execute query: %1").arg(m_lastError));
        LOG_WARN("Database", QString("Query: %1").arg(query));
        return q;
    }

    // Store last insert ID
    m_lastInsertId = q.lastInsertId().toInt();

    return q;
}

bool Database::beginTransaction() {
    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("Database", QString("Failed to begin transaction: %1").arg(m_lastError));
        return false;
    }
    return true;
}

bool Database::commitTransaction() {
    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("Database", QString("Failed to commit transaction: %1").arg(m_lastError));
        return false;
    }
    return true;
}

bool Database::rollbackTransaction() {
    if (!m_db.rollback()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("Database", QString("Failed to rollback transaction: %1").arg(m_lastError));
        return false;
    }
    return true;
}

QString Database::lastError() const {
    return m_lastError;
}

int Database::lastInsertId() const {
    return m_lastInsertId;
}

bool Database::initSchema() {
    LOG_DEBUG("Database", "Initializing database schema...");

    QString schemaSql = loadSchemaSql();
    if (schemaSql.isEmpty()) {
        m_lastError = "Failed to load schema SQL";
        return false;
    }

    // Split SQL into individual statements
    QStringList statements = schemaSql.split(';', Qt::SkipEmptyParts);

    for (const QString& statement : statements) {
        // Remove comment lines (lines starting with --)
        QStringList lines = statement.split('\n');
        QStringList cleanedLines;
        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty() && !trimmedLine.startsWith("--")) {
                cleanedLines.append(line);
            }
        }

        QString cleaned = cleanedLines.join('\n').trimmed();
        if (cleaned.isEmpty()) {
            continue;
        }

        QSqlQuery query(m_db);
        if (!query.exec(cleaned)) {
            m_lastError = QString("Schema init failed: %1").arg(query.lastError().text());
            LOG_WARN("Database", m_lastError);
            LOG_WARN("Database", QString("Statement: %1").arg(cleaned));
            return false;
        }
    }

    LOG_DEBUG("Database", "Database schema initialized successfully");

    // Set application ID and schema version for new database
    setApplicationId(TR4QT_APP_ID);
    setUserVersion(CURRENT_SCHEMA_VERSION);

    return true;
}

QString Database::loadSchemaSql() {
    // Try to load from embedded resource (production)
    QFile schemaFile(":/data/schema.sql");
    if (!schemaFile.exists()) {
        LOG_WARN("Database", "Resource :/data/schema.sql not found, trying fallback paths");
        // Fallback to actual file paths (development/testing)
        // Try multiple paths to handle different working directories
        QStringList fallbackPaths = {
            "src/data/schema.sql",           // From project root
            "../src/data/schema.sql",        // From build directory
            "../../src/data/schema.sql"      // From nested build directory
        };

        bool found = false;
        for (const QString& path : fallbackPaths) {
            schemaFile.setFileName(path);
            if (schemaFile.exists()) {
                LOG_WARN("Database", QString("Found schema at fallback path: %1").arg(path));
                found = true;
                break;
            }
        }

        if (!found) {
            LOG_WARN("Database", "Schema file not found in any fallback path");
        }
    } else {
        LOG_WARN("Database", "Found schema in Qt resources");
    }

    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Database", QString("Failed to open schema file: %1").arg(schemaFile.fileName()));
        return QString();
    }

    QString sql = QString::fromUtf8(schemaFile.readAll());
    schemaFile.close();

    LOG_WARN("Database", QString("Loaded schema SQL: %1 bytes").arg(sql.length()));
    if (sql.length() > 0) {
        LOG_WARN("Database", QString("First 200 chars: %1").arg(sql.left(200)));
    } else {
        LOG_WARN("Database", "ERROR: Schema SQL is empty!");
    }

    return sql;
}

bool Database::migrateSchema() {
    LOG_DEBUG("Database", "Checking for schema migrations...");

    QSqlQuery query(m_db);

    // Migration 1: Add guid column to qsos table (v2.74.0)
    // Check if guid column exists
    query.exec("PRAGMA table_info(qsos)");
    bool hasGuidColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "guid") {
            hasGuidColumn = true;
            break;
        }
    }

    if (!hasGuidColumn) {
        LOG_INFO("Database", "Migrating schema: Adding guid column to qsos table");

        // Add guid column (allow NULL temporarily for migration)
        if (!query.exec("ALTER TABLE qsos ADD COLUMN guid TEXT")) {
            m_lastError = QString("Failed to add guid column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        // Generate GUIDs for all existing QSOs
        LOG_INFO("Database", "Generating GUIDs for existing QSOs...");

        // Get all QSO IDs
        if (!query.exec("SELECT id FROM qsos WHERE guid IS NULL")) {
            m_lastError = QString("Failed to query existing QSOs: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        QList<int> qsoIds;
        while (query.next()) {
            qsoIds.append(query.value(0).toInt());
        }

        LOG_INFO("Database", QString("Updating %1 QSOs with GUIDs...").arg(qsoIds.size()));

        // Update each QSO with a GUID
        QSqlQuery updateQuery(m_db);
        updateQuery.prepare("UPDATE qsos SET guid = ? WHERE id = ?");

        for (int qsoId : qsoIds) {
            QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
            updateQuery.bindValue(0, guid);
            updateQuery.bindValue(1, qsoId);

            if (!updateQuery.exec()) {
                m_lastError = QString("Failed to update QSO %1 with GUID: %2")
                    .arg(qsoId).arg(updateQuery.lastError().text());
                LOG_ERROR("Database", m_lastError);
                return false;
            }
        }

        LOG_INFO("Database", QString("Successfully migrated %1 QSOs with GUIDs").arg(qsoIds.size()));

        // Create unique index on guid column
        if (!query.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_qsos_guid ON qsos(guid)")) {
            LOG_WARN("Database", QString("Failed to create guid index: %1").arg(query.lastError().text()));
            // Not critical, continue
        }
    }

    // Migration 2: Add dxcc_entity_code column to qsos table (v2.77.0)
    query.exec("PRAGMA table_info(qsos)");
    bool hasDxccEntityCodeColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "dxcc_entity_code") {
            hasDxccEntityCodeColumn = true;
            break;
        }
    }

    if (!hasDxccEntityCodeColumn) {
        LOG_INFO("Database", "Migrating schema: Adding dxcc_entity_code column to qsos table");

        // Add dxcc_entity_code column
        if (!query.exec("ALTER TABLE qsos ADD COLUMN dxcc_entity_code INTEGER")) {
            m_lastError = QString("Failed to add dxcc_entity_code column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "dxcc_entity_code column added successfully");
        LOG_INFO("Database", "Note: DXCC entity codes will be populated from CTY.DAT on next QSO lookup");
    }

    // Migration 3: Add contest_class column to qsos table (v2.96.1)
    query.exec("PRAGMA table_info(qsos)");
    bool hasContestClassColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "contest_class") {
            hasContestClassColumn = true;
            break;
        }
    }

    if (!hasContestClassColumn) {
        LOG_INFO("Database", "Migrating schema: Adding contest_class column to qsos table");

        // Add contest_class column
        if (!query.exec("ALTER TABLE qsos ADD COLUMN contest_class TEXT")) {
            m_lastError = QString("Failed to add contest_class column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "contest_class column added successfully");
        LOG_INFO("Database", "Note: Contest class will be populated for new QSOs from exchange data");
    }

    // Migration 4: Add grid_square and iota_reference columns to qsos table (v3.8.0)
    query.exec("PRAGMA table_info(qsos)");
    bool hasGridSquareColumn = false;
    bool hasIotaReferenceColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "grid_square") {
            hasGridSquareColumn = true;
        }
        if (columnName == "iota_reference") {
            hasIotaReferenceColumn = true;
        }
    }

    if (!hasGridSquareColumn) {
        LOG_INFO("Database", "Migrating schema: Adding grid_square column to qsos table");

        // Add grid_square column
        if (!query.exec("ALTER TABLE qsos ADD COLUMN grid_square TEXT")) {
            m_lastError = QString("Failed to add grid_square column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "grid_square column added successfully");
        LOG_INFO("Database", "Note: Grid squares will be populated from exchange data in VHF/UHF contests");
    }

    if (!hasIotaReferenceColumn) {
        LOG_INFO("Database", "Migrating schema: Adding iota_reference column to qsos table");

        // Add iota_reference column
        if (!query.exec("ALTER TABLE qsos ADD COLUMN iota_reference TEXT")) {
            m_lastError = QString("Failed to add iota_reference column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "iota_reference column added successfully");
        LOG_INFO("Database", "Note: IOTA references will be populated from exchange data in IOTA contest");
    }

    // Migration 5: Add submode column to qsos table (v3.9.0)
    query.exec("PRAGMA table_info(qsos)");
    bool hasSubmodeColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "submode") {
            hasSubmodeColumn = true;
            break;
        }
    }

    if (!hasSubmodeColumn) {
        LOG_INFO("Database", "Migrating schema: Adding submode column to qsos table");

        // Add submode column
        if (!query.exec("ALTER TABLE qsos ADD COLUMN submode TEXT")) {
            m_lastError = QString("Failed to add submode column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "submode column added successfully");
        LOG_INFO("Database", "Note: ADIF SUBMODE support (e.g., FT4 as submode of MFSK)");
    }

    // Migration 6: Add is_run_qso column to qsos table (v3.15.0)
    query.exec("PRAGMA table_info(qsos)");
    bool hasIsRunQSOColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "is_run_qso") {
            hasIsRunQSOColumn = true;
            break;
        }
    }

    if (!hasIsRunQSOColumn) {
        LOG_INFO("Database", "Migrating schema: Adding is_run_qso column to qsos table");

        // Add is_run_qso column (0 = S&P, 1 = CQ/Run)
        if (!query.exec("ALTER TABLE qsos ADD COLUMN is_run_qso INTEGER DEFAULT 0")) {
            m_lastError = QString("Failed to add is_run_qso column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "is_run_qso column added successfully");
        LOG_INFO("Database", "Note: Tracks whether QSO was made in CQ (run) mode or S&P mode");
    }

    // Migration 7: Add contest_type column to contests table (v3.25.0)
    // Separates contest type (registry ID) from contest_id (unique identifier)
    query.exec("PRAGMA table_info(contests)");
    bool hasContestTypeColumn = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "contest_type") {
            hasContestTypeColumn = true;
            break;
        }
    }

    if (!hasContestTypeColumn) {
        LOG_INFO("Database", "Migrating schema: Adding contest_type column to contests table");

        // Add contest_type column
        if (!query.exec("ALTER TABLE contests ADD COLUMN contest_type TEXT")) {
            m_lastError = QString("Failed to add contest_type column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        LOG_INFO("Database", "contest_type column added successfully");

        // Populate contest_type for existing contests by parsing contest_id
        // Format: "CONTESTTYPE_MODE_YYYY_MM_DD" or "CONTESTTYPE_YYYY_MM_DD" → "CONTESTTYPE"
        if (!query.exec("SELECT id, contest_id FROM contests")) {
            m_lastError = QString("Failed to query existing contests: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }

        QList<QPair<int, QString>> contestsToUpdate;
        while (query.next()) {
            int id = query.value(0).toInt();
            QString contestId = query.value(1).toString();
            contestsToUpdate.append(qMakePair(id, contestId));
        }

        LOG_INFO("Database", QString("Updating contest_type for %1 existing contests...").arg(contestsToUpdate.size()));

        // Update each contest with parsed contest_type
        QSqlQuery updateQuery(m_db);
        updateQuery.prepare("UPDATE contests SET contest_type = ? WHERE id = ?");

        for (const auto& pair : contestsToUpdate) {
            int id = pair.first;
            QString contestId = pair.second;

            // Parse contest type from contest_id
            // Remove date suffix (YYYY_MM_DD) if present
            QStringList parts = contestId.split('_');
            if (parts.size() >= 3) {
                bool ok1, ok2, ok3;
                int year = parts[parts.size() - 3].toInt(&ok1);
                int month = parts[parts.size() - 2].toInt(&ok2);
                int day = parts[parts.size() - 1].toInt(&ok3);

                if (ok1 && ok2 && ok3 && year >= 2000 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                    // Remove date suffix
                    parts.removeLast();
                    parts.removeLast();
                    parts.removeLast();
                }
            }

            // Remove mode suffix (_CW or _SSB) if present
            if (!parts.isEmpty() && (parts.last() == "CW" || parts.last() == "SSB")) {
                parts.removeLast();
            }

            // Rejoin to get contest type
            QString contestType = parts.join('_');

            updateQuery.bindValue(0, contestType);
            updateQuery.bindValue(1, id);

            if (!updateQuery.exec()) {
                m_lastError = QString("Failed to update contest %1 with type '%2': %3")
                    .arg(id).arg(contestType).arg(updateQuery.lastError().text());
                LOG_ERROR("Database", m_lastError);
                return false;
            }

            LOG_DEBUG("Database", QString("Contest ID %1: '%2' → type: '%3'").arg(id).arg(contestId).arg(contestType));
        }

        LOG_INFO("Database", QString("Successfully migrated %1 contests with contest_type").arg(contestsToUpdate.size()));
        LOG_INFO("Database", "Note: contest_type stores the registry ID for reliable contest loading");
    }

    // Migration 8: Add exchange field columns to qsos table (v3.31.1)
    // These columns store contest-specific exchange data
    query.exec("PRAGMA table_info(qsos)");
    bool hasSerialNumberReceived = false;
    bool hasPrecedence = false;
    bool hasSweepstakesCheck = false;
    bool hasPower = false;
    bool hasOperatorName = false;
    bool hasItuZoneExchange = false;
    while (query.next()) {
        QString columnName = query.value(1).toString();
        if (columnName == "serial_number_received") hasSerialNumberReceived = true;
        if (columnName == "precedence") hasPrecedence = true;
        if (columnName == "sweepstakes_check") hasSweepstakesCheck = true;
        if (columnName == "power") hasPower = true;
        if (columnName == "operator_name") hasOperatorName = true;
        if (columnName == "itu_zone_exchange") hasItuZoneExchange = true;
    }

    if (!hasSerialNumberReceived) {
        LOG_INFO("Database", "Migrating schema: Adding serial_number_received column to qsos table");
        if (!query.exec("ALTER TABLE qsos ADD COLUMN serial_number_received INTEGER DEFAULT 0")) {
            m_lastError = QString("Failed to add serial_number_received column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }
        LOG_INFO("Database", "serial_number_received column added successfully");
    }

    if (!hasPrecedence) {
        LOG_INFO("Database", "Migrating schema: Adding precedence column to qsos table");
        if (!query.exec("ALTER TABLE qsos ADD COLUMN precedence TEXT")) {
            m_lastError = QString("Failed to add precedence column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }
        LOG_INFO("Database", "precedence column added (for Sweepstakes)");
    }

    if (!hasSweepstakesCheck) {
        LOG_INFO("Database", "Migrating schema: Adding sweepstakes_check column to qsos table");
        if (!query.exec("ALTER TABLE qsos ADD COLUMN sweepstakes_check TEXT")) {
            m_lastError = QString("Failed to add sweepstakes_check column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }
        LOG_INFO("Database", "sweepstakes_check column added (for Sweepstakes)");
    }

    if (!hasPower) {
        LOG_INFO("Database", "Migrating schema: Adding power column to qsos table");
        if (!query.exec("ALTER TABLE qsos ADD COLUMN power TEXT")) {
            m_lastError = QString("Failed to add power column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }
        LOG_INFO("Database", "power column added (for ARRL DX)");
    }

    if (!hasOperatorName) {
        LOG_INFO("Database", "Migrating schema: Adding operator_name column to qsos table");
        if (!query.exec("ALTER TABLE qsos ADD COLUMN operator_name TEXT")) {
            m_lastError = QString("Failed to add operator_name column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }
        LOG_INFO("Database", "operator_name column added (for NAQP)");
    }

    if (!hasItuZoneExchange) {
        LOG_INFO("Database", "Migrating schema: Adding itu_zone_exchange column to qsos table");
        if (!query.exec("ALTER TABLE qsos ADD COLUMN itu_zone_exchange TEXT")) {
            m_lastError = QString("Failed to add itu_zone_exchange column: %1").arg(query.lastError().text());
            LOG_ERROR("Database", m_lastError);
            return false;
        }
        LOG_INFO("Database", "itu_zone_exchange column added (for IARU HF)");
    }

    // Set application ID if not already set (for databases created before versioning)
    uint32_t currentAppId = getApplicationId();
    if (currentAppId == 0) {
        LOG_INFO("Database", "Setting application_id for pre-versioning database");
        setApplicationId(TR4QT_APP_ID);
    } else if (currentAppId != TR4QT_APP_ID) {
        LOG_WARN("Database", QString("Database has unexpected application_id: 0x%1 (expected 0x%2)")
            .arg(currentAppId, 8, 16, QChar('0'))
            .arg(TR4QT_APP_ID, 8, 16, QChar('0')));
    }

    // Update schema version to current
    setUserVersion(CURRENT_SCHEMA_VERSION);

    LOG_DEBUG("Database", "Schema migration complete");
    return true;
}

int Database::getUserVersion() const {
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA user_version")) {
        LOG_WARN("Database", QString("Failed to read user_version: %1").arg(query.lastError().text()));
        return 0;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

void Database::setUserVersion(int version) {
    QSqlQuery query(m_db);
    // Note: PRAGMA user_version cannot use bind parameters
    QString sql = QString("PRAGMA user_version = %1").arg(version);
    if (!query.exec(sql)) {
        LOG_WARN("Database", QString("Failed to set user_version to %1: %2")
            .arg(version).arg(query.lastError().text()));
    } else {
        LOG_DEBUG("Database", QString("Set database schema version to %1").arg(version));
    }
}

uint32_t Database::getApplicationId() const {
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA application_id")) {
        LOG_WARN("Database", QString("Failed to read application_id: %1").arg(query.lastError().text()));
        return 0;
    }

    if (query.next()) {
        return query.value(0).toUInt();
    }

    return 0;
}

void Database::setApplicationId(uint32_t appId) {
    QSqlQuery query(m_db);
    // Note: PRAGMA application_id cannot use bind parameters
    QString sql = QString("PRAGMA application_id = %1").arg(appId);
    if (!query.exec(sql)) {
        LOG_WARN("Database", QString("Failed to set application_id to 0x%1: %2")
            .arg(appId, 8, 16, QChar('0')).arg(query.lastError().text()));
    } else {
        LOG_DEBUG("Database", QString("Set database application_id to 0x%1 (TR4QT)")
            .arg(appId, 8, 16, QChar('0')));
    }
}

} // namespace TR4QT
