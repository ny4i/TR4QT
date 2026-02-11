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

#include "SMeterWidget.h"
#include "../../utils/ThemeManager.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>

namespace TR4QT {

// S-meter value mapping constants (from radio protocol specs)
namespace SMeterConstants {
    // Icom CI-V S-meter values (0-255)
    constexpr int ICOM_S0 = 0;
    constexpr int ICOM_S9 = 120;        // S9 threshold
    constexpr int ICOM_S9_PLUS_20 = 160; // +20dB over S9
    constexpr int ICOM_S9_PLUS_40 = 200; // +40dB over S9
    constexpr int ICOM_S9_PLUS_60 = 241; // +60dB over S9 (max)

    // K4 S-meter values (0-30)
    constexpr int K4_S0 = 0;
    constexpr int K4_S9 = 18;           // S9 threshold
    constexpr int K4_S9_PLUS_20 = 22;   // +20dB over S9
    constexpr int K4_S9_PLUS_40 = 26;   // +40dB over S9
    constexpr int K4_S9_PLUS_60 = 30;   // +60dB over S9 (max)

    // S-unit levels
    constexpr int NUM_S_UNITS = 9;      // S1 through S9
    constexpr int TOTAL_BARS = 12;      // S1-S9 + 20/+40/+60

    // dBm reference levels (ITU-R S-meter calibration)
    constexpr int DB_PER_S_UNIT = 6;    // Each S-unit = 6 dB (ITU-R standard)
    constexpr int DBM_S0 = -127;        // S0 threshold (noise floor)
    constexpr int DBM_S1 = DBM_S0 + DB_PER_S_UNIT;       // -121 dBm
    constexpr int DBM_S3 = DBM_S0 + (DB_PER_S_UNIT * 3); // -109 dBm
    constexpr int DBM_S5 = DBM_S0 + (DB_PER_S_UNIT * 5); // -97 dBm
    constexpr int DBM_S7 = DBM_S0 + (DB_PER_S_UNIT * 7); // -85 dBm
    constexpr int DBM_S9 = -73;         // S9 threshold (industry standard)
    constexpr int DBM_S9_PLUS_20 = -53; // S9+20dB
    constexpr int DBM_S9_PLUS_40 = -33; // S9+40dB
    constexpr int DBM_S9_PLUS_60 = -13; // S9+60dB (maximum)

    constexpr int DBM_FLOOR = DBM_S0;
    constexpr int DBM_CEILING = DBM_S9_PLUS_60;
    constexpr int DBM_RANGE = DBM_CEILING - DBM_FLOOR;  // 114 dB span

    // Layout constants
    constexpr int LABEL_FONT_SIZE = 8;
    constexpr int SCALE_FONT_SIZE = 7;
    constexpr int LABEL_BOX_H_PADDING = 6;  // Horizontal padding inside label box
    constexpr int LABEL_BOX_V_PADDING = 2;  // Vertical padding inside label box
    constexpr int BAR_LABEL_GAP = 3;        // Gap between label box and bar
    constexpr int BAR_SCALE_GAP = 2;        // Gap between bar and scale labels
    constexpr int OUTER_MARGIN = 2;         // Outer widget margin
    constexpr int TRACK_CORNER_RADIUS = 3;  // Rounded corners for track/bar
    constexpr int BAR_FILL_INSET = 1;       // Pixels between track border and gradient fill
    constexpr int PEAK_LINE_WIDTH = 2;

    // Widget size hints
    constexpr int PREFERRED_WIDTH = 300;
    constexpr int MIN_WIDTH = 150;
    constexpr int MIN_TRACK_WIDTH = 10;

    // Gradient color stops (green → yellow → orange → red)
    struct GradientStop {
        double position;
        QColor color;
    };

    const GradientStop RX_GRADIENT[] = {
        {0.00, QColor(0, 200, 0)},     // Green (weak signals)
        {0.30, QColor(100, 255, 0)},   // Yellow-green
        {0.45, QColor(220, 220, 0)},   // Yellow
        {0.60, QColor(255, 180, 0)},   // Orange
        {0.75, QColor(255, 100, 0)},   // Orange-red (S9 area)
        {1.00, QColor(255, 0, 0)},     // Red (S9+60)
    };
    constexpr int RX_GRADIENT_COUNT = sizeof(RX_GRADIENT) / sizeof(RX_GRADIENT[0]);

