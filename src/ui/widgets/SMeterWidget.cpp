#include "SMeterWidget.h"
#include "../../utils/ThemeManager.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

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
    constexpr int NUM_OVER_UNITS = 3;   // +20, +40, +60
    constexpr int TOTAL_BARS = NUM_S_UNITS + NUM_OVER_UNITS;  // 12 total bars
}

SMeterWidget::SMeterWidget(QWidget* parent)
    : QWidget(parent)
    , m_isTxMode(false)
    , m_rawValue(0)
    , m_currentSUnit(0)
    , m_powerWatts(0)
    , m_currentPowerLevel(0)
{
    // Derive all dimensions from font metrics (no magic numbers)
    QFont labelFont;
    labelFont.setPointSize(8);
    QFontMetrics fm(labelFont);

    // Bar dimensions derived from font size
    m_labelHeight = fm.height();
    m_barHeight = m_labelHeight * 2;       // Bars are 2x label height
    m_barSpacing = fm.height() / 4;        // Spacing is 1/4 label height

    // Bar width must accommodate widest label ("+20", "+40", "+60")
    // Add small padding so labels don't touch
    int widestLabelWidth = fm.horizontalAdvance("+60");
    m_barWidth = widestLabelWidth + (fm.height() / 8);  // Add 1/8 label height as padding

    // Total height = labels + bars + margins
    m_totalHeight = m_labelHeight + m_barSpacing + m_barHeight + m_barSpacing;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(m_totalHeight);
    setMaximumHeight(m_totalHeight);

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &SMeterWidget::applyTheme);
    applyTheme();
}

void SMeterWidget::setValue(int rawValue) {
    if (m_rawValue != rawValue) {
        m_rawValue = rawValue;
        m_currentSUnit = rawToSUnit(rawValue);
        update();  // Trigger repaint
    }
}

void SMeterWidget::updateFromRadioState(const RadioState& state) {
    bool wasTxMode = m_isTxMode;
    m_isTxMode = state.isTransmitting;
    bool needsUpdate = false;

    if (m_isTxMode) {
        // TX mode: display forward power
        int powerTenths = state.powerOutput;
        int newPowerWatts = powerTenths / 10;
        int newPowerLevel = powerToLevel(powerTenths);

        if (m_powerWatts != newPowerWatts || m_currentPowerLevel != newPowerLevel) {
            m_powerWatts = newPowerWatts;
            m_currentPowerLevel = newPowerLevel;
            needsUpdate = true;
        }
    } else {
        // RX mode: display S-meter
        if (m_rawValue != state.signalStrength) {
            m_rawValue = state.signalStrength;
            m_currentSUnit = rawToSUnit(state.signalStrength);
            needsUpdate = true;
        }
    }

    // Repaint if mode changed or values updated
    if (wasTxMode != m_isTxMode || needsUpdate) {
        update();
    }
}

void SMeterWidget::clear() {
    setValue(0);
    m_isTxMode = false;
    m_powerWatts = 0;
    m_currentPowerLevel = 0;
}

void SMeterWidget::setMaxPower(int maxWatts) {
    if (m_maxPowerWatts != maxWatts) {
        m_maxPowerWatts = maxWatts;
        // Recalculate power level with new scale
        if (m_isTxMode) {
            int powerTenths = m_powerWatts * 10;
            m_currentPowerLevel = powerToLevel(powerTenths);
            update();
        }
    }
}

