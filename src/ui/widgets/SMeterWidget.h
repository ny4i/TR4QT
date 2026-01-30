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
#include "../../radio/RadioInterface.h"

namespace TR4QT {

/**
 * S-Meter Widget - Displays received signal strength with discrete bars
 *
 * Shows signal levels as discrete bars (S1-S9, +20, +40, +60):
 * - 9 bars for S1 through S9 (baseline signal strength)
 * - 3 bars for +20, +40, +60 dB over S9 (strong signals)
 *
 * Supports both Icom (CI-V S-meter values 0-255) and K4 (direct S-meter 0-30).
 * Values are mapped to S-units:
 * - Icom: 0-120 → S0-S9 (13.3 per S-unit), 121-241 → +20/+40/+60 (40 per 20dB)
 * - K4:   0-18 → S0-S9 (2 per S-unit), 19-30 → +20/+40/+60 (4 per 20dB)
 *
 * Design principles:
 * - No magic numbers: all dimensions derived from font metrics
 * - No magic colors: all colors from ThemeManager
 * - Discrete bar visualization (not continuous gradient)
 */
class SMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SMeterWidget(QWidget* parent = nullptr);
    ~SMeterWidget() override = default;

    /**
     * Update S-meter with new signal strength value
     * @param rawValue Raw S-meter value from radio (0-255 for Icom, 0-30 for K4)
     */
    void setValue(int rawValue);

    /**
     * Update from radio state (auto-switches between RX and TX modes)
     * @param state Current radio state (checks isTransmitting, powerOutput, signalStrength)
     */
    void updateFromRadioState(const RadioState& state);

    /**
     * Clear display (no signal)
     */
    void clear();

    /**
     * Get current raw value
     */
    int value() const { return m_rawValue; }

    /**
     * Set maximum power for TX meter scale
     * @param maxWatts Maximum power in watts (default 150W for radios, 1500W for amplifiers)
     */
    void setMaxPower(int maxWatts);

    // Size hints for layout
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    /**
     * Convert raw S-meter value to S-unit level (0-12)
     * 0 = no signal, 1-9 = S1-S9, 10 = +20, 11 = +40, 12 = +60
     * (Public for testing)
     */
    int rawToSUnit(int rawValue) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:

    /**
     * Convert power (tenths of watts) to power level (0-12 bars)
     * Scale: 0-150W → 0-12 bars (12.5W per bar)
     */
    int powerToLevel(int powerTenths) const;

    /**
     * Get label for S-unit level
     */
    QString sUnitLabel(int sUnit) const;

    /**
     * Get label for power level
     */
    QString powerLabel(int level) const;

    /**
     * Apply current theme colors
     */
    void applyTheme();

    bool m_isTxMode{false}; // TX mode (show power) vs RX mode (show S-meter)
    int m_rawValue;         // Raw value from radio (0-255 or 0-30)
    int m_currentSUnit;     // Current S-unit level (0-12)
    int m_powerWatts;       // Current power output in watts (for display)
    int m_currentPowerLevel; // Current power bar level (0-12)
    int m_maxPowerWatts{150}; // Maximum power for scale (150W radio default, 1500W for amplifier)

    // Layout dimensions (derived from font metrics in constructor)
    int m_barWidth;         // Width of each discrete bar
    int m_barHeight;        // Height of each S-meter bar
    int m_barSpacing;       // Spacing between bars
    int m_labelHeight;      // Height for label text
    int m_totalHeight;      // Total widget height
};

} // namespace TR4QT

#endif // SMETERWIDGET_H
