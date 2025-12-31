#include "SCPCallsignExtractor.h"
#include "../data/SCPRepository.h"
#include "../core/Constants.h"
#include "../logging/LogMacros.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSet>

namespace TR4QT {

SCPCallsignExtractor::SCPCallsignExtractor() {
}

QStringList SCPCallsignExtractor::extractFromAllContests() {
    // Get contest logs directory
    QString logsDir = QDir::homePath() + "/" + CONFIG_DIR + "/logs";
    QDir dir(logsDir);

    if (!dir.exists()) {
        m_lastError = QString("Logs directory does not exist: %1").arg(logsDir);
        LOG_WARN("SCPCallsignExtractor", m_lastError);
        return QStringList();
    }

    // Find all *.db files
    QStringList dbFiles = dir.entryList({"*.db"}, QDir::Files);

    if (dbFiles.isEmpty()) {
        m_lastError = "No contest logs found";
        LOG_DEBUG("SCPCallsignExtractor", m_lastError);
        return QStringList();
    }

    LOG_DEBUG("SCPCallsignExtractor", QString("Found %1 contest databases").arg(dbFiles.size()));

    // Use QSet for automatic deduplication
    QSet<QString> uniqueCallsigns;

    // Extract callsigns from each database
    for (const QString& dbFile : dbFiles) {
        QString dbPath = dir.filePath(dbFile);
        QStringList calls = extractFromContest(dbPath);

        for (const QString& call : calls) {
            uniqueCallsigns.insert(call);
        }
    }

    LOG_DEBUG("SCPCallsignExtractor",
        QString("Extracted %1 unique callsigns from %2 contest logs")
        .arg(uniqueCallsigns.size()).arg(dbFiles.size()));

    return uniqueCallsigns.values();
}

QStringList SCPCallsignExtractor::extractFromContest(const QString& dbPath) {
    QStringList callsigns;

    // Create a temporary connection name to avoid conflicts
    QString connectionName = QString("scp_extract_%1").arg(reinterpret_cast<quintptr>(this));

    // Remove any existing connection with this name
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        m_lastError = QString("Failed to open database %1: %2")
            .arg(dbPath).arg(db.lastError().text());
        LOG_WARN("SCPCallsignExtractor", m_lastError);
        QSqlDatabase::removeDatabase(connectionName);
        return callsigns;
    }

    // Query for distinct callsigns (exclude deleted QSOs)
    QString sql = "SELECT DISTINCT callsign FROM qsos WHERE deleted = 0";
    QSqlQuery query(db);

    if (!query.exec(sql)) {
        m_lastError = QString("Query failed for %1: %2")
            .arg(dbPath).arg(query.lastError().text());
        LOG_WARN("SCPCallsignExtractor", m_lastError);
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return callsigns;
    }

    while (query.next()) {
        QString callsign = query.value(0).toString().toUpper().trimmed();
        if (!callsign.isEmpty()) {
            callsigns << callsign;
        }
    }

    LOG_DEBUG("SCPCallsignExtractor",
        QString("Extracted %1 callsigns from %2")
        .arg(callsigns.size()).arg(QFileInfo(dbPath).fileName()));

    db.close();
    QSqlDatabase::removeDatabase(connectionName);

    return callsigns;
}

int SCPCallsignExtractor::updateSCPFromLocalLogs() {
    LOG_DEBUG("SCPCallsignExtractor", "Updating SCP database from local contest logs...");

    // Extract callsigns from all contests
    QStringList callsigns = extractFromAllContests();

    if (callsigns.isEmpty()) {
        LOG_DEBUG("SCPCallsignExtractor", "No callsigns found in local logs");
        return 0;
    }

    SCPRepository repo;

    // Clear old local callsigns
    int cleared = repo.clearBySource("local_log");
    LOG_DEBUG("SCPCallsignExtractor", QString("Cleared %1 old local callsigns").arg(cleared));

    // Bulk insert from local logs
    int inserted = repo.bulkInsert(callsigns, "local_log");

    // Update metadata
    repo.setMetadata("local_log_count", QString::number(inserted));
    repo.setMetadata("local_log_last_update", QDateTime::currentDateTime().toString(Qt::ISODate));

    LOG_INFO("SCPCallsignExtractor",
        QString("Updated SCP database: %1 callsigns from local logs").arg(inserted));

    return inserted;
}

} // namespace TR4QT