int SMeterWidget::rawToSUnit(int rawValue) const {
    // Detect format: dBm (negative or zero) vs raw Icom/K4 (positive)
    // Note: 0 dBm is an extremely strong signal (S9+60+), so include 0 in dBm handling
    if (rawValue <= 0) {
        // dBm format (K4 with SMH1; enabled, or calculated values)
        // S0 = -127 dBm, S9 = -73 dBm (6 dB per S-unit)
        // Above S9: -53 dBm = +20, -33 dBm = +40, -13 dBm = +60
        constexpr int S0_DBM = -127;
        constexpr int S9_DBM = -73;
        constexpr int S9_PLUS_20_DBM = -53;
        constexpr int S9_PLUS_40_DBM = -33;
        constexpr int S9_PLUS_60_DBM = -13;

        if (rawValue <= S0_DBM) {
            return 0;  // No signal
        } else if (rawValue <= S9_DBM) {
            // S1-S9: each S-unit is 6 dB
            int sUnit = ((rawValue - S0_DBM) / 6) + 1;
            return qBound(1, sUnit, SMeterConstants::NUM_S_UNITS);
        } else if (rawValue < S9_PLUS_20_DBM) {
            // Above S9 but below S9+20: still display as S9
            return 9;
        } else if (rawValue < S9_PLUS_40_DBM) {
            return 10;  // S9+20 through S9+39 dB
        } else if (rawValue < S9_PLUS_60_DBM) {
            return 11;  // S9+40 through S9+59 dB
        } else {
            return 12;  // S9+60 and above
        }
    }

    // Positive values: auto-detect Icom vs K4 based on value range
    bool isK4 = (rawValue <= SMeterConstants::K4_S9_PLUS_60);

    if (isK4) {
        // K4 mapping (0-30 segments)
        if (rawValue <= SMeterConstants::K4_S0) {
            return 0;  // No signal
        } else if (rawValue <= SMeterConstants::K4_S9) {
            // S1-S9: linear mapping
            int sUnit = (rawValue * SMeterConstants::NUM_S_UNITS) / SMeterConstants::K4_S9;
            return qBound(1, sUnit, SMeterConstants::NUM_S_UNITS);
        } else if (rawValue <= SMeterConstants::K4_S9_PLUS_20) {
            return 10;  // +20dB
        } else if (rawValue <= SMeterConstants::K4_S9_PLUS_40) {
            return 11;  // +40dB
        } else {
            return 12;  // +60dB
        }
    } else {
        // Icom mapping (0-255)
        if (rawValue <= SMeterConstants::ICOM_S0) {
            return 0;  // No signal
        } else if (rawValue <= SMeterConstants::ICOM_S9) {
            // S1-S9: linear mapping
            int sUnit = (rawValue * SMeterConstants::NUM_S_UNITS) / SMeterConstants::ICOM_S9;
            return qBound(1, sUnit, SMeterConstants::NUM_S_UNITS);
        } else if (rawValue <= SMeterConstants::ICOM_S9_PLUS_20) {
            return 10;  // +20dB
        } else if (rawValue <= SMeterConstants::ICOM_S9_PLUS_40) {
            return 11;  // +40dB
        } else {
            return 12;  // +60dB
        }
    }
}

int SMeterWidget::powerToLevel(int powerTenths) const {
    // Convert power (tenths of watts) to bar level (0-12)
    // Scale: 0-maxPowerWatts → 0-12 bars
    // Default: 150W for radios, 1500W for KPA1500 amplifier
    int maxPowerTenths = m_maxPowerWatts * 10;
    constexpr int BARS = SMeterConstants::TOTAL_BARS;

    if (powerTenths <= 0) {
        return 0;
    } else if (powerTenths >= maxPowerTenths) {
        return BARS;
    } else {
        // Linear mapping: 0-maxPowerWatts → 0-12 bars
        return (powerTenths * BARS) / maxPowerTenths;
    }
}

QString SMeterWidget::sUnitLabel(int sUnit) const {
    if (sUnit == 0) {
        return "S0";
    } else if (sUnit <= 9) {
        return QString("S%1").arg(sUnit);
    } else if (sUnit == 10) {
        return "+20";
    } else if (sUnit == 11) {
        return "+40";
    } else if (sUnit == 12) {
        return "+60";
    }
    return "";
}

QString SMeterWidget::powerLabel(int level) const {
    // Power labels scale with max power
    // Level 0 = 0W, 12 = maxPowerWatts (evenly distributed)
    int watts = (level * m_maxPowerWatts) / SMeterConstants::TOTAL_BARS;
    return QString::number(watts);
}

void SMeterWidget::applyTheme() {
    update();  // Repaint with new theme colors
}

