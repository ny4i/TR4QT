#include "ContestChooserDialog.h"
#include "../../core/Constants.h"
#include "../../contests/ContestRegistry.h"
#include "../../contests/ContestMetadata.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDialogButtonBox>

namespace TR4QT {

ContestChooserDialog::ContestChooserDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Contest Chooser");
    setupUI();
    loadExistingContests();
    resize(700, 500);
}

void ContestChooserDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top section: Existing contests
    QGroupBox* existingGroup = new QGroupBox("Existing Contests", this);
    QVBoxLayout* existingLayout = new QVBoxLayout(existingGroup);

    m_existingContestsList = new QListWidget(this);
    m_existingContestsList->setAlternatingRowColors(true);
    connect(m_existingContestsList, &QListWidget::itemSelectionChanged,
            this, &ContestChooserDialog::onExistingContestSelected);
    connect(m_existingContestsList, &QListWidget::itemDoubleClicked,
            this, &ContestChooserDialog::onResumeContest);

    existingLayout->addWidget(m_existingContestsList);

    // Buttons for existing contests
    QHBoxLayout* existingButtonLayout = new QHBoxLayout();
    m_resumeButton = new QPushButton("Resume Selected", this);
    m_resumeButton->setEnabled(false);
    connect(m_resumeButton, &QPushButton::clicked, this, &ContestChooserDialog::onResumeContest);

    m_deleteButton = new QPushButton("Delete Selected", this);
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, &QPushButton::clicked, this, &ContestChooserDialog::onDeleteContest);

    existingButtonLayout->addWidget(m_resumeButton);
    existingButtonLayout->addWidget(m_deleteButton);
    existingButtonLayout->addStretch();

    existingLayout->addLayout(existingButtonLayout);
    mainLayout->addWidget(existingGroup);

    // Bottom section: New contest
    QGroupBox* newGroup = new QGroupBox("Create New Contest", this);
    QFormLayout* newLayout = new QFormLayout(newGroup);

    // Contest type
    m_contestTypeCombo = new QComboBox(this);
    populateContestTypes();
    connect(m_contestTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ContestChooserDialog::onContestTypeChanged);
    newLayout->addRow("Contest Type:", m_contestTypeCombo);

    // Contest name
    m_contestNameEdit = new QLineEdit(this);
    m_contestNameEdit->setPlaceholderText("e.g., CQ WW DX CW 2024");
    newLayout->addRow("Contest Name:", m_contestNameEdit);

    // Start date/time
    m_startDateEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    newLayout->addRow("Start Date/Time:", m_startDateEdit);

    // Mode
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItems({"CW", "SSB", "Mixed"});
    newLayout->addRow("Mode:", m_modeCombo);

    // Create button
    QHBoxLayout* createButtonLayout = new QHBoxLayout();
    m_createButton = new QPushButton("Create New Contest", this);
    m_createButton->setDefault(true);
    connect(m_createButton, &QPushButton::clicked, this, &ContestChooserDialog::onNewContest);
    createButtonLayout->addWidget(m_createButton);
    createButtonLayout->addStretch();

    newLayout->addRow("", createButtonLayout);
    mainLayout->addWidget(newGroup);

    // Cancel button at bottom
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Initialize with default contest type
    onContestTypeChanged(0);
}

void ContestChooserDialog::populateContestTypes() {
    // Get all registered contests from the factory
    QList<ContestMetadata> contests = ContestRegistry::instance().availableContests();

    for (const ContestMetadata& meta : contests) {
        if (meta.hasSeparateContests) {
            // Add separate entries for CW and SSB/Phone
            for (ModeType mode : meta.supportedModes) {
                if (mode == ModeType::None) continue;  // Skip "Mixed" mode indicator

                QString displayName = meta.getDisplayName(mode);
                // Store base contest ID without mode suffix
                // Mode is selected separately via mode combo box
                m_contestTypeCombo->addItem(displayName, meta.id);
            }
        } else {
            // Single entry for mixed-mode contests
            m_contestTypeCombo->addItem(meta.displayName, meta.id);
        }
    }
}

void ContestChooserDialog::loadExistingContests() {
    m_existingContestsList->clear();

    // Get database directory
    QString dbDir = QDir::homePath() + "/" + QString(DB_DIR);
    QDir dir(dbDir);

    if (!dir.exists()) {
        return;
    }

    // Find all .db files
    QStringList filters;
    filters << "*.db";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    for (const QFileInfo& fileInfo : files) {
        QString fileName = fileInfo.fileName();
        QString displayName = fileName;
        displayName.replace(".db", "");
        displayName.replace("_", " ");

        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setToolTip(fileInfo.absoluteFilePath());
        m_existingContestsList->addItem(item);
    }
}

