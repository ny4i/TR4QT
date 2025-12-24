#ifndef BACKUPRESTOREDIALOG_H
#define BACKUPRESTOREDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include "../../data/BackupManager.h"
#include "ContestChooserDialog.h"

namespace TR4QT {

/**
 * Dialog for backup and restore operations
 *
 * Provides two tabs:
 * 1. Backup - Manual backup with location selection
 * 2. Restore - List and restore from available backups
 */
class BackupRestoreDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param contestInfo Current contest information
     * @param parent Parent widget
     */
    explicit BackupRestoreDialog(const ContestInfo& contestInfo, QWidget* parent = nullptr);
    ~BackupRestoreDialog() override = default;

private slots:
    // Backup tab slots
    void onBrowseBackupLocation();
    void onCreateBackup();

    // Restore tab slots
    void onRefreshBackupList();
    void onBackupSelected(QListWidgetItem* item);
    void onRestoreBackup();

private:
    // UI setup
    void setupUI();
    QWidget* createBackupTab();
    QWidget* createRestoreTab();

    // Helper methods
    void updateBackupPreview();
    void loadBackupList();
    QString formatFileSize(qint64 bytes);
    bool confirmRestore(const QString& backupName);

    // Contest info
    ContestInfo m_contestInfo;

    // Backup tab widgets
    QLineEdit* m_backupLocationEdit;
    QPushButton* m_browseButton;
    QLabel* m_backupPreviewLabel;
    QPushButton* m_createBackupButton;

    // Restore tab widgets
    QListWidget* m_backupListWidget;
    QPushButton* m_refreshButton;
    QLabel* m_restorePreviewLabel;
    QPushButton* m_restoreButton;

    // Progress
    QProgressBar* m_progressBar;

    // Tabs
    QTabWidget* m_tabWidget;
};

} // namespace TR4QT

#endif // BACKUPRESTOREDIALOG_H
