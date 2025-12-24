#include "MultiplierWidget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QPalette>

namespace TR4QT {

MultiplierWidget::MultiplierWidget(QWidget* parent)
    : QWidget(parent)
    , m_type(MultiplierType::Country)
    , m_currentBand(BandType::None)
{
    setupUI();
    loadMultiplierList();
    updateDisplay();
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
            emit multiplierSelected(item->text());
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
    if (!m_workedMultipliers[value].contains(band)) {
        m_workedMultipliers[value].append(band);
    }
    updateDisplay();
}

void MultiplierWidget::setMultiplierNeeded(const QString& value) {
    // Remove from worked list
    m_workedMultipliers.remove(value);
    updateDisplay();
}

void MultiplierWidget::clear() {
    m_workedMultipliers.clear();
    updateDisplay();
}

MultiplierStatus MultiplierWidget::getStatus(const QString& value, BandType band) const {
    if (!m_workedMultipliers.contains(value)) {
        return MultiplierStatus::Needed;
    }

    const QList<BandType>& bands = m_workedMultipliers[value];

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

    int itemsPerColumn = (m_allMultipliers.size() + 3) / 4;  // Divide into 4 columns
    m_table->setRowCount(itemsPerColumn);

    int multIndex = 0;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < itemsPerColumn; ++row) {
            if (multIndex >= m_allMultipliers.size()) {
                break;
            }

            QString mult = m_allMultipliers[multIndex];
            MultiplierStatus status = getStatus(mult, m_currentBand);

            QTableWidgetItem* item = new QTableWidgetItem(mult);
            item->setTextAlignment(Qt::AlignCenter);

            // Set color based on status
            QColor color = getColorForStatus(status);
            item->setBackground(color);

            // Worked multipliers in gray text
            if (status == MultiplierStatus::Worked) {
                item->setForeground(QColor(128, 128, 128));
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
    switch (status) {
    case MultiplierStatus::Worked:
        return QColor(220, 220, 220);  // Light gray for worked
    case MultiplierStatus::Needed:
        return QColor(255, 255, 200);  // Light yellow for needed
    case MultiplierStatus::Confirmed:
        return QColor(144, 238, 144);  // Light green for confirmed
    default:
        return Qt::white;
    }
}

} // namespace TR4QT
