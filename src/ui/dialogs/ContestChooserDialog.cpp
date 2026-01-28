#include "ContestChooserDialog.h"
#include "../../core/Constants.h"
#include "../../contests/ContestRegistry.h"
#include "../../contests/ContestMetadata.h"
#include "../../utils/DialogHelper.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/PathManager.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include "../../data/Database.h"
#include "../../data/ContestRepository.h"
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
#include <QRegularExpression>
#include <algorithm>

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
    m_contestTypeCombo->addItem("-- Select Contest Type --", "");  // Placeholder
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

    // Exchange sent (contest-specific, shown when contest type selected)
    m_exchangeSentEdit = new QLineEdit(this);
    m_exchangeSentEdit->setPlaceholderText("e.g., 1H WCF for Winter Field Day");
    m_exchangeSentLabel = new QLabel("Sent Exchange:", this);
    newLayout->addRow(m_exchangeSentLabel, m_exchangeSentEdit);
    m_exchangeSentEdit->setVisible(false);
    m_exchangeSentLabel->setVisible(false);

    // Contest Configuration (for Cabrillo export)
    // Category dropdown
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItems({"SINGLE-OP", "MULTI-OP", "MULTI-TWO", "MULTI-SINGLE", "CHECKLOG"});
    m_categoryCombo->setToolTip("Competition category for Cabrillo export");
    newLayout->addRow("Category:", m_categoryCombo);

    // Power class dropdown
    m_powerClassCombo = new QComboBox(this);
    m_powerClassCombo->addItems({"HIGH", "LOW", "QRP"});
    m_powerClassCombo->setToolTip("Power class: HIGH (>100W), LOW (5-100W), QRP (<=5W)");
    newLayout->addRow("Power:", m_powerClassCombo);

    // Assisted dropdown
    m_assistedCombo = new QComboBox(this);
    m_assistedCombo->addItems({"NON-ASSISTED", "ASSISTED"});
    m_assistedCombo->setToolTip("ASSISTED = using DX Cluster, RBN, or skimmers");
    newLayout->addRow("Assisted:", m_assistedCombo);

    // Dynamic contest config fields section
    // This layout holds fields dynamically created based on contest type
    m_configFieldsLayout = new QFormLayout();
    newLayout->addRow(m_configFieldsLayout);

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

    // Start with placeholder selected (don't auto-select first contest)
    m_contestTypeCombo->setCurrentIndex(0);  // Placeholder
    m_createButton->setEnabled(false);  // Disable until contest selected
}

void ContestChooserDialog::populateContestTypes() {
    // Get all registered contests from the factory
    QList<ContestMetadata> allContests = ContestRegistry::instance().availableContests();

    // Structure to hold contest info with next occurrence date
    struct ContestWithDate {
        ContestMetadata meta;
        ModeType mode;
        QDate nextOccurrence;
        QString displayName;
    };

    QList<ContestWithDate> datedContests;
    QList<ContestWithDate> undatedContests;

    // Separate contests into dated (has floating dates) and undated
    // Each contest appears once in the dropdown - mode is selected separately
    for (const ContestMetadata& meta : allContests) {
        ContestWithDate cwd;
        cwd.meta = meta;
        cwd.mode = ModeType::None;  // Mode will be selected separately in Mode combo
        cwd.displayName = meta.displayName;

        // Calculate next occurrence if floating dates exist
        if (!meta.floatingDates.isEmpty()) {
            cwd.nextOccurrence = meta.floatingDates[0].calculateNextOccurrence();
            datedContests.append(cwd);
        } else {
            undatedContests.append(cwd);
        }
    }

    // Sort dated contests by next occurrence (soonest first)
    std::sort(datedContests.begin(), datedContests.end(),
              [](const ContestWithDate& a, const ContestWithDate& b) {
                  // Put invalid dates at the end
                  if (!a.nextOccurrence.isValid()) return false;
                  if (!b.nextOccurrence.isValid()) return true;
                  return a.nextOccurrence < b.nextOccurrence;
              });

    // Add dated contests with next occurrence date in label
    for (const ContestWithDate& cwd : datedContests) {
        QString label = cwd.displayName;
        if (cwd.nextOccurrence.isValid()) {
            // Format: "ARRL Sweepstakes - CW (Nov 2)"
            label += QString(" (%1)").arg(cwd.nextOccurrence.toString("MMM d"));
        }
        m_contestTypeCombo->addItem(label, cwd.meta.id);
    }

    // Add separator between dated and undated contests
    if (!datedContests.isEmpty() && !undatedContests.isEmpty()) {
        m_contestTypeCombo->insertSeparator(m_contestTypeCombo->count());
    }

    // Add undated contests (alphabetically)
    std::sort(undatedContests.begin(), undatedContests.end(),
              [](const ContestWithDate& a, const ContestWithDate& b) {
                  return a.displayName < b.displayName;
              });

    for (const ContestWithDate& cwd : undatedContests) {
        m_contestTypeCombo->addItem(cwd.displayName, cwd.meta.id);
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

    ContestRepository repo;

    for (const QFileInfo& fileInfo : files) {
        QString dbPath = fileInfo.absoluteFilePath();

        // Use ContestRepository to query contests from database
        QList<ContestRecord> contests = repo.findAll(dbPath);
        if (contests.isEmpty()) {
            LOG_WARN("ContestChooserDialog", QString("Database has no contest record: %1").arg(dbPath));
            continue;
        }

        // Get first (and usually only) contest
        const ContestRecord& record = contests.first();
        QString contestName = record.contestName;
        QString contestType = record.contestType;
        QDateTime startDate = record.startTime;

        // Read schema version by temporarily opening database
        Database& db = Database::instance();
        if (!db.open(dbPath)) {
            LOG_WARN("ContestChooserDialog", QString("Failed to open database for version check: %1").arg(dbPath));
            continue;
        }
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
            versionItem->setForeground(QBrush(ThemeManager::instance().color(ColorRole::WarningText)));
            versionItem->setToolTip("This database was created before versioning (will be migrated on open)");
        }
        m_existingContestsTable->setItem(row, 3, versionItem);
    }
}

