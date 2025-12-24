#include "BandMapWidget.h"
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
    : QWidget(parent)
    , m_currentFrequency(0)
    , m_selectedIndex(-1)
    , m_columnCount(1)
    , m_columnWidth(200)
{
    setMinimumWidth(200);
    setMinimumHeight(300);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // Dark background like TR4W
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0, 0, 0));
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(pal);

    // Enable context menu
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void BandMapWidget::addSpot(const Spot& spot) {
    // Check if spot already exists (update it)
    for (int i = 0; i < m_spots.size(); ++i) {
        if (m_spots[i].callsign == spot.callsign) {
            m_spots[i] = spot;
            sortSpots();
            QWidget::update();
            return;
        }
    }

    // Add new spot
    m_spots.append(spot);
    sortSpots();
    QWidget::update();
}

void BandMapWidget::removeSpot(const QString& callsign) {
    for (int i = 0; i < m_spots.size(); ++i) {
        if (m_spots[i].callsign == callsign) {
            m_spots.removeAt(i);
            QWidget::update();
            return;
        }
    }
}

void BandMapWidget::clearSpots() {
    m_spots.clear();
    m_selectedIndex = -1;
    QWidget::update();
}

void BandMapWidget::setCurrentFrequency(freq_t freq) {
    m_currentFrequency = freq;
    QWidget::update();
}

void BandMapWidget::sortSpots() {
    std::sort(m_spots.begin(), m_spots.end(),
             [](const Spot& a, const Spot& b) {
                 return a.frequency < b.frequency;
             });
}

void BandMapWidget::calculateColumnLayout() {
    // Calculate column count based on widget width
    // Each column needs minimum 150 pixels (for "14025.0 M W1ABC" format)
    const int MIN_COLUMN_WIDTH = 150;
    int availableWidth = width();

    if (availableWidth < MIN_COLUMN_WIDTH) {
        m_columnCount = 1;
        m_columnWidth = availableWidth;
    } else {
        m_columnCount = availableWidth / MIN_COLUMN_WIDTH;
        if (m_columnCount < 1) m_columnCount = 1;
        m_columnWidth = availableWidth / m_columnCount;
    }
}

void BandMapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(QWidget::rect(), Qt::black);

    QFont font("Monospace", 9);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int lineHeight = rowHeight();

    // Draw each spot in multi-column layout
    for (int i = 0; i < m_spots.size(); ++i) {
        const Spot& spot = m_spots[i];

        // Calculate row and column for this spot
        int col = i / ((height() - 50) / lineHeight);  // Which column
        int row = i % ((height() - 50) / lineHeight);  // Which row in that column

        // Skip if this spot would be off-screen
        if (col >= m_columnCount) {
            continue;
        }

        int x = col * m_columnWidth;
        int y = row * lineHeight;

        // Determine text color
        QColor textColor;
        if (spot.isMultiplier) {
            textColor = QColor(100, 149, 237);  // Cornflower blue for multipliers
        } else if (spot.isWorked) {
            textColor = QColor(128, 128, 128);  // Gray for worked stations
        } else {
            textColor = Qt::white;  // White for unworked non-mults
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

        // Highlight selected spot
        if (i == m_selectedIndex) {
            painter.setPen(Qt::yellow);
            painter.drawRect(x + 2, y + 1, m_columnWidth - 4, lineHeight - 2);
        }
    }

    // Draw spot count at bottom
    painter.setPen(Qt::cyan);
    QString countStr = QString("%1 spots").arg(m_spots.size());
    int countY = height() - fm.height() - 5;
    painter.drawText(width() / 2 - fm.horizontalAdvance(countStr) / 2,
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
            QWidget::update();
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
    Q_UNUSED(event);
    calculateColumnLayout();
    QWidget::update();
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

    QAction* clearAction = menu.addAction("Clear All Spots");
    connect(clearAction, &QAction::triggered, this, &BandMapWidget::clearSpots);

    menu.exec(event->globalPos());
}

int BandMapWidget::findSpotAtPosition(const QPoint& pos) {
    int lineHeight = rowHeight();
    int rowsPerColumn = (height() - 50) / lineHeight;

    // Determine which column was clicked
    int col = pos.x() / m_columnWidth;
    if (col < 0 || col >= m_columnCount) {
        return -1;
    }

    // Determine which row in that column was clicked
    int row = pos.y() / lineHeight;
    if (row < 0 || row >= rowsPerColumn) {
        return -1;
    }

    // Calculate spot index from column and row
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
        qDebug() << "BandMapWidget::formatFrequency:" << freq << "Hz ->" << result << "MHz";
        debugCount++;
    }

    return result;
}

} // namespace TR4QT