    const GradientStop TX_GRADIENT[] = {
        {0.00, QColor(0, 200, 0)},     // Green (low power)
        {0.35, QColor(100, 255, 0)},   // Yellow-green
        {0.50, QColor(220, 220, 0)},   // Yellow (mid power)
        {0.70, QColor(255, 180, 0)},   // Orange
        {0.85, QColor(255, 80, 0)},    // Orange-red
        {1.00, QColor(255, 0, 0)},     // Red (max power)
    };
    constexpr int TX_GRADIENT_COUNT = sizeof(TX_GRADIENT) / sizeof(TX_GRADIENT[0]);

    // TX power scale label positions (0%, 25%, 50%, 75%, 100%)
    constexpr double TX_LABEL_POSITIONS[] = {0.0, 0.25, 0.5, 0.75, 1.0};
    constexpr int TX_LABEL_COUNT = sizeof(TX_LABEL_POSITIONS) / sizeof(TX_LABEL_POSITIONS[0]);

    // RX scale labels: S-unit dBm values and display text
    struct ScaleLabel {
        int dbm;
        const char* text;
    };

    const ScaleLabel RX_SCALE_LABELS[] = {
        {DBM_S1,         "1"},
        {DBM_S3,         "3"},
        {DBM_S5,         "5"},
        {DBM_S7,         "7"},
        {DBM_S9,         "9"},
        {DBM_S9_PLUS_20, "+20"},
        {DBM_S9_PLUS_40, "+40"},
        {DBM_S9_PLUS_60, "+60"},
    };
    constexpr int RX_SCALE_LABEL_COUNT = sizeof(RX_SCALE_LABELS) / sizeof(RX_SCALE_LABELS[0]);
}

// Helper: convert dBm value to fill ratio (0.0-1.0)
static double dbmToRatio(int dbm) {
    if (dbm <= SMeterConstants::DBM_FLOOR) return 0.0;
    if (dbm >= SMeterConstants::DBM_CEILING) return 1.0;
    return static_cast<double>(dbm - SMeterConstants::DBM_FLOOR) / SMeterConstants::DBM_RANGE;
}

SMeterWidget::SMeterWidget(QWidget* parent)
    : QWidget(parent)
{
    // Derive all dimensions from font metrics
    QFont labelFont;
    labelFont.setPointSize(SMeterConstants::LABEL_FONT_SIZE);
    labelFont.setBold(true);
    QFontMetrics labelFm(labelFont);

    QFont scaleFont;
    scaleFont.setPointSize(SMeterConstants::SCALE_FONT_SIZE);
    QFontMetrics scaleFm(scaleFont);

    // Label box width: widest label ("Po") + padding
    int labelTextWidth = labelFm.horizontalAdvance("Po");
    m_labelBoxWidth = labelTextWidth + (SMeterConstants::LABEL_BOX_H_PADDING * 2);

    // Bar height: proportional to label font
    m_barHeight = labelFm.height() + (SMeterConstants::LABEL_BOX_V_PADDING * 2);

    // Scale labels height
    m_scaleHeight = scaleFm.height();

    // Total: margin + bar + gap + scale + margin
    m_totalHeight = SMeterConstants::OUTER_MARGIN + m_barHeight +
                    SMeterConstants::BAR_SCALE_GAP + m_scaleHeight +
                    SMeterConstants::OUTER_MARGIN;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(m_totalHeight);
    setMaximumHeight(m_totalHeight);

    // Decay timer for smooth animation
    m_decayTimer = new QTimer(this);
    m_decayTimer->setInterval(DECAY_INTERVAL_MS);
    connect(m_decayTimer, &QTimer::timeout, this, &SMeterWidget::decayValues);

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &SMeterWidget::applyTheme);
    applyTheme();
}

SMeterWidget::~SMeterWidget() = default;

void SMeterWidget::updateDisplayState(double newTargetRatio) {
    m_targetRatio = newTargetRatio;

    // Instant rise, slow decay
    if (m_targetRatio > m_displayRatio) {
        m_displayRatio = m_targetRatio;
    }

    // Update peak hold
    if (m_targetRatio > m_peakRatio) {
        m_peakRatio = m_targetRatio;
        m_peakHoldTicks = PEAK_HOLD_TICKS;
    }

    // Start decay timer if not already running
    if (!m_decayTimer->isActive()) {
        m_decayTimer->start();
    }

    update();
}