void ContestChooserDialog::onContestTypeChanged(int index) {
    Q_UNUSED(index);

    QString contestType = m_contestTypeCombo->currentData().toString();

    // Enable/disable create button based on selection
    bool hasValidSelection = !contestType.isEmpty();
    m_createButton->setEnabled(hasValidSelection);

    // If placeholder selected, clear the contest name and hide fields
    if (!hasValidSelection) {
        m_contestNameEdit->clear();
        m_exchangeSentEdit->setVisible(false);
        m_exchangeSentLabel->setVisible(false);
        clearConfigFields();
        return;
    }

    QDateTime now = QDateTime::currentDateTime();

    // Parse mode from contestType (e.g., "CQWW_CW" -> "CW", "WFD" -> "Mixed")
    QStringList parts = contestType.split('_');
    QString modeStr = parts.size() > 1 ? parts[1] : "Mixed";

    // Get contest metadata from registry using full contestType (e.g., "NAQP_SSB")
    if (ContestRegistry::instance().hasContest(contestType)) {
        ContestMetadata meta = ContestRegistry::instance().getMetadata(contestType);

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

        // Hide legacy exchange field (no longer using hardcoded WFD check)
        m_exchangeSentEdit->setVisible(false);
        m_exchangeSentLabel->setVisible(false);

        // Update dynamic config fields based on contest requirements
        updateConfigFields(contestType);
    }
}

void ContestChooserDialog::updateConfigFields(const QString& contestType) {
    // Clear existing dynamic fields
    clearConfigFields();

    // Create temporary contest to query config fields
    if (!ContestRegistry::instance().hasContest(contestType)) {
        LOG_DEBUG("ContestChooserDialog", QString("Contest type '%1' not found in registry").arg(contestType));
        return;
    }

    // Create dummy station info for temporary contest
    StationInfo dummyStation;
    dummyStation.callsign = "W1AW";
    dummyStation.cqZone = 5;

    // Determine mode for contest creation
    ModeType mode = ModeType::CW;
    if (contestType.endsWith("_SSB")) {
        mode = ModeType::USB;
    } else if (contestType.endsWith("_RTTY")) {
        mode = ModeType::RTTY;
    }

    // Create temporary contest to get config fields
    ContestBase* tempContest = ContestRegistry::instance().createContest(contestType, mode, dummyStation);
    if (!tempContest) {
        return;
    }

    // Query config fields from contest class
    m_configFields = tempContest->getConfigFields();
    delete tempContest;

    if (m_configFields.isEmpty()) {
        LOG_DEBUG("ContestChooserDialog", QString("Contest '%1' has no config fields").arg(contestType));
        return;
    }

    LOG_DEBUG("ContestChooserDialog", QString("Contest '%1' has %2 config fields")
        .arg(contestType).arg(m_configFields.size()));

    // Create UI widgets for each config field
    for (const ContestConfigField& field : m_configFields) {
        QWidget* widget = nullptr;

        if (field.type == ContestConfigField::Type::DropDown) {
            // Create dropdown
            QComboBox* combo = new QComboBox(this);
            combo->addItems(field.options);
            combo->setObjectName(field.id);  // Store ID for later retrieval
            widget = combo;
        } else {
            // Create text input
            QLineEdit* edit = new QLineEdit(this);
            edit->setPlaceholderText(field.placeholder);
            edit->setObjectName(field.id);  // Store ID for later retrieval

            if (field.maxLength > 0) {
                edit->setMaxLength(field.maxLength);
            }

            // Pre-fill from AppSettings if settingsKey provided
            if (!field.settingsKey.isEmpty()) {
                QString defaultValue = AppSettings::instance().getValue(field.settingsKey).toUpper();
                if (!defaultValue.isEmpty()) {
                    edit->setText(defaultValue);
                }
            }

            widget = edit;
        }

        // Add to layout with label
        m_configFieldsLayout->addRow(field.label, widget);
        m_configFieldWidgets.append(widget);
    }
}

void ContestChooserDialog::clearConfigFields() {
    // Remove all widgets from layout and delete them
    while (m_configFieldsLayout->count() > 0) {
        QLayoutItem* item = m_configFieldsLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        if (item->layout()) {
            delete item->layout();
        }
        delete item;
    }

    m_configFields.clear();
    m_configFieldWidgets.clear();
}

