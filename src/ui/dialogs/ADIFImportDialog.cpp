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

#include "ADIFImportDialog.h"
#include "../../importers/ADIFImporter.h"
#include "../../importers/ADIFValidationError.h"
#include "../../utils/CountryFile.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include "../../utils/DialogHelper.h"
#include <QFileInfo>
#include <QCheckBox>
#include <QDir>
#include <QStandardPaths>

namespace TR4QT {

ADIFImportDialog::ADIFImportDialog(CountryFile* countryFile, QWidget* parent)
    : QDialog(parent)
    , m_countryFile(countryFile)
{
    setupUI();
}

void ADIFImportDialog::setupUI() {
    setWindowTitle("Import ADIF File");
    setMinimumWidth(600);
    setMinimumHeight(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // File selection section
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel("ADIF File:"));

    m_fileLabel = new QLabel("<No file selected>");
    m_fileLabel->setWordWrap(true);
    m_fileLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    fileLayout->addWidget(m_fileLabel, 1);

    m_selectFileButton = new QPushButton("Browse...");
    connect(m_selectFileButton, &QPushButton::clicked, this, &ADIFImportDialog::onSelectFileClicked);
    fileLayout->addWidget(m_selectFileButton);

    mainLayout->addLayout(fileLayout);

    // Validation options
    m_autoCorrectCheckBox = new QCheckBox("Auto-correct validation errors (recommended)");
    m_autoCorrectCheckBox->setChecked(true);
    m_autoCorrectCheckBox->setToolTip("Automatically fix data quality issues like country/state mismatches.\n"
                                       "If unchecked, you'll be prompted to review errors before importing.");
    mainLayout->addWidget(m_autoCorrectCheckBox);

    // Rescore option
    m_rescoreCheckBox = new QCheckBox("Rescore contest after import (recommended)");
    m_rescoreCheckBox->setChecked(true);
    m_rescoreCheckBox->setToolTip("Automatically recalculate QSO points, multipliers, and duplicates after import.\n"
                                   "This ensures imported QSOs are scored correctly for the active contest.");
    mainLayout->addWidget(m_rescoreCheckBox);

    // Status section
    m_statusLabel = new QLabel("Ready to import. Select an ADIF file to begin.");
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    // Results text area
    QLabel* resultsLabel = new QLabel("Import Results:");
    mainLayout->addWidget(resultsLabel);

    m_resultsText = new QTextEdit();
    m_resultsText->setReadOnly(true);
    m_resultsText->setMinimumHeight(200);
    m_resultsText->setPlainText("No import performed yet.");
    mainLayout->addWidget(m_resultsText);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_importButton = new QPushButton("Import");
    m_importButton->setEnabled(false);
    connect(m_importButton, &QPushButton::clicked, this, &ADIFImportDialog::onImportClicked);
    buttonLayout->addWidget(m_importButton);

    m_closeButton = new QPushButton("Close");
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void ADIFImportDialog::onSelectFileClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select ADIF File",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),  // Default to Desktop
        "ADIF Files (*.adi *.adif);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        m_selectedFile = fileName;
        m_fileLabel->setText(fileName);
        m_fileLabel->setStyleSheet("QLabel { color: #000; font-style: normal; }");
        m_importButton->setEnabled(true);
        updateStatus(QString("Selected: %1. Click Import to begin.").arg(QFileInfo(fileName).fileName()));
    }
}

