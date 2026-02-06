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

#include "KPA1500PanelController.h"
#include "../../core/Constants.h"
#include <QApplication>
#include <QColor>

namespace TR4QT {

KPA1500PanelController::KPA1500PanelController() {
    initializeButtonMappings();
    initializeLedMappings();
}

// ========== SVG Resource ==========

QString KPA1500PanelController::getSvgResourcePath() const {
    // Use Qt resource system - SVG is embedded in executable
    return ":/svg/kpa1500_panel.svg";
}

QString KPA1500PanelController::getSvgFallbackPath() const {
    // No fallback needed - resource is always available
    return ":/svg/kpa1500_panel.svg";
}

// ========== Button Configuration ==========

QStringList KPA1500PanelController::getButtonIds() const {
    return {
        // Band buttons - top row
        "btn_160m", "btn_80m", "btn_40m", "btn_20m", "btn_15m", "btn_10m",
        // Band buttons - bottom row
        "btn_60m", "btn_30m", "btn_17m", "btn_12m", "btn_6m",
        // Control buttons (right side)
        "btn_ON_OFF", "btn_RESET_INFO", "btn_MODE", "btn_ANTENNA", "btn_ATU_TUNE", "btn_AUX",
        // Menu/navigation buttons (below LCD)
        "btn_STATUS", "btn_MENU_EDIT", "btn_UP", "btn_DOWN"
    };
}

QString KPA1500PanelController::getButtonCommand(const QString& buttonId,
                                                  const AmplifierState& currentState) const {
    // Special handling for toggle buttons
    if (buttonId == "btn_ON_OFF" || buttonId == "btn_MODE") {
        // Toggle between operate and standby
        // ^OS1; = Operate mode, ^OS0; = Standby mode
        return currentState.operateMode ? "^OS0;" : "^OS1;";
    }

    // Look up static command mapping
    return m_buttonCommands.value(buttonId, QString());
}

QString KPA1500PanelController::getButtonLabel(const QString& buttonId) const {
    return m_buttonLabels.value(buttonId, buttonId);
}

void KPA1500PanelController::initializeButtonMappings() {
    // KPA1500 band commands: ^BN00; = 160m through ^BN10; = 6m
    m_buttonCommands = {
        {"btn_160m", "^BN00;"},
        {"btn_80m",  "^BN01;"},
        {"btn_60m",  "^BN02;"},
        {"btn_40m",  "^BN03;"},
        {"btn_30m",  "^BN04;"},
        {"btn_20m",  "^BN05;"},
        {"btn_17m",  "^BN06;"},
        {"btn_15m",  "^BN07;"},
        {"btn_12m",  "^BN08;"},
        {"btn_10m",  "^BN09;"},
        {"btn_6m",   "^BN10;"},
        // Control buttons (right side)
        {"btn_RESET_INFO", "^RS;"},
        {"btn_MODE", "^MD;"},
        {"btn_ANTENNA", "^AN;"},
        {"btn_ATU_TUNE", "^TN;"},     // ATU tune
        {"btn_AUX", "^AX;"},
        // Menu/navigation buttons (below LCD)
        {"btn_STATUS", "^ST;"},      // Status display toggle
        {"btn_MENU_EDIT", "^MN;"},   // Menu/Edit mode
        {"btn_UP", "^UP;"},          // Menu navigation up / value increment
        {"btn_DOWN", "^DN;"}         // Menu navigation down / value decrement
    };

    // Human-readable labels
    m_buttonLabels = {
        {"btn_160m", "160m"},
        {"btn_80m",  "80m"},
        {"btn_60m",  "60m"},
        {"btn_40m",  "40m"},
        {"btn_30m",  "30m"},
        {"btn_20m",  "20m"},
        {"btn_17m",  "17m"},
        {"btn_15m",  "15m"},
        {"btn_12m",  "12m"},
        {"btn_10m",  "10m"},
        {"btn_6m",   "6m"},
        {"btn_ON_OFF", "ON/OFF"},
        {"btn_RESET_INFO", "RESET/INFO"},
        {"btn_MODE", "MODE"},
        {"btn_ANTENNA", "ANTENNA"},
        {"btn_ATU_TUNE", "ATU TUNE"},
        {"btn_AUX", "AUX"},
        {"btn_STATUS", "STATUS"},
        {"btn_MENU_EDIT", "MENU/EDIT"},
        {"btn_UP", "UP"},
        {"btn_DOWN", "DOWN"}
    };
}

// ========== LED Configuration ==========

QStringList KPA1500PanelController::getLedIds() const {
    return {
        // Status LEDs
        "led_OPER", "led_STBY", "led_FAULT", "led_TX", "led_OVR",
        // Antenna LEDs
        "led_ANT1", "led_ANT2",
        // ATU LEDs
        "led_ATU_IN", "led_ATU_BYP",
        // SWR bargraph LEDs
        "led_SWR_1", "led_SWR_2", "led_SWR_3", "led_SWR_4", "led_SWR_5",
        "led_SWR_6", "led_SWR_7", "led_SWR_8", "led_SWR_9", "led_SWR_10"
    };
}

bool KPA1500PanelController::getLedState(const QString& ledId,
                                          const AmplifierState& state) const {
    // OPER/STBY are a binary pair - only one can be lit at a time
    // When amplifier is in operate mode: OPER=green, STBY=off
    // When amplifier is in standby mode: OPER=off, STBY=amber
    if (ledId == "led_OPER") {
        return state.operateMode;  // Green when operating
    } else if (ledId == "led_STBY") {
        return !state.operateMode;  // Amber when in standby (off when operating)
    }

    // Fault LED - only on when fault detected
    if (ledId == "led_FAULT") {
        return state.faultDetected;
    }

    // TX LED - only on when actively transmitting (power > 0)
    if (ledId == "led_TX") {
        return state.forwardPowerWatts > 0;
    }

    // Overdrive LED - only on when overdrive condition exists
    if (ledId == "led_OVR") {
        return state.reflectedPowerWatts > 50 || state.swr > 2.5f;
    }

    // Antenna LEDs - TODO: need antenna state from amplifier
    // ATU LEDs - TODO: need ATU state from amplifier

    // SWR bargraph LEDs are handled separately via updateSwrMeter()
    // They default to OFF here - only lit when SWR metering data is present

    // All other LEDs default to OFF
    return false;
}

QColor KPA1500PanelController::getLedOnColor(const QString& ledId) const {
    return m_ledOnColors.value(ledId, QColor(LedColors::GREEN));  // Default green
}

void KPA1500PanelController::initializeLedMappings() {
    // Using Elecraft official brand colors for authentic look
    // OPER/STBY are a binary pair - only one lit at a time
    m_ledOnColors = {
        // Status LEDs (OPER and STBY are mutually exclusive)
        {"led_OPER",    QColor(LedColors::GREEN)},            // Bright green when operating
        {"led_STBY",    QColor(ElecraftColors::STARSHIP)},    // Elecraft yellow when standby
        {"led_FAULT",   QColor(LedColors::RED)},              // Red - fault
        {"led_TX",      QColor(LedColors::RED)},              // Red - transmitting
        {"led_OVR",     QColor(LedColors::RED)},              // Red - overdrive
        // Antenna LEDs
        {"led_ANT1",    QColor(LedColors::GREEN)},            // Green - antenna 1 selected
        {"led_ANT2",    QColor(LedColors::GREEN)},            // Green - antenna 2 selected
        // ATU LEDs
        {"led_ATU_IN",  QColor(LedColors::GREEN)},            // Green - ATU engaged
        {"led_ATU_BYP", QColor(ElecraftColors::RED_DAMASK)},  // Elecraft orange/amber - ATU bypassed
        // SWR LEDs use dynamic colors based on SWR value (default to OFF)
    };
}

// ========== Power Meter Configuration ==========

QStringList KPA1500PanelController::getPowerMeterLedIds() const {
    // Power meter LEDs need to be renamed in SVG from rect28-rect58 to led_PWR_01-led_PWR_31
    // For now, return empty list - TODO: implement when SVG is updated
    return {};
}

QColor KPA1500PanelController::getPowerMeterColor(double proportion) const {
    if (proportion < POWER_GREEN_THRESHOLD) {
        return QColor(LedColors::GREEN);   // Green
    } else if (proportion < POWER_YELLOW_THRESHOLD) {
        return QColor(LedColors::YELLOW);  // Yellow
    } else {
        return QColor(LedColors::RED);     // Red
    }
}

// ========== SWR Meter Configuration ==========

QStringList KPA1500PanelController::getSwrMeterLedIds() const {
    return {
        "led_SWR_1", "led_SWR_2", "led_SWR_3", "led_SWR_4", "led_SWR_5",
        "led_SWR_6", "led_SWR_7", "led_SWR_8", "led_SWR_9", "led_SWR_10"
    };
}

QColor KPA1500PanelController::getSwrMeterColor(float swr) const {
    if (swr < SWR_GREEN_THRESHOLD) {
        return QColor(LedColors::GREEN);   // Green - good SWR
    } else if (swr < SWR_YELLOW_THRESHOLD) {
        return QColor(LedColors::YELLOW);  // Yellow - acceptable SWR
    } else {
        return QColor(LedColors::RED);     // Red - high SWR
    }
}

QColor KPA1500PanelController::getSwrLedColor(int ledIndex, int totalLeds) const {
    // Gradient LUT for SWR meter: green → yellow → red
    // 10 LEDs: 1-3 green, 4-6 yellow-ish, 7-10 orange to red
    if (totalLeds <= 1) return QColor(LedColors::GREEN);

    // Pre-computed gradient for 10 LEDs (the KPA1500 standard)
    static const QColor SWR_GRADIENT_LUT[] = {
        QColor(0, 200, 0),      // LED 1: Dark green (SWR ~1.0-1.4)
        QColor(0, 255, 0),      // LED 2: Bright green
        QColor(100, 255, 0),    // LED 3: Yellow-green (SWR ~1.4-1.8)
        QColor(180, 255, 0),    // LED 4: Lime
        QColor(220, 220, 0),    // LED 5: Yellow (SWR ~1.8-2.2)
        QColor(255, 200, 0),    // LED 6: Golden yellow
        QColor(255, 150, 0),    // LED 7: Orange (SWR ~2.2-3.0)
        QColor(255, 100, 0),    // LED 8: Red-orange
        QColor(255, 50, 0),     // LED 9: Red-orange (SWR ~3.0-4.0)
        QColor(255, 0, 0),      // LED 10: Bright red (SWR > 4.0)
    };

    // Handle different LED counts by interpolating
    if (totalLeds == 10) {
        return SWR_GRADIENT_LUT[qBound(0, ledIndex, 9)];
    }

    // For other LED counts, use the base class interpolation
    return IAmplifierPanelController::getSwrLedColor(ledIndex, totalLeds);
}

QColor KPA1500PanelController::getPowerLedColor(int ledIndex, int totalLeds) const {
    // Gradient LUT for power meter: green → yellow → orange → red
    // Power meter on KPA1500 has ~31 LEDs but SVG IDs not yet renamed
    if (totalLeds <= 1) return QColor(LedColors::GREEN);

    float proportion = static_cast<float>(ledIndex) / (totalLeds - 1);

    // More gradual transition than default:
    // 0-50%: green to yellow-green (safe operating range)
    // 50-75%: yellow-green to orange (moderate power)
    // 75-100%: orange to red (high power)
    int r, g;
    if (proportion < 0.5f) {
        // Green to Yellow-green (0-50% power)
        float t = proportion * 2.0f;
        r = static_cast<int>(180 * t);
        g = 200 + static_cast<int>(55 * t);  // 200 → 255
    } else if (proportion < 0.75f) {
        // Yellow-green to Orange (50-75% power)
        float t = (proportion - 0.5f) * 4.0f;
        r = 180 + static_cast<int>(75 * t);  // 180 → 255
        g = 255 - static_cast<int>(105 * t); // 255 → 150
    } else {
        // Orange to Red (75-100% power)
        float t = (proportion - 0.75f) * 4.0f;
        r = 255;
        g = static_cast<int>(150 * (1.0f - t));  // 150 → 0
    }
    return QColor(r, g, 0);
}

} // namespace TR4QT
