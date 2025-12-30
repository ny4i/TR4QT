#include "MultiplierWidget.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/ArrlSectionHelper.h"
#include "../../utils/CountryFile.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QPalette>
#include <QMenu>
#include <QSet>
#include <QResizeEvent>

namespace TR4QT {

MultiplierWidget::MultiplierWidget(QWidget* parent)
    : QWidget(parent)
    , m_type(MultiplierType::Country)
    , m_currentBand(BandType::None)
    , m_hideWorked(false)
    , m_columnCount(4)  // Start with 4 columns, will be recalculated on resize
{
    setupUI();
    loadMultiplierList();
    updateDisplay();

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MultiplierWidget::applyTheme);
    applyTheme();
}

void MultiplierWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(m_columnCount);  // Dynamic column count
    m_table->horizontalHeader()->setVisible(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(true);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Enable context menu
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QWidget::customContextMenuRequested,
            this, &MultiplierWidget::onContextMenuRequested);

    // Set font
    QFont font("Monospace", 9);
    m_table->setFont(font);

    // Set initial column widths (will be recalculated on resize)
    for (int i = 0; i < m_columnCount; ++i) {
        m_table->setColumnWidth(i, MIN_COLUMN_WIDTH);
    }

    connect(m_table, &QTableWidget::cellClicked,
            this, [this](int row, int col) {
        QTableWidgetItem* item = m_table->item(row, col);
        if (item) {
            // Use stored original multiplier value (without checkmark)
            QString mult = item->data(Qt::UserRole).toString();
            if (!mult.isEmpty()) {
                emit multiplierSelected(mult);
            } else {
                // Fallback to text (remove checkmark if present)
                QString text = item->text();
                if (text.startsWith("✓ ")) {
                    text = text.mid(2);  // Remove checkmark and space
                }
                emit multiplierSelected(text);
            }
        }
    });

    layout->addWidget(m_table);

    setMinimumSize(300, 400);
}

void MultiplierWidget::loadMultiplierList() {
    m_allMultipliers.clear();

    switch (m_type) {
    case MultiplierType::State:
        // US states + Canadian provinces (for contests like RTTY Roundup, NAQP)
        m_allMultipliers = Arrl::getStatesAndProvinces();
        break;

    case MultiplierType::Section:
        // ARRL/RAC sections (for contests like Field Day, Sweepstakes)
        m_allMultipliers = Arrl::getAllSections();
        break;

    case MultiplierType::Country:
        // DXCC primary prefixes - will be loaded from CountryFile dynamically
        // This provides a fallback list if CountryFile isn't available yet
        m_allMultipliers = {"(Loading country list...)"};
        break;

    case MultiplierType::CQZone:
        // CQ Zones 1-40
        for (int i = 1; i <= 40; ++i) {
            m_allMultipliers.append(QString::number(i));
        }
        break;

    case MultiplierType::ITUZone:
        // ITU Zones 1-90
        for (int i = 1; i <= 90; ++i) {
            m_allMultipliers.append(QString::number(i));
        }
        break;

    case MultiplierType::Prefix:
        // WPX prefixes - too many to list, will be populated dynamically
        // as they're worked
        m_allMultipliers = {"(Worked prefixes will appear here)"};
        break;

    case MultiplierType::Grid:
        // Grids - too many to list, will be populated dynamically
        m_allMultipliers = {"(Worked grids will appear here)"};
        break;

    case MultiplierType::Custom:
        // Custom multipliers defined by contest
        m_allMultipliers = {"(Custom multipliers)"};
        break;

    default:
        // Fallback to sections
        m_allMultipliers = Arrl::getAllSections();
        break;
    }

    // Sort alphabetically (except for zones which are already numeric)
    if (m_type != MultiplierType::CQZone && m_type != MultiplierType::ITUZone) {
        m_allMultipliers.sort();
    }
}

void MultiplierWidget::setMultiplierType(MultiplierType type) {
    m_type = type;
    loadMultiplierList();
    updateDisplay();
}

void MultiplierWidget::setCountryList(const QStringList& prefixes) {
    if (m_type == MultiplierType::Country) {
        m_allMultipliers = prefixes;
        updateDisplay();
    }
}

void MultiplierWidget::setMultiplierWorked(const QString& value, BandType band) {
    // Normalize to uppercase for case-insensitive storage
    QString normalizedValue = value.toUpper();
    if (!m_workedMultipliers[normalizedValue].contains(band)) {
        m_workedMultipliers[normalizedValue].append(band);
    }
    updateDisplay();
}

void MultiplierWidget::setMultiplierNeeded(const QString& value) {
    // Normalize to uppercase for case-insensitive removal
    QString normalizedValue = value.toUpper();
    m_workedMultipliers.remove(normalizedValue);
    updateDisplay();
}

void MultiplierWidget::clear() {
    m_workedMultipliers.clear();
    updateDisplay();
}

MultiplierStatus MultiplierWidget::getStatus(const QString& value, BandType band) const {
    // Normalize to uppercase for case-insensitive lookup
    QString normalizedValue = value.toUpper();
    if (!m_workedMultipliers.contains(normalizedValue)) {
        return MultiplierStatus::Needed;
    }

    const QList<BandType>& bands = m_workedMultipliers[normalizedValue];

    if (band == BandType::None) {
        // All-band view: check if worked on any band
        return bands.isEmpty() ? MultiplierStatus::Needed : MultiplierStatus::Worked;
    } else {
        // Band-specific view: check if worked on this band
        return bands.contains(band) ? MultiplierStatus::Worked : MultiplierStatus::Needed;
    }
}