void SMeterWidget::setValue(int rawValue) {
    if (m_rawValue == rawValue) {
        return;
    }
    m_rawValue = rawValue;
    updateDisplayState(rawToRatio(rawValue));
}

void SMeterWidget::updateFromRadioState(const RadioState& state) {
    bool wasTxMode = m_isTxMode;
    m_isTxMode = state.isTransmitting;

    if (wasTxMode != m_isTxMode) {
        // Mode changed — reset display state for clean transition
        m_displayRatio = 0.0;
        m_peakRatio = 0.0;
        m_peakHoldTicks = 0;
    }

    if (m_isTxMode) {
        double newTarget = powerToRatio(state.powerOutput);
        if (qAbs(m_targetRatio - newTarget) > RATIO_EPSILON || wasTxMode != m_isTxMode) {
            updateDisplayState(newTarget);
        }
    } else {
        if (m_rawValue != state.signalStrength || wasTxMode != m_isTxMode) {
            m_rawValue = state.signalStrength;
            updateDisplayState(rawToRatio(state.signalStrength));
        }
    }
}

void SMeterWidget::clear() {
    m_rawValue = 0;
    m_isTxMode = false;
    m_targetRatio = 0.0;
    m_displayRatio = 0.0;
    m_peakRatio = 0.0;
    m_peakHoldTicks = 0;
    m_decayTimer->stop();
    update();
}

void SMeterWidget::setMaxPower(int maxWatts) {
    if (m_maxPowerWatts != maxWatts) {
        m_maxPowerWatts = maxWatts;
        if (m_isTxMode) {
            update();
        }
    }
}

double SMeterWidget::rawToRatio(int rawValue) const {
    // dBm format (negative or zero)
    if (rawValue <= 0) {
        return dbmToRatio(rawValue);
    }

    // Positive values: auto-detect Icom vs K4 based on value range
    if (rawValue <= SMeterConstants::K4_S9_PLUS_60) {
        return qBound(0.0, static_cast<double>(rawValue) / SMeterConstants::K4_S9_PLUS_60, 1.0);
    }
    return qBound(0.0, static_cast<double>(rawValue) / SMeterConstants::ICOM_S9_PLUS_60, 1.0);
}

double SMeterWidget::powerToRatio(int powerTenths) const {
    int maxPowerTenths = m_maxPowerWatts * 10;
    if (powerTenths <= 0 || maxPowerTenths <= 0) {
        return 0.0;
    }
    return qBound(0.0, static_cast<double>(powerTenths) / maxPowerTenths, 1.0);
}

int SMeterWidget::rawToSUnit(int rawValue) const {
    // Unchanged from discrete bar version — used by tests
    if (rawValue <= 0) {
        if (rawValue <= SMeterConstants::DBM_S0) {
            return 0;
        } else if (rawValue <= SMeterConstants::DBM_S9) {
            int sUnit = ((rawValue - SMeterConstants::DBM_S0) / SMeterConstants::DB_PER_S_UNIT) + 1;
            return qBound(1, sUnit, SMeterConstants::NUM_S_UNITS);
        } else if (rawValue < SMeterConstants::DBM_S9_PLUS_20) {
            return 9;
        } else if (rawValue < SMeterConstants::DBM_S9_PLUS_40) {
            return 10;
        } else if (rawValue < SMeterConstants::DBM_S9_PLUS_60) {
            return 11;
        } else {
            return 12;
        }
    }

    bool isK4 = (rawValue <= SMeterConstants::K4_S9_PLUS_60);

    if (isK4) {
        if (rawValue <= SMeterConstants::K4_S0) {
            return 0;
        } else if (rawValue <= SMeterConstants::K4_S9) {
            int sUnit = (rawValue * SMeterConstants::NUM_S_UNITS) / SMeterConstants::K4_S9;
            return qBound(1, sUnit, SMeterConstants::NUM_S_UNITS);
        } else if (rawValue <= SMeterConstants::K4_S9_PLUS_20) {
            return 10;
        } else if (rawValue <= SMeterConstants::K4_S9_PLUS_40) {
            return 11;
        } else {
            return 12;
        }
    } else {
        if (rawValue <= SMeterConstants::ICOM_S0) {
            return 0;
        } else if (rawValue <= SMeterConstants::ICOM_S9) {
            int sUnit = (rawValue * SMeterConstants::NUM_S_UNITS) / SMeterConstants::ICOM_S9;
            return qBound(1, sUnit, SMeterConstants::NUM_S_UNITS);
        } else if (rawValue <= SMeterConstants::ICOM_S9_PLUS_20) {
            return 10;
        } else if (rawValue <= SMeterConstants::ICOM_S9_PLUS_40) {
            return 11;
        } else {
            return 12;
        }
    }
}

