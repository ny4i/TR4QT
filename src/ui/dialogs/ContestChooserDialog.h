#ifndef CONTESTCHOOSERDIALOG_H
#define CONTESTCHOOSERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QPushButton>

namespace TR4QT {

/**
 * Contest information structure
 */
struct ContestInfo {
    QString contestId;          // Unique ID (used for database filename)
    QString contestName;        // Display name (e.g., "CQ WW DX CW 2024")
    QString contestType;        // "CQWW_CW", "CQWW_SSB", "CQWPX_CW", "CQWPX_SSB", "WFD"
    QDateTime startDate;        // Contest start date/time
    QString mode;               // "CW", "SSB", "Mixed"
    bool isExisting;            // true if resuming existing contest
    QString databasePath;       // Full path to database file
};

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
    void onContestTypeChanged(int index);
    void onExistingContestSelected();

private:
    void setupUI();
    void loadExistingContests();
    void populateContestTypes();
    QString generateContestId(const QString& type, const QDateTime& startDate);

    // UI components
    QListWidget* m_existingContestsList;
    QPushButton* m_resumeButton;
    QPushButton* m_deleteButton;

    QComboBox* m_contestTypeCombo;
    QLineEdit* m_contestNameEdit;
    QDateTimeEdit* m_startDateEdit;
    QComboBox* m_modeCombo;
    QPushButton* m_createButton;

    // Selected contest info
    ContestInfo m_contestInfo;
};

} // namespace TR4QT

#endif // CONTESTCHOOSERDIALOG_H
