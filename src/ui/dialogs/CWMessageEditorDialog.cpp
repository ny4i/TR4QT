#include "CWMessageEditorDialog.h"
#include "../../utils/AppSettings.h"
#include "../../cw/CWTemplateEngine.h"
#include "../../contests/ContestBase.h"
#include "../../contests/RSTValidator.h"
#include "../../radio/RadioController.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMessageBox>

namespace TR4QT {

static const int NUM_F_KEYS = 12;

CWMessageEditorDialog::CWMessageEditorDialog(RadioController* radio, ContestBase* contest, QWidget* parent)
    : QDialog(parent)
    , m_radio(radio)
    , m_contest(contest)
{
    setWindowTitle("CW Messages Editor");
    setMinimumSize(800, 600);
    setupUI();
    loadMessagesFromSettings();
}

void CWMessageEditorDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Tab widget for different modes
    m_tabWidget = new QTabWidget(this);
    m_cqTable = createMessageTable();
    m_spTable = createMessageTable();
    m_ctrlFTable = createMessageTable();
    m_altFTable = createMessageTable();
    m_cutNumbersTable = createCutNumbersTable();

    m_autoSendTable = createAutoSendMessagesTable();

    m_tabWidget->addTab(m_cqTable, "CQ Mode");
    m_tabWidget->addTab(m_spTable, "S&P Mode");
    m_tabWidget->addTab(m_ctrlFTable, "Ctrl+F Keys");
    m_tabWidget->addTab(m_altFTable, "Alt+F Keys");
    m_tabWidget->addTab(m_cutNumbersTable, "Cut Numbers");
    m_tabWidget->addTab(m_autoSendTable, "Auto-Send Messages");

    mainLayout->addWidget(m_tabWidget);

    // Template variables help text
    auto* helpGroup = new QGroupBox("Template Variables", this);
    auto* helpLayout = new QVBoxLayout(helpGroup);
    auto* helpLabel = new QLabel(
        "\\  = My Call    "
        "@  = His Call    "
        "#  = QSO Number    "
        "+  = GMT Time    "
        "^  = Half Space\n"
        "*  = Salutation    "
        "%  = Name DB    "
        "!  = Serial    "
        "|  = Name from Exchange"
    );
    helpLabel->setWordWrap(true);
    helpLayout->addWidget(helpLabel);
    mainLayout->addWidget(helpGroup);

    // Preview section
    auto* previewGroup = new QGroupBox("Preview (F1 with current context)", this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    m_previewText = new QLineEdit(this);
    m_previewText->setReadOnly(true);
    m_previewText->setPlaceholderText("Select a message to see preview...");
    previewLayout->addWidget(m_previewText);
    mainLayout->addWidget(previewGroup);

    // Bottom buttons
    auto* buttonLayout = new QHBoxLayout();
    m_loadDefaultsButton = new QPushButton("Load TR4W Defaults", this);
    m_testButton = new QPushButton("Test Selected", this);
    m_helpButton = new QPushButton("Help", this);

    buttonLayout->addWidget(m_loadDefaultsButton);
    buttonLayout->addWidget(m_testButton);
    buttonLayout->addWidget(m_helpButton);
    buttonLayout->addStretch();

    auto* dialogButtons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this
    );
    m_applyButton = dialogButtons->button(QDialogButtonBox::Apply);

    buttonLayout->addWidget(dialogButtons);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(dialogButtons, &QDialogButtonBox::accepted, this, &CWMessageEditorDialog::accept);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &CWMessageEditorDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &CWMessageEditorDialog::onApply);
    connect(m_loadDefaultsButton, &QPushButton::clicked, this, &CWMessageEditorDialog::onLoadDefaults);
    connect(m_testButton, &QPushButton::clicked, this, &CWMessageEditorDialog::onTestSelected);
    connect(m_helpButton, &QPushButton::clicked, this, &CWMessageEditorDialog::onShowHelp);

    // Connect table signals
    connect(m_cqTable, &QTableWidget::cellChanged, this, &CWMessageEditorDialog::onTableCellChanged);
    connect(m_spTable, &QTableWidget::cellChanged, this, &CWMessageEditorDialog::onTableCellChanged);
    connect(m_ctrlFTable, &QTableWidget::cellChanged, this, &CWMessageEditorDialog::onTableCellChanged);
    connect(m_altFTable, &QTableWidget::cellChanged, this, &CWMessageEditorDialog::onTableCellChanged);
    connect(m_cutNumbersTable, &QTableWidget::cellChanged, this, &CWMessageEditorDialog::onTableCellChanged);
    connect(m_autoSendTable, &QTableWidget::cellChanged, this, &CWMessageEditorDialog::onTableCellChanged);

    connect(m_cqTable, &QTableWidget::itemSelectionChanged, this, &CWMessageEditorDialog::onTableSelectionChanged);
    connect(m_spTable, &QTableWidget::itemSelectionChanged, this, &CWMessageEditorDialog::onTableSelectionChanged);
    connect(m_ctrlFTable, &QTableWidget::itemSelectionChanged, this, &CWMessageEditorDialog::onTableSelectionChanged);
    connect(m_altFTable, &QTableWidget::itemSelectionChanged, this, &CWMessageEditorDialog::onTableSelectionChanged);
    connect(m_cutNumbersTable, &QTableWidget::itemSelectionChanged, this, &CWMessageEditorDialog::onTableSelectionChanged);
    connect(m_autoSendTable, &QTableWidget::itemSelectionChanged, this, &CWMessageEditorDialog::onTableSelectionChanged);
}

