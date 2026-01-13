#include "DataIntegrityManager.h"
#include "../data/Database.h"
#include "../data/QSORepository.h"
#include "../logging/LogMacros.h"
#include <QSqlQuery>
#include <QDateTime>
#include <QtMath>

using namespace TR4QT;

DataIntegrityManager::DataIntegrityManager(const Config& config)
    : m_config(config)
{
}

DataIntegrityManager::QuickCheckResult DataIntegrityManager::quickIntegrityCheck(int memoryCount)
{
    QuickCheckResult result;
    result.memoryCount = memoryCount;
    result.dbCount = 0;
    result.passed = true;

    if (m_config.currentContestDbId < 0) {
        return result;  // No active contest
    }

    // Check if database is open before running integrity check
    Database& db = Database::instance();
    if (!db.isOpen()) {
        LOG_DEBUG("DataIntegrityManager", "Skipping integrity check - database is not open");
        return result;  // Not an error, just skip the check
    }

    // Count non-deleted QSOs in database
    QSqlQuery query = db.execute(
        "SELECT COUNT(*) FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_config.currentContestDbId});

    if (query.next()) {
        result.dbCount = query.value(0).toInt();
    }

    if (memoryCount != result.dbCount) {
        LOG_ERROR("DataIntegrityManager", QString("INTEGRITY CHECK FAILED: Memory=%1 DB=%2")
            .arg(memoryCount).arg(result.dbCount));
        result.passed = false;
        return result;
    }

    LOG_DEBUG("DataIntegrityManager", QString("Integrity check passed: %1 QSOs").arg(memoryCount));
    return result;
}

QString DataIntegrityManager::fullIntegrityCheck(const QList<QSO>& memoryQSOs, bool criticalOnly)
{
    QString report;
    report += "=== LOG INTEGRITY CHECK REPORT ===\n\n";
    report += QString("Check time: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    report += QString("Mode: %1\n\n").arg(criticalOnly ? "Critical issues only" : "All issues (critical + informational)");

    int memoryCount = memoryQSOs.size();
    Database& db = Database::instance();

    // Check if database is open before running integrity checks
    if (!db.isOpen()) {
        report += "✗ CRITICAL: Database is not open!\n\n";
        report += "Cannot perform integrity check on closed database.\n";
        report += "This may occur if the contest was closed or the database connection failed.\n\n";
        report += "=== END OF REPORT ===\n";
        return report;
    }

    // CRITICAL: Checkpoint WAL to ensure all recent writes are visible
    // Without this, the integrity check may report false positives for recently-logged QSOs
    // that are in the WAL but not yet visible to new queries
    QSqlQuery checkpointQuery = db.execute("PRAGMA wal_checkpoint(PASSIVE)", {});
    checkpointQuery.next();  // Execute the checkpoint

    // Count database QSOs
    QSqlQuery countQuery = db.execute(
        "SELECT COUNT(*) FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_config.currentContestDbId});
    int dbCount = 0;
    if (countQuery.next()) {
        dbCount = countQuery.value(0).toInt();
    }

    report += QString("QSOs in memory: %1\n").arg(memoryCount);
    report += QString("QSOs in database: %1\n\n").arg(dbCount);

    // Check 1: Count match
    if (memoryCount == dbCount) {
        report += "✓ QSO count matches\n\n";
    } else {
        report += QString("✗ QSO COUNT MISMATCH (diff: %1)\n\n")
            .arg(qAbs(memoryCount - dbCount));
    }

    // Check 2: Verify all memory QSOs exist in database
    QStringList missingInDB;
    QSORepository repo;
    for (int row = 0; row < memoryCount; ++row) {
        const QSO& qso = memoryQSOs[row];
        if (qso.id < 0) {
            missingInDB.append(QString("Row %1: %2 (no database ID)")
                .arg(row).arg(qso.callsign));
        } else {
            QSO dbQso = repo.findById(qso.id);
            if (dbQso.id < 0) {
                missingInDB.append(QString("Row %1: %2 (ID=%3 not found in DB)")
                    .arg(row).arg(qso.callsign).arg(qso.id));
            }
        }
    }

    if (missingInDB.isEmpty()) {
        report += "✓ All memory QSOs exist in database\n\n";
    } else {
        report += QString("✗ %1 QSOs in memory not found in database:\n")
            .arg(missingInDB.size());
        for (const QString& item : missingInDB) {
            report += QString("  - %1\n").arg(item);
        }
        report += "\n";
    }

    // Check 3: Look for orphaned QSOs in database
    QSqlQuery dbQuery = db.execute(
        "SELECT id, callsign FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_config.currentContestDbId});

    QList<int> dbIds;
    QMap<int, QString> dbCallsigns;
    while (dbQuery.next()) {
        int id = dbQuery.value(0).toInt();
        QString callsign = dbQuery.value(1).toString();
        dbIds.append(id);
        dbCallsigns[id] = callsign;
    }

    QSet<int> memoryIds;
    for (const QSO& qso : memoryQSOs) {
        if (qso.id >= 0) {
            memoryIds.insert(qso.id);
        }
    }

    QStringList orphanedInDB;
    for (int dbId : dbIds) {
        if (!memoryIds.contains(dbId)) {
            orphanedInDB.append(QString("ID=%1: %2")
                .arg(dbId).arg(dbCallsigns[dbId]));
        }
    }

    if (orphanedInDB.isEmpty()) {
        report += "✓ No orphaned QSOs in database\n\n";
    } else {
        report += QString("✗ %1 QSOs in database not loaded in memory:\n")
            .arg(orphanedInDB.size());
        for (const QString& item : orphanedInDB) {
            report += QString("  - %1\n").arg(item);
        }
        report += "\n";
    }

    // Check 4: Verify critical fields match
    int fieldMismatches = 0;
    for (int row = 0; row < qMin(memoryCount, 100); ++row) {  // Sample first 100
        const QSO& memQso = memoryQSOs[row];
        if (memQso.id < 0) continue;

        QSO dbQso = repo.findById(memQso.id);
        if (dbQso.id < 0) continue;

        if (memQso.callsign != dbQso.callsign ||
            memQso.qsoPoints != dbQso.qsoPoints ||
            memQso.band != dbQso.band) {
            fieldMismatches++;
        }
    }

    if (fieldMismatches == 0) {
        report += QString("✓ Field values match (sampled first 100 QSOs)\n\n");
    } else {
        report += QString("✗ %1 field mismatches detected in sample\n\n")
            .arg(fieldMismatches);
    }

    // Check 5: Detect QSOs with Unknown/None band (CRITICAL)
    QSqlQuery unknownBandQuery = db.execute(
        "SELECT id, callsign, timestamp, band FROM qsos "
        "WHERE contest_id = ? AND deleted = 0 "
        "AND (band = 'Unknown' OR band = 'None' OR band = '')",
        {m_config.currentContestDbId});

    QStringList unknownBands;
    while (unknownBandQuery.next()) {
        int id = unknownBandQuery.value(0).toInt();
        QString callsign = unknownBandQuery.value(1).toString();
        QString timestamp = unknownBandQuery.value(2).toString();
        QString band = unknownBandQuery.value(3).toString();
        unknownBands.append(QString("ID=%1: %2 at %3 (band='%4')")
            .arg(id).arg(callsign).arg(timestamp).arg(band));
    }

    if (unknownBands.isEmpty()) {
        report += "✓ No QSOs with Unknown/None band\n\n";
    } else {
        report += QString("✗ CRITICAL: %1 QSOs with Unknown/None band:\n")
            .arg(unknownBands.size());
        for (const QString& item : unknownBands) {
            report += QString("  - %1\n").arg(item);
        }
        report += "  Recommendation: Manually edit these QSOs to set correct band\n\n";
    }

    // Check 6: Detect lowercase data in critical fields (INFORMATIONAL)
    // This is non-critical but indicates data entry inconsistency
    if (!criticalOnly) {
        QSqlQuery lowercaseQuery = db.execute(
            "SELECT id, callsign, rst_sent, rst_received, exchange_sent, exchange_received "
            "FROM qsos WHERE contest_id = ? AND deleted = 0",
            {m_config.currentContestDbId});

        QStringList lowercaseIssues;
        while (lowercaseQuery.next()) {
            int id = lowercaseQuery.value(0).toInt();
            QString callsign = lowercaseQuery.value(1).toString();
            QString rstSent = lowercaseQuery.value(2).toString();
            QString rstReceived = lowercaseQuery.value(3).toString();
            QString exchangeSent = lowercaseQuery.value(4).toString();
            QString exchangeReceived = lowercaseQuery.value(5).toString();

            QStringList fields;
            if (callsign != callsign.toUpper()) fields << "callsign";
            if (rstSent != rstSent.toUpper()) fields << "rst_sent";
            if (rstReceived != rstReceived.toUpper()) fields << "rst_received";
            if (exchangeSent != exchangeSent.toUpper()) fields << "exchange_sent";
            if (exchangeReceived != exchangeReceived.toUpper()) fields << "exchange_received";

            if (!fields.isEmpty()) {
                lowercaseIssues.append(QString("ID=%1: %2 (%3)")
                    .arg(id).arg(callsign).arg(fields.join(", ")));
            }
        }

        if (lowercaseIssues.isEmpty()) {
            report += "✓ All text fields are uppercase\n\n";
        } else {
            report += QString("ℹ INFO: %1 QSOs with lowercase data:\n")
                .arg(lowercaseIssues.size());
            // Limit to first 10 to avoid overwhelming output
            int displayed = qMin(10, lowercaseIssues.size());
            for (int i = 0; i < displayed; i++) {
                report += QString("  - %1\n").arg(lowercaseIssues[i]);
            }
            if (lowercaseIssues.size() > 10) {
                report += QString("  ... and %1 more\n").arg(lowercaseIssues.size() - 10);
            }
            report += "  Note: This is informational only. Uppercase validation added in v3.30.0.\n\n";
        }
    }

    // Check 7: Database schema version validation (CRITICAL)
    QSqlQuery versionQuery = db.execute("PRAGMA user_version", {});
    int dbSchemaVersion = 0;
    if (versionQuery.next()) {
        dbSchemaVersion = versionQuery.value(0).toInt();
    }

    const int EXPECTED_SCHEMA_VERSION = 8;  // From Database.h CURRENT_SCHEMA_VERSION
    bool schemaVersionMismatch = false;
    if (dbSchemaVersion == EXPECTED_SCHEMA_VERSION) {
        report += QString("✓ Database schema version matches (v%1)\n\n").arg(EXPECTED_SCHEMA_VERSION);
    } else {
        schemaVersionMismatch = true;
        report += QString("✗ CRITICAL: Schema version mismatch!\n");
        report += QString("  Database: v%1\n").arg(dbSchemaVersion);
        report += QString("  Expected: v%1\n").arg(EXPECTED_SCHEMA_VERSION);
        report += "  Recommendation: Restart TR4QT to trigger automatic migration\n\n";
    }

    // Check 8: Required columns existence check (CRITICAL)
    QSqlQuery columnsQuery = db.execute("PRAGMA table_info(qsos)", {});
    QSet<QString> existingColumns;
    while (columnsQuery.next()) {
        existingColumns.insert(columnsQuery.value(1).toString());
    }

    QStringList requiredColumns = {
        "id", "contest_id", "callsign", "timestamp", "frequency", "band", "mode",
        "rst_sent", "rst_received", "exchange_sent", "exchange_received",
        "serial_number", "serial_number_received", "precedence", "sweepstakes_check",
        "power", "operator_name", "itu_zone_exchange",
        "dxcc_entity", "cq_zone", "itu_zone", "continent",
        "qso_points", "is_dupe", "is_multiplier", "deleted"
    };

    QStringList missingColumns;
    for (const QString& col : requiredColumns) {
        if (!existingColumns.contains(col)) {
            missingColumns.append(col);
        }
    }

    if (missingColumns.isEmpty()) {
        report += QString("✓ All %1 required columns exist\n\n").arg(requiredColumns.size());
    } else {
        report += QString("✗ CRITICAL: %1 required columns missing from qsos table:\n")
            .arg(missingColumns.size());
        for (const QString& col : missingColumns) {
            report += QString("  - %1\n").arg(col);
        }
        report += "  Recommendation: Restart TR4QT to trigger automatic migration\n\n";
    }

    // Check 9: QSO load validation test (CRITICAL)
    int loadFailures = 0;
    int sampleSize = qMin(10, dbCount);  // Test first 10 QSOs

    if (sampleSize > 0) {
        QSqlQuery sampleQuery = db.execute(
            "SELECT id FROM qsos WHERE contest_id = ? AND deleted = 0 LIMIT ?",
            {m_config.currentContestDbId, sampleSize});

        while (sampleQuery.next()) {
            int qsoId = sampleQuery.value(0).toInt();
            QSO loadedQso = repo.findById(qsoId);

            // Verify QSO was actually loaded (not just default-constructed)
            if (loadedQso.id != qsoId || loadedQso.callsign.isEmpty()) {
                loadFailures++;
            }
        }

        if (loadFailures == 0) {
            report += QString("✓ QSO load test passed (sampled %1 QSOs)\n\n").arg(sampleSize);
        } else {
            report += QString("✗ CRITICAL: %1/%2 QSOs failed to load correctly\n")
                .arg(loadFailures).arg(sampleSize);
            report += "  This suggests a database schema issue or data corruption\n";
            report += "  Recommendation: Check logs for SQL errors, verify schema version\n\n";
        }
    } else {
        report += "✓ QSO load test skipped (no QSOs in database)\n\n";
    }

    // Summary
    report += "=== SUMMARY ===\n";
    bool criticalIssues = (memoryCount != dbCount) ||
                          !missingInDB.isEmpty() ||
                          !orphanedInDB.isEmpty() ||
                          (fieldMismatches > 0) ||
                          !unknownBands.isEmpty() ||
                          schemaVersionMismatch ||
                          !missingColumns.isEmpty() ||
                          (loadFailures > 0);

    if (!criticalIssues) {
        report += "✓ ALL CRITICAL CHECKS PASSED - Log integrity verified\n";
        if (!criticalOnly) {
            report += "  (Informational checks may have reported non-critical issues above)\n";
        }
    } else {
        report += "✗ CRITICAL ISSUES DETECTED - See details above\n";
        report += "\nRecommendation: Consider reloading contest from database\n";
    }

    LOG_INFO("DataIntegrityManager", QString("Full integrity check: %1")
        .arg(!criticalIssues ? "PASSED" : "FAILED"));

    report += "\n=== END OF REPORT ===\n";
    return report;
}

RescoreStats DataIntegrityManager::rescoreContestSilent(
    QList<QSO>& qsos,
    ContestBase* contest,
    const StationInfo& myStation)
{
    RescoreStats stats;

    if (!contest || !m_config.countryFile) {
        return stats;  // Cannot rescore without contest or country file
    }

    QList<MultiplierDefinition> multDefs = contest->getMultiplierTypes();

    // Track worked multipliers as we go through QSOs in chronological order
    QMap<MultiplierType, QStringList> workedMults;

    // Track worked QSOs for duplicate detection
    QSet<QString> workedQSOs;
    DuplicateCheckingRule dupeRule = contest->getDuplicateCheckingRule();

    // Iterate through all QSOs in chronological order
    for (QSO& qso : qsos) {
        // Check for duplicate based on contest rules
        QString dupeKey;
        switch (dupeRule) {
            case DuplicateCheckingRule::PerBandMode:
                dupeKey = QString("%1_%2_%3")
                    .arg(qso.callsign)
                    .arg(bandToString(qso.band))
                    .arg(modeToString(qso.mode));
                break;
            case DuplicateCheckingRule::AllBandMode:
                dupeKey = QString("%1_%2")
                    .arg(qso.callsign)
                    .arg(modeToString(qso.mode));
                break;
            case DuplicateCheckingRule::PerBand:
                dupeKey = QString("%1_%2")
                    .arg(qso.callsign)
                    .arg(bandToString(qso.band));
                break;
            case DuplicateCheckingRule::AllBand:
                dupeKey = qso.callsign;
                break;
        }

        // Check if this is a duplicate
        bool isDupe = workedQSOs.contains(dupeKey);
        qso.isDupe = isDupe;

        // Recalculate QSO points
        if (isDupe) {
            qso.qsoPoints = 0;
            stats.dupesFound++;
        } else {
            qso.qsoPoints = contest->calculateQSOPoints(qso, myStation);
            workedQSOs.insert(dupeKey);
        }

        // Check for multipliers
        qso.isMultiplier = false;
        QStringList multiplierValues;

        for (const MultiplierDefinition& multDef : multDefs) {
            QStringList relevantWorked;
            if (multDef.scope == MultiplierScope::PerBand) {
                QString bandPrefix = bandToString(qso.band) + ":";
                for (const QString& worked : workedMults[multDef.type]) {
                    if (worked.startsWith(bandPrefix)) {
                        relevantWorked.append(worked.mid(bandPrefix.length()));
                    }
                }
            } else {
                relevantWorked = workedMults[multDef.type];
            }

            QString multValue = contest->getMultiplierValue(qso, multDef.type, relevantWorked);

            if (!multValue.isEmpty()) {
                qso.isMultiplier = true;
                stats.multsMarked++;
                QString typeStr = multDef.type == MultiplierType::Country ? "Country" :
                                 multDef.type == MultiplierType::CQZone ? "CQZone" :
                                 multDef.type == MultiplierType::ITUZone ? "ITUZone" :
                                 multDef.type == MultiplierType::State ? "State" :
                                 multDef.type == MultiplierType::Section ? "Section" :
                                 multDef.type == MultiplierType::Prefix ? "Prefix" : "Custom";
                multiplierValues.append(QString("%1:%2").arg(typeStr, multValue));

                if (multDef.scope == MultiplierScope::PerBand) {
                    workedMults[multDef.type].append(bandToString(qso.band) + ":" + multValue);
                } else {
                    workedMults[multDef.type].append(multValue);
                }
            } else {
                // Track this multiplier even if not new (for future duplicate checking)
                QString existingValue = contest->getMultiplierValue(qso, multDef.type, QStringList());
                if (!existingValue.isEmpty()) {
                    QString trackValue = (multDef.scope == MultiplierScope::PerBand) ?
                                            bandToString(qso.band) + ":" + existingValue :
                                            existingValue;
                    if (!workedMults[multDef.type].contains(trackValue)) {
                        workedMults[multDef.type].append(trackValue);
                    }
                }
            }
        }

        qso.multipliers = multiplierValues;

        // Update in database
        QSORepository repo;
        if (repo.updateQSO(qso)) {
            stats.qsosUpdated++;
        } else {
            LOG_WARN("DataIntegrityManager", QString("Failed to update QSO %1 in database").arg(qso.id));
        }
    }

    return stats;
}
