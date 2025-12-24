#include "BandMapWidget.h"
#include "../../utils/ThemeManager.h"
#include "../../logging/LogMacros.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <algorithm>

namespace TR4QT {

BandMapWidget::BandMapWidget(QWidget* parent)
    : QAbstractScrollArea(parent)
    , m_currentFrequency(0)
    , m_selectedIndex(-1)
    , m_columnCount(1)
    , m_columnWidth(200)
    , m_sortMode(BandMapSortMode::Frequency)  // Default: sort by frequency
{
    setMinimumWidth(200);
    setMinimumHeight(50);  // Allow very short windows (reduced from 300)
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // White background with black text (system default style)
    QPalette pal = viewport()->palette();
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Text, Qt::black);
    viewport()->setAutoFillBackground(true);
    viewport()->setPalette(pal);

    // Enable scrollbars as needed (both vertical and horizontal)
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable context menu
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // Initialize scrollbar ranges
    updateScrollBars();

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &BandMapWidget::applyTheme);
    applyTheme();
}

void BandMapWidget::addSpot(const Spot& spot) {
    // Check if spot already exists (update it)
    for (int i = 0; i < m_spots.size(); ++i) {
        if (m_spots[i].callsign == spot.callsign) {
            m_spots[i] = spot;
            sortSpots();
            calculateColumnLayout();  // Recalculate columns when spots change
            updateScrollBars();
            viewport()->update();
            return;
        }
    }

    // Add new spot
    m_spots.append(spot);
    sortSpots();
    calculateColumnLayout();  // Recalculate columns when spots change
    updateScrollBars();
    viewport()->update();
}

void BandMapWidget::removeSpot(const QString& callsign) {
    for (int i = 0; i < m_spots.size(); ++i) {
        if (m_spots[i].callsign == callsign) {
            m_spots.removeAt(i);
            calculateColumnLayout();  // Recalculate columns when spots change
            updateScrollBars();
            viewport()->update();
            return;
        }
    }
}

void BandMapWidget::clearSpots() {
    m_spots.clear();
    m_selectedIndex = -1;
    calculateColumnLayout();  // Recalculate columns when spots change
    updateScrollBars();
    viewport()->update();
}

void BandMapWidget::setCurrentFrequency(freq_t freq) {
    m_currentFrequency = freq;
    viewport()->update();
}

void BandMapWidget::sortSpots() {
    if (m_sortMode == BandMapSortMode::Frequency) {
        // Sort by frequency (ascending)
        std::sort(m_spots.begin(), m_spots.end(),
                 [](const Spot& a, const Spot& b) {
                     return a.frequency < b.frequency;
                 });
    } else {
        // Sort alphabetically by callsign
        std::sort(m_spots.begin(), m_spots.end(),
                 [](const Spot& a, const Spot& b) {
                     return a.callsign < b.callsign;
                 });
    }
}

void BandMapWidget::calculateColumnLayout() {
    // Column-first layout: Calculate columns needed based on available height
    // Each column needs minimum 150 pixels (for "14025.0 M W1ABC" format)
    const int MIN_COLUMN_WIDTH = 150;
    m_columnWidth = MIN_COLUMN_WIDTH;

    // Calculate how many rows fit in available height
    QFontMetrics fm(QFont("Monospace", 9));
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewport()->height() - FOOTER_HEIGHT;
    int lineHeight = rowHeight();
    int maxRowsPerColumn = qMax(1, availableHeight / lineHeight);

    // Calculate how many columns needed to show all spots
    m_columnCount = (m_spots.size() + maxRowsPerColumn - 1) / maxRowsPerColumn;
    if (m_columnCount < 1) m_columnCount = 1;

    // Note: If total width (m_columnCount * m_columnWidth) exceeds viewport width,
    // horizontal scrollbar will appear automatically
}

void BandMapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), Qt::white);

    QFont font("Monospace", 9);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int lineHeight = rowHeight();
    int scrollY = verticalScrollBar()->value();
    int scrollX = horizontalScrollBar()->value();
    int viewportHeight = viewport()->height();
    int viewportWidth = viewport()->width();

    // Reserve space at bottom for spot count
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewportHeight - FOOTER_HEIGHT;
    int rowsPerColumn = availableHeight / lineHeight;
    if (rowsPerColumn < 1) rowsPerColumn = 1;

    // Draw each spot in column-first layout (top-to-bottom, then left-to-right)
    for (int i = 0; i < m_spots.size(); ++i) {
        const Spot& spot = m_spots[i];

        // Column-first layout: spots flow top-to-bottom within each column
        int col = i / rowsPerColumn;  // Which column
        int row = i % rowsPerColumn;  // Which row in that column

        int x = col * m_columnWidth - scrollX;  // Account for horizontal scroll
        int y = row * lineHeight - scrollY;     // Account for vertical scroll

        // Skip if this spot would be off-screen (horizontally or vertically)
        if (x + m_columnWidth < 0 || x >= viewportWidth ||
            y + lineHeight < 0 || y >= availableHeight) {
            continue;
        }

        // Determine text color (for white background)
        ThemeManager& theme = ThemeManager::instance();
        QColor textColor;
        if (spot.isMultiplier) {
            textColor = theme.color(ColorRole::MultiplierText);
        } else if (spot.isWorked) {
            textColor = theme.color(ColorRole::WorkedStationText);
        } else {
            textColor = Qt::black;  // Black for unworked non-mults
        }

        painter.setPen(textColor);

        // Draw frequency (left side of column)
        QString freqStr = formatFrequency(spot.frequency);
        painter.drawText(x + 5, y + fm.ascent() + 2, freqStr);

        // Draw "M" marker for multipliers
        if (spot.isMultiplier) {
            painter.setPen(Qt::red);
            painter.drawText(x + 50, y + fm.ascent() + 2, "M");
            painter.setPen(textColor);
        }

        // Draw callsign (right side of column entry)
        painter.drawText(x + 70, y + fm.ascent() + 2, spot.callsign);

        // Highlight selected spot with blue rectangle
        if (i == m_selectedIndex) {
            painter.setPen(QPen(Qt::blue, 2));
            painter.drawRect(x + 2, y + 1, m_columnWidth - 4, lineHeight - 2);
        }
    }

    // Draw spot count at bottom (always visible, not scrolled)
    painter.setPen(Qt::darkBlue);
    QString countStr = QString("%1 spots").arg(m_spots.size());
    int countY = viewportHeight - fm.height() - 5;
    painter.drawText(viewport()->width() / 2 - fm.horizontalAdvance(countStr) / 2,
                    countY + fm.ascent(), countStr);
}

void BandMapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int spotIndex = findSpotAtPosition(event->pos());
        if (spotIndex >= 0 && spotIndex < m_spots.size()) {
            m_selectedIndex = spotIndex;
            // Single click: QSY to frequency AND populate callsign
            emit qsyRequested(m_spots[spotIndex].frequency);
            emit callsignSelected(m_spots[spotIndex].callsign);
            viewport()->update();
        }
    }
}

void BandMapWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int spotIndex = findSpotAtPosition(event->pos());
        if (spotIndex >= 0 && spotIndex < m_spots.size()) {
            emit callsignSelected(m_spots[spotIndex].callsign);
        }
    }
}

void BandMapWidget::resizeEvent(QResizeEvent* event) {
    QAbstractScrollArea::resizeEvent(event);
    calculateColumnLayout();
    updateScrollBars();
    viewport()->update();
}

void BandMapWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);

    // Check if user right-clicked on a specific spot
    int spotIndex = findSpotAtPosition(event->pos());
    if (spotIndex >= 0 && spotIndex < m_spots.size()) {
        const Spot& spot = m_spots[spotIndex];
        QString deleteText = QString("Delete Spot: %1").arg(spot.callsign);
        QAction* deleteAction = menu.addAction(deleteText);

        // Capture callsign by value for the lambda
        QString callsign = spot.callsign;
        connect(deleteAction, &QAction::triggered, this, [this, callsign]() {
            removeSpot(callsign);
        });

        menu.addSeparator();
    }

    // Sort options
    QAction* sortByFreqAction = menu.addAction("Sort by Frequency");
    sortByFreqAction->setCheckable(true);
    sortByFreqAction->setChecked(m_sortMode == BandMapSortMode::Frequency);
    connect(sortByFreqAction, &QAction::triggered, this, [this]() {
        m_sortMode = BandMapSortMode::Frequency;
        sortSpots();
        viewport()->update();
    });

    QAction* sortByCallAction = menu.addAction("Sort by Callsign");
    sortByCallAction->setCheckable(true);
    sortByCallAction->setChecked(m_sortMode == BandMapSortMode::Callsign);
    connect(sortByCallAction, &QAction::triggered, this, [this]() {
        m_sortMode = BandMapSortMode::Callsign;
        sortSpots();
        viewport()->update();
    });

    menu.addSeparator();

    QAction* clearAction = menu.addAction("Clear All Spots");
    connect(clearAction, &QAction::triggered, this, &BandMapWidget::clearSpots);

    menu.exec(event->globalPos());
}

