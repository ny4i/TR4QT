#include "BandMapWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLabel>
#include <algorithm>

namespace TR4QT {

BandMapWidget::BandMapWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentFrequency(0)
    , m_selectedIndex(-1)
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

void BandMapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(QWidget::rect(), Qt::black);

    QFont font("Monospace", 9);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int lineHeight = rowHeight();

    // Draw each spot
    for (int i = 0; i < m_spots.size(); ++i) {
        const Spot& spot = m_spots[i];
        int y = i * lineHeight;

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

        // Draw frequency (left side)
        QString freqStr = formatFrequency(spot.frequency);
        painter.drawText(5, y + fm.ascent() + 2, freqStr);

        // Draw "M" marker for multipliers
        if (spot.isMultiplier) {
            painter.setPen(Qt::red);
            painter.drawText(50, y + fm.ascent() + 2, "M");
            painter.setPen(textColor);
        }

        // Draw callsign (right side)
        painter.drawText(70, y + fm.ascent() + 2, spot.callsign);

        // Highlight selected spot
        if (i == m_selectedIndex) {
            painter.setPen(Qt::yellow);
            painter.drawRect(2, y + 1, width() - 4, lineHeight - 2);
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
        int spotIndex = findSpotAtPosition(event->pos().y());
        if (spotIndex >= 0 && spotIndex < m_spots.size()) {
            m_selectedIndex = spotIndex;
            emit qsyRequested(m_spots[spotIndex].frequency);
            QWidget::update();
        }
    }
}

void BandMapWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int spotIndex = findSpotAtPosition(event->pos().y());
        if (spotIndex >= 0 && spotIndex < m_spots.size()) {
            emit callsignSelected(m_spots[spotIndex].callsign);
        }
    }
}

void BandMapWidget::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    QWidget::update();
}

int BandMapWidget::findSpotAtPosition(int y) {
    int lineHeight = rowHeight();
    int spotIndex = y / lineHeight;
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
    // Convert Hz to kHz with 1 decimal place
    double freqKhz = freq / 1000.0;
    return QString::number(freqKhz, 'f', 1);
}

} // namespace TR4QT