void SMeterWidget::applyTheme() {
    update();
}

void SMeterWidget::decayValues() {
    bool needsUpdate = false;

    // Decay display ratio toward target
    if (m_displayRatio > m_targetRatio + RATIO_EPSILON) {
        m_displayRatio -= DECAY_RATE;
        if (m_displayRatio < m_targetRatio) {
            m_displayRatio = m_targetRatio;
        }
        needsUpdate = true;
    }

    // Peak hold countdown, then decay
    if (m_peakRatio > RATIO_EPSILON) {
        if (m_peakHoldTicks > 0) {
            --m_peakHoldTicks;
        } else {
            m_peakRatio -= PEAK_DECAY_RATE;
            if (m_peakRatio < 0.0) {
                m_peakRatio = 0.0;
            }
            needsUpdate = true;
        }
    }

    // Stop timer when everything has settled
    bool displaySettled = qAbs(m_displayRatio - m_targetRatio) < RATIO_EPSILON;
    bool peakSettled = m_peakRatio < RATIO_EPSILON && m_peakHoldTicks == 0;

    if (displaySettled && peakSettled) {
        m_decayTimer->stop();
    }

    if (needsUpdate) {
        update();
    }
}

QSize SMeterWidget::sizeHint() const {
    return QSize(SMeterConstants::PREFERRED_WIDTH, m_totalHeight);
}

QSize SMeterWidget::minimumSizeHint() const {
    return QSize(SMeterConstants::MIN_WIDTH, m_totalHeight);
}

void SMeterWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeManager& theme = ThemeManager::instance();
    QColor borderColor = theme.color(ColorRole::BorderColor);

    // --- Label box (left side: "S" for RX, "Po" for TX) ---
    QFont labelFont;
    labelFont.setPointSize(SMeterConstants::LABEL_FONT_SIZE);
    labelFont.setBold(true);
    QFontMetrics labelFm(labelFont);

    QString meterLabel = m_isTxMode ? "Po" : "S";
    int labelBoxX = SMeterConstants::OUTER_MARGIN;
    int labelBoxY = SMeterConstants::OUTER_MARGIN;
    QRect labelBoxRect(labelBoxX, labelBoxY, m_labelBoxWidth, m_barHeight);

    // Dark background for label box
    painter.setPen(borderColor);
    painter.setBrush(QColor(40, 40, 40));
    painter.drawRoundedRect(labelBoxRect, SMeterConstants::TRACK_CORNER_RADIUS,
                            SMeterConstants::TRACK_CORNER_RADIUS);

    // Label text (white on dark)
    painter.setFont(labelFont);
    painter.setPen(Qt::white);
    int labelTextX = labelBoxRect.x() + (labelBoxRect.width() - labelFm.horizontalAdvance(meterLabel)) / 2;
    int labelTextY = labelBoxRect.y() + (labelBoxRect.height() + labelFm.ascent() - labelFm.descent()) / 2;
    painter.drawText(labelTextX, labelTextY, meterLabel);

    // --- Track (the bar area, right of label box) ---
    int trackX = labelBoxRect.right() + SMeterConstants::BAR_LABEL_GAP;
    int trackY = labelBoxY;
    int trackWidth = width() - trackX - SMeterConstants::OUTER_MARGIN;
    QRect trackRect(trackX, trackY, trackWidth, m_barHeight);

    if (trackWidth < SMeterConstants::MIN_TRACK_WIDTH) {
        return;  // Widget too narrow
    }

    // Draw track background (dark)
    painter.setPen(borderColor);
    painter.setBrush(QColor(30, 30, 30));
    painter.drawRoundedRect(trackRect, SMeterConstants::TRACK_CORNER_RADIUS,
                            SMeterConstants::TRACK_CORNER_RADIUS);

    // --- Scale labels area ---
    QRect scaleRect(trackX, trackRect.bottom() + SMeterConstants::BAR_SCALE_GAP,
                    trackWidth, m_scaleHeight);

    drawMeterBar(painter, trackRect, scaleRect);
}

