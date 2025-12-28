#include "BackupRestoreDialog.h"
#include "../../data/Database.h"
#include "../../data/QSORepository.h"
#include "../../utils/AppSettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QInputDialog>

namespace TR4QT {

BackupRestoreDialog::BackupRestoreDialog(const ContestInfo& contestInfo, QWidget* parent)
    : QDialog(parent)
    , m_contestInfo(contestInfo)
{
    setWindowTitle("Backup and Restore - " + contestInfo.contestName);
    setMinimumSize(600, 500);

    setupUI();
}

void BackupRestoreDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createBackupTab(), "Backup");
    m_tabWidget->addTab(createRestoreTab(), "Restore");

    mainLayout->addWidget(m_tabWidget);

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Initialize
    updateBackupPreview();
    loadBackupList();
}

QWidget* BackupRestoreDialog::createBackupTab() {
    QWidget* widget = new QWidget(this);
    widget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Backup location group
    QGroupBox* locationGroup = new QGroupBox("Backup Location", widget);
    QFormLayout* locationLayout = new QFormLayout(locationGroup);

    m_backupLocationEdit = new QLineEdit(widget);
    m_backupLocationEdit->setPlaceholderText("Leave empty for default location");

    // Load saved backup directory
    BackupManager& backup = BackupManager::instance();
    QString backupDir = backup.getBackupDirectory();
    if (backupDir.isEmpty()) {
        backupDir = QDir::homePath() + "/.tr4qt/backups";
    }
    m_backupLocationEdit->setText(backupDir);

    m_browseButton = new QPushButton("Browse...", widget);
    connect(m_browseButton, &QPushButton::clicked,
            this, &BackupRestoreDialog::onBrowseBackupLocation);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(m_backupLocationEdit);
    pathLayout->addWidget(m_browseButton);

    locationLayout->addRow("Directory:", pathLayout);

    layout->addWidget(locationGroup);

    // Preview group
    QGroupBox* previewGroup = new QGroupBox("Backup Preview", widget);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_backupPreviewLabel = new QLabel(widget);
    m_backupPreviewLabel->setWordWrap(true);
    previewLayout->addWidget(m_backupPreviewLabel);

    layout->addWidget(previewGroup);

    // Create backup button
    m_createBackupButton = new QPushButton("Create Backup Now", widget);
    m_createBackupButton->setStyleSheet("QPushButton { padding: 10px; font-weight: bold; border-radius: 3px; }");
    connect(m_createBackupButton, &QPushButton::clicked,
            this, &BackupRestoreDialog::onCreateBackup);

    layout->addWidget(m_createBackupButton);

    layout->addStretch();

    return widget;
}

QWidget* BackupRestoreDialog::createRestoreTab() {
    QWidget* widget = new QWidget(this);
    widget->setAutoFillBackground(true);  // Prevent transparent/blank rendering
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // List group
    QGroupBox* listGroup = new QGroupBox("Available Backups", widget);
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    // Refresh button
    m_refreshButton = new QPushButton("Refresh List", widget);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &BackupRestoreDialog::onRefreshBackupList);
    listLayout->addWidget(m_refreshButton);

    // Backup list
    m_backupListWidget = new QListWidget(widget);
    connect(m_backupListWidget, &QListWidget::itemClicked,
            this, &BackupRestoreDialog::onBackupSelected);
    connect(m_backupListWidget, &QListWidget::itemDoubleClicked,
            this, &BackupRestoreDialog::onRestoreBackup);
    listLayout->addWidget(m_backupListWidget);

    layout->addWidget(listGroup);

    // Preview group
    QGroupBox* previewGroup = new QGroupBox("Selected Backup Details", widget);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_restorePreviewLabel = new QLabel("Select a backup from the list", widget);
    m_restorePreviewLabel->setWordWrap(true);
    previewLayout->addWidget(m_restorePreviewLabel);

    layout->addWidget(previewGroup);

    // Restore button
    m_restoreButton = new QPushButton("Restore from Selected Backup", widget);
    m_restoreButton->setStyleSheet("QPushButton { padding: 10px; font-weight: bold; border-radius: 3px; }");
    m_restoreButton->setEnabled(false);
    connect(m_restoreButton, &QPushButton::clicked,
            this, &BackupRestoreDialog::onRestoreBackup);

    layout->addWidget(m_restoreButton);

    return widget;
}

