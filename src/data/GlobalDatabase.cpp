#include "GlobalDatabase.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVariant>
#include <QStandardPaths>

namespace TR4QT {

GlobalDatabase& GlobalDatabase::instance() {
    static GlobalDatabase instance;
    return instance;
}

GlobalDatabase::GlobalDatabase() {
    // Create database connection with unique name for global database
    // This allows us to have both the global DB and contest-specific DB open simultaneously
    m_db = QSqlDatabase::addDatabase("QSQLITE", "tr4qt_global");
}

GlobalDatabase::~GlobalDatabase() {
    close();
}

QString GlobalDatabase::defaultDatabasePath() {
    // Store in user's home directory: ~/.tr4qt/tr4qt_global.db
    QString homeDir = QDir::homePath();
    return homeDir + "/.tr4qt/tr4qt_global.db";
}

bool GlobalDatabase::open() {
    return open(defaultDatabasePath());
}

bool GlobalDatabase::open(const QString& dbPath) {
    if (m_db.isOpen()) {
        // Already open at this path - no need to reopen
        if (m_db.databaseName() == dbPath) {
            LOG_DEBUG("GlobalDatabase", QString("Database already open: %1").arg(dbPath));
            return true;
        }
        // Opening different path - close first
        close();
    }

    // Create directory if it doesn't exist
    QFileInfo fileInfo(dbPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = QString("Failed to create directory: %1").arg(dir.path());
            LOG_WARN("GlobalDatabase", m_lastError);
            return false;
        }
    }

    // Check if this is a new database
    bool isNewDatabase = !QFile::exists(dbPath);

    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("GlobalDatabase", QString("Failed to open database: %1").arg(m_lastError));
        return false;
    }

    LOG_DEBUG("GlobalDatabase", QString("Global database opened: %1").arg(dbPath));

    // Enable foreign keys
    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON");

    // Initialize schema if new database
    if (isNewDatabase) {
        if (!initSchema()) {
            close();
            return false;
        }
    } else {
        // Verify existing database has schema
        query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='lotw_users'");
        if (!query.next()) {
            // Schema missing - try to initialize it
            LOG_WARN("GlobalDatabase", "Global database exists but schema is missing, initializing...");
            if (!initSchema()) {
                m_lastError = "Global database schema is missing and initialization failed";
                LOG_WARN("GlobalDatabase", m_lastError);
                close();
                return false;
            }
        }

        // Migrate existing database schema if needed
        if (!migrateSchema()) {
            LOG_WARN("GlobalDatabase", "Failed to migrate database schema");
            // Don't fail - continue with existing schema
        }
    }

    return true;
}

void GlobalDatabase::close() {
    if (m_db.isOpen()) {
        QString dbName = m_db.databaseName();
        m_db.close();
        LOG_DEBUG("GlobalDatabase", QString("Global database closed: %1").arg(dbName));
    }
}

bool GlobalDatabase::isOpen() const {
    return m_db.isOpen();
}

QSqlDatabase& GlobalDatabase::connection() {
    return m_db;
}

QSqlQuery GlobalDatabase::execute(const QString& query, const QVariantList& values) {
    QSqlQuery q(m_db);

    if (!q.prepare(query)) {
        m_lastError = q.lastError().text();
        LOG_WARN("GlobalDatabase", QString("Failed to prepare query: %1").arg(m_lastError));
        LOG_WARN("GlobalDatabase", QString("Query: %1").arg(query));
        return q;
    }

    // Bind values
    for (const QVariant& value : values) {
        q.addBindValue(value);
    }

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        LOG_WARN("GlobalDatabase", QString("Failed to execute query: %1").arg(m_lastError));
        LOG_WARN("GlobalDatabase", QString("Query: %1").arg(query));
        return q;
    }

    // Store last insert ID
    m_lastInsertId = q.lastInsertId().toInt();

    return q;
}

bool GlobalDatabase::beginTransaction() {
    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("GlobalDatabase", QString("Failed to begin transaction: %1").arg(m_lastError));
        return false;
    }
    return true;
}

bool GlobalDatabase::commitTransaction() {
    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("GlobalDatabase", QString("Failed to commit transaction: %1").arg(m_lastError));
        return false;
    }
    return true;
}

bool GlobalDatabase::rollbackTransaction() {
    if (!m_db.rollback()) {
        m_lastError = m_db.lastError().text();
        LOG_WARN("GlobalDatabase", QString("Failed to rollback transaction: %1").arg(m_lastError));
        return false;
    }
    return true;
}