void MultiplierWidget::updateDisplay() {
    m_table->clear();

    // Build list of multipliers to display (filter out worked if m_hideWorked is true)
    QStringList displayList;

    // For dynamic types (Prefix, Grid, Custom), show worked multipliers
    // For static types (State, Section, Country, Zones), show pre-populated list
    bool isDynamicType = (m_type == MultiplierType::Prefix ||
                          m_type == MultiplierType::Grid ||
                          m_type == MultiplierType::Custom) &&
                         (m_allMultipliers.size() == 1 &&
                          m_allMultipliers.first().contains("Worked"));

    if (isDynamicType) {
        // Dynamic type - show only worked multipliers
        QSet<QString> uniqueMults;
        for (const QString& mult : m_workedMultipliers.keys()) {
            uniqueMults.insert(mult);
        }
        displayList = uniqueMults.values();
        displayList.sort();
    } else {
        // Static type - show pre-populated list
        for (const QString& mult : m_allMultipliers) {
            MultiplierStatus status = getStatus(mult, m_currentBand);

            // Skip worked multipliers if hiding them
            if (m_hideWorked && status == MultiplierStatus::Worked) {
                continue;
            }

            displayList.append(mult);
        }
    }

    // Calculate rows needed based on current column count
    int itemsPerColumn = (displayList.size() + m_columnCount - 1) / m_columnCount;
    m_table->setRowCount(itemsPerColumn);

    int multIndex = 0;
    for (int col = 0; col < m_columnCount; ++col) {
        for (int row = 0; row < itemsPerColumn; ++row) {
            if (multIndex >= displayList.size()) {
                break;
            }

            QString mult = displayList[multIndex];
            MultiplierStatus status = getStatus(mult, m_currentBand);

            // Add checkmark for worked multipliers
            QString displayText = mult;
            if (status == MultiplierStatus::Worked) {
                displayText = "✓ " + mult;
            }

            QTableWidgetItem* item = new QTableWidgetItem(displayText);
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole, mult);  // Store original mult value for selection

            // Set color based on status
            QColor color = getColorForStatus(status);
            item->setBackground(color);

            // Worked multipliers in gray text
            if (status == MultiplierStatus::Worked) {
                ThemeManager& theme = ThemeManager::instance();
                item->setForeground(theme.color(ColorRole::WorkedStationText));
            }

            m_table->setItem(row, col, item);
            multIndex++;
        }
    }

    // Adjust row heights
    for (int row = 0; row < m_table->rowCount(); ++row) {
        m_table->setRowHeight(row, 20);
    }
}

QColor MultiplierWidget::getColorForStatus(MultiplierStatus status) const {
    ThemeManager& theme = ThemeManager::instance();

    switch (status) {
    case MultiplierStatus::Worked:
        // Use slightly lighter version of WorkedStationText for background
        // (WorkedStationText is for text color, so we use a light gray background)
        return QColor(220, 220, 220);  // Light gray for worked
    case MultiplierStatus::Needed:
        return theme.color(ColorRole::NeededMultiplierBackground);
    case MultiplierStatus::Confirmed:
        return theme.color(ColorRole::ConfirmedMultiplierBackground);
    default:
        return Qt::white;
    }
}

void MultiplierWidget::applyTheme() {
    // Refresh display to update colors
    updateDisplay();
}

void MultiplierWidget::onContextMenuRequested(const QPoint& pos) {
    QMenu contextMenu(this);

    // Add toggle action
    QAction* toggleAction = contextMenu.addAction(
        m_hideWorked ? "Show All Multipliers" : "Hide Worked Multipliers"
    );
    connect(toggleAction, &QAction::triggered, this, &MultiplierWidget::onToggleHideWorked);

    // Show menu at cursor position
    contextMenu.exec(m_table->mapToGlobal(pos));
}

void MultiplierWidget::onToggleHideWorked() {
    m_hideWorked = !m_hideWorked;
    updateDisplay();
}

void MultiplierWidget::calculateColumnCount() {
    // Calculate optimal number of columns based on widget width
    int availableWidth = width() - 10;  // Account for margins

    if (availableWidth < MIN_COLUMN_WIDTH) {
        m_columnCount = 1;
        return;
    }

    // Calculate how many columns can fit
    int maxColumns = availableWidth / MIN_COLUMN_WIDTH;

    // At least 1 column, at most what can fit
    m_columnCount = qMax(1, maxColumns);
}

void MultiplierWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // Recalculate column count based on new width
    int oldColumnCount = m_columnCount;
    calculateColumnCount();

    // If column count changed, update the table
    if (oldColumnCount != m_columnCount) {
        m_table->setColumnCount(m_columnCount);

        // Update column widths
        for (int i = 0; i < m_columnCount; ++i) {
            m_table->setColumnWidth(i, MIN_COLUMN_WIDTH);
        }

        // Refresh the display with new column count
        updateDisplay();
    }
}

} // namespace TR4QT
