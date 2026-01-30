/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "RateDisplayWidget.h"
#include "../../utils/ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

namespace TR4QT {

// ============================================================================
// RateBarGraph
// ============================================================================

RateBarGraph::RateBarGraph(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(minimumSizeHint());
}

void RateBarGraph::setValues(const QVector<int>& values, const QStringList& labels) {
    m_values = values;
    m_labels = labels;
    update();
}

void RateBarGraph::paintEvent(QPaintEvent* /*event*/) {
    if (m_values.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int barCount = qMin(m_values.size(), 4);
    const int margin = 4;
    const int labelHeight = 12;
    const int barWidth = (width() - margin * 2) / barCount - 4;
    const int barMaxHeight = height() - margin * 2 - labelHeight;

    // Determine max value for scaling
    int maxVal = m_maxValue;
    if (maxVal <= 0) {
        for (int v : m_values) {
            maxVal = qMax(maxVal, v);
        }
    }
    if (maxVal <= 0) maxVal = 100;

    // Colors
    QColor barColor = ThemeManager::instance().color(ColorRole::ConnectedStatus);
    QColor textColor = ThemeManager::instance().color(ColorRole::PrimaryText);
    QColor bgColor = ThemeManager::instance().color(ColorRole::WindowBackground);

    // Background
    painter.fillRect(rect(), bgColor);

    // Draw bars
    QFont labelFont = font();
    labelFont.setPointSize(8);
    painter.setFont(labelFont);

    for (int i = 0; i < barCount; ++i) {
        int x = margin + i * (barWidth + 4);
        int barHeight = static_cast<int>(static_cast<double>(m_values[i]) / maxVal * barMaxHeight);
        barHeight = qMax(barHeight, 2);  // Minimum visible height

        // Bar
        QRect barRect(x, margin + barMaxHeight - barHeight, barWidth, barHeight);
        painter.fillRect(barRect, barColor);

        // Label
        if (i < m_labels.size()) {
            painter.setPen(textColor);
            QRect labelRect(x, height() - labelHeight, barWidth, labelHeight);
            painter.drawText(labelRect, Qt::AlignCenter, m_labels[i]);
        }
    }
}

// ============================================================================
// RateLineGraph
// ============================================================================

RateLineGraph::RateLineGraph(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(minimumSizeHint());
}

void RateLineGraph::setHistory(const QVector<HourlyRatePoint>& history) {
    m_history = history;
    update();
}

void RateLineGraph::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int margin = 4;
    const int graphWidth = width() - margin * 2;
    const int graphHeight = height() - margin * 2;

    // Colors
    QColor lineColor = ThemeManager::instance().color(ColorRole::ConnectedStatus);
    QColor goalColor = ThemeManager::instance().color(ColorRole::MultiplierText);
    QColor textColor = ThemeManager::instance().color(ColorRole::SecondaryText);
    QColor bgColor = ThemeManager::instance().color(ColorRole::WindowBackground);
    QColor gridColor = textColor;
    gridColor.setAlpha(50);

    // Background
    painter.fillRect(rect(), bgColor);

    if (m_history.isEmpty()) {
        painter.setPen(textColor);
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    // Find max value for scaling
    int maxVal = m_goalRate;
    for (const auto& point : m_history) {
        maxVal = qMax(maxVal, point.rate);
    }
    if (maxVal <= 0) maxVal = 100;
    maxVal = static_cast<int>(maxVal * 1.1);  // Add 10% headroom

    // Draw grid lines
    painter.setPen(QPen(gridColor, 1, Qt::DotLine));
    for (int i = 1; i <= 3; ++i) {
        int y = margin + graphHeight - (graphHeight * i / 4);
        painter.drawLine(margin, y, width() - margin, y);
    }

    // Draw goal line if set
    if (m_goalRate > 0) {
        int goalY = margin + graphHeight - static_cast<int>(static_cast<double>(m_goalRate) / maxVal * graphHeight);
        painter.setPen(QPen(goalColor, 2, Qt::DashLine));
        painter.drawLine(margin, goalY, width() - margin, goalY);
    }

    // Draw rate line
    if (m_history.size() >= 2) {
        painter.setPen(QPen(lineColor, 2));

        QPainterPath path;
        bool first = true;

        for (int i = 0; i < m_history.size(); ++i) {
            double x = margin + static_cast<double>(i) / (m_history.size() - 1) * graphWidth;
            double y = margin + graphHeight - static_cast<double>(m_history[i].rate) / maxVal * graphHeight;

            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }

        painter.drawPath(path);

        // Draw points
        painter.setBrush(lineColor);
        for (int i = 0; i < m_history.size(); ++i) {
            double x = margin + static_cast<double>(i) / (m_history.size() - 1) * graphWidth;
            double y = margin + graphHeight - static_cast<double>(m_history[i].rate) / maxVal * graphHeight;
            painter.drawEllipse(QPointF(x, y), 3, 3);
        }
    }
}

// ============================================================================
// RateDisplayWidget
// ============================================================================

RateDisplayWidget::RateDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void RateDisplayWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(4);

    // Top row: Big numbers with titles
    QHBoxLayout* numbersLayout = new QHBoxLayout();
    numbersLayout->setSpacing(16);

    // Helper to create a rate display column
    auto createRateColumn = [this](const QString& title, QLabel*& titleLabel, QLabel*& valueLabel) {
        QVBoxLayout* col = new QVBoxLayout();
        col->setSpacing(0);

        titleLabel = new QLabel(title, this);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(9);
        titleLabel->setFont(titleFont);

        valueLabel = new QLabel("0", this);
        valueLabel->setAlignment(Qt::AlignCenter);
        QFont valueFont = valueLabel->font();
        valueFont.setPointSize(18);
        valueFont.setBold(true);
        valueLabel->setFont(valueFont);

        col->addWidget(titleLabel);
        col->addWidget(valueLabel);

        return col;
    };

    numbersLayout->addLayout(createRateColumn("10-QSO", m_rate10Label, m_rate10Value));
    numbersLayout->addLayout(createRateColumn("60-min", m_rate60Label, m_rate60Value));
    numbersLayout->addLayout(createRateColumn("This Hr", m_rateThisHrLabel, m_rateThisHrValue));
    numbersLayout->addLayout(createRateColumn("Average", m_rateAvgLabel, m_rateAvgValue));
    numbersLayout->addStretch();

    mainLayout->addLayout(numbersLayout);

    // Bottom row: Graphs
    QHBoxLayout* graphsLayout = new QHBoxLayout();
    graphsLayout->setSpacing(8);

    m_barGraph = new RateBarGraph(this);
    m_barGraph->setFixedSize(120, 60);
    graphsLayout->addWidget(m_barGraph);

    m_lineGraph = new RateLineGraph(this);
    m_lineGraph->setMinimumWidth(150);
    graphsLayout->addWidget(m_lineGraph, 1);

    mainLayout->addLayout(graphsLayout);
}

void RateDisplayWidget::updateRates(const RateSnapshot& snapshot) {
    m_lastSnapshot = snapshot;

    // Update big numbers
    m_rate10Value->setText(QString::number(snapshot.qso10Rate));
    m_rate60Value->setText(QString::number(snapshot.last60MinRate));
    m_rateThisHrValue->setText(QString::number(snapshot.thisHourCount));
    m_rateAvgValue->setText(QString::number(snapshot.averageRate));

    // Update bar graph
    QVector<int> barValues = {
        snapshot.qso10Rate,
        snapshot.qso100Rate,
        snapshot.last60MinRate,
        snapshot.thisHourRate
    };
    QStringList barLabels = {"10", "100", "60m", "Hr"};
    m_barGraph->setValues(barValues, barLabels);
}

void RateDisplayWidget::updateHistory(const QVector<HourlyRatePoint>& history) {
    m_lineGraph->setHistory(history);
}

void RateDisplayWidget::setGoalRate(int rate) {
    m_lineGraph->setGoalRate(rate);
}

} // namespace TR4QT