void SMeterWidget::drawMeterBar(QPainter& painter, const QRect& trackRect,
                                const QRect& scaleRect) const {
    ThemeManager& theme = ThemeManager::instance();
    QColor textColor = theme.color(ColorRole::PrimaryText);

    // Inner bar area (inset from track border)
    constexpr int INSET = SMeterConstants::BAR_FILL_INSET;
    QRect barArea(trackRect.x() + INSET, trackRect.y() + INSET,
                  trackRect.width() - (INSET * 2), trackRect.height() - (INSET * 2));

    // --- Gradient fill up to m_displayRatio ---
    if (m_displayRatio > RATIO_EPSILON) {
        int fillWidth = static_cast<int>(barArea.width() * m_displayRatio);
        if (fillWidth > 0) {
            QRect fillRect(barArea.x(), barArea.y(), fillWidth, barArea.height());

            // Build gradient from stops
            QLinearGradient gradient(barArea.x(), 0, barArea.right(), 0);
            const auto& stops = m_isTxMode ? SMeterConstants::TX_GRADIENT : SMeterConstants::RX_GRADIENT;
            int stopCount = m_isTxMode ? SMeterConstants::TX_GRADIENT_COUNT : SMeterConstants::RX_GRADIENT_COUNT;

            for (int i = 0; i < stopCount; ++i) {
                gradient.setColorAt(stops[i].position, stops[i].color);
            }

            painter.save();
            painter.setClipRect(fillRect);
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            int cornerRadius = SMeterConstants::TRACK_CORNER_RADIUS - INSET;
            painter.drawRoundedRect(barArea, cornerRadius, cornerRadius);
            painter.restore();
        }
    }

    // --- Peak indicator (thin white vertical line) ---
    if (m_peakRatio > m_displayRatio + PEAK_VISIBILITY_GAP) {
        int peakX = barArea.x() + static_cast<int>(barArea.width() * m_peakRatio);
        peakX = qBound(barArea.x(), peakX, barArea.right());

        painter.setPen(QPen(Qt::white, SMeterConstants::PEAK_LINE_WIDTH));
        painter.drawLine(peakX, barArea.y() + 1, peakX, barArea.bottom() - 1);
    }

    // --- Scale labels and tick marks below the bar ---
    QFont scaleFont;
    scaleFont.setPointSize(SMeterConstants::SCALE_FONT_SIZE);
    painter.setFont(scaleFont);
    QFontMetrics scaleFm(scaleFont);

    if (m_isTxMode) {
        for (int i = 0; i < SMeterConstants::TX_LABEL_COUNT; ++i) {
            double pos = SMeterConstants::TX_LABEL_POSITIONS[i];
            int watts = static_cast<int>(pos * m_maxPowerWatts);
            QString label = QString::number(watts);
            int x = barArea.x() + static_cast<int>(barArea.width() * pos);

            // Tick mark on track bottom edge
            painter.setPen(QPen(textColor, 1));
            painter.drawLine(x, trackRect.bottom() - 2, x, trackRect.bottom());

            // Label below
            int textWidth = scaleFm.horizontalAdvance(label);
            int textX = x - textWidth / 2;
            textX = qBound(scaleRect.x(), textX, scaleRect.right() - textWidth);
            painter.drawText(textX, scaleRect.y() + scaleFm.ascent(), label);
        }
    } else {
        for (int i = 0; i < SMeterConstants::RX_SCALE_LABEL_COUNT; ++i) {
            double pos = dbmToRatio(SMeterConstants::RX_SCALE_LABELS[i].dbm);
            int x = barArea.x() + static_cast<int>(barArea.width() * pos);

            // Tick mark
            painter.setPen(QPen(textColor, 1));
            painter.drawLine(x, trackRect.bottom() - 2, x, trackRect.bottom());

            // Label
            QString label = QString::fromLatin1(SMeterConstants::RX_SCALE_LABELS[i].text);
            int textWidth = scaleFm.horizontalAdvance(label);
            int textX = x - textWidth / 2;
            textX = qBound(scaleRect.x(), textX, scaleRect.right() - textWidth);
            painter.drawText(textX, scaleRect.y() + scaleFm.ascent(), label);
        }
    }
}

} // namespace TR4QT
