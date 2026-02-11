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

#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H

#include <QWidget>
#include <QTimer>
#include "../../radio/RadioInterface.h"

namespace TR4QT {

/**
 * S-Meter Widget - Smooth gradient bar with peak hold animation
 *
 * Displays signal strength (RX) or forward power (TX) as a smooth
 * horizontal gradient bar with peak hold indicator, inspired by QK4.
 *
 * Supports dBm, Icom CI-V (0-255), and K4 (0-30) input formats.
 * Raw values are mapped to a continuous fill ratio (0.0-1.0) for
 * smooth rendering. Peak hold marker decays slower than the main bar.
 *
 * Layout:
 *   [Label] [============ gradient bar ============]
 *            1   3   5   7   9  +20  +40  +60
 */
class SMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SMeterWidget(QWidget* parent = nullptr);
    ~SMeterWidget() override;

    void setValue(int rawValue);
    void updateFromRadioState(const RadioState& state);
    void clear();

    int value() const { return m_rawValue; }
    void setMaxPower(int maxWatts);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    /**
     * Convert raw S-meter value to S-unit level (0-12)
     * 0 = no signal, 1-9 = S1-S9, 10 = +20, 11 = +40, 12 = +60
     * (Public for testing — unchanged from discrete bar version)
     */
    int rawToSUnit(int rawValue) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double rawToRatio(int rawValue) const;
    double powerToRatio(int powerTenths) const;

    /**
     * Update display/peak state for a new target ratio.
     * Handles instant rise, peak hold update, and timer start.
     */
    void updateDisplayState(double newTargetRatio);

    void drawMeterBar(QPainter& painter, const QRect& barRect,
                      const QRect& scaleRect) const;

    void applyTheme();

private slots:
    void decayValues();

private:
    bool m_isTxMode{false};
    int m_rawValue{0};
    int m_maxPowerWatts{150};

    // Smooth display values
    double m_targetRatio{0.0};     // Where the bar should be (set instantly on new value)
    double m_displayRatio{0.0};    // Current rendered position (decays toward target)
    double m_peakRatio{0.0};       // Peak hold marker position
    int m_peakHoldTicks{0};        // Countdown before peak starts decaying
    QTimer* m_decayTimer{nullptr};

    // Animation constants
    static constexpr int DECAY_INTERVAL_MS = 50;
    static constexpr double DECAY_RATE = 0.10;         // Main bar decay per tick
    static constexpr double PEAK_DECAY_RATE = 0.05;    // Peak decays slower
    static constexpr int PEAK_HOLD_TICKS = 10;         // 500ms hold at 50ms interval
    static constexpr double RATIO_EPSILON = 0.001;     // Minimum detectable ratio change
    static constexpr double PEAK_VISIBILITY_GAP = 0.01; // Peak must lead display by this much to draw

    // Layout dimensions (derived from font metrics in constructor)
    int m_labelBoxWidth;   // Width of the "S" / "Po" label box on the left
    int m_barHeight;       // Height of the gradient bar
    int m_scaleHeight;     // Height of scale labels below bar
    int m_totalHeight;     // Total widget height
};

} // namespace TR4QT

#endif // SMETERWIDGET_H
