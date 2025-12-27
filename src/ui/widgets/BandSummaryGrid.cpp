#include "BandSummaryGrid.h"
#include "../../utils/ThemeManager.h"
#include <QHBoxLayout>
#include <QFont>
#include <QMouseEvent>
#include <QCursor>

namespace TR4QT {

BandSummaryGrid::BandSummaryGrid(QWidget* parent)
    : QWidget(parent)
{
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

    QFont headerFont;
    headerFont.setBold(true);
    headerFont.setPointSize(10);  // Larger header font

    QFont dataFont("Monospace", 11);  // Larger data font

    // Row 0: Band headers (clickable)
    QList<BandType> headerBands = {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };
    QStringList bandHeaders = {"160", "80", "40", "20", "15", "10", "All"};
    for (int col = 0; col < bandHeaders.size(); ++col) {
        QLabel* header = new QLabel(bandHeaders[col], this);
        header->setFont(headerFont);
        header->setAlignment(Qt::AlignCenter);
        header->setMinimumWidth(50);  // Ensure readable width

        // Make band headers clickable (except "All")
        if (col < headerBands.size()) {
            header->setCursor(Qt::PointingHandCursor);
            // Hover style will be set by applyTheme()
            header->installEventFilter(this);
            m_bandHeaders[headerBands[col]] = header;
        }

        m_gridLayout->addWidget(header, 0, col + 1);
    }

    // Column 0: Row labels
    QLabel* qsoLabel = new QLabel("QSOs", this);
    qsoLabel->setFont(headerFont);
    qsoLabel->setMinimumWidth(70);
    m_gridLayout->addWidget(qsoLabel, 1, 0);

    QLabel* multLabel = new QLabel("Mults", this);
    multLabel->setFont(headerFont);
    multLabel->setMinimumWidth(70);
    m_gridLayout->addWidget(multLabel, 2, 0);

    QLabel* zoneLabel = new QLabel("Zones", this);
    zoneLabel->setFont(headerFont);
    zoneLabel->setMinimumWidth(70);
    m_gridLayout->addWidget(zoneLabel, 3, 0);

    QLabel* pointsLabel = new QLabel("Points", this);
    pointsLabel->setFont(headerFont);
    pointsLabel->setMinimumWidth(70);
    m_gridLayout->addWidget(pointsLabel, 4, 0);

    // Create data labels for each band
    QList<BandType> bands = {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };

    for (int col = 0; col < bands.size(); ++col) {
        BandType band = bands[col];

        // QSO count
        QLabel* qso = new QLabel("0", this);
        qso->setFont(dataFont);
        qso->setAlignment(Qt::AlignCenter);
        qso->setMinimumWidth(50);
        m_gridLayout->addWidget(qso, 1, col + 1);
        m_qsoLabels[band] = qso;

        // Mult count
        QLabel* mult = new QLabel("0", this);
        mult->setFont(dataFont);
        mult->setAlignment(Qt::AlignCenter);
        mult->setMinimumWidth(50);
        m_gridLayout->addWidget(mult, 2, col + 1);
        m_multLabels[band] = mult;

        // Zone count
        QLabel* zone = new QLabel("0", this);
        zone->setFont(dataFont);
        zone->setAlignment(Qt::AlignCenter);
        zone->setMinimumWidth(50);
        m_gridLayout->addWidget(zone, 3, col + 1);
        m_zoneLabels[band] = zone;

        // Points count
        QLabel* points = new QLabel("0", this);
        points->setFont(dataFont);
        points->setAlignment(Qt::AlignCenter);
        points->setMinimumWidth(50);
        m_gridLayout->addWidget(points, 4, col + 1);
        m_pointsLabels[band] = points;
    }

    // "All" column (totals)
    m_qsoAllLabel = new QLabel("0", this);
    m_qsoAllLabel->setFont(dataFont);
    m_qsoAllLabel->setAlignment(Qt::AlignCenter);
    m_qsoAllLabel->setMinimumWidth(50);
    m_gridLayout->addWidget(m_qsoAllLabel, 1, 7);

    m_multAllLabel = new QLabel("0", this);
    m_multAllLabel->setFont(dataFont);
    m_multAllLabel->setAlignment(Qt::AlignCenter);
    m_multAllLabel->setMinimumWidth(50);
    m_gridLayout->addWidget(m_multAllLabel, 2, 7);

    m_zoneAllLabel = new QLabel("0", this);
    m_zoneAllLabel->setFont(dataFont);
    m_zoneAllLabel->setAlignment(Qt::AlignCenter);
    m_zoneAllLabel->setMinimumWidth(50);
    m_gridLayout->addWidget(m_zoneAllLabel, 3, 7);

    m_pointsAllLabel = new QLabel("0", this);
    m_pointsAllLabel->setFont(dataFont);
    m_pointsAllLabel->setAlignment(Qt::AlignCenter);
    m_pointsAllLabel->setMinimumWidth(50);
    m_gridLayout->addWidget(m_pointsAllLabel, 4, 7);

    // Total points label (right of "All" column, row 1)
    m_totalPointsLabel = new QLabel("0 Pts", this);
    m_totalPointsLabel->setFont(headerFont);
    m_totalPointsLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_totalPointsLabel->setMinimumWidth(100);
    m_gridLayout->addWidget(m_totalPointsLabel, 1, 8, 1, 2);

    // "Both:" field (right side, row 4 now since we added Points row)
    QLabel* bothLabelText = new QLabel("Both:", this);
    bothLabelText->setFont(headerFont);
    m_gridLayout->addWidget(bothLabelText, 4, 8);

    m_bothLabel = new QLabel("", this);
    m_bothLabel->setFont(dataFont);
    m_bothLabel->setMinimumWidth(100);
    m_gridLayout->addWidget(m_bothLabel, 4, 9);

    m_gridLayout->setColumnStretch(10, 1);  // Push everything left
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
    m_qsoAllLabel->setText(QString::number(count));
}

void BandSummaryGrid::setAllMults(int count) {
    m_multAllLabel->setText(QString::number(count));
}

void BandSummaryGrid::setAllZones(int count) {
    m_zoneAllLabel->setText(QString::number(count));
}

void BandSummaryGrid::setAllPoints(int points) {
    m_pointsAllLabel->setText(QString::number(points));
}

void BandSummaryGrid::setFinalScore(int score) {
    m_totalPointsLabel->setText(QString("%1 Pts").arg(score));
}

void BandSummaryGrid::setBothNeeded(const QString& bands) {
    m_bothLabel->setText(bands);
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

    // Clear totals
    m_qsoAllLabel->setText("0");
    m_multAllLabel->setText("0");
    m_zoneAllLabel->setText("0");
    m_pointsAllLabel->setText("0");
    m_totalPointsLabel->setText("0 Pts");
    m_bothLabel->setText("");
}

QString BandSummaryGrid::bandToColumnLabel(BandType band) const {
    switch (band) {
    case BandType::Band160M: return "160";
    case BandType::Band80M:  return "80";
    case BandType::Band40M:  return "40";
    case BandType::Band20M:  return "20";
    case BandType::Band15M:  return "15";
    case BandType::Band10M:  return "10";
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

} // namespace TR4QT