void ADIFImportDialog::onImportClicked() {
    if (m_selectedFile.isEmpty()) {
        DialogHelper::warning(this, "No File Selected", "Please select an ADIF file to import.");
        return;
    }

    // Disable buttons during import
    m_importButton->setEnabled(false);
    m_selectFileButton->setEnabled(false);
    m_closeButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    updateStatus("Importing ADIF file...");
    m_resultsText->clear();
    m_validationDetails.clear();

    // Perform import with validation (using MainWindow's CountryFile)
    ADIFImporter importer;
    if (m_countryFile) {
        importer.setCountryFile(m_countryFile);
    }
    QList<QSO> qsos;

    m_progressBar->setValue(50);
    updateStatus("Parsing and validating ADIF records...");

    bool success = importer.importFile(m_selectedFile, qsos);

    m_progressBar->setValue(100);

    // Collect validation details
    QList<ADIFValidationError> validationErrors = importer.validationErrors();
    int errorCount = 0;
    int warningCount = 0;

    if (!validationErrors.isEmpty()) {
        m_validationDetails = "\n=== Validation Issues ===\n\n";

        for (const auto& error : validationErrors) {
            if (error.severity == ADIFValidationError::Error) {
                errorCount++;
            } else {
                warningCount++;
            }

            m_validationDetails += error.toString() + "\n";
        }
    }

    // Show results
    if (success) {
        m_importedQSOs = qsos;
        showResults(importer.importedCount(), importer.failedCount(),
                    importer.warnings(), errorCount, warningCount);

        if (importer.failedCount() == 0 && errorCount == 0) {
            updateStatus(QString("Import completed successfully! %1 QSOs imported.")
                .arg(importer.importedCount()));
        } else if (errorCount > 0) {
            updateStatus(QString("Import completed with data quality issues. %1 succeeded, %2 failed, %3 validation errors.")
                .arg(importer.importedCount()).arg(importer.failedCount()).arg(errorCount));
        } else {
            updateStatus(QString("Import completed with warnings. %1 succeeded, %2 failed.")
                .arg(importer.importedCount()).arg(importer.failedCount()));
        }

        // Enable OK button to accept dialog
        m_closeButton->setText("OK");
        m_closeButton->setEnabled(true);
        m_closeButton->disconnect();
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    } else {
        m_importedQSOs.clear();
        updateStatus(QString("Import failed: %1").arg(importer.lastError()));
        m_resultsText->setPlainText(QString("Error: %1").arg(importer.lastError()));

        // Re-enable controls for retry
        m_importButton->setEnabled(true);
        m_selectFileButton->setEnabled(true);
        m_closeButton->setEnabled(true);
    }

    m_progressBar->setVisible(false);
}

void ADIFImportDialog::updateStatus(const QString& message) {
    m_statusLabel->setText(message);
    LOG_INFO("ADIFImportDialog", message);
}

void ADIFImportDialog::showResults(int imported, int failed, const QStringList& warnings,
                                    int validationErrors, int validationWarnings) {
    QString results;

    results += QString("=== Import Summary ===\n\n");
    results += QString("Successfully imported: %1 QSOs\n").arg(imported);

    if (failed > 0) {
        results += QString("Failed to import: %1 records\n").arg(failed);

        if (!warnings.isEmpty()) {
            results += QString("\nImport Warnings (%1):\n").arg(warnings.size());
            for (int i = 0; i < warnings.size() && i < 20; i++) {
                results += QString("  %1\n").arg(warnings[i]);
            }
            if (warnings.size() > 20) {
                results += QString("  ... and %1 more warnings\n").arg(warnings.size() - 20);
            }
        }
    }

    // Validation errors/warnings
    if (validationErrors > 0 || validationWarnings > 0) {
        results += QString("\n=== Data Quality Issues ===\n\n");

        if (validationErrors > 0) {
            results += QString("ERRORS: %1 record(s) have data inconsistencies\n").arg(validationErrors);
        }

        if (validationWarnings > 0) {
            results += QString("WARNINGS: %1 record(s) have minor issues\n").arg(validationWarnings);
        }

        // Append detailed validation output
        if (!m_validationDetails.isEmpty()) {
            results += m_validationDetails;
        }

        results += QString("\nNote: QSOs were imported despite validation issues.\n");
        results += QString("Please review the data quality issues above.\n");
    } else if (failed == 0) {
        results += QString("\nAll records imported successfully with no validation issues!");
    }

    m_resultsText->setPlainText(results);
}

bool ADIFImportDialog::shouldRescore() const {
    return m_rescoreCheckBox->isChecked();
}

} // namespace TR4QT
