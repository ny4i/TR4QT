#include "ContestChooserDialog.h"
#include "../../core/Constants.h"
#include "../../contests/ContestRegistry.h"
#include "../../contests/ContestMetadata.h"
#include "../../utils/DialogHelper.h"
#include "../../utils/PathManager.h"
#include "../../logging/LogMacros.h"
#include "../../data/Database.h"
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
#include <QDesktopServices>
#include <QUrl>
#include <QHeaderView>
#include <QBrush>
#include <QSqlQuery>

namespace TR4QT {

ContestChooserDialog::ContestChooserDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Contest Chooser");
    setupUI();
    loadExistingContests();
    resize(UIDefaults::CONTEST_CHOOSER_WIDTH, UIDefaults::CONTEST_CHOOSER_HEIGHT);
}

void ContestChooserDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top section: Existing contests
    QGroupBox* existingGroup = new QGroupBox("Existing Contests", this);
    QVBoxLayout* existingLayout = new QVBoxLayout(existingGroup);

    m_existingContestsTable = new QTableWidget(this);
    m_existingContestsTable->setColumnCount(4);
    m_existingContestsTable->setHorizontalHeaderLabels({"Contest Name", "Type", "Start Date", "Version"});
    m_existingContestsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_existingContestsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_existingContestsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_existingContestsTable->setAlternatingRowColors(true);
    // Use native macOS scroll behavior - let system show scroll indicators when appropriate
    // Visual affordance: partially clipped last row indicates more content (Apple HIG)
    m_existingContestsTable->horizontalHeader()->setStretchLastSection(false);
    m_existingContestsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // Contest Name stretches
    m_existingContestsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Type
    m_existingContestsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Start Date
    m_existingContestsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Version
    m_existingContestsTable->verticalHeader()->setVisible(false);
    connect(m_existingContestsTable, &QTableWidget::itemSelectionChanged,
            this, &ContestChooserDialog::onExistingContestSelected);
    connect(m_existingContestsTable, &QTableWidget::itemDoubleClicked,
            this, &ContestChooserDialog::onResumeContest);

    existingLayout->addWidget(m_existingContestsTable);

    // Buttons for existing contests
    QHBoxLayout* existingButtonLayout = new QHBoxLayout();
    m_resumeButton = new QPushButton("Resume Selected", this);
    m_resumeButton->setEnabled(false);
    connect(m_resumeButton, &QPushButton::clicked, this, &ContestChooserDialog::onResumeContest);

    m_deleteButton = new QPushButton("Delete Selected", this);
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, &QPushButton::clicked, this, &ContestChooserDialog::onDeleteContest);

    QPushButton* showFolderButton = new QPushButton("Show Database Folder", this);
    connect(showFolderButton, &QPushButton::clicked, this, &ContestChooserDialog::onShowDatabaseFolder);

    existingButtonLayout->addWidget(m_resumeButton);
    existingButtonLayout->addWidget(m_deleteButton);
    existingButtonLayout->addStretch();
    existingButtonLayout->addWidget(showFolderButton);

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
    m_existingContestsTable->setRowCount(0);

    // Get database directory
    QString dbDir = PathManager::getLogsDir();
    QDir dir(dbDir);

    if (!dir.exists()) {
        return;
    }

    // Find all .db files
    QStringList filters;
    filters << "*.db";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    for (const QFileInfo& fileInfo : files) {
        QString dbPath = fileInfo.absoluteFilePath();

        // Open database to read contest info and version
        Database& db = Database::instance();
        if (!db.open(dbPath)) {
            LOG_WARN("ContestChooserDialog", QString("Failed to open database: %1").arg(dbPath));
            continue;
        }

        // Read contest name, type, and start time
        QSqlQuery query = db.execute("SELECT contest_name, contest_type, start_time FROM contests LIMIT 1", {});
        if (!query.next()) {
            LOG_WARN("ContestChooserDialog", QString("Database has no contest record: %1").arg(dbPath));
            db.close();
            continue;
        }

        QString contestName = query.value(0).toString();
        QString contestType = query.value(1).toString();
        qint64 startTime = query.value(2).toLongLong();
        QDateTime startDate = QDateTime::fromSecsSinceEpoch(startTime);

        // Read schema version
        int schemaVersion = db.getUserVersion();

        db.close();

        // Add row to table
        int row = m_existingContestsTable->rowCount();
        m_existingContestsTable->insertRow(row);

        // Column 0: Contest Name
        QTableWidgetItem* nameItem = new QTableWidgetItem(contestName);
        nameItem->setData(Qt::UserRole, dbPath);  // Store database path in first column
        nameItem->setToolTip(dbPath);
        m_existingContestsTable->setItem(row, 0, nameItem);

        // Column 1: Type
        QTableWidgetItem* typeItem = new QTableWidgetItem(contestType);
        m_existingContestsTable->setItem(row, 1, typeItem);

        // Column 2: Start Date
        QString startDateStr = startDate.toString("yyyy-MM-dd");
        QTableWidgetItem* dateItem = new QTableWidgetItem(startDateStr);
        m_existingContestsTable->setItem(row, 2, dateItem);

        // Column 3: Version
        QString versionStr = schemaVersion > 0 ? QString("v%1").arg(schemaVersion) : "v0 (old)";
        QTableWidgetItem* versionItem = new QTableWidgetItem(versionStr);
        if (schemaVersion == 0) {
            // Highlight old databases in yellow
            versionItem->setForeground(QBrush(QColor("#d68910")));  // Orange warning color
            versionItem->setToolTip("This database was created before versioning (will be migrated on open)");
        }
        m_existingContestsTable->setItem(row, 3, versionItem);
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
    bool hasSelection = !m_existingContestsTable->selectedItems().isEmpty();
    m_resumeButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

void ContestChooserDialog::onResumeContest() {
    int currentRow = m_existingContestsTable->currentRow();
    if (currentRow < 0) {
        return;
    }

    // Get database path from first column's UserRole data
    QTableWidgetItem* nameItem = m_existingContestsTable->item(currentRow, 0);
    if (!nameItem) {
        return;
    }

    QString dbPath = nameItem->data(Qt::UserRole).toString();

    // Open database and read contest info (don't guess from filename!)
    Database& db = Database::instance();
    if (!db.open(dbPath)) {
        DialogHelper::critical(this, "Database Error",
                            QString("Failed to open contest database:\n%1").arg(db.lastError()));
        return;
    }

    // Read contest_id, contest_name, contest_type, and start_time from database
    QSqlQuery query = db.execute("SELECT contest_id, contest_name, contest_type, start_time FROM contests LIMIT 1", {});
    if (!query.next()) {
        DialogHelper::warning(this, "Invalid Database",
                           "Contest database has no contest record. The database may be corrupted.");
        db.close();
        return;
    }

    QString contestId = query.value(0).toString();
    QString contestName = query.value(1).toString();
    QString contestType = query.value(2).toString();
    qint64 startTime = query.value(3).toLongLong();

    db.close();

    // Build ContestInfo from database (no filename parsing!)
    m_contestInfo.isExisting = true;
    m_contestInfo.databasePath = dbPath;
    m_contestInfo.contestId = contestId;
    m_contestInfo.contestName = contestName;
    m_contestInfo.contestType = contestType;  // Read from database, not filename!
    m_contestInfo.startDate = QDateTime::fromSecsSinceEpoch(startTime);

    // Determine mode from contest_id for backward compatibility
    if (contestId.contains("_CW")) {
        m_contestInfo.mode = "CW";
    } else if (contestId.contains("_SSB")) {
        m_contestInfo.mode = "SSB";
    } else {
        m_contestInfo.mode = "Mixed";
    }

    LOG_DEBUG("ContestChooserDialog", QString("Resume contest: type='%1', mode='%2', name='%3'")
        .arg(contestType, m_contestInfo.mode, contestName));

    accept();
}

void ContestChooserDialog::onDeleteContest() {
    int currentRow = m_existingContestsTable->currentRow();
    if (currentRow < 0) {
        return;
    }

    // Get database path and contest name from first column
    QTableWidgetItem* nameItem = m_existingContestsTable->item(currentRow, 0);
    if (!nameItem) {
        return;
    }

    QString contestName = nameItem->text();
    QString dbPath = nameItem->data(Qt::UserRole).toString();

    QMessageBox::StandardButton reply = DialogHelper::question(
        this, "Delete Contest?",
        QString("Are you sure you want to delete the contest:\n\n%1\n\nThis action cannot be undone!")
            .arg(contestName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QFile file(dbPath);
        if (file.remove()) {
            DialogHelper::information(this, "Contest Deleted",
                                   QString("Contest '%1' has been deleted.").arg(contestName));
            loadExistingContests();  // Refresh list
        } else {
            DialogHelper::warning(this, "Delete Failed",
                               QString("Failed to delete contest database:\n%1").arg(file.errorString()));
        }
    }
}

void ContestChooserDialog::onShowDatabaseFolder() {
    // Get database directory
    QString dbDir = PathManager::getLogsDir();

    // Open the folder in Finder (macOS) or Explorer (Windows) or file manager (Linux)
    QUrl url = QUrl::fromLocalFile(dbDir);
    if (!QDesktopServices::openUrl(url)) {
        DialogHelper::warning(this, "Failed to Open Folder",
                           QString("Could not open the database folder:\n%1\n\nYou can navigate there manually.")
                               .arg(dbDir));
    }
}

void ContestChooserDialog::onNewContest() {
    QString contestName = m_contestNameEdit->text().trimmed();
    if (contestName.isEmpty()) {
        DialogHelper::warning(this, "Invalid Input", "Please enter a contest name.");
        m_contestNameEdit->setFocus();
        return;
    }

    QString contestType = m_contestTypeCombo->currentData().toString();
    QDateTime startDate = m_startDateEdit->dateTime();
    QString mode = m_modeCombo->currentText();

    // Generate contest ID (used for database filename)
    QString contestId = generateContestId(contestType, startDate);

    // Create database path
    QString dbDir = PathManager::getLogsDir();
    // Directory is created automatically by PathManager

    QString dbPath = dbDir + "/" + contestId + ".db";

    // Check if file already exists
    if (QFile::exists(dbPath)) {
        QMessageBox::StandardButton reply = DialogHelper::question(
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
    m_contestInfo.contestId = contestId;  // Unique identifier (e.g., "GENERAL_2026_01_02")
    m_contestInfo.contestName = contestName;  // Display name (user input)
    m_contestInfo.contestType = contestType;  // Registry ID (e.g., "GENERAL")
    m_contestInfo.startDate = startDate;
    m_contestInfo.mode = mode;
    m_contestInfo.databasePath = dbPath;

    LOG_DEBUG("ContestChooserDialog", QString("New contest: id='%1', type='%2', name='%3'")
        .arg(contestId, contestType, contestName));

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
