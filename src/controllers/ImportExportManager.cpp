#include "ImportExportManager.h"
#include "../ui/dialogs/ADIFImportDialog.h"
#include "../ui/dialogs/ExportPreviewDialog.h"
#include "../utils/ADIFExporter.h"
#include "../utils/CabrilloExporter.h"
#include "../utils/DialogHelper.h"
#include "../utils/AppSettings.h"
#include "../ui/models/QSOTableModel.h"
#include "../data/QSORepository.h"
#include "../controllers/DataIntegrityManager.h"
#include "../logging/LogMacros.h"
#include "../core/Types.h"
#include <QFileInfo>
#include <QMessageBox>

namespace TR4QT {

ImportExportManager::ImportExportManager(const Config& config, QWidget* parent)
    : m_config(config)
    , m_parent(parent)
{
}

ImportExportManager::~ImportExportManager() = default;

void ImportExportManager::updateConfig(const Config& config) {
    m_config = config;
}

ImportResult ImportExportManager::importADIF() {
    ImportResult result;

    // Check if we have an active contest
    if (!m_config.hasActiveContest) {
        DialogHelper::warning(m_parent, "Import ADIF",
                            "Please create or open a contest before importing QSOs.");
        result.errorMessage = "No active contest";
        return result;
    }

    // Open import dialog with pointer to CountryFile
    ADIFImportDialog dialog(m_config.countryFile, m_parent);

    if (dialog.exec() == QDialog::Accepted) {
        QList<QSO> importedQSOs = dialog.getImportedQSOs();

        if (importedQSOs.isEmpty()) {
            result.statusMessage = "No QSOs imported";
            result.success = true;  // Not an error, just nothing to import
            return result;
        }

        // Save imported QSOs to database
        QSORepository repo;
        int successCount = 0;
        int failureCount = 0;

        for (const QSO& qsoConst : importedQSOs) {
            QSO qso = qsoConst;  // Make mutable copy (saveQSO modifies GUID if needed)
            if (repo.saveQSO(qso, m_config.currentContestDbId)) {
                successCount++;
            } else {
                failureCount++;
                LOG_WARN("ImportExportManager", QString("Failed to import QSO: %1 - %2")
                    .arg(qso.callsign)
                    .arg(repo.lastError()));
            }
        }

        // Check if any QSOs were successfully imported
        if (successCount == 0) {
            DialogHelper::critical(m_parent, "Import Failed",
                QString("Failed to import any QSOs.\n\nError: %1").arg(repo.lastError()));
            result.errorMessage = repo.lastError();
            return result;
        }

        // Reload QSOs from database to refresh UI (handled by MainWindow)
        result.success = true;
        result.successCount = successCount;
        result.failureCount = failureCount;

        // Build status message
        result.statusMessage = QString("Imported %1 QSO%2%3")
            .arg(successCount)
            .arg(successCount == 1 ? "" : "s")
            .arg(failureCount > 0 ? QString(" (%1 failed)").arg(failureCount) : "");

        LOG_INFO("ImportExportManager", QString("ADIF import completed: %1 successful, %2 failed")
            .arg(successCount)
            .arg(failureCount));

        // Rescore if user enabled the checkbox
        if (dialog.shouldRescore() && m_config.activeContest) {
            // Get all QSOs from database (including newly imported ones)
            QList<QSO> allQSOs = repo.findByContest(m_config.currentContestDbId);

            // Build station info for scoring
            StationInfo myStation;
            myStation.callsign = AppSettings::instance().getMyCallsign();
            myStation.grid = AppSettings::instance().getMyGridSquare();

            // Lookup my station's country/zone from CountryFile
            if (m_config.countryFile) {
                CountryData myCountry = m_config.countryFile->lookup(myStation.callsign);
                if (myCountry.isValid()) {
                    myStation.cqZone = myCountry.cqZone;
                    myStation.ituZone = myCountry.ituZone;
                    myStation.country = myCountry.name;
                    myStation.continent = continentToString(myCountry.continent);
                }
            }

            // Run rescore
            DataIntegrityManager integrityMgr({m_config.countryFile, m_config.currentContestDbId});
            result.rescoreStats = integrityMgr.rescoreContestSilent(
                allQSOs,
                m_config.activeContest,
                myStation);

            // Update status message with rescore stats
            result.statusMessage = QString("Imported %1 QSO%2, rescored: %3 updated, %4 mults, %5 dupes")
                .arg(successCount)
                .arg(successCount == 1 ? "" : "s")
                .arg(result.rescoreStats.qsosUpdated)
                .arg(result.rescoreStats.multsMarked)
                .arg(result.rescoreStats.dupesFound);

            LOG_INFO("ImportExportManager", QString("Auto-rescore after import: %1 updated, %2 mults, %3 dupes")
                .arg(result.rescoreStats.qsosUpdated)
                .arg(result.rescoreStats.multsMarked)
                .arg(result.rescoreStats.dupesFound));
        }
    } else {
        // User cancelled the dialog
        result.success = true;  // Not an error
        result.statusMessage = "Import cancelled";
    }

    return result;
}

ExportResult ImportExportManager::exportADIF() {
    ExportResult result;

    // Get all QSOs from the table model
    QList<QSO> qsos;
    for (int i = 0; i < m_config.qsoTableModel->count(); ++i) {
        qsos.append(m_config.qsoTableModel->getQSO(i));
    }

    if (qsos.isEmpty()) {
        DialogHelper::information(m_parent, "Export ADIF", "No QSOs to export.");
        result.success = true;  // Not an error, just nothing to export
        result.statusMessage = "No QSOs to export";
        return result;
    }

    // Run integrity check before export
    if (!runIntegrityCheckBeforeExport("ADIF", qsos)) {
        result.statusMessage = "Export cancelled by user";
        result.success = true;  // Not an error, user chose to abort
        return result;
    }

    // Generate ADIF content
    ADIFExporter exporter;
    QString operatorCall = AppSettings::instance().getMyCallsign();
    QString adifContent = exporter.generateADIF(qsos, m_config.activeContest, operatorCall);

    // Generate default filename
    QString defaultFileName = m_config.hasActiveContest ?
        m_config.currentContestName.toLower().replace(" ", "_") + ".adi" :
        "log.adi";

    // Show preview dialog
    ExportPreviewDialog preview(
        QString("ADIF Export Preview - %1 QSOs").arg(qsos.size()),
        adifContent,
        "ADIF Files (*.adi *.adif);;All Files (*)",
        defaultFileName,
        m_parent);

    preview.exec();

    // Update result if saved
    if (preview.wasSaved()) {
        result.success = true;
        result.savedFilePath = preview.getSaveFilePath();
        result.statusMessage = QString("Exported %1 QSOs to %2")
            .arg(qsos.size())
            .arg(QFileInfo(result.savedFilePath).fileName());
    } else {
        result.success = true;  // Not an error, user just didn't save
        result.statusMessage = "Export not saved";
    }

    return result;
}

ExportResult ImportExportManager::exportCabrillo() {
    ExportResult result;

    // Check if we have QSOs to export
    if (m_config.qsoTableModel->count() == 0) {
        DialogHelper::information(m_parent, "Export Cabrillo", "No QSOs to export.");
        result.success = true;  // Not an error, just nothing to export
        result.statusMessage = "No QSOs to export";
        return result;
    }

    // Run integrity check before export
    QList<QSO> qsos;
    for (int i = 0; i < m_config.qsoTableModel->count(); ++i) {
        qsos.append(m_config.qsoTableModel->getQSO(i));
    }

    if (!runIntegrityCheckBeforeExport("Cabrillo", qsos)) {
        result.statusMessage = "Export cancelled by user";
        result.success = true;  // Not an error, user chose to abort
        return result;
    }

    // Warn if no contest is active
    if (!m_config.hasActiveContest || !m_config.activeContest) {
        QMessageBox::StandardButton reply = DialogHelper::question(
            m_parent, "Export Cabrillo",
            "No active contest selected. Export anyway with generic formatting?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (reply == QMessageBox::No) {
            result.statusMessage = "Export cancelled by user";
            result.success = true;  // Not an error, user chose to abort
            return result;
        }
    }

    if (qsos.isEmpty()) {
        DialogHelper::information(m_parent, "Export Cabrillo", "No QSOs to export.");
        result.success = true;  // Not an error, just nothing to export
        result.statusMessage = "No QSOs to export";
        return result;
    }

    // Generate default filename
    QString defaultFileName;
    if (m_config.hasActiveContest && !m_config.currentContestName.isEmpty()) {
        defaultFileName = QString("%1.cbr").arg(m_config.currentContestName.replace(' ', '_'));
    } else {
        defaultFileName = "log.cbr";
    }

    // Set up exporter with station information
    CabrilloExporter exporter;
    exporter.setStationInfo(
        AppSettings::instance().getMyCallsign(),
        AppSettings::instance().getMyGridSquare(),
        AppSettings::instance().getMyCallsign(),  // Name
        "",  // Address
        "",  // City
        "",  // State/Province
        "",  // Postal Code
        "",  // Country
        ""   // Email
    );

    // TODO: Get category information from contest dialog
    exporter.setCategory(
        "NON-ASSISTED",  // Assisted
        "ALL",           // Band
        "MIXED",         // Mode
        "SINGLE-OP",     // Operator
        "LOW",           // Power
        "FIXED",         // Station
        "",              // Time
        "ONE",           // Transmitter
        ""               // Overlay
    );

    // Calculate claimed score
    // TODO: Get actual score from scoring engine
    const int CLAIMED_SCORE = 0;
    exporter.setClaimedScore(CLAIMED_SCORE);
    exporter.setOperators(AppSettings::instance().getMyCallsign());

    // Generate Cabrillo content
    QString cabrilloContent = exporter.generateCabrillo(qsos, m_config.activeContest);

    // Show preview dialog
    ExportPreviewDialog preview(
        QString("Cabrillo Export Preview - %1 QSOs").arg(qsos.size()),
        cabrilloContent,
        "Cabrillo Files (*.cbr *.log);;All Files (*)",
        defaultFileName,
        m_parent);

    preview.exec();

    // Update result if saved
    if (preview.wasSaved()) {
        result.success = true;
        result.savedFilePath = preview.getSaveFilePath();
        result.statusMessage = QString("Exported %1 QSOs to %2")
            .arg(qsos.size())
            .arg(QFileInfo(result.savedFilePath).fileName());
    } else {
        result.success = true;  // Not an error, user just didn't save
        result.statusMessage = "Export not saved";
    }

    return result;
}

bool ImportExportManager::runIntegrityCheckBeforeExport(
    const QString& exportFormat,
    const QList<QSO>& qsos)
{
    // Run full integrity check before export (all issues)
    if (m_config.hasActiveContest) {
        DataIntegrityManager integrityMgr({m_config.countryFile, m_config.currentContestDbId});
        QString integrityReport = integrityMgr.fullIntegrityCheck(qsos, false);  // Check all issues

        // Check if there are any critical issues in the report
        if (integrityReport.contains("✗ CRITICAL ISSUES DETECTED")) {
            QMessageBox::StandardButton reply = DialogHelper::warning(
                m_parent,
                "Data Integrity Warning",
                "Log integrity check found CRITICAL issues!\n\n"
                "Exporting may result in incomplete or incorrect data.\n\n"
                "View integrity report?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes);

            if (reply == QMessageBox::Yes) {
                // Show full report
                DialogHelper::information(m_parent, "Integrity Check Report", integrityReport);
                return false;  // Abort export so user can fix issues
            }
        } else if (integrityReport.contains("ℹ INFO:")) {
            // Informational issues only - just log them, don't block export
            LOG_INFO("ImportExportManager",
                QString("Pre-export %1 integrity check found informational issues:\n%2")
                    .arg(exportFormat)
                    .arg(integrityReport));
        } else {
            LOG_INFO("ImportExportManager",
                QString("Pre-export %1 integrity check passed").arg(exportFormat));
        }
    }

    return true;  // Continue with export
}

} // namespace TR4QT