void BackupRestoreDialog::onBrowseBackupLocation() {
    QString currentPath = m_backupLocationEdit->text();
    if (currentPath.isEmpty()) {
        currentPath = QDir::homePath();
    }

    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Backup Directory",
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_backupLocationEdit->setText(dir);
        updateBackupPreview();
    }
}

void BackupRestoreDialog::onCreateBackup() {
    QString backupDir = m_backupLocationEdit->text().trimmed();

    // Validate directory
    if (!backupDir.isEmpty() && !QDir(backupDir).exists()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Create Directory?",
            QString("Backup directory does not exist:\n\n%1\n\nCreate it?").arg(backupDir),
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }

        QDir dir;
        if (!dir.mkpath(backupDir)) {
            QMessageBox::critical(this, "Error",
                                "Failed to create backup directory:\n" + backupDir);
            return;
        }
    }

    // Create backup
    m_createBackupButton->setEnabled(false);
    m_createBackupButton->setText("Creating backup...");

    BackupManager& backup = BackupManager::instance();
    QString backupPath;

    bool success = backup.createBackup(m_contestInfo.databasePath, backupDir, backupPath);

    m_createBackupButton->setEnabled(true);
    m_createBackupButton->setText("Create Backup Now");

    if (success) {
        QFileInfo backupInfo(backupPath);
        QMessageBox::information(this, "Backup Complete",
                               QString("Backup created successfully:\n\n"
                                     "%1\n\nSize: %2")
                                   .arg(backupInfo.fileName())
                                   .arg(formatFileSize(backupInfo.size())));

        // Refresh backup list if on restore tab
        if (m_tabWidget->currentIndex() == 1) {
            loadBackupList();
        }
    } else {
        QMessageBox::critical(this, "Backup Failed",
                            "Failed to create backup:\n\n" + backup.lastError());
    }
}

void BackupRestoreDialog::onRefreshBackupList() {
    loadBackupList();
}

void BackupRestoreDialog::onBackupSelected(QListWidgetItem* item) {
    if (!item) {
        m_restorePreviewLabel->setText("Select a backup from the list");
        m_restoreButton->setEnabled(false);
        return;
    }

    // Get backup info from item data
    BackupInfo info = item->data(Qt::UserRole).value<BackupInfo>();

    // Update preview
    QString preview;
    preview += "<b>File:</b> " + info.fileName + "<br>";
    preview += "<b>Date:</b> " + info.timestamp.toString("yyyy-MM-dd HH:mm:ss") + "<br>";
    preview += "<b>Size:</b> " + formatFileSize(info.fileSize) + "<br>";
    preview += "<b>QSOs:</b> " + QString::number(info.qsoCount) + "<br>";
    preview += "<b>Valid:</b> " + QString(info.isValid ? "Yes" : "No") + "<br>";

    m_restorePreviewLabel->setText(preview);
    m_restoreButton->setEnabled(info.isValid);
}

void BackupRestoreDialog::onRestoreBackup() {
    QListWidgetItem* item = m_backupListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "No Selection",
                           "Please select a backup to restore.");
        return;
    }

    BackupInfo info = item->data(Qt::UserRole).value<BackupInfo>();

    if (!info.isValid) {
        QMessageBox::critical(this, "Invalid Backup",
                            "The selected backup is not a valid SQLite database.");
        return;
    }

    // Confirm restore
    if (!confirmRestore(info.fileName)) {
        return;
    }

    // Perform restore
    m_restoreButton->setEnabled(false);
    m_restoreButton->setText("Restoring...");

    BackupManager& backup = BackupManager::instance();
    bool success = backup.restoreFromBackup(info.filePath, m_contestInfo.databasePath);

    m_restoreButton->setEnabled(true);
    m_restoreButton->setText("Restore from Selected Backup");

    if (success) {
        QMessageBox::information(this, "Restore Complete",
                               QString("Database restored successfully from:\n\n%1\n\n"
                                     "Please restart the application to reload the contest.")
                                   .arg(info.fileName));
        accept();  // Close dialog
    } else {
        QMessageBox::critical(this, "Restore Failed",
                            "Failed to restore from backup:\n\n" + backup.lastError());
    }
}