QSize SMeterWidget::sizeHint() const {
    // Width accommodates all bars + spacing
    int totalWidth = (SMeterConstants::TOTAL_BARS * m_barWidth) +
                     ((SMeterConstants::TOTAL_BARS - 1) * m_barSpacing) +
                     (m_barSpacing * 2);  // Left/right margins
    return QSize(totalWidth, m_totalHeight);
}

QSize SMeterWidget::minimumSizeHint() const {
    return sizeHint();  // Fixed size based on font metrics
}

void SMeterWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeManager& theme = ThemeManager::instance();

    // Get colors from theme
    QColor textColor = theme.color(ColorRole::PrimaryText);
    QColor borderColor = theme.color(ColorRole::BorderColor);

    // TX mode: orange bars for power, RX mode: green bars for S-meter
    QColor barActiveColor = m_isTxMode ? QColor(255, 140, 0) : theme.color(ColorRole::ConnectedStatus);  // Orange TX, Green RX
    QColor barInactiveColor = theme.color(ColorRole::SecondaryText);  // Gray for inactive

    // Setup fonts
    QFont labelFont;
    labelFont.setPointSize(8);
    painter.setFont(labelFont);
    QFontMetrics fm(labelFont);

    // Calculate layout dynamically based on available width
    const int horizontalMargin = fm.height() / 2;  // Margin on left/right
    const int availableWidth = width() - (2 * horizontalMargin);
    const int totalSpacing = (SMeterConstants::TOTAL_BARS - 1) * m_barSpacing;
    const int barWidth = (availableWidth - totalSpacing) / SMeterConstants::TOTAL_BARS;

    // Ensure minimum bar width for readability
    const int minBarWidth = fm.horizontalAdvance("W");
    if (barWidth < minBarWidth) {
        return;  // Widget too narrow to display properly
    }

    int startX = horizontalMargin;
    int labelY = m_labelHeight;
    int barY = labelY + m_barSpacing;

    // Draw each bar (RX: S1-S9/+20/+40/+60, TX: 0-150W in 12 bars)
    for (int i = 0; i < SMeterConstants::TOTAL_BARS; ++i) {
        int level = i + 1;  // Level 1-12
        int x = startX + (i * (barWidth + m_barSpacing));

        // Draw labels
        if (m_isTxMode) {
            // TX mode: show power labels (0W, 25W, 50W, 75W, 100W, 125W, 150W)
            // Show labels at bars: 0, 2, 4, 6, 8, 10, 12
            bool showLabel = (i == 0) || (i == 2) || (i == 4) || (i == 6) ||
                             (i == 8) || (i == 10) || (i == 11);
            if (showLabel) {
                QString label = powerLabel(level);
                int textWidth = fm.horizontalAdvance(label);
                painter.setPen(textColor);
                painter.drawText(x + (barWidth - textWidth) / 2, labelY, label);
            }

            // Draw bar (filled if power >= this level)
            QRect barRect(x, barY, barWidth, m_barHeight);
            bool isActive = (m_currentPowerLevel >= level);

            QColor barColor = isActive ? barActiveColor : barInactiveColor;
            painter.setPen(borderColor);
            painter.setBrush(barColor);
            painter.drawRect(barRect);
        } else {
            // RX mode: show S-meter labels (S1, S3, S5, S7, S9, +20, +40, +60)
            bool showLabel = (i == 0) || (i == 2) || (i == 4) || (i == 6) ||
                             (i == 8) || (i == 9) || (i == 10) || (i == 11);
            if (showLabel) {
                QString label = sUnitLabel(level);
                int textWidth = fm.horizontalAdvance(label);
                painter.setPen(textColor);
                painter.drawText(x + (barWidth - textWidth) / 2, labelY, label);
            }

            // Draw bar (filled if signal >= this S-unit)
            QRect barRect(x, barY, barWidth, m_barHeight);
            bool isActive = (m_currentSUnit >= level);

            QColor barColor = isActive ? barActiveColor : barInactiveColor;
            painter.setPen(borderColor);
            painter.setBrush(barColor);
            painter.drawRect(barRect);
        }
    }
}

} // namespace TR4QT
