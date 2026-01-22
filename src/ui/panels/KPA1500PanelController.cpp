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
    return QApplication::applicationDirPath() + "/../../../resources/images/kpa1500_panel.svg";
}

QString KPA1500PanelController::getSvgFallbackPath() const {
    // Development fallback path
    return "/Users/toms/projects/TR4QT/resources/images/kpa1500_panel.svg";
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

} // namespace TR4QT
