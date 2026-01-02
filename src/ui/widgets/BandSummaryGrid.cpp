#include "BandSummaryGrid.h"
#include "../../utils/ThemeManager.h"
#include <QHBoxLayout>
#include <QFont>
#include <QMouseEvent>
#include <QCursor>

namespace TR4QT {

BandSummaryGrid::BandSummaryGrid(QWidget* parent)
    : QWidget(parent),
      m_usesModeGroupBreakdown(false),
      m_usesZoneMultipliers(true),  // Default: show zones
      m_totalPointsLabel(nullptr),
      m_bothLabel(nullptr),
      m_qsoAllLabel(nullptr),
      m_multAllLabel(nullptr),
      m_zoneAllLabel(nullptr),
      m_pointsAllLabel(nullptr)
{
    // Default: Standard HF contest bands
    m_visibleBands = { BandType::Band160M, BandType::Band80M, BandType::Band40M,
                       BandType::Band20M, BandType::Band15M, BandType::Band10M };

    // Ensure widget fills its background (prevents transparent/black rendering)
    setAutoFillBackground(true);

    setupUI();

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &BandSummaryGrid::applyTheme);
    applyTheme();
}

void BandSummaryGrid::setupUI() {
    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setSpacing(8);  // More spacing between cells
    m_gridLayout->setContentsMargins(10, 10, 10, 10);  // More padding around edges

    // Build initial grid with default configuration
    rebuildGrid();
}

void BandSummaryGrid::setQSOCount(BandType band, int count) {
    if (m_qsoLabels.contains(band)) {
        m_qsoLabels[band]->setText(QString::number(count));
    }
}

void BandSummaryGrid::setMultCount(BandType band, int count) {
    if (m_multLabels.contains(band)) {
        m_multLabels[band]->setText(QString::number(count));
    }
}

void BandSummaryGrid::setZoneCount(BandType band, int count) {
    if (m_zoneLabels.contains(band)) {
        m_zoneLabels[band]->setText(QString::number(count));
    }
}

void BandSummaryGrid::setPointsCount(BandType band, int points) {
    if (m_pointsLabels.contains(band)) {
        m_pointsLabels[band]->setText(QString::number(points));
    }
}

void BandSummaryGrid::setAllQSOs(int count) {
    if (m_qsoAllLabel) m_qsoAllLabel->setText(QString::number(count));
}

void BandSummaryGrid::setAllMults(int count) {
    if (m_multAllLabel) m_multAllLabel->setText(QString::number(count));
}

void BandSummaryGrid::setAllZones(int count) {
    if (m_zoneAllLabel) m_zoneAllLabel->setText(QString::number(count));
}

void BandSummaryGrid::setAllPoints(int points) {
    if (m_pointsAllLabel) m_pointsAllLabel->setText(QString::number(points));
}

void BandSummaryGrid::setFinalScore(int score) {
    if (m_totalPointsLabel) m_totalPointsLabel->setText(QString("%1 Pts").arg(score));
}

void BandSummaryGrid::setBothNeeded(const QString& bands) {
    if (m_bothLabel) m_bothLabel->setText(bands);
}

void BandSummaryGrid::clearAll() {
    // Clear all band counts
    for (auto label : m_qsoLabels) {
        label->setText("0");
    }
    for (auto label : m_multLabels) {
        label->setText("0");
    }
    for (auto label : m_zoneLabels) {
        label->setText("0");
    }
    for (auto label : m_pointsLabels) {
        label->setText("0");
    }

    // Clear mode group labels (if using mode breakdown)
    for (auto& bandMap : m_modeGroupLabels) {
        for (auto label : bandMap) {
            label->setText("0");
        }
    }
    for (auto label : m_modeGroupAllLabels) {
        label->setText("0");
    }

    // Clear totals
    if (m_qsoAllLabel) m_qsoAllLabel->setText("0");
    if (m_multAllLabel) m_multAllLabel->setText("0");
    if (m_zoneAllLabel) m_zoneAllLabel->setText("0");
    if (m_pointsAllLabel) m_pointsAllLabel->setText("0");
    if (m_totalPointsLabel) m_totalPointsLabel->setText("0 Pts");
    if (m_bothLabel) m_bothLabel->setText("");
}

QString BandSummaryGrid::bandToColumnLabel(BandType band) const {
    switch (band) {
    case BandType::Band160M: return "160";
    case BandType::Band80M:  return "80";
    case BandType::Band40M:  return "40";
    case BandType::Band20M:  return "20";
    case BandType::Band15M:  return "15";
    case BandType::Band10M:  return "10";
    case BandType::Band6M:   return "6";
    case BandType::Band2M:   return "2";
    case BandType::Band70CM: return "70cm";
    default: return "";
    }
}

