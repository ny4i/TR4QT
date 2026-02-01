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

#ifndef IAMPLIFIERPANELCONTROLLER_H
#define IAMPLIFIERPANELCONTROLLER_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QRectF>
#include "../../amplifiers/IAmplifierController.h"

namespace TR4QT {

/**
 * @brief Interface for amplifier-specific panel controllers
 *
 * Each amplifier type (KPA1500, KPA500, ACOM, SPE, etc.) can have its own
 * panel controller that defines:
 * - SVG resource path for the front panel image
 * - Button IDs and their command mappings
 * - LED IDs and their state mappings
 * - Power/SWR meter configurations
 */
class IAmplifierPanelController {
public:
    virtual ~IAmplifierPanelController() = default;

    // ========== SVG Resource ==========

    /**
     * @brief Get the path to the SVG front panel image
     */
    virtual QString getSvgResourcePath() const = 0;

    /**
     * @brief Get fallback SVG path for development (absolute path)
     */
    virtual QString getSvgFallbackPath() const = 0;

    // ========== Button Configuration ==========

    /**
     * @brief Get list of clickable button element IDs in the SVG
     */
    virtual QStringList getButtonIds() const = 0;

    /**
     * @brief Get the command to send when a button is pressed
     * @param buttonId The SVG element ID of the button
     * @param currentState Current amplifier state (for toggle buttons)
     * @return Command string to send, or empty if no action
     */
    virtual QString getButtonCommand(const QString& buttonId,
                                     const AmplifierState& currentState) const = 0;

    /**
     * @brief Get human-readable label for a button
     */
    virtual QString getButtonLabel(const QString& buttonId) const = 0;

    // ========== LED Configuration ==========

    /**
     * @brief Get list of LED element IDs in the SVG
     */
    virtual QStringList getLedIds() const = 0;

    /**
     * @brief Determine LED state from amplifier state
     * @param ledId The SVG element ID of the LED
     * @param state Current amplifier state
     * @return true if LED should be on, false if off
     */
    virtual bool getLedState(const QString& ledId,
                            const AmplifierState& state) const = 0;

    /**
     * @brief Get the color for an LED when it's ON
     */
    virtual QColor getLedOnColor(const QString& ledId) const = 0;

    /**
     * @brief Get the color for an LED when it's OFF (usually dark gray)
     */
    virtual QColor getLedOffColor() const { return QColor("#202020"); }

    // ========== Power Meter Configuration ==========

    /**
     * @brief Get the maximum power in watts for this amplifier
     */
    virtual int getMaxPowerWatts() const = 0;

    /**
     * @brief Get power meter LED IDs (ordered from low to high power)
     */
    virtual QStringList getPowerMeterLedIds() const = 0;

    /**
     * @brief Get the color for a power meter LED based on power level
     * @param proportion Power as proportion of max (0.0-1.0)
     * @deprecated Use getPowerLedColor() for gradient effect
     */
    virtual QColor getPowerMeterColor(double proportion) const = 0;

    /**
     * @brief Get the gradient color for a power meter LED based on position
     * @param ledIndex The LED index (0-based)
     * @param totalLeds Total number of LEDs in the meter
     * @return Color for this LED position (green→yellow→red gradient)
     */
    virtual QColor getPowerLedColor(int ledIndex, int totalLeds) const {
        // Default implementation: gradient from green to red
        if (totalLeds <= 1) return QColor(0, 255, 0);  // Green

        float proportion = static_cast<float>(ledIndex) / (totalLeds - 1);

        // Green (0.0) → Yellow (0.5) → Red (1.0)
        int r, g;
        if (proportion < 0.5f) {
            // Green to Yellow
            float t = proportion * 2.0f;
            r = static_cast<int>(255 * t);
            g = 255;
        } else {
            // Yellow to Red
            float t = (proportion - 0.5f) * 2.0f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
        }
        return QColor(r, g, 0);
    }

    // ========== SWR Meter Configuration ==========

    /**
     * @brief Get SWR meter LED IDs (ordered from low to high SWR)
     */
    virtual QStringList getSwrMeterLedIds() const = 0;

    /**
     * @brief Get the color for an SWR meter LED based on SWR value
     * @param swr The SWR value
     * @deprecated Use getSwrLedColor() for gradient effect
     */
    virtual QColor getSwrMeterColor(float swr) const = 0;

    /**
     * @brief Get the gradient color for an SWR meter LED based on position
     * @param ledIndex The LED index (0-based)
     * @param totalLeds Total number of LEDs in the meter
     * @return Color for this LED position (green→yellow→red gradient)
     */
    virtual QColor getSwrLedColor(int ledIndex, int totalLeds) const {
        // Default implementation: gradient from green to red
        if (totalLeds <= 1) return QColor(0, 255, 0);  // Green

        float proportion = static_cast<float>(ledIndex) / (totalLeds - 1);

        // Green (0.0) → Yellow (0.5) → Red (1.0)
        int r, g;
        if (proportion < 0.5f) {
            // Green to Yellow
            float t = proportion * 2.0f;
            r = static_cast<int>(255 * t);
            g = 255;
        } else {
            // Yellow to Red
            float t = (proportion - 0.5f) * 2.0f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
        }
        return QColor(r, g, 0);
    }

    /**
     * @brief Get the maximum SWR to display on the meter
     */
    virtual float getMaxSwrDisplay() const { return 5.0f; }

    // ========== Display Configuration ==========

    /**
     * @brief Get the amplifier name for display
     */
    virtual QString getAmplifierName() const = 0;

    /**
     * @brief Get minimum window size for this panel
     */
    virtual QSize getMinimumWindowSize() const = 0;
};

} // namespace TR4QT

#endif // IAMPLIFIERPANELCONTROLLER_H
