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

bool DataIntegrityManager::quickIntegrityCheck(int memoryCount)
{
    if (m_config.currentContestDbId < 0) {
        return true;  // No active contest
    }

    // Check if database is open before running integrity check
    Database& db = Database::instance();
    if (!db.isOpen()) {
        LOG_DEBUG("DataIntegrityManager", "Skipping integrity check - database is not open");
        return true;  // Not an error, just skip the check
    }

    // Count non-deleted QSOs in database
    QSqlQuery query = db.execute(
        "SELECT COUNT(*) FROM qsos WHERE contest_id = ? AND deleted = 0",
        {m_config.currentContestDbId});

    int dbCount = 0;
    if (query.next()) {
        dbCount = query.value(0).toInt();
    }

    if (memoryCount != dbCount) {
        LOG_ERROR("DataIntegrityManager", QString("INTEGRITY CHECK FAILED: Memory=%1 DB=%2")
            .arg(memoryCount).arg(dbCount));
        return false;
    }

    LOG_DEBUG("DataIntegrityManager", QString("Integrity check passed: %1 QSOs").arg(memoryCount));
    return true;
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
        report += "\n";
    }

    report += "=== END OF REPORT ===\n";
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