QMap<QString, QString> ContestChooserDialog::getConfigFieldValues() const {
    QMap<QString, QString> values;

    for (int i = 0; i < m_configFields.size() && i < m_configFieldWidgets.size(); ++i) {
        const ContestConfigField& field = m_configFields[i];
        QWidget* widget = m_configFieldWidgets[i];

        QString value;
        if (field.type == ContestConfigField::Type::DropDown) {
            QComboBox* combo = qobject_cast<QComboBox*>(widget);
            if (combo) {
                value = combo->currentText();
            }
        } else {
            QLineEdit* edit = qobject_cast<QLineEdit*>(widget);
            if (edit) {
                value = edit->text().trimmed().toUpper();
            }
        }

        values[field.id] = value;
    }

    return values;
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

    // Use ContestRepository to read contest info
    ContestRepository repo;
    QList<ContestRecord> contests = repo.findAll(dbPath);
    if (contests.isEmpty()) {
        DialogHelper::warning(this, "Invalid Database",
                           "Contest database has no contest record. The database may be corrupted.");
        return;
    }

    // Get first (and usually only) contest
    const ContestRecord& record = contests.first();
    QString contestId = record.contestId;
    QString contestName = record.contestName;
    QString contestType = record.contestType;
    QString exchangeSent = record.exchangeSent;

    // Build ContestInfo from database (no filename parsing!)
    m_contestInfo.isExisting = true;
    m_contestInfo.databasePath = dbPath;
    m_contestInfo.contestId = contestId;
    m_contestInfo.contestName = contestName;
    m_contestInfo.contestType = contestType;  // Read from database, not filename!
    m_contestInfo.startDate = record.startTime;
    m_contestInfo.exchangeSent = exchangeSent;  // Load exchange from database

    // Determine mode from contest_id for backward compatibility
    if (contestId.contains("_CW")) {
        m_contestInfo.mode = "CW";
    } else if (contestId.contains("_SSB")) {
        m_contestInfo.mode = "SSB";
    } else {
        m_contestInfo.mode = "Mixed";
    }

    LOG_DEBUG("ContestChooserDialog", QString("Resume contest: type='%1', mode='%2', name='%3', exchange='%4'")
        .arg(contestType, m_contestInfo.mode, contestName, exchangeSent));

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

    // Get dynamic config field values
    QMap<QString, QString> configValues = getConfigFieldValues();

    // Build exchangeSent from config field values
    // Concatenate all field values in order (e.g., "A 95 WMA" for SS, "1H WCF" for WFD)
    QStringList exchangeParts;
    for (const ContestConfigField& field : m_configFields) {
        QString value = configValues.value(field.id);
        if (!value.isEmpty()) {
            exchangeParts.append(value);
        }
    }
    QString exchangeSent = exchangeParts.join(" ");

    // Fill contest info
    m_contestInfo.contestId = contestId;  // Unique identifier (e.g., "GENERAL_2026_01_02")
    m_contestInfo.contestName = contestName;  // Display name (user input)
    m_contestInfo.contestType = contestType;  // Registry ID (e.g., "GENERAL")
    m_contestInfo.startDate = startDate;
    m_contestInfo.mode = mode;
    m_contestInfo.databasePath = dbPath;
    m_contestInfo.exchangeSent = exchangeSent;

    // Contest configuration for Cabrillo export
    m_contestInfo.category = m_categoryCombo->currentText();
    m_contestInfo.powerClass = m_powerClassCombo->currentText();
    m_contestInfo.assisted = m_assistedCombo->currentText();
    m_contestInfo.operatorName = configValues.value("NAME");

    // Update AppSettings if user changed state or operator name
    // (these are used by QSOLogger for exchange substitution)
    QString newState = configValues.value("STATE");
    if (!newState.isEmpty() && newState != AppSettings::instance().getMyState().toUpper()) {
        AppSettings::instance().setMyState(newState);
        LOG_DEBUG("ContestChooserDialog", QString("Updated station state to: %1").arg(newState));
    }

    QString newFirstName = configValues.value("NAME");
    if (!newFirstName.isEmpty() && newFirstName != AppSettings::instance().getMyFirstName().toUpper()) {
        AppSettings::instance().setMyFirstName(newFirstName);
        LOG_DEBUG("ContestChooserDialog", QString("Updated first name to: %1").arg(newFirstName));
    }

    // Update section if provided
    QString newSection = configValues.value("SECTION");
    if (!newSection.isEmpty() && newSection != AppSettings::instance().getMyARRLSection().toUpper()) {
        AppSettings::instance().setMyARRLSection(newSection);
        LOG_DEBUG("ContestChooserDialog", QString("Updated ARRL section to: %1").arg(newSection));
    }

    LOG_DEBUG("ContestChooserDialog", QString("New contest: id='%1', type='%2', name='%3', exchange='%4', category='%5'")
        .arg(contestId, contestType, contestName, m_contestInfo.exchangeSent, m_contestInfo.category));

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
