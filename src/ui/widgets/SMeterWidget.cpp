#include "SMeterWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <QFontMetrics>

namespace TR4QT {

SMeterWidget::SMeterWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentValue(-73)  // Default to S9
{
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SMeterWidget::setValue(int dbm) {
    if (m_currentValue != dbm) {
        m_currentValue = dbm;
        update();  // Trigger repaint
    }
}

QString SMeterWidget::dbmToSMeter(int dbm) {
    // S0 = -127 dBm, each S-unit = 6 dB
    // S9 = -73 dBm
    // Above S9: +10dB, +20dB, etc.

    if (dbm <= -127) {
        return "S0";
    } else if (dbm >= -73) {
        // Above S9
        int over = dbm + 73;
        if (over == 0) {
            return "S9";
        } else {
            return QString("S9+%1").arg(over);
        }
    } else {
        // S1 to S8
        int sValue = ((dbm + 127) / 6) + 1;
        sValue = qBound(1, sValue, 9);
        return QString("S%1").arg(sValue);
    }
}

int SMeterWidget::dbmToPercentage(int dbm) const {
    // Map dBm to 0-100% for bar drawing
    // S0 = -127 dBm = 0%
    // S9 = -73 dBm = 60%
    // S9+60 = -13 dBm = 100%

    if (dbm <= -127) {
        return 0;
    } else if (dbm >= -13) {
        return 100;
    } else {
        // Linear mapping from -127 to -13 dBm to 0-100%
        return ((dbm + 127) * 100) / 114;
    }
}

QSize SMeterWidget::sizeHint() const {
    return QSize(300, 60);
}

QSize SMeterWidget::minimumSizeHint() const {
    return QSize(200, 60);
}

void SMeterWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawScale(painter);
    drawBar(painter);
    drawValue(painter);
}

void SMeterWidget::drawBackground(QPainter& painter) {
    // Draw widget background
    painter.fillRect(rect(), QColor(40, 40, 40));

    // Draw meter background (recessed area)
    int margin = 5;
    QRect meterRect = rect().adjusted(margin, margin + 20, -margin, -margin);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(20, 20, 20));
    painter.drawRoundedRect(meterRect, 3, 3);

    // Inner shadow effect
    painter.setPen(QPen(QColor(0, 0, 0, 100), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(meterRect.adjusted(1, 1, -1, -1), 2, 2);
}

void SMeterWidget::drawScale(QPainter& painter) {
    int margin = 5;
    int scaleTop = margin;
    int scaleHeight = 15;

    QRect scaleRect = rect().adjusted(margin + 10, scaleTop, -margin - 10, -(rect().height() - scaleTop - scaleHeight));

    painter.setPen(QColor(200, 200, 200));
    QFont scaleFont("Sans", 7);
    painter.setFont(scaleFont);

    // S-meter scale: S1, S3, S5, S7, S9, +20, +40, +60
    QStringList labels = {"1", "3", "5", "7", "9", "+20", "+40", "+60"};
    QList<int> positions = {10, 23, 36, 49, 60, 73, 86, 100};  // Percentage positions

    for (int i = 0; i < labels.size(); ++i) {
        int x = scaleRect.left() + (scaleRect.width() * positions[i] / 100);

        // Draw tick mark
        painter.drawLine(x, scaleRect.bottom(), x, scaleRect.bottom() + 3);

        // Draw label
        QFontMetrics fm(scaleFont);
        int textWidth = fm.horizontalAdvance(labels[i]);
        painter.drawText(x - textWidth / 2, scaleRect.top() + fm.height(), labels[i]);
    }

    // Draw "S" label at the start
    painter.drawText(scaleRect.left() - 8, scaleRect.top() + painter.fontMetrics().height(), "S");
}

void SMeterWidget::drawBar(QPainter& painter) {
    int margin = 5;
    QRect meterRect = rect().adjusted(margin + 10, margin + 25, -margin - 10, -margin - 5);

    int percentage = dbmToPercentage(m_currentValue);
    int barWidth = (meterRect.width() * percentage) / 100;

    if (barWidth > 0) {
        QRect barRect(meterRect.left(), meterRect.top(), barWidth, meterRect.height());

        // Create gradient based on signal strength
        QLinearGradient gradient(barRect.topLeft(), barRect.topRight());

        // Green zone (0-60%: S0-S9)
        gradient.setColorAt(0.0, QColor(0, 200, 0));
        gradient.setColorAt(0.50, QColor(0, 255, 0));

        // Yellow zone (60-75%: S9 to S9+20)
        if (percentage > 60) {
            gradient.setColorAt(0.60, QColor(255, 255, 0));
        }

        // Red zone (75-100%: S9+20 and above)
        if (percentage > 75) {
            gradient.setColorAt(0.75, QColor(255, 200, 0));
            gradient.setColorAt(1.0, QColor(255, 0, 0));
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRoundedRect(barRect, 2, 2);

        // Add highlight on top edge
        QLinearGradient highlightGradient(barRect.topLeft(), barRect.bottomLeft());
        highlightGradient.setColorAt(0.0, QColor(255, 255, 255, 80));
        highlightGradient.setColorAt(0.3, QColor(255, 255, 255, 0));
        painter.setBrush(highlightGradient);
        painter.drawRoundedRect(barRect.adjusted(0, 0, 0, -barRect.height() / 2), 2, 2);
    }
}

void SMeterWidget::drawValue(QPainter& painter) {
    // Draw current value text
    QString valueText = dbmToSMeter(m_currentValue);

    painter.setPen(QColor(255, 255, 255));
    QFont valueFont("Sans", 10, QFont::Bold);
    painter.setFont(valueFont);

    QFontMetrics fm(valueFont);
    int textWidth = fm.horizontalAdvance(valueText);
    int textHeight = fm.height();

    int x = rect().right() - textWidth - 10;
    int y = rect().bottom() - 8;

    // Draw text with shadow for better visibility
    painter.setPen(QColor(0, 0, 0));
    painter.drawText(x + 1, y + 1, valueText);

    painter.setPen(QColor(255, 255, 0));
    painter.drawText(x, y, valueText);
}

} // namespace TR4QT