QTableWidget* CWMessageEditorDialog::createMessageTable() {
    auto* table = new QTableWidget(NUM_F_KEYS, 2, this);  // 2 columns: Key, Template
    table->setHorizontalHeaderLabels({"Key", "Message Template"});
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set column widths
    table->setColumnWidth(0, 60);   // Key column
    table->horizontalHeader()->setStretchLastSection(true);  // Template column stretches

    // Populate key column (read-only)
    for (int i = 0; i < NUM_F_KEYS; i++) {
        auto* keyItem = new QTableWidgetItem(QString("F%1").arg(i + 1));
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);  // Read-only
        table->setItem(i, 0, keyItem);

        // Template column (editable)
        auto* templateItem = new QTableWidgetItem("");
        table->setItem(i, 1, templateItem);
    }

    return table;
}

QTableWidget* CWMessageEditorDialog::createCutNumbersTable() {
    const int NUM_DIGITS = 10;  // 0-9
    auto* table = new QTableWidget(NUM_DIGITS, 2, this);  // 2 columns: Digit, Message
    table->setHorizontalHeaderLabels({"Digit", "Cut Number Message"});
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set column widths
    table->setColumnWidth(0, 60);   // Digit column
    table->horizontalHeader()->setStretchLastSection(true);  // Message column stretches

    // Populate digit column (read-only) and message column (editable)
    for (int i = 0; i < NUM_DIGITS; i++) {
        // Digit column (read-only)
        auto* digitItem = new QTableWidgetItem(QString::number(i));
        digitItem->setFlags(digitItem->flags() & ~Qt::ItemIsEditable);  // Read-only
        table->setItem(i, 0, digitItem);

        // Message column (editable)
        auto* messageItem = new QTableWidgetItem("");
        table->setItem(i, 1, messageItem);
    }

    return table;
}

QTableWidget* CWMessageEditorDialog::createAutoSendMessagesTable() {
    const int NUM_AUTO_SEND = 9;  // Number of auto-send message types
    auto* table = new QTableWidget(NUM_AUTO_SEND, 2, this);  // 2 columns: Command, Message
    table->setHorizontalHeaderLabels({"Command", "Message"});
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set column widths
    table->setColumnWidth(0, 220);   // Command column
    table->horizontalHeader()->setStretchLastSection(true);  // Message column stretches

    // Message types (matching TR4W screenshot order)
    QStringList commands = {
        "CALL_OK_NOW_CW_MESSAGE",
        "CQ_CW_EXCHANGE",
        "CQ_CW_EXCHANGE_NAME_KNOWN",
        "QSL_CW_MESSAGE",
        "QSO_BEFORE_CW_MESSAGE",
        "QUICK_QSL_CW_MESSAGE",
        "REPEAT_S&P_CW_EXCHANGE",
        "S&P_CW_EXCHANGE",
        "TAIL_END_CW_MESSAGE"
    };

    // Populate command column (read-only) and message column (editable)
    for (int i = 0; i < NUM_AUTO_SEND; i++) {
        // Command column (read-only)
        auto* commandItem = new QTableWidgetItem(commands[i]);
        commandItem->setFlags(commandItem->flags() & ~Qt::ItemIsEditable);  // Read-only
        table->setItem(i, 0, commandItem);

        // Message column (editable)
        auto* messageItem = new QTableWidgetItem("");
        table->setItem(i, 1, messageItem);
    }

    return table;
}

void CWMessageEditorDialog::loadMessagesFromSettings() {
    loadCQMessages();
    loadSPMessages();
    loadCtrlFMessages();
    loadAltFMessages();
    loadCutNumbers();
    loadAutoSendMessages();
}