void BackupRestoreDialog::updateBackupPreview() {
    BackupManager& backup = BackupManager::instance();

    // Get current QSO count
    QSORepository repo;
    Database& db = Database::instance();

    int qsoCount = 0;
    if (db.isOpen()) {
        // Get contest database ID
        QSqlQuery query = db.execute(
            "SELECT id FROM contests WHERE contest_id = ?",
            {m_contestInfo.contestId});

        if (query.next()) {
            int contestDbId = query.value(0).toInt();
            qsoCount = repo.getQSOCount(contestDbId);
        }
    }

    // Generate backup filename preview
    QString backupDir = m_backupLocationEdit->text().trimmed();
    if (backupDir.isEmpty()) {
        backupDir = QDir::homePath() + "/.tr4qt/backups";
    }

    QFileInfo dbInfo(m_contestInfo.databasePath);
    QString backupFileName = backup.generateBackupFileName(dbInfo.fileName());

    // Update preview
    QString preview;
    preview += "<b>Backup will be created as:</b><br>";
    preview += backupFileName + "<br><br>";
    preview += "<b>Location:</b><br>";
    preview += backupDir + "<br><br>";
    preview += "<b>Current QSOs:</b> " + QString::number(qsoCount);

    m_backupPreviewLabel->setText(preview);
}

void BackupRestoreDialog::loadBackupList() {
    m_backupListWidget->clear();

    QString backupDir = m_backupLocationEdit->text().trimmed();
    if (backupDir.isEmpty()) {
        backupDir = QDir::homePath() + "/.tr4qt/backups";
    }

    // Get base name for filtering
    QFileInfo dbInfo(m_contestInfo.databasePath);
    BackupManager& backup = BackupManager::instance();
    QString baseName = backup.extractBaseName(dbInfo.fileName());

    // List backups
    QList<BackupInfo> backups = backup.listBackups(backupDir, baseName);

    if (backups.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No backups found");
        item->setFlags(Qt::NoItemFlags);
        m_backupListWidget->addItem(item);
        return;
    }

    // Add backups to list
    for (const BackupInfo& info : backups) {
        QString displayText = QString("%1 - %2 QSOs - %3")
                                  .arg(info.timestamp.toString("yyyy-MM-dd HH:mm"))
                                  .arg(info.qsoCount)
                                  .arg(formatFileSize(info.fileSize));

        if (!info.isValid) {
            displayText += " (INVALID)";
        }

        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, QVariant::fromValue(info));

        if (!info.isValid) {
            item->setForeground(QColor(Qt::red));
        }

        m_backupListWidget->addItem(item);
    }
}

QString BackupRestoreDialog::formatFileSize(qint64 bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
}

bool BackupRestoreDialog::confirmRestore(const QString& backupName) {
    // First confirmation
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        "Confirm Restore",
        QString("<b>WARNING: This will REPLACE your current contest database!</b><br><br>"
              "Backup: %1<br><br>"
              "The current database will be saved as .broken for safety.<br><br>"
              "Are you sure you want to continue?")
            .arg(backupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return false;
    }

    // Second confirmation - type contest name
    bool ok;
    QString contestName = QInputDialog::getText(
        this,
        "Confirm Restore",
        QString("To confirm, type the contest name:\n\n%1").arg(m_contestInfo.contestName),
        QLineEdit::Normal,
        QString(),
        &ok);

    if (!ok || contestName.trimmed() != m_contestInfo.contestName) {
        QMessageBox::information(this, "Cancelled",
                               "Restore cancelled - contest name did not match.");
        return false;
    }

    return true;
}

} // namespace TR4QT

// Required for QVariant::fromValue<BackupInfo>
Q_DECLARE_METATYPE(TR4QT::BackupInfo)
