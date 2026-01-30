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

/**
 * MaintenanceService - Implementation
 */

#include "MaintenanceService.h"
#include "../data/QSORepository.h"
#include "../data/ExchangeMemoryRepository.h"
#include "../data/BackupManager.h"
#include "../utils/PathManager.h"
#include "../utils/DialogHelper.h"
#include "../logging/LogMacros.h"
#include <QFileInfo>
#include <QMessageBox>

namespace TR4QT {

MaintenanceService::MaintenanceService(QWidget* parentWidget)
    : m_parentWidget(parentWidget)
{
}

ClearLogResult MaintenanceService::clearLogWithBackup(const ClearLogRequest& request) {
    ClearLogResult result;

    LOG_INFO("MaintenanceService", QString("clearLogWithBackup called for contestDbId=%1, qsoCount=%2")
             .arg(request.contestDbId).arg(request.qsoCount));

    // Step 1: Check if already empty
    if (request.qsoCount == 0) {
        result.status = ClearLogResult::Status::AlreadyEmpty;
        LOG_DEBUG("MaintenanceService", "Log already empty");
        return result;
    }

    // Step 2: Ask about backup
    QMessageBox::StandardButton backupReply = DialogHelper::question(
        m_parentWidget, "Create Backup?",
        QString("Would you like to create a backup before clearing %1 QSOs?\n\n"
                "The backup will be saved as an archived copy for safety.")
            .arg(request.qsoCount),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);

    if (backupReply == QMessageBox::Cancel) {
        result.status = ClearLogResult::Status::UserCancelled;
        LOG_DEBUG("MaintenanceService", "User cancelled at backup prompt");
        return result;
    }

    // Step 3: Create backup if requested
    if (backupReply == QMessageBox::Yes) {
        QString backupPath;
        if (!createBackup(request.databasePath, backupPath)) {
            // Backup failed - ask if user wants to continue
            QMessageBox::StandardButton continueReply = DialogHelper::warning(
                m_parentWidget, "Backup Failed",
                QString("Failed to create backup.\n\nDo you still want to clear the log?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (continueReply != QMessageBox::Yes) {
                result.status = ClearLogResult::Status::BackupFailedUserAborted;
                LOG_WARN("MaintenanceService", "Backup failed and user chose not to continue");
                return result;
            }
            // User chose to continue despite backup failure
            result.status = ClearLogResult::Status::BackupFailed;
        } else {
            result.backupCreated = true;
            result.backupPath = backupPath;
            LOG_INFO("MaintenanceService", QString("Backup created: %1").arg(backupPath));
        }
    }

    // Step 4: Confirm clear
    QMessageBox::StandardButton clearReply = DialogHelper::question(
        m_parentWidget, "Clear Log",
        QString("Are you sure you want to clear all %1 QSOs from the log?\n\nThis action cannot be undone.")
            .arg(request.qsoCount),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (clearReply != QMessageBox::Yes) {
        result.status = ClearLogResult::Status::UserCancelled;
        LOG_DEBUG("MaintenanceService", "User cancelled at clear confirmation");
        return result;
    }

    // Step 5: Clear database
    QString errorMessage;
    if (!clearDatabase(request.contestDbId, errorMessage)) {
        result.status = ClearLogResult::Status::ClearFailed;
        result.errorMessage = errorMessage;
        LOG_ERROR("MaintenanceService", QString("Failed to clear database: %1").arg(errorMessage));
        return result;
    }

    // Step 6: Clear exchange memory
    clearExchangeMemory(request.contestType);

    // Success!
    result.status = ClearLogResult::Status::Success;
    LOG_INFO("MaintenanceService", "Log cleared successfully");

    return result;
}

ClearLogResult MaintenanceService::clearLogHeadless(const ClearLogRequest& request, bool doCreateBackup) {
    ClearLogResult result;

    LOG_INFO("MaintenanceService", QString("clearLogHeadless called for contestDbId=%1").arg(request.contestDbId));

    // Check if already empty
    if (request.qsoCount == 0) {
        result.status = ClearLogResult::Status::AlreadyEmpty;
        return result;
    }

    // Create backup if requested
    if (doCreateBackup) {
        QString backupPath;
        if (createBackup(request.databasePath, backupPath)) {
            result.backupCreated = true;
            result.backupPath = backupPath;
        } else {
            result.status = ClearLogResult::Status::BackupFailed;
            return result;
        }
    }

    // Clear database
    QString errorMessage;
    if (!clearDatabase(request.contestDbId, errorMessage)) {
        result.status = ClearLogResult::Status::ClearFailed;
        result.errorMessage = errorMessage;
        return result;
    }

    // Clear exchange memory
    clearExchangeMemory(request.contestType);

    result.status = ClearLogResult::Status::Success;
    return result;
}

bool MaintenanceService::createBackup(const QString& databasePath, QString& backupPath) {
    BackupManager& backupMgr = BackupManager::instance();
    QString backupDir = PathManager::getBackupsDir();

    if (!backupMgr.createBackup(databasePath, backupDir, backupPath)) {
        LOG_ERROR("MaintenanceService", QString("Backup failed: %1").arg(backupMgr.lastError()));
        return false;
    }

    LOG_INFO("MaintenanceService", QString("Backup created at: %1").arg(backupPath));
    return true;
}

bool MaintenanceService::clearDatabase(int contestDbId, QString& errorMessage) {
    QSORepository repo;
    if (!repo.deleteAllQSOs(contestDbId)) {
        errorMessage = repo.lastError();
        return false;
    }
    return true;
}

void MaintenanceService::clearExchangeMemory(const QString& contestType) {
    ExchangeMemoryRepository memRepo;
    memRepo.clearForContest(contestType);
    LOG_DEBUG("MaintenanceService", QString("Cleared exchange memory for contest type: %1").arg(contestType));
}

} // namespace TR4QT
