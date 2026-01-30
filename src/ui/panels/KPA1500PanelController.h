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

#ifndef KPA1500PANELCONTROLLER_H
#define KPA1500PANELCONTROLLER_H

#include "IAmplifierPanelController.h"
#include <QHash>

namespace TR4QT {

/**
 * @brief KPA1500-specific panel controller
 *
 * Implements the panel controller interface for the Elecraft KPA1500 amplifier.
 * Defines:
 * - SVG front panel layout (kpa1500_panel.svg)
 * - Band button IDs and their ^BN commands
 * - LED IDs for OPER, STBY, FAULT, SWR bargraph
 * - Power meter configuration (0-1500W)
 */
class KPA1500PanelController : public IAmplifierPanelController {
public:
    KPA1500PanelController();
    ~KPA1500PanelController() override = default;

    // ========== SVG Resource ==========
    QString getSvgResourcePath() const override;
    QString getSvgFallbackPath() const override;

    // ========== Button Configuration ==========
    QStringList getButtonIds() const override;
    QString getButtonCommand(const QString& buttonId,
                            const AmplifierState& currentState) const override;
    QString getButtonLabel(const QString& buttonId) const override;

    // ========== LED Configuration ==========
    QStringList getLedIds() const override;
    bool getLedState(const QString& ledId, const AmplifierState& state) const override;
    QColor getLedOnColor(const QString& ledId) const override;

    // ========== Power Meter Configuration ==========
    int getMaxPowerWatts() const override { return MAX_POWER_WATTS; }
    QStringList getPowerMeterLedIds() const override;
    QColor getPowerMeterColor(double proportion) const override;

    // ========== SWR Meter Configuration ==========
    QStringList getSwrMeterLedIds() const override;
    QColor getSwrMeterColor(float swr) const override;

    // ========== Display Configuration ==========
    QString getAmplifierName() const override { return "Elecraft KPA1500"; }
    QSize getMinimumWindowSize() const override { return QSize(600, 200); }

private:
    // KPA1500 specific constants
    static constexpr int MAX_POWER_WATTS = 1500;

    // Power meter color thresholds (as proportions of max power)
    static constexpr double POWER_GREEN_THRESHOLD = 0.66;   // 0-66% = green
    static constexpr double POWER_YELLOW_THRESHOLD = 0.85;  // 66-85% = yellow
    // Above 85% = red

    // SWR color thresholds
    static constexpr float SWR_GREEN_THRESHOLD = 1.5f;   // SWR < 1.5 = green
    static constexpr float SWR_YELLOW_THRESHOLD = 2.0f;  // SWR 1.5-2.0 = yellow
    // SWR > 2.0 = red

    // Button ID to command mapping
    QHash<QString, QString> m_buttonCommands;

    // Button ID to label mapping
    QHash<QString, QString> m_buttonLabels;

    // LED ID to on-color mapping
    QHash<QString, QColor> m_ledOnColors;

    // Initialize mappings
    void initializeButtonMappings();
    void initializeLedMappings();
};

} // namespace TR4QT

#endif // KPA1500PANELCONTROLLER_H