void BandSummaryGrid::setFontSize(int pointSize) {
    QFont font("Monospace", pointSize);

    // Apply to all labels
    for (auto label : m_qsoLabels.values()) {
        if (label) label->setFont(font);
    }
    for (auto label : m_multLabels.values()) {
        if (label) label->setFont(font);
    }
    for (auto label : m_zoneLabels.values()) {
        if (label) label->setFont(font);
    }
    for (auto label : m_pointsLabels.values()) {
        if (label) label->setFont(font);
    }

    if (m_qsoAllLabel) m_qsoAllLabel->setFont(font);
    if (m_multAllLabel) m_multAllLabel->setFont(font);
    if (m_zoneAllLabel) m_zoneAllLabel->setFont(font);
    if (m_pointsAllLabel) m_pointsAllLabel->setFont(font);
    if (m_totalPointsLabel) m_totalPointsLabel->setFont(font);
    if (m_bothLabel) m_bothLabel->setFont(font);

    // Also update all labels in the grid layout (headers)
    for (int i = 0; i < m_gridLayout->count(); ++i) {
        QWidget* widget = m_gridLayout->itemAt(i)->widget();
        if (QLabel* label = qobject_cast<QLabel*>(widget)) {
            label->setFont(font);
        }
    }
}

bool BandSummaryGrid::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // Check if clicked object is one of our band headers
            for (auto it = m_bandHeaders.begin(); it != m_bandHeaders.end(); ++it) {
                if (it.value() == obj) {
                    emit bandClicked(it.key());
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void BandSummaryGrid::applyTheme() {
    ThemeManager& theme = ThemeManager::instance();

    // Update hover style for band headers
    QString hoverStyle = QString("QLabel:hover { background-color: %1; }")
        .arg(theme.color(ColorRole::HoverHighlight).name());

    for (auto it = m_bandHeaders.begin(); it != m_bandHeaders.end(); ++it) {
        it.value()->setStyleSheet(hoverStyle);
    }
}

void BandSummaryGrid::setMultipliersEnabled(bool enabled) {
    // Gray out multiplier row for contests that don't use multipliers
    QString style = enabled ? "" : "color: #808080;";  // Gray text when disabled

    // Update all mult labels with gray style
    for (auto it = m_multLabels.begin(); it != m_multLabels.end(); ++it) {
        it.value()->setStyleSheet(style);
        if (!enabled) {
            it.value()->setText("-");  // Show dash instead of 0
        }
    }

    // Update "All" mult label
    if (m_multAllLabel) {
        m_multAllLabel->setStyleSheet(style);
        if (!enabled) {
            m_multAllLabel->setText("-");
        }
    }
}

void BandSummaryGrid::setBandSelectionEnabled(bool enabled) {
    // Enable/disable band header buttons (when radio disconnected)
    // Statistics remain visible, but band selection is disabled
    for (auto it = m_bandHeaders.begin(); it != m_bandHeaders.end(); ++it) {
        QLabel* header = it.value();
        header->setEnabled(enabled);

        // Update cursor to indicate clickability
        if (enabled) {
            header->setCursor(Qt::PointingHandCursor);
        } else {
            header->setCursor(Qt::ArrowCursor);
        }
    }
}

void BandSummaryGrid::setVisibleBands(const QList<BandType>& bands) {
    // Update visible bands and rebuild grid
    m_visibleBands = bands;
    rebuildGrid();
}

void BandSummaryGrid::configureForContest(bool usesModeGroupBreakdown, bool usesZoneMultipliers) {
    m_usesModeGroupBreakdown = usesModeGroupBreakdown;
    m_usesZoneMultipliers = usesZoneMultipliers;

    // Rebuild the grid with new configuration
    rebuildGrid();
}

void BandSummaryGrid::rebuildGrid() {
    // Clear existing layout and delete widgets
    QLayoutItem* item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();  // Delete widget to prevent orphaned top-level windows
        }
        delete item;
    }

    // Clear label maps
    m_qsoLabels.clear();
    m_multLabels.clear();
    m_zoneLabels.clear();
    m_pointsLabels.clear();
    m_bandHeaders.clear();
    m_modeGroupLabels.clear();
    m_modeGroupAllLabels.clear();
    m_modeGroupRowHeaders.clear();

    // CRITICAL: Clear member pointers to prevent use-after-free crashes
    // These widgets were deleted above but pointers weren't cleared
    m_qsoAllLabel = nullptr;
    m_multAllLabel = nullptr;
    m_zoneAllLabel = nullptr;
    m_pointsAllLabel = nullptr;
    m_totalPointsLabel = nullptr;
    m_bothLabel = nullptr;

    // Now rebuild from scratch
    QFont headerFont;
    headerFont.setBold(true);
    headerFont.setPointSize(10);

    QFont dataFont("Monospace", 11);

    int currentRow = 0;

    // Row 0: Band headers (clickable)
    // Use visible bands from m_visibleBands (contest-specific)
    for (int col = 0; col < m_visibleBands.size(); ++col) {
        BandType band = m_visibleBands[col];
        QString bandName = bandToColumnLabel(band);

        QLabel* header = new QLabel(bandName, this);
        header->setFont(headerFont);
        header->setAlignment(Qt::AlignCenter);
        header->setMinimumWidth(50);

        // Make band headers clickable
        header->setCursor(Qt::PointingHandCursor);
        header->installEventFilter(this);
        m_bandHeaders[band] = header;

        m_gridLayout->addWidget(header, currentRow, col + 1);
    }

    // "All" column header
    QLabel* allHeader = new QLabel("All", this);
    allHeader->setFont(headerFont);
    allHeader->setAlignment(Qt::AlignCenter);
    allHeader->setMinimumWidth(50);
    m_gridLayout->addWidget(allHeader, currentRow, m_visibleBands.size() + 1);

    currentRow++;

    // Create data rows based on configuration
    QList<BandType> bands = m_visibleBands;

    if (m_usesModeGroupBreakdown) {
        // Mixed-mode: Show Phone, CW, Digital rows
        QList<ModeGroup> modeGroups = {ModeGroup::Phone, ModeGroup::CW, ModeGroup::Digital};

        for (ModeGroup group : modeGroups) {
            // Row header
            QLabel* rowHeader = new QLabel(modeGroupToString(group), this);
            rowHeader->setFont(headerFont);
            rowHeader->setMinimumWidth(70);
            m_gridLayout->addWidget(rowHeader, currentRow, 0);
            m_modeGroupRowHeaders[group] = rowHeader;

            // Data labels for each band
            for (int col = 0; col < bands.size(); ++col) {
                BandType band = bands[col];
                QLabel* label = new QLabel("0", this);
                label->setFont(dataFont);
                label->setAlignment(Qt::AlignCenter);
                label->setMinimumWidth(50);
                m_gridLayout->addWidget(label, currentRow, col + 1);
                m_modeGroupLabels[group][band] = label;
            }

            // "All" column for this mode group
            QLabel* allLabel = new QLabel("0", this);
            allLabel->setFont(dataFont);
            allLabel->setAlignment(Qt::AlignCenter);
            allLabel->setMinimumWidth(50);
            m_gridLayout->addWidget(allLabel, currentRow, m_visibleBands.size() + 1);
            m_modeGroupAllLabels[group] = allLabel;

            currentRow++;
        }
    } else {
        // Single-mode: Show just "QSOs" row
        QLabel* qsoLabel = new QLabel("QSOs", this);
        qsoLabel->setFont(headerFont);
        qsoLabel->setMinimumWidth(70);
        m_gridLayout->addWidget(qsoLabel, currentRow, 0);

        for (int col = 0; col < bands.size(); ++col) {
            BandType band = bands[col];
            QLabel* qso = new QLabel("0", this);
            qso->setFont(dataFont);
            qso->setAlignment(Qt::AlignCenter);
            qso->setMinimumWidth(50);
            m_gridLayout->addWidget(qso, currentRow, col + 1);
            m_qsoLabels[band] = qso;
        }

        m_qsoAllLabel = new QLabel("0", this);
        m_qsoAllLabel->setFont(dataFont);
        m_qsoAllLabel->setAlignment(Qt::AlignCenter);
        m_qsoAllLabel->setMinimumWidth(50);
        m_gridLayout->addWidget(m_qsoAllLabel, currentRow, m_visibleBands.size() + 1);

        // Total points label (right of "All" column, spans 2 columns)
        if (!m_totalPointsLabel) {
            m_totalPointsLabel = new QLabel("0 Pts", this);
        }
        m_totalPointsLabel->setFont(headerFont);
        m_totalPointsLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
        m_totalPointsLabel->setMinimumWidth(100);
        m_gridLayout->addWidget(m_totalPointsLabel, currentRow, 8, 1, 2);

        currentRow++;
    }

    // Mults row
    QLabel* multLabel = new QLabel("Mults", this);
    multLabel->setFont(headerFont);
    multLabel->setMinimumWidth(70);
    m_gridLayout->addWidget(multLabel, currentRow, 0);

    for (int col = 0; col < bands.size(); ++col) {
        BandType band = bands[col];
        QLabel* mult = new QLabel("0", this);
        mult->setFont(dataFont);
        mult->setAlignment(Qt::AlignCenter);
        mult->setMinimumWidth(50);
        m_gridLayout->addWidget(mult, currentRow, col + 1);
        m_multLabels[band] = mult;
    }

    m_multAllLabel = new QLabel("0", this);
    m_multAllLabel->setFont(dataFont);
    m_multAllLabel->setAlignment(Qt::AlignCenter);
    m_multAllLabel->setMinimumWidth(50);
    m_gridLayout->addWidget(m_multAllLabel, currentRow, m_visibleBands.size() + 1);
    currentRow++;

    // Zones row (only if contest uses zone multipliers)
    if (m_usesZoneMultipliers) {
        QLabel* zoneLabel = new QLabel("Zones", this);
        zoneLabel->setFont(headerFont);
        zoneLabel->setMinimumWidth(70);
        m_gridLayout->addWidget(zoneLabel, currentRow, 0);

        for (int col = 0; col < bands.size(); ++col) {
            BandType band = bands[col];
            QLabel* zone = new QLabel("0", this);
            zone->setFont(dataFont);
            zone->setAlignment(Qt::AlignCenter);
            zone->setMinimumWidth(50);
            m_gridLayout->addWidget(zone, currentRow, col + 1);
            m_zoneLabels[band] = zone;
        }

        m_zoneAllLabel = new QLabel("0", this);
        m_zoneAllLabel->setFont(dataFont);
        m_zoneAllLabel->setAlignment(Qt::AlignCenter);
        m_zoneAllLabel->setMinimumWidth(50);
        m_gridLayout->addWidget(m_zoneAllLabel, currentRow, m_visibleBands.size() + 1);
        currentRow++;
    }

    // Points row
    QLabel* pointsLabel = new QLabel("Points", this);
    pointsLabel->setFont(headerFont);
    pointsLabel->setMinimumWidth(70);
    m_gridLayout->addWidget(pointsLabel, currentRow, 0);

    for (int col = 0; col < bands.size(); ++col) {
        BandType band = bands[col];
        QLabel* points = new QLabel("0", this);
        points->setFont(dataFont);
        points->setAlignment(Qt::AlignCenter);
        points->setMinimumWidth(50);
        m_gridLayout->addWidget(points, currentRow, col + 1);
        m_pointsLabels[band] = points;
    }

    m_pointsAllLabel = new QLabel("0", this);
    m_pointsAllLabel->setFont(dataFont);
    m_pointsAllLabel->setAlignment(Qt::AlignCenter);
    m_pointsAllLabel->setMinimumWidth(50);
    m_gridLayout->addWidget(m_pointsAllLabel, currentRow, m_visibleBands.size() + 1);

    // "Both:" field (right side of points row)
    QLabel* bothLabelText = new QLabel("Both:", this);
    bothLabelText->setFont(headerFont);
    m_gridLayout->addWidget(bothLabelText, currentRow, 8);

    if (!m_bothLabel) {
        m_bothLabel = new QLabel("", this);
    }
    m_bothLabel->setFont(dataFont);
    m_bothLabel->setMinimumWidth(100);
    m_gridLayout->addWidget(m_bothLabel, currentRow, 9);

    // For mode breakdown contests, show total points on points row
    if (m_usesModeGroupBreakdown) {
        if (!m_totalPointsLabel) {
            m_totalPointsLabel = new QLabel("0 Pts", this);
        }
        // Move total points to span across columns 8-9 on row 1 (first mode group row)
        // Actually, let's put it at the top right, spanning multiple rows
        m_totalPointsLabel->setFont(headerFont);
        m_totalPointsLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
        m_totalPointsLabel->setMinimumWidth(100);
        m_gridLayout->addWidget(m_totalPointsLabel, 1, 8, 1, 2);  // Row 1, span 2 cols
    }

    currentRow++;

    m_gridLayout->setColumnStretch(10, 1);  // Push everything left

    // Apply theme to new widgets
    applyTheme();
}

void BandSummaryGrid::setModeGroupQSOCount(BandType band, ModeGroup group, int count) {
    if (m_modeGroupLabels.contains(group) && m_modeGroupLabels[group].contains(band)) {
        m_modeGroupLabels[group][band]->setText(QString::number(count));
    }
}

void BandSummaryGrid::setAllModeGroupQSOs(ModeGroup group, int count) {
    if (m_modeGroupAllLabels.contains(group)) {
        m_modeGroupAllLabels[group]->setText(QString::number(count));
    }
}

} // namespace TR4QT