void CWMessageEditorDialog::loadCQMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = AppSettings::instance().getCQMessage(i + 1);
        m_cqTable->item(i, 1)->setText(msg);
    }
}

void CWMessageEditorDialog::loadSPMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = AppSettings::instance().getSPMessage(i + 1);
        m_spTable->item(i, 1)->setText(msg);
    }
}

void CWMessageEditorDialog::loadCtrlFMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = AppSettings::instance().getCtrlFMessage(i + 1, true);  // CQ mode for now
        m_ctrlFTable->item(i, 1)->setText(msg);
    }
}

void CWMessageEditorDialog::loadAltFMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = AppSettings::instance().getAltFMessage(i + 1, true);  // CQ mode for now
        m_altFTable->item(i, 1)->setText(msg);
    }
}

void CWMessageEditorDialog::loadCutNumbers() {
    for (int digit = 0; digit < 10; digit++) {
        QString msg = AppSettings::instance().getShortMessage(digit);
        m_cutNumbersTable->item(digit, 1)->setText(msg);
    }
}

void CWMessageEditorDialog::loadAutoSendMessages() {
    AppSettings& settings = AppSettings::instance();
    m_autoSendTable->item(0, 1)->setText(settings.getCallOkNowCWMessage());
    m_autoSendTable->item(1, 1)->setText(settings.getCQCWExchange());
    m_autoSendTable->item(2, 1)->setText(settings.getCQCWExchangeNameKnown());
    m_autoSendTable->item(3, 1)->setText(settings.getQSLCWMessage());
    m_autoSendTable->item(4, 1)->setText(settings.getQSOBeforeCWMessage());
    m_autoSendTable->item(5, 1)->setText(settings.getQuickQSLCWMessage());
    m_autoSendTable->item(6, 1)->setText(settings.getRepeatSPCWExchange());
    m_autoSendTable->item(7, 1)->setText(settings.getSPCWExchange());
    m_autoSendTable->item(8, 1)->setText(settings.getTailEndCWMessage());
}

void CWMessageEditorDialog::saveMessagesToSettings() {
    saveCQMessages();
    saveSPMessages();
    saveCtrlFMessages();
    saveAltFMessages();
    saveCutNumbers();
    saveAutoSendMessages();
}

void CWMessageEditorDialog::saveCQMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = m_cqTable->item(i, 1)->text();
        AppSettings::instance().setCQMessage(i + 1, msg);
    }
}

void CWMessageEditorDialog::saveSPMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = m_spTable->item(i, 1)->text();
        AppSettings::instance().setSPMessage(i + 1, msg);
    }
}

void CWMessageEditorDialog::saveCtrlFMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = m_ctrlFTable->item(i, 1)->text();
        AppSettings::instance().setCtrlFMessage(i + 1, true, msg);  // CQ mode
        AppSettings::instance().setCtrlFMessage(i + 1, false, msg); // S&P mode (same for now)
    }
}

void CWMessageEditorDialog::saveAltFMessages() {
    for (int i = 0; i < NUM_F_KEYS; i++) {
        QString msg = m_altFTable->item(i, 1)->text();
        AppSettings::instance().setAltFMessage(i + 1, true, msg);  // CQ mode
        AppSettings::instance().setAltFMessage(i + 1, false, msg); // S&P mode (same for now)
    }
}

void CWMessageEditorDialog::saveCutNumbers() {
    for (int digit = 0; digit < 10; digit++) {
        QString msg = m_cutNumbersTable->item(digit, 1)->text();
        AppSettings::instance().setShortMessage(digit, msg);
    }
}

void CWMessageEditorDialog::saveAutoSendMessages() {
    AppSettings& settings = AppSettings::instance();
    settings.setCallOkNowCWMessage(m_autoSendTable->item(0, 1)->text());
    settings.setCQCWExchange(m_autoSendTable->item(1, 1)->text());
    settings.setCQCWExchangeNameKnown(m_autoSendTable->item(2, 1)->text());
    settings.setQSLCWMessage(m_autoSendTable->item(3, 1)->text());
    settings.setQSOBeforeCWMessage(m_autoSendTable->item(4, 1)->text());
    settings.setQuickQSLCWMessage(m_autoSendTable->item(5, 1)->text());
    settings.setRepeatSPCWExchange(m_autoSendTable->item(6, 1)->text());
    settings.setSPCWExchange(m_autoSendTable->item(7, 1)->text());
    settings.setTailEndCWMessage(m_autoSendTable->item(8, 1)->text());
}