void ContestChooserDialog::onContestTypeChanged(int index) {
    Q_UNUSED(index);

    QString contestType = m_contestTypeCombo->currentData().toString();
    QDateTime now = QDateTime::currentDateTime();

    // Parse contest ID and mode from contestType (e.g., "CQWW_CW" or "WFD")
    QStringList parts = contestType.split('_');
    QString contestId = parts[0];
    QString modeStr = parts.size() > 1 ? parts[1] : "Mixed";

    // Get contest metadata from registry
    if (ContestRegistry::instance().hasContest(contestId)) {
        ContestMetadata meta = ContestRegistry::instance().getMetadata(contestId);

        // Auto-generate contest name: "Contest Name MODE YEAR"
        QString name;
        if (meta.hasSeparateContests) {
            name = QString("%1 %2 %3")
                .arg(meta.displayName)
                .arg(modeStr)
                .arg(now.date().year());
        } else {
            name = QString("%1 %2")
                .arg(meta.displayName)
                .arg(now.date().year());
        }

        m_contestNameEdit->setText(name);
        m_modeCombo->setCurrentText(modeStr);
    }
}

void ContestChooserDialog::onExistingContestSelected() {
    bool hasSelection = !m_existingContestsList->selectedItems().isEmpty();
    m_resumeButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

void ContestChooserDialog::onResumeContest() {
    QListWidgetItem* item = m_existingContestsList->currentItem();
    if (!item) {
        return;
    }

    QString dbPath = item->data(Qt::UserRole).toString();

    // Parse contest info from filename
    QFileInfo fileInfo(dbPath);
    QString baseName = fileInfo.baseName();

    m_contestInfo.isExisting = true;
    m_contestInfo.databasePath = dbPath;
    m_contestInfo.contestId = baseName;
    m_contestInfo.contestName = item->text();

    // Try to determine contest type from filename
    if (baseName.contains("CQWW", Qt::CaseInsensitive) && baseName.contains("CW", Qt::CaseInsensitive)) {
        m_contestInfo.contestType = "CQWW_CW";
        m_contestInfo.mode = "CW";
    } else if (baseName.contains("CQWW", Qt::CaseInsensitive)) {
        m_contestInfo.contestType = "CQWW_SSB";
        m_contestInfo.mode = "SSB";
    } else if (baseName.contains("WPX", Qt::CaseInsensitive) && baseName.contains("CW", Qt::CaseInsensitive)) {
        m_contestInfo.contestType = "CQWPX_CW";
        m_contestInfo.mode = "CW";
    } else if (baseName.contains("WPX", Qt::CaseInsensitive)) {
        m_contestInfo.contestType = "CQWPX_SSB";
        m_contestInfo.mode = "SSB";
    } else if (baseName.contains("WFD", Qt::CaseInsensitive) || baseName.contains("Winter", Qt::CaseInsensitive)) {
        m_contestInfo.contestType = "WFD";
        m_contestInfo.mode = "Mixed";
    } else {
        m_contestInfo.contestType = "CQWW_CW";  // Default
        m_contestInfo.mode = "CW";
    }

    accept();
}

void ContestChooserDialog::onDeleteContest() {
    QListWidgetItem* item = m_existingContestsList->currentItem();
    if (!item) {
        return;
    }

    QString contestName = item->text();
    QString dbPath = item->data(Qt::UserRole).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Contest?",
        QString("Are you sure you want to delete the contest:\n\n%1\n\nThis action cannot be undone!")
            .arg(contestName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QFile file(dbPath);
        if (file.remove()) {
            QMessageBox::information(this, "Contest Deleted",
                                   QString("Contest '%1' has been deleted.").arg(contestName));
            loadExistingContests();  // Refresh list
        } else {
            QMessageBox::warning(this, "Delete Failed",
                               QString("Failed to delete contest database:\n%1").arg(file.errorString()));
        }
    }
}

void ContestChooserDialog::onNewContest() {
    QString contestName = m_contestNameEdit->text().trimmed();
    if (contestName.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a contest name.");
        m_contestNameEdit->setFocus();
        return;
    }

    QString contestType = m_contestTypeCombo->currentData().toString();
    QDateTime startDate = m_startDateEdit->dateTime();
    QString mode = m_modeCombo->currentText();

    // Generate contest ID (used for database filename)
    QString contestId = generateContestId(contestType, startDate);

    // Create database path
    QString dbDir = QDir::homePath() + "/" + QString(DB_DIR);
    QDir dir;
    if (!dir.exists(dbDir)) {
        dir.mkpath(dbDir);
    }

    QString dbPath = dbDir + "/" + contestId + ".db";

    // Check if file already exists
    if (QFile::exists(dbPath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Contest Exists",
            QString("A contest with this name already exists:\n\n%1\n\nDo you want to resume it instead?")
                .arg(contestId),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_contestInfo.isExisting = true;
        } else {
            return;  // User cancelled
        }
    } else {
        m_contestInfo.isExisting = false;
    }

    // Fill contest info
    m_contestInfo.contestId = contestId;
    m_contestInfo.contestName = contestName;
    m_contestInfo.contestType = contestType;
    m_contestInfo.startDate = startDate;
    m_contestInfo.mode = mode;
    m_contestInfo.databasePath = dbPath;

    accept();
}

QString ContestChooserDialog::generateContestId(const QString& type, const QDateTime& startDate) {
    // Generate a unique contest ID from type and date
    // Format: CONTESTTYPE_YYYY_MM_DD
    QString id = type;
    id += "_" + startDate.toString("yyyy_MM_dd");
    return id;
}

} // namespace TR4QT