int BandMapWidget::findSpotAtPosition(const QPoint& pos) {
    int lineHeight = rowHeight();
    int scrollY = verticalScrollBar()->value();
    int scrollX = horizontalScrollBar()->value();

    QFontMetrics fm(QFont("Monospace", 9));
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewport()->height() - FOOTER_HEIGHT;
    int rowsPerColumn = qMax(1, availableHeight / lineHeight);

    // Determine which column was clicked (accounting for horizontal scroll)
    int col = (pos.x() + scrollX) / m_columnWidth;
    if (col < 0 || col >= m_columnCount) {
        return -1;
    }

    // Determine which row in that column was clicked (accounting for vertical scroll)
    int row = (pos.y() + scrollY) / lineHeight;
    if (row < 0 || row >= rowsPerColumn) {
        return -1;
    }

    // Calculate spot index from column-first layout (top-to-bottom, then left-to-right)
    int spotIndex = (col * rowsPerColumn) + row;

    if (spotIndex >= 0 && spotIndex < m_spots.size()) {
        return spotIndex;
    }

    return -1;
}

int BandMapWidget::rowHeight() const {
    QFont font("Monospace", 9);
    QFontMetrics fm(font);
    return fm.height() + 4;  // 2px padding top and bottom
}

QString BandMapWidget::formatFrequency(freq_t freq) const {
    // Convert Hz to MHz with 3 decimal places (like TR4W)
    // Examples: 7.051, 14.074, 28.200
    // Max frequency: 435.000 MHz = 7 chars
    // Format with fixed width (right-aligned, 7 chars) for consistent column
    double freqMhz = freq / 1000000.0;
    QString result = QString("%1").arg(freqMhz, 7, 'f', 3);

    // Debug: log first few conversions to verify format
    static int debugCount = 0;
    if (debugCount < 5) {
        LOG_DEBUG("BandMapWidget", QString("formatFrequency: %1 Hz -> %2 MHz").arg(freq).arg(result));
        debugCount++;
    }

    return result;
}

void BandMapWidget::updateScrollBars() {
    // Column-first layout: Calculate scrollbar ranges for both directions
    int lineHeight = rowHeight();

    QFontMetrics fm(QFont("Monospace", 9));
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewport()->height() - FOOTER_HEIGHT;
    int availableWidth = viewport()->width();

    // Safety check
    if (m_columnCount < 1) m_columnCount = 1;

    // Calculate rows per column based on available height
    int rowsPerColumn = qMax(1, availableHeight / lineHeight);

    // Vertical scrollbar: Only needed if spots in a column exceed available height
    // (This is rare with column-first layout, as columns are sized to fit height)
    int contentHeight = rowsPerColumn * lineHeight;
    verticalScrollBar()->setPageStep(availableHeight);
    verticalScrollBar()->setRange(0, qMax(0, contentHeight - availableHeight));

    // Horizontal scrollbar: Needed when columns extend beyond viewport width
    int totalWidth = m_columnCount * m_columnWidth;
    horizontalScrollBar()->setPageStep(availableWidth);
    horizontalScrollBar()->setRange(0, qMax(0, totalWidth - availableWidth));
}

void BandMapWidget::scrollContentsBy(int dx, int dy) {
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    viewport()->update();
}

void BandMapWidget::applyTheme() {
    // Repaint to update colors
    viewport()->update();
}

} // namespace TR4QT