void CWMessageEditorDialog::accept() {
    saveMessagesToSettings();
    QDialog::accept();
}

void CWMessageEditorDialog::onApply() {
    saveMessagesToSettings();
    LOG_INFO("CWMessageEditorDialog", "CW messages saved via Apply button");
}

void CWMessageEditorDialog::onLoadDefaults() {
    // TODO: Load TR4W default messages
    QMessageBox::information(this, "Load Defaults",
        "TR4W default messages will be loaded in Phase 2 completion.");
}

void CWMessageEditorDialog::onTestSelected() {
    // Get currently selected table and row
    QTableWidget* currentTable = qobject_cast<QTableWidget*>(m_tabWidget->currentWidget());
    if (!currentTable) return;

    QList<QTableWidgetItem*> selected = currentTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a message to test.");
        return;
    }

    int row = selected.first()->row();
    QString templateStr = currentTable->item(row, 1)->text();

    if (templateStr.isEmpty()) {
        QMessageBox::warning(this, "Empty Message", "The selected message is empty.");
        return;
    }

    if (!m_radio) {
        QMessageBox::warning(this, "No Radio", "Radio not available for testing.");
        return;
    }

    // Build context and send
    CWTemplateEngine::Context ctx;
    ctx.myCall = AppSettings::instance().getMyCallsign();
    ctx.hisCall = "TEST";
    ctx.qsoNumber = 123;
    ctx.mode = ModeType::CW;
    ctx.band = BandType::Band20M;

    if (m_contest) {
        ctx.contestName = m_contest->getContestName();
        ctx.sentExchange = m_contest->formatSentExchange(123, "599");
    }

    QString cwText = CWTemplateEngine::substitute(templateStr, ctx);
    m_radio->sendCW(cwText);

    LOG_INFO("CWMessageEditorDialog", QString("Test message sent: %1").arg(cwText));
}

void CWMessageEditorDialog::onShowHelp() {
    QMessageBox::information(this, "CW Message Template Help",
        "Template Variables:\n\n"
        "\\  = My Call\n"
        "@  = His Call\n"
        "#  = QSO Number (serial)\n"
        "!  = Serial Number (same as #)\n"
        "+  = GMT Time (HHMM)\n"
        "^  = Half Space\n\n"
        "Special Placeholders:\n\n"
        "Set_by_the_MY_CALL = Sends your callsign\n"
        "Set_by_S&P_EXCHANGE = Sends your exchange\n\n"
        "Examples:\n\n"
        "CQ WFD \\ \\ TEST → CQ WFD NY4I NY4I TEST\n"
        "TU \\ TEST → TU NY4I TEST\n"
        "@ 599 # → W1AW 599 001"
    );
}

void CWMessageEditorDialog::onTableCellChanged(int row, int column) {
    // Only care about template column changes
    if (column != 1) return;
    updatePreview();
}

void CWMessageEditorDialog::onTableSelectionChanged() {
    updatePreview();
}

void CWMessageEditorDialog::updatePreview() {
    QTableWidget* currentTable = qobject_cast<QTableWidget*>(m_tabWidget->currentWidget());
    if (!currentTable) return;

    QList<QTableWidgetItem*> selected = currentTable->selectedItems();
    if (selected.isEmpty()) {
        m_previewText->clear();
        m_previewText->setPlaceholderText("Select a message to see preview...");
        return;
    }

    int row = selected.first()->row();
    QString templateStr = currentTable->item(row, 1)->text();

    if (templateStr.isEmpty()) {
        m_previewText->clear();
        m_previewText->setPlaceholderText("(empty message)");
        return;
    }

    QString preview = getPreviewText(templateStr);
    m_previewText->setText(preview);
}

QString CWMessageEditorDialog::getPreviewText(const QString& templateStr) {
    // Build a sample context for preview
    CWTemplateEngine::Context ctx;
    ctx.myCall = AppSettings::instance().getMyCallsign();
    if (ctx.myCall.isEmpty()) {
        ctx.myCall = "NY4I";  // Example callsign
    }
    ctx.hisCall = "W1AW";
    ctx.qsoNumber = 42;
    ctx.mode = ModeType::CW;
    ctx.band = BandType::Band20M;

    if (m_contest) {
        ctx.contestName = m_contest->getContestName();
        ctx.sentExchange = m_contest->formatSentExchange(42, "599");
    } else {
        ctx.sentExchange = "599 042";
    }

    return CWTemplateEngine::substitute(templateStr, ctx);
}

} // namespace TR4QT
