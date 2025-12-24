#include "Database.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVariant>

namespace TR4QT {

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

    // Initialize schema if new database
    if (isNewDatabase) {
        if (!initSchema()) {
            close();
            return false;
        }
    } else {
        // Check if existing database has schema (graceful error handling)
        query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='contests'");
        if (!query.next()) {
            m_lastError = "Database file exists but schema is missing or incompatible.\n"
                         "This database was likely created with an older version of TR4QT.\n"
                         "Please create a new contest or delete the old database file.";
            LOG_WARN("Database", m_lastError);
            LOG_WARN("Database", QString("Database path: %1").arg(dbPath));
            // TODO: Offer to migrate/upgrade database schema in a future version
            //       Show dialog: "Upgrade database schema?" [Yes] [No] [Delete and recreate]
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
    return true;
}

QString Database::loadSchemaSql() {
    // Try to load from embedded resource (production)
    QFile schemaFile(":/data/schema.sql");
    if (!schemaFile.exists()) {
        LOG_WARN("Database", "Resource :/data/schema.sql not found, trying fallback path");
        // Fallback to actual file path (development)
        schemaFile.setFileName("src/data/schema.sql");
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

} // namespace TR4QT
