#ifndef CONTESTCHOOSERDIALOG_H
#define CONTESTCHOOSERDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include "../../contests/ContestBase.h"
#include "../../models/ContestInfo.h"  // ContestInfo struct (shared with headless server)

namespace TR4QT {

/**
 * Contest Chooser Dialog
 * Allows user to:
 * - Create a new contest log
 * - Resume an existing contest
 * - Delete old contests
 * - Set contest parameters (type, dates, mode)
 */
class ContestChooserDialog : public QDialog {
    Q_OBJECT

public:
    explicit ContestChooserDialog(QWidget* parent = nullptr);
    ~ContestChooserDialog() override = default;

    // Get selected contest information
    ContestInfo getContestInfo() const { return m_contestInfo; }

    // Check if user wants to resume existing or create new
    bool isResumingExisting() const { return m_contestInfo.isExisting; }

private slots:
    void onNewContest();
    void onResumeContest();
    void onDeleteContest();
    void onShowDatabaseFolder();
    void onContestTypeChanged(int index);
    void onExistingContestSelected();

private:
    void setupUI();
    void loadExistingContests();
    void populateContestTypes();
    QString generateContestId(const QString& type, const QDateTime& startDate);

    /**
     * Update dynamic config fields based on selected contest type
     * Queries getConfigFields() from contest class and creates UI widgets
     */
    void updateConfigFields(const QString& contestType);

    /**
     * Clear all dynamic config field widgets
     */
    void clearConfigFields();

    /**
     * Get values from dynamic config field widgets
     * @return Map of field ID to value
     */
    QMap<QString, QString> getConfigFieldValues() const;

    // UI components
    QTableWidget* m_existingContestsTable;
    QPushButton* m_resumeButton;
    QPushButton* m_deleteButton;

    QComboBox* m_contestTypeCombo;
    QLineEdit* m_contestNameEdit;
    QDateTimeEdit* m_startDateEdit;
    QComboBox* m_modeCombo;
    QLineEdit* m_exchangeSentEdit;
    QLabel* m_exchangeSentLabel;

    // Contest configuration fields (Cabrillo export - always visible)
    QComboBox* m_categoryCombo;
    QComboBox* m_powerClassCombo;
    QComboBox* m_assistedCombo;

    QPushButton* m_createButton;

    // Dynamic config fields from contest class
    QFormLayout* m_configFieldsLayout;      // Layout for dynamic fields
    QList<ContestConfigField> m_configFields;  // Current field definitions
    QList<QWidget*> m_configFieldWidgets;   // Dynamic widgets (for cleanup)

    // Selected contest info
    ContestInfo m_contestInfo;
};

} // namespace TR4QT

#endif // CONTESTCHOOSERDIALOG_H
