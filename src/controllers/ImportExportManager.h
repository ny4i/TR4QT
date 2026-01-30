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

#ifndef IMPORTEXPORTMANAGER_H
#define IMPORTEXPORTMANAGER_H

#include <QString>
#include <QList>
#include <QWidget>
#include "../models/QSO.h"
#include "../contests/ContestBase.h"
#include "../controllers/DataIntegrityManager.h"

namespace TR4QT {

// Forward declarations
class CountryFile;
class QSOTableModel;

/**
 * Result structure for import operations
 */
struct ImportResult {
    bool success = false;
    QString errorMessage;
    int successCount = 0;
    int failureCount = 0;
    QString statusMessage;
    RescoreStats rescoreStats;  // Only populated if rescore was performed
};

/**
 * Result structure for export operations
 */
struct ExportResult {
    bool success = false;
    QString errorMessage;
    QString savedFilePath;  // Empty if not saved
    QString statusMessage;
};

/**
 * ImportExportManager
 *
 * Handles ADIF import/export and Cabrillo export operations.
 * Separates import/export business logic from MainWindow.
 *
 * Responsibilities:
 * - Show ADIF import dialog and save imported QSOs to database
 * - Run optional rescore after import
 * - Generate ADIF export with integrity checking
 * - Generate Cabrillo export with integrity checking
 * - Show export preview dialogs
 *
 * UI updates are NOT handled here - MainWindow uses the returned
 * Result structures to update status labels.
 */
class ImportExportManager {
public:
    /**
     * Configuration for ImportExportManager
     */
    struct Config {
        CountryFile* countryFile = nullptr;
        QSOTableModel* qsoTableModel = nullptr;
        ContestBase* activeContest = nullptr;
        int currentContestDbId = -1;
        QString currentContestName;
        QString currentDatabasePath;
        bool hasActiveContest = false;

        // Contest configuration for Cabrillo export
        QString category;        // SINGLE-OP, MULTI-OP, CHECKLOG, etc.
        QString powerClass;      // HIGH, LOW, QRP
        QString assisted;        // ASSISTED, NON-ASSISTED
    };

    /**
     * Construct an ImportExportManager
     * @param config Configuration with dependencies
     * @param parent Parent widget for dialogs
     */
    explicit ImportExportManager(const Config& config, QWidget* parent = nullptr);

    /**
     * Destructor
     */
    ~ImportExportManager();

    /**
     * Import QSOs from ADIF file
     *
     * Shows the ADIFImportDialog, processes selected file,
     * saves QSOs to database, optionally rescores contest.
     *
     * @return ImportResult with success/error and statistics
     */
    ImportResult importADIF();

    /**
     * Export QSOs to ADIF file
     *
     * Generates ADIF content from QSOTableModel,
     * runs integrity check, shows preview dialog.
     *
     * @return ExportResult with success/error and file path
     */
    ExportResult exportADIF();

    /**
     * Export QSOs to Cabrillo file
     *
     * Generates Cabrillo content from QSOTableModel,
     * runs integrity check, shows preview dialog.
     *
     * @return ExportResult with success/error and file path
     */
    ExportResult exportCabrillo();

    /**
     * Update configuration (for when contest changes)
     * @param config New configuration
     */
    void updateConfig(const Config& config);

private:
    /**
     * Run integrity check before export
     * @param exportFormat Format name for dialog titles ("ADIF" or "Cabrillo")
     * @param qsos List of QSOs to check
     * @return true to continue export, false to abort
     */
    bool runIntegrityCheckBeforeExport(const QString& exportFormat, const QList<QSO>& qsos);

private:
    Config m_config;
    QWidget* m_parent;
};

} // namespace TR4QT

#endif // IMPORTEXPORTMANAGER_H
