#include "MultiplierWidget.h"
#include "../../utils/ThemeManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QPalette>
#include <QMenu>

namespace TR4QT {

MultiplierWidget::MultiplierWidget(QWidget* parent)
    : QWidget(parent)
    , m_type(MultiplierType::Country)
    , m_currentBand(BandType::None)
    , m_hideWorked(false)
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
    m_table->setColumnCount(4);  // 4 columns like TR4W
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

    // Set column widths
    for (int i = 0; i < 4; ++i) {
        m_table->setColumnWidth(i, 60);
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
    // Load DXCC prefix list (abbreviated 2-3 letter codes)
    // This is a subset based on the TR4W screenshot
    m_allMultipliers = {
        "Ab", "Lax", "Oh", "Sv",
        "Ak", "Mb", "Ok", "Ter",
        "Al", "MDC", "One", "Tn",
        "Ar", "Me", "Onn", "Ut",
        "Az", "Mi", "Ons", "Va",
        "Bc", "Mn", "Or", "Vi",
        "C", "Mo", "Orn", "Vt",
        "Ct", "Ms", "Pac", "WcF",
        "De", "Mt", "PE", "Wi",
        "Eb", "Nb", "Pr", "WMa",
        "EMA", "Nc", "Qc", "WNy",
        "ENy", "Nd", "Ri", "WPa",
        "EPA", "Ne", "Sb", "WTx",
        "EWA", "NFj", "Sc", "Wv",
        "Ga", "Nh", "Scv", "WWa",
        "Gh", "Ni", "Sd", "Wy",
        "Ia", "NLj", "Sdg",
        "Id", "NM", "Sf",
        "Il", "NNj", "Sft",
        "In", "NNy", "Sjv",
        "Ks", "Ns", "Sk",
        "Ky", "NTx", "SNj",
        "La", "Nv", "STx"
    };

    // Sort alphabetically
    m_allMultipliers.sort();
}

void MultiplierWidget::setMultiplierType(MultiplierType type) {
    m_type = type;
    loadMultiplierList();
    updateDisplay();
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
    for (const QString& mult : m_allMultipliers) {
        MultiplierStatus status = getStatus(mult, m_currentBand);

        // Skip worked multipliers if hiding them
        if (m_hideWorked && status == MultiplierStatus::Worked) {
            continue;
        }

        displayList.append(mult);
    }

    int itemsPerColumn = (displayList.size() + 3) / 4;  // Divide into 4 columns
    m_table->setRowCount(itemsPerColumn);

    int multIndex = 0;
    for (int col = 0; col < 4; ++col) {
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

} // namespace TR4QT