QString GlobalDatabase::lastError() const {
    return m_lastError;
}

int GlobalDatabase::lastInsertId() const {
    return m_lastInsertId;
}

bool GlobalDatabase::initSchema() {
    LOG_DEBUG("GlobalDatabase", "Initializing global database schema...");

    QString schemaSql = loadSchemaSql();
    if (schemaSql.isEmpty()) {
        m_lastError = "Failed to load global schema SQL";
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
            m_lastError = QString("Global schema init failed: %1").arg(query.lastError().text());
            LOG_WARN("GlobalDatabase", m_lastError);
            LOG_WARN("GlobalDatabase", QString("Statement: %1").arg(cleaned));
            return false;
        }
    }

    LOG_DEBUG("GlobalDatabase", "Global database schema initialized successfully");
    return true;
}

bool GlobalDatabase::migrateSchema() {
    LOG_DEBUG("GlobalDatabase", "Checking for schema migrations...");

    // Check if dx_spots table exists
    QSqlQuery checkTable(m_db);
    checkTable.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='dx_spots'");
    if (!checkTable.next()) {
        // Table doesn't exist yet - no migration needed
        LOG_DEBUG("GlobalDatabase", "dx_spots table does not exist - no migration needed");
        return true;
    }

    // Check if azimuth column exists
    QSqlQuery checkColumn(m_db);
    checkColumn.exec("PRAGMA table_info(dx_spots)");

    bool hasAzimuth = false;
    bool hasDistance = false;

    while (checkColumn.next()) {
        QString columnName = checkColumn.value("name").toString();
        if (columnName == "azimuth") {
            hasAzimuth = true;
        }
        if (columnName == "distance") {
            hasDistance = true;
        }
    }

    // Add azimuth column if missing
    if (!hasAzimuth) {
        LOG_DEBUG("GlobalDatabase", "Adding azimuth column to dx_spots table");
        QSqlQuery addAzimuth(m_db);
        if (!addAzimuth.exec("ALTER TABLE dx_spots ADD COLUMN azimuth REAL DEFAULT -1.0")) {
            m_lastError = QString("Failed to add azimuth column: %1").arg(addAzimuth.lastError().text());
            LOG_WARN("GlobalDatabase", m_lastError);
            return false;
        }
        LOG_DEBUG("GlobalDatabase", "Added azimuth column successfully");
    }

    // Add distance column if missing
    if (!hasDistance) {
        LOG_DEBUG("GlobalDatabase", "Adding distance column to dx_spots table");
        QSqlQuery addDistance(m_db);
        if (!addDistance.exec("ALTER TABLE dx_spots ADD COLUMN distance REAL DEFAULT -1.0")) {
            m_lastError = QString("Failed to add distance column: %1").arg(addDistance.lastError().text());
            LOG_WARN("GlobalDatabase", m_lastError);
            return false;
        }
        LOG_DEBUG("GlobalDatabase", "Added distance column successfully");
    }

    if (!hasAzimuth || !hasDistance) {
        LOG_DEBUG("GlobalDatabase", "Schema migration completed successfully");
    } else {
        LOG_DEBUG("GlobalDatabase", "No schema migration needed - database is up to date");
    }

    return true;
}

QString GlobalDatabase::loadSchemaSql() {
    // Try to load from embedded resource (production)
    QFile schemaFile(":/data/global_schema.sql");
    if (!schemaFile.exists()) {
        LOG_DEBUG("GlobalDatabase", "Resource :/data/global_schema.sql not found, trying fallback path");
        // Fallback to actual file path (development)
        schemaFile.setFileName("src/data/global_schema.sql");
    } else {
        LOG_DEBUG("GlobalDatabase", "Found global schema in Qt resources");
    }

    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("GlobalDatabase", QString("Failed to open global schema file: %1").arg(schemaFile.fileName()));
        return QString();
    }

    QString sql = QString::fromUtf8(schemaFile.readAll());
    schemaFile.close();

    LOG_DEBUG("GlobalDatabase", QString("Loaded global schema SQL: %1 bytes").arg(sql.length()));
    if (sql.length() > 0) {
        LOG_DEBUG("GlobalDatabase", QString("First 200 chars: %1").arg(sql.left(200)));
    } else {
        LOG_WARN("GlobalDatabase", "ERROR: Global schema SQL is empty!");
    }

    return sql;
}

} // namespace TR4QT
