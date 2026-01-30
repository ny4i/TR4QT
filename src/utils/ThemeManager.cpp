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

#include "ThemeManager.h"
#include "../core/Constants.h"
#include <QSettings>

namespace TR4QT {

ThemeManager::ThemeManager() {
    // Load theme from settings on startup
    loadFromSettings();
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::setTheme(ThemeType theme) {
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        emit themeChanged();
    }
}

QColor ThemeManager::color(ColorRole role) const {
    // If custom theme and custom color is set for this role, use it
    if (m_currentTheme == ThemeType::Custom && m_customColors.contains(role)) {
        return m_customColors.value(role);
    }

    // Otherwise, get color from current theme
    QMap<ColorRole, QColor> themeColors = getThemeColors(m_currentTheme);
    return themeColors.value(role, QColor(Qt::black)); // Fallback to black
}

QString ThemeManager::colorName(ColorRole role) const {
    return color(role).name();
}

void ThemeManager::setCustomColor(ColorRole role, const QColor& color) {
    m_customColors[role] = color;

    // Automatically switch to Custom theme when setting custom colors
    if (m_currentTheme != ThemeType::Custom) {
        m_currentTheme = ThemeType::Custom;
    }

    emit themeChanged();
}

QColor ThemeManager::customColor(ColorRole role) const {
    return m_customColors.value(role, QColor()); // Returns invalid QColor if not set
}

bool ThemeManager::hasCustomColor(ColorRole role) const {
    return m_customColors.contains(role);
}

void ThemeManager::clearCustomColors() {
    m_customColors.clear();
    emit themeChanged();
}

QString ThemeManager::colorRoleName(ColorRole role) {
    switch (role) {
        // Display Colors
        case ColorRole::VfoBackground: return "VFO Background";
        case ColorRole::VfoText: return "VFO Text";
        case ColorRole::WindowBackground: return "Window Background";
        case ColorRole::TextDisplayBackground: return "Text Display Background";

        // Status Colors
        case ColorRole::ConnectedStatus: return "Connected Status";
        case ColorRole::DisconnectedStatus: return "Disconnected Status";
        case ColorRole::FrozenIndicator: return "Frozen Indicator";

        // Functional Colors
        case ColorRole::DupeText: return "Duplicate Text";
        case ColorRole::NewMultiplierBackground: return "New Multiplier Background";
        case ColorRole::WorkedStationText: return "Worked Station Text";
        case ColorRole::MultiplierText: return "Multiplier Text";
        case ColorRole::LotwUserText: return "LOTW User Text";
        case ColorRole::NeededMultiplierBackground: return "Needed Multiplier Background";
        case ColorRole::ConfirmedMultiplierBackground: return "Confirmed Multiplier Background";

        // Spot Aging Colors
        case ColorRole::NewSpotText: return "New Spot Text";
        case ColorRole::NewSpotBackground: return "New Spot Background";
        case ColorRole::AgingSpotText: return "Aging Spot Text";
        case ColorRole::AgingSpotBackground: return "Aging Spot Background";

        // UI Colors
        case ColorRole::PrimaryText: return "Primary Text";
        case ColorRole::SecondaryText: return "Secondary Text";
        case ColorRole::HoverHighlight: return "Hover Highlight";
        case ColorRole::BorderColor: return "Border Color";

        // Validation State Colors
        case ColorRole::ValidationValidBorder: return "Validation Valid Border";
        case ColorRole::ValidationValidBackground: return "Validation Valid Background";
        case ColorRole::ValidationWarningBorder: return "Validation Warning Border";
        case ColorRole::ValidationWarningBackground: return "Validation Warning Background";
        case ColorRole::ValidationErrorBorder: return "Validation Error Border";
        case ColorRole::ValidationErrorBackground: return "Validation Error Background";

        // Status and Highlight Colors
        case ColorRole::WarningText: return "Warning Text";
        case ColorRole::SCPMatchText: return "SCP Match Text";
        case ColorRole::StatusFlashBackground: return "Status Flash Background";
        case ColorRole::StatusFlashText: return "Status Flash Text";
        case ColorRole::StatusFlashBorder: return "Status Flash Border";

        // Map Sidebar Colors
        case ColorRole::SectionHeaderText: return "Section Header Text";
        case ColorRole::SidebarBackground: return "Sidebar Background";
        case ColorRole::SidebarListBackground: return "Sidebar List Background";
        case ColorRole::CompletionText: return "Completion Text";
        case ColorRole::MapButtonBackground: return "Map Button Background";
        case ColorRole::MapButtonHover: return "Map Button Hover";

        // Toggle Button State Colors
        case ColorRole::ButtonCheckedBackground: return "Button Checked Background";
        case ColorRole::ButtonCheckedBorder: return "Button Checked Border";
        case ColorRole::ButtonCheckedHover: return "Button Checked Hover";
        case ColorRole::ButtonUncheckedBackground: return "Button Unchecked Background";
        case ColorRole::ButtonUncheckedBorder: return "Button Unchecked Border";
        case ColorRole::ButtonHoverBackground: return "Button Hover Background";

        // Amplifier-specific Colors
        case ColorRole::AmplifierSuccessBackground: return "Amplifier Success Background";
        case ColorRole::AmplifierSuccessHover: return "Amplifier Success Hover";
        case ColorRole::LcdDisplayText: return "LCD Display Text";
        case ColorRole::SvgPanelBackground: return "SVG Panel Background";

        default: return "Unknown";
    }
}

QString ThemeManager::themeName(ThemeType theme) {
    switch (theme) {
        case ThemeType::TR4WDefault: return "TR4W Default";
        case ThemeType::DarkMode: return "Dark Mode";
        case ThemeType::HighContrast: return "High Contrast";
        case ThemeType::Custom: return "Custom";
        default: return "Unknown";
    }
}

void ThemeManager::saveToSettings() {
    QSettings settings(APP_ORG, APP_NAME);

    // Save current theme
    settings.setValue("Theme/currentTheme", themeTypeToString(m_currentTheme));

    // Save custom colors (only if Custom theme)
    if (m_currentTheme == ThemeType::Custom) {
        settings.beginGroup("Theme/customColors");

        for (auto it = m_customColors.constBegin(); it != m_customColors.constEnd(); ++it) {
            QString roleStr = colorRoleToString(it.key());
            settings.setValue(roleStr, it.value().name());
        }

        settings.endGroup();
    }
}

void ThemeManager::loadFromSettings() {
    QSettings settings(APP_ORG, APP_NAME);

    // Load current theme
    QString themeStr = settings.value("Theme/currentTheme", "TR4WDefault").toString();
    bool ok = false;
    ThemeType theme = stringToThemeType(themeStr, &ok);
    if (ok) {
        m_currentTheme = theme;
    } else {
        m_currentTheme = ThemeType::TR4WDefault; // Fallback to default
    }

    // Load custom colors
    m_customColors.clear();
    settings.beginGroup("Theme/customColors");
    QStringList keys = settings.childKeys();

    for (const QString& key : keys) {
        bool roleOk = false;
        ColorRole role = stringToColorRole(key, &roleOk);
        if (roleOk) {
            QString colorStr = settings.value(key).toString();
            QColor color(colorStr);
            if (color.isValid()) {
                m_customColors[role] = color;
            }
        }
    }

    settings.endGroup();
}

QMap<ColorRole, QColor> ThemeManager::getThemeColors(ThemeType theme) const {
    QMap<ColorRole, QColor> colors;

    switch (theme) {
        case ThemeType::TR4WDefault:
            // TR4W-compatible theme (current TR4QT appearance)
            // Display Colors
            colors[ColorRole::VfoBackground] = QColor("#00FFFF");  // Cyan
            colors[ColorRole::VfoText] = QColor(Qt::black);
            colors[ColorRole::WindowBackground] = QColor("#f0f0f0");  // Light gray
            colors[ColorRole::TextDisplayBackground] = QColor(Qt::white);

            // Status Colors
            colors[ColorRole::ConnectedStatus] = QColor(Qt::green);
            colors[ColorRole::DisconnectedStatus] = QColor(Qt::red);
            colors[ColorRole::FrozenIndicator] = QColor("#FFB6C1");  // Light pink

            // Functional Colors
            colors[ColorRole::DupeText] = QColor(Qt::red);
            colors[ColorRole::NewMultiplierBackground] = QColor("#90EE90");  // Light green
            colors[ColorRole::WorkedStationText] = QColor("#A0A0A0");  // Gray
            colors[ColorRole::MultiplierText] = QColor(Qt::blue);
            colors[ColorRole::LotwUserText] = QColor("#006400");  // Dark green
            colors[ColorRole::NeededMultiplierBackground] = QColor("#FFFFC8");  // Light yellow
            colors[ColorRole::ConfirmedMultiplierBackground] = QColor("#90EE90");  // Light green

            // Spot Aging Colors
            colors[ColorRole::NewSpotText] = QColor("#006400");  // Dark green
            colors[ColorRole::NewSpotBackground] = QColor("#E0FFFF");  // Light cyan
            colors[ColorRole::AgingSpotText] = QColor("#FF8C00");  // Dark orange
            colors[ColorRole::AgingSpotBackground] = QColor("#FFFACD");  // Light yellow

            // UI Colors
            colors[ColorRole::PrimaryText] = QColor(Qt::black);
            colors[ColorRole::SecondaryText] = QColor(Qt::darkGray);
            colors[ColorRole::HoverHighlight] = QColor("#e0e0e0");
            colors[ColorRole::BorderColor] = QColor("#cccccc");

            // Validation State Colors
            colors[ColorRole::ValidationValidBorder] = QColor("#00aa00");  // Green
            colors[ColorRole::ValidationValidBackground] = QColor("#f0fff0");  // Light green
            colors[ColorRole::ValidationWarningBorder] = QColor("#ffaa00");  // Orange
            colors[ColorRole::ValidationWarningBackground] = QColor("#fffef0");  // Light yellow
            colors[ColorRole::ValidationErrorBorder] = QColor("#ff0000");  // Red
            colors[ColorRole::ValidationErrorBackground] = QColor("#fff0f0");  // Light red

            // Status and Highlight Colors
            colors[ColorRole::WarningText] = QColor("#ff6600");  // Orange
            colors[ColorRole::SCPMatchText] = QColor("#0066cc");  // Blue
            colors[ColorRole::StatusFlashBackground] = QColor("#ff0000");  // Red
            colors[ColorRole::StatusFlashText] = QColor("#ffffff");  // White
            colors[ColorRole::StatusFlashBorder] = QColor("#aa0000");  // Dark red

            // Map Sidebar Colors
            colors[ColorRole::SectionHeaderText] = QColor("#FFD700");  // Gold
            colors[ColorRole::SidebarBackground] = QColor("#2C3E50");  // Dark blue-gray
            colors[ColorRole::SidebarListBackground] = QColor("#34495E");  // Slightly lighter
            colors[ColorRole::CompletionText] = QColor("#2ECC71");  // Green
            colors[ColorRole::MapButtonBackground] = QColor("#3498DB");  // Blue
            colors[ColorRole::MapButtonHover] = QColor("#2980B9");  // Darker blue

            // Toggle Button State Colors
            colors[ColorRole::ButtonCheckedBackground] = QColor("#4CAF50");  // Green
            colors[ColorRole::ButtonCheckedBorder] = QColor("#2E7D32");  // Dark green
            colors[ColorRole::ButtonCheckedHover] = QColor("#45A049");  // Medium green
            colors[ColorRole::ButtonUncheckedBackground] = QColor("#E0E0E0");  // Light gray
            colors[ColorRole::ButtonUncheckedBorder] = QColor("#808080");  // Gray
            colors[ColorRole::ButtonHoverBackground] = QColor("#D0D0D0");  // Medium gray

            // Amplifier-specific Colors
            colors[ColorRole::AmplifierSuccessBackground] = QColor("#006600");  // Dark green
            colors[ColorRole::AmplifierSuccessHover] = QColor("#008800");  // Medium green
            colors[ColorRole::LcdDisplayText] = QColor("#000032");  // Dark blue
            colors[ColorRole::SvgPanelBackground] = QColor("#2a2a2a");  // Dark gray

            // Map Colors
            colors[ColorRole::MapBackground] = QColor("#E8F4F8");  // Theme default: light blue
            colors[ColorRole::MapNotWorked] = QColor("#CCCCCC");  // Theme default: gray
            colors[ColorRole::MapFirstContact] = QColor("#3498DB");  // Theme default: blue
            colors[ColorRole::MapSecondContact] = QColor("#E74C3C");  // Theme default: red
            colors[ColorRole::MapFew] = QColor("#0D5D0D");  // Theme default: dark green
            colors[ColorRole::MapSome] = QColor("#137A13");  // Theme default: green
            colors[ColorRole::MapMany] = QColor("#1A9E1A");  // Theme default: medium green
            colors[ColorRole::MapManyMore] = QColor("#2ECC71");  // Theme default: bright green
            colors[ColorRole::MapHundreds] = QColor("#5ED68F");  // Theme default: light green
            colors[ColorRole::MapHundredsMore] = QColor("#8EE0AD");  // Theme default: lighter green
            colors[ColorRole::MapThousands] = QColor("#C8F0DC");  // Theme default: very light green
            break;

        case ThemeType::DarkMode:
            // Dark theme for low-light environments
            // Display Colors
            colors[ColorRole::VfoBackground] = QColor("#006666");  // Dark cyan
            colors[ColorRole::VfoText] = QColor(Qt::white);
            colors[ColorRole::WindowBackground] = QColor("#2b2b2b");  // Dark gray
            colors[ColorRole::TextDisplayBackground] = QColor("#1e1e1e");  // Darker gray

            // Status Colors
            colors[ColorRole::ConnectedStatus] = QColor("#00FF00");  // Bright green
            colors[ColorRole::DisconnectedStatus] = QColor("#FF6666");  // Light red
            colors[ColorRole::FrozenIndicator] = QColor("#FF69B4");  // Hot pink

            // Functional Colors
            colors[ColorRole::DupeText] = QColor("#FF6666");  // Light red
            colors[ColorRole::NewMultiplierBackground] = QColor("#006400");  // Dark green
            colors[ColorRole::WorkedStationText] = QColor("#808080");  // Gray
            colors[ColorRole::MultiplierText] = QColor("#6495ED");  // Cornflower blue
            colors[ColorRole::LotwUserText] = QColor("#00FF00");  // Bright green
            colors[ColorRole::NeededMultiplierBackground] = QColor("#8B8B00");  // Dark yellow
            colors[ColorRole::ConfirmedMultiplierBackground] = QColor("#006400");  // Dark green

            // Spot Aging Colors
            colors[ColorRole::NewSpotText] = QColor("#00FF00");  // Bright green
            colors[ColorRole::NewSpotBackground] = QColor("#004040");  // Dark cyan
            colors[ColorRole::AgingSpotText] = QColor("#FFA500");  // Orange
            colors[ColorRole::AgingSpotBackground] = QColor("#8B8B00");  // Dark yellow

            // UI Colors
            colors[ColorRole::PrimaryText] = QColor("#e0e0e0");  // Light gray
            colors[ColorRole::SecondaryText] = QColor("#808080");  // Gray
            colors[ColorRole::HoverHighlight] = QColor("#3c3c3c");  // Lighter dark gray
            colors[ColorRole::BorderColor] = QColor("#555555");  // Medium gray

            // Validation State Colors (darker for dark mode)
            colors[ColorRole::ValidationValidBorder] = QColor("#00cc00");  // Brighter green
            colors[ColorRole::ValidationValidBackground] = QColor("#1a3d1a");  // Dark green
            colors[ColorRole::ValidationWarningBorder] = QColor("#ffbb33");  // Brighter orange
            colors[ColorRole::ValidationWarningBackground] = QColor("#3d3d1a");  // Dark yellow
            colors[ColorRole::ValidationErrorBorder] = QColor("#ff4444");  // Brighter red
            colors[ColorRole::ValidationErrorBackground] = QColor("#3d1a1a");  // Dark red

            // Status and Highlight Colors
            colors[ColorRole::WarningText] = QColor("#ff8833");  // Brighter orange for dark bg
            colors[ColorRole::SCPMatchText] = QColor("#6699ff");  // Brighter blue for dark bg
            colors[ColorRole::StatusFlashBackground] = QColor("#cc0000");  // Slightly darker red
            colors[ColorRole::StatusFlashText] = QColor("#ffffff");  // White
            colors[ColorRole::StatusFlashBorder] = QColor("#880000");  // Darker red

            // Map Sidebar Colors (dark mode)
            colors[ColorRole::SectionHeaderText] = QColor("#FFD700");  // Gold
            colors[ColorRole::SidebarBackground] = QColor("#1a2530");  // Dark blue-gray
            colors[ColorRole::SidebarListBackground] = QColor("#1e2d3d");  // Slightly lighter
            colors[ColorRole::CompletionText] = QColor("#2ECC71");  // Green
            colors[ColorRole::MapButtonBackground] = QColor("#2980B9");  // Darker blue
            colors[ColorRole::MapButtonHover] = QColor("#2471A3");  // Even darker blue

            // Toggle Button State Colors (dark mode)
            colors[ColorRole::ButtonCheckedBackground] = QColor("#2E7D32");  // Dark green
            colors[ColorRole::ButtonCheckedBorder] = QColor("#1B5E20");  // Very dark green
            colors[ColorRole::ButtonCheckedHover] = QColor("#388E3C");  // Medium dark green
            colors[ColorRole::ButtonUncheckedBackground] = QColor("#424242");  // Dark gray
            colors[ColorRole::ButtonUncheckedBorder] = QColor("#616161");  // Medium gray
            colors[ColorRole::ButtonHoverBackground] = QColor("#535353");  // Slightly lighter gray

            // Amplifier-specific Colors (dark mode)
            colors[ColorRole::AmplifierSuccessBackground] = QColor("#004400");  // Darker green
            colors[ColorRole::AmplifierSuccessHover] = QColor("#006600");  // Medium green
            colors[ColorRole::LcdDisplayText] = QColor("#a0a0ff");  // Light blue for dark bg
            colors[ColorRole::SvgPanelBackground] = QColor("#1a1a1a");  // Very dark gray

            // Map Colors (darker palette for dark mode)
            colors[ColorRole::MapBackground] = QColor("#1a2530");  // Theme default: dark blue-gray
            colors[ColorRole::MapNotWorked] = QColor("#555555");  // Theme default: dark gray
            colors[ColorRole::MapFirstContact] = QColor("#2980b9");  // Theme default: darker blue
            colors[ColorRole::MapSecondContact] = QColor("#c0392b");  // Theme default: darker red
            colors[ColorRole::MapFew] = QColor("#1a4d1a");  // Theme default: very dark green
            colors[ColorRole::MapSome] = QColor("#246624");  // Theme default: dark green
            colors[ColorRole::MapMany] = QColor("#2d862d");  // Theme default: medium dark green
            colors[ColorRole::MapManyMore] = QColor("#27ae60");  // Theme default: green
            colors[ColorRole::MapHundreds] = QColor("#52c073");  // Theme default: light green
            colors[ColorRole::MapHundredsMore] = QColor("#7dd999");  // Theme default: lighter green
            colors[ColorRole::MapThousands] = QColor("#a8e6bf");  // Theme default: very light green
            break;

        case ThemeType::HighContrast:
            // High contrast theme for accessibility
            // Display Colors
            colors[ColorRole::VfoBackground] = QColor(Qt::black);
            colors[ColorRole::VfoText] = QColor(Qt::yellow);
            colors[ColorRole::WindowBackground] = QColor(Qt::white);
            colors[ColorRole::TextDisplayBackground] = QColor(Qt::white);

            // Status Colors
            colors[ColorRole::ConnectedStatus] = QColor("#00FF00");  // Bright green
            colors[ColorRole::DisconnectedStatus] = QColor("#FF0000");  // Bright red
            colors[ColorRole::FrozenIndicator] = QColor("#FF00FF");  // Magenta

            // Functional Colors
            colors[ColorRole::DupeText] = QColor("#FF0000");  // Bright red
            colors[ColorRole::NewMultiplierBackground] = QColor("#00FF00");  // Bright green
            colors[ColorRole::WorkedStationText] = QColor("#808080");  // Gray
            colors[ColorRole::MultiplierText] = QColor("#0000FF");  // Bright blue
            colors[ColorRole::LotwUserText] = QColor("#00AA00");  // Medium green
            colors[ColorRole::NeededMultiplierBackground] = QColor("#FFFF00");  // Bright yellow
            colors[ColorRole::ConfirmedMultiplierBackground] = QColor("#00FF00");  // Bright green

            // Spot Aging Colors
            colors[ColorRole::NewSpotText] = QColor("#008000");  // Green
            colors[ColorRole::NewSpotBackground] = QColor("#00FFFF");  // Cyan
            colors[ColorRole::AgingSpotText] = QColor("#FF4500");  // Orange-red
            colors[ColorRole::AgingSpotBackground] = QColor("#FFFF00");  // Bright yellow

            // UI Colors
            colors[ColorRole::PrimaryText] = QColor(Qt::black);
            colors[ColorRole::SecondaryText] = QColor("#404040");  // Dark gray
            colors[ColorRole::HoverHighlight] = QColor("#d0d0d0");  // Light gray
            colors[ColorRole::BorderColor] = QColor(Qt::black);

            // Validation State Colors (high contrast)
            colors[ColorRole::ValidationValidBorder] = QColor("#00FF00");  // Bright green
            colors[ColorRole::ValidationValidBackground] = QColor("#ccffcc");  // Very light green
            colors[ColorRole::ValidationWarningBorder] = QColor("#FF8800");  // Bright orange
            colors[ColorRole::ValidationWarningBackground] = QColor("#ffffcc");  // Very light yellow
            colors[ColorRole::ValidationErrorBorder] = QColor("#FF0000");  // Bright red
            colors[ColorRole::ValidationErrorBackground] = QColor("#ffcccc");  // Very light red

            // Status and Highlight Colors
            colors[ColorRole::WarningText] = QColor("#FF6600");  // Bright orange
            colors[ColorRole::SCPMatchText] = QColor("#0000FF");  // Bright blue
            colors[ColorRole::StatusFlashBackground] = QColor("#FF0000");  // Bright red
            colors[ColorRole::StatusFlashText] = QColor("#FFFFFF");  // White
            colors[ColorRole::StatusFlashBorder] = QColor("#880000");  // Dark red

            // Map Sidebar Colors (high contrast)
            colors[ColorRole::SectionHeaderText] = QColor("#FFFF00");  // Bright yellow
            colors[ColorRole::SidebarBackground] = QColor("#000000");  // Black
            colors[ColorRole::SidebarListBackground] = QColor("#1a1a1a");  // Near black
            colors[ColorRole::CompletionText] = QColor("#00FF00");  // Bright green
            colors[ColorRole::MapButtonBackground] = QColor("#0000FF");  // Bright blue
            colors[ColorRole::MapButtonHover] = QColor("#0000CC");  // Dark blue

            // Toggle Button State Colors (high contrast)
            colors[ColorRole::ButtonCheckedBackground] = QColor("#00FF00");  // Bright green
            colors[ColorRole::ButtonCheckedBorder] = QColor("#008000");  // Green
            colors[ColorRole::ButtonCheckedHover] = QColor("#00CC00");  // Medium green
            colors[ColorRole::ButtonUncheckedBackground] = QColor("#C0C0C0");  // Silver
            colors[ColorRole::ButtonUncheckedBorder] = QColor(Qt::black);  // Black
            colors[ColorRole::ButtonHoverBackground] = QColor("#A0A0A0");  // Gray

            // Amplifier-specific Colors (high contrast)
            colors[ColorRole::AmplifierSuccessBackground] = QColor("#008800");  // Green
            colors[ColorRole::AmplifierSuccessHover] = QColor("#00AA00");  // Brighter green
            colors[ColorRole::LcdDisplayText] = QColor(Qt::black);  // Black
            colors[ColorRole::SvgPanelBackground] = QColor(Qt::black);  // Black

            // Map Colors (high contrast palette)
            colors[ColorRole::MapBackground] = QColor("#FFFFFF");  // Theme default: white
            colors[ColorRole::MapNotWorked] = QColor("#C0C0C0");  // Theme default: light gray
            colors[ColorRole::MapFirstContact] = QColor("#0000FF");  // Theme default: bright blue
            colors[ColorRole::MapSecondContact] = QColor("#FF0000");  // Theme default: bright red
            colors[ColorRole::MapFew] = QColor("#004000");  // Theme default: very dark green
            colors[ColorRole::MapSome] = QColor("#006000");  // Theme default: dark green
            colors[ColorRole::MapMany] = QColor("#008000");  // Theme default: green
            colors[ColorRole::MapManyMore] = QColor("#00A000");  // Theme default: bright green
            colors[ColorRole::MapHundreds] = QColor("#00C000");  // Theme default: brighter green
            colors[ColorRole::MapHundredsMore] = QColor("#00E000");  // Theme default: very bright green
            colors[ColorRole::MapThousands] = QColor("#00FF00");  // Theme default: max green
            break;

        case ThemeType::Custom:
            // Custom theme - use custom colors if set, otherwise fall back to TR4W Default
            {
                QMap<ColorRole, QColor> defaultColors = getThemeColors(ThemeType::TR4WDefault);
                for (auto it = defaultColors.constBegin(); it != defaultColors.constEnd(); ++it) {
                    if (m_customColors.contains(it.key())) {
                        colors[it.key()] = m_customColors.value(it.key());
                    } else {
                        colors[it.key()] = it.value();
                    }
                }
            }
            break;
    }

    return colors;
}

QString ThemeManager::colorRoleToString(ColorRole role) {
    switch (role) {
        case ColorRole::VfoBackground: return "VfoBackground";
        case ColorRole::VfoText: return "VfoText";
        case ColorRole::WindowBackground: return "WindowBackground";
        case ColorRole::TextDisplayBackground: return "TextDisplayBackground";
        case ColorRole::ConnectedStatus: return "ConnectedStatus";
        case ColorRole::DisconnectedStatus: return "DisconnectedStatus";
        case ColorRole::FrozenIndicator: return "FrozenIndicator";
        case ColorRole::DupeText: return "DupeText";
        case ColorRole::NewMultiplierBackground: return "NewMultiplierBackground";
        case ColorRole::WorkedStationText: return "WorkedStationText";
        case ColorRole::MultiplierText: return "MultiplierText";
        case ColorRole::LotwUserText: return "LotwUserText";
        case ColorRole::NeededMultiplierBackground: return "NeededMultiplierBackground";
        case ColorRole::ConfirmedMultiplierBackground: return "ConfirmedMultiplierBackground";
        case ColorRole::NewSpotText: return "NewSpotText";
        case ColorRole::NewSpotBackground: return "NewSpotBackground";
        case ColorRole::AgingSpotText: return "AgingSpotText";
        case ColorRole::AgingSpotBackground: return "AgingSpotBackground";
        case ColorRole::PrimaryText: return "PrimaryText";
        case ColorRole::SecondaryText: return "SecondaryText";
        case ColorRole::HoverHighlight: return "HoverHighlight";
        case ColorRole::BorderColor: return "BorderColor";
        case ColorRole::ValidationValidBorder: return "ValidationValidBorder";
        case ColorRole::ValidationValidBackground: return "ValidationValidBackground";
        case ColorRole::ValidationWarningBorder: return "ValidationWarningBorder";
        case ColorRole::ValidationWarningBackground: return "ValidationWarningBackground";
        case ColorRole::ValidationErrorBorder: return "ValidationErrorBorder";
        case ColorRole::ValidationErrorBackground: return "ValidationErrorBackground";
        case ColorRole::WarningText: return "WarningText";
        case ColorRole::SCPMatchText: return "SCPMatchText";
        case ColorRole::StatusFlashBackground: return "StatusFlashBackground";
        case ColorRole::StatusFlashText: return "StatusFlashText";
        case ColorRole::StatusFlashBorder: return "StatusFlashBorder";
        case ColorRole::SectionHeaderText: return "SectionHeaderText";
        case ColorRole::SidebarBackground: return "SidebarBackground";
        case ColorRole::SidebarListBackground: return "SidebarListBackground";
        case ColorRole::CompletionText: return "CompletionText";
        case ColorRole::MapButtonBackground: return "MapButtonBackground";
        case ColorRole::MapButtonHover: return "MapButtonHover";
        case ColorRole::ButtonCheckedBackground: return "ButtonCheckedBackground";
        case ColorRole::ButtonCheckedBorder: return "ButtonCheckedBorder";
        case ColorRole::ButtonCheckedHover: return "ButtonCheckedHover";
        case ColorRole::ButtonUncheckedBackground: return "ButtonUncheckedBackground";
        case ColorRole::ButtonUncheckedBorder: return "ButtonUncheckedBorder";
        case ColorRole::ButtonHoverBackground: return "ButtonHoverBackground";
        case ColorRole::AmplifierSuccessBackground: return "AmplifierSuccessBackground";
        case ColorRole::AmplifierSuccessHover: return "AmplifierSuccessHover";
        case ColorRole::LcdDisplayText: return "LcdDisplayText";
        case ColorRole::SvgPanelBackground: return "SvgPanelBackground";
        default: return "Unknown";
    }
}

ColorRole ThemeManager::stringToColorRole(const QString& str, bool* ok) {
    if (ok) *ok = true;

    if (str == "VfoBackground") return ColorRole::VfoBackground;
    if (str == "VfoText") return ColorRole::VfoText;
    if (str == "WindowBackground") return ColorRole::WindowBackground;
    if (str == "TextDisplayBackground") return ColorRole::TextDisplayBackground;
    if (str == "ConnectedStatus") return ColorRole::ConnectedStatus;
    if (str == "DisconnectedStatus") return ColorRole::DisconnectedStatus;
    if (str == "FrozenIndicator") return ColorRole::FrozenIndicator;
    if (str == "DupeText") return ColorRole::DupeText;
    if (str == "NewMultiplierBackground") return ColorRole::NewMultiplierBackground;
    if (str == "WorkedStationText") return ColorRole::WorkedStationText;
    if (str == "MultiplierText") return ColorRole::MultiplierText;
    if (str == "LotwUserText") return ColorRole::LotwUserText;
    if (str == "NeededMultiplierBackground") return ColorRole::NeededMultiplierBackground;
    if (str == "ConfirmedMultiplierBackground") return ColorRole::ConfirmedMultiplierBackground;
    if (str == "NewSpotText") return ColorRole::NewSpotText;
    if (str == "NewSpotBackground") return ColorRole::NewSpotBackground;
    if (str == "AgingSpotText") return ColorRole::AgingSpotText;
    if (str == "AgingSpotBackground") return ColorRole::AgingSpotBackground;
    if (str == "PrimaryText") return ColorRole::PrimaryText;
    if (str == "SecondaryText") return ColorRole::SecondaryText;
    if (str == "HoverHighlight") return ColorRole::HoverHighlight;
    if (str == "BorderColor") return ColorRole::BorderColor;
    if (str == "ValidationValidBorder") return ColorRole::ValidationValidBorder;
    if (str == "ValidationValidBackground") return ColorRole::ValidationValidBackground;
    if (str == "ValidationWarningBorder") return ColorRole::ValidationWarningBorder;
    if (str == "ValidationWarningBackground") return ColorRole::ValidationWarningBackground;
    if (str == "ValidationErrorBorder") return ColorRole::ValidationErrorBorder;
    if (str == "ValidationErrorBackground") return ColorRole::ValidationErrorBackground;
    if (str == "WarningText") return ColorRole::WarningText;
    if (str == "SCPMatchText") return ColorRole::SCPMatchText;
    if (str == "StatusFlashBackground") return ColorRole::StatusFlashBackground;
    if (str == "StatusFlashText") return ColorRole::StatusFlashText;
    if (str == "StatusFlashBorder") return ColorRole::StatusFlashBorder;
    if (str == "SectionHeaderText") return ColorRole::SectionHeaderText;
    if (str == "SidebarBackground") return ColorRole::SidebarBackground;
    if (str == "SidebarListBackground") return ColorRole::SidebarListBackground;
    if (str == "CompletionText") return ColorRole::CompletionText;
    if (str == "MapButtonBackground") return ColorRole::MapButtonBackground;
    if (str == "MapButtonHover") return ColorRole::MapButtonHover;
    if (str == "ButtonCheckedBackground") return ColorRole::ButtonCheckedBackground;
    if (str == "ButtonCheckedBorder") return ColorRole::ButtonCheckedBorder;
    if (str == "ButtonCheckedHover") return ColorRole::ButtonCheckedHover;
    if (str == "ButtonUncheckedBackground") return ColorRole::ButtonUncheckedBackground;
    if (str == "ButtonUncheckedBorder") return ColorRole::ButtonUncheckedBorder;
    if (str == "ButtonHoverBackground") return ColorRole::ButtonHoverBackground;
    if (str == "AmplifierSuccessBackground") return ColorRole::AmplifierSuccessBackground;
    if (str == "AmplifierSuccessHover") return ColorRole::AmplifierSuccessHover;
    if (str == "LcdDisplayText") return ColorRole::LcdDisplayText;
    if (str == "SvgPanelBackground") return ColorRole::SvgPanelBackground;

    if (ok) *ok = false;
    return ColorRole::PrimaryText; // Default fallback
}

QString ThemeManager::themeTypeToString(ThemeType theme) {
    switch (theme) {
        case ThemeType::TR4WDefault: return "TR4WDefault";
        case ThemeType::DarkMode: return "DarkMode";
        case ThemeType::HighContrast: return "HighContrast";
        case ThemeType::Custom: return "Custom";
        default: return "TR4WDefault";
    }
}

ThemeType ThemeManager::stringToThemeType(const QString& str, bool* ok) {
    if (ok) *ok = true;

    if (str == "TR4WDefault") return ThemeType::TR4WDefault;
    if (str == "DarkMode") return ThemeType::DarkMode;
    if (str == "HighContrast") return ThemeType::HighContrast;
    if (str == "Custom") return ThemeType::Custom;

    if (ok) *ok = false;
    return ThemeType::TR4WDefault; // Default fallback
}

} // namespace TR4QT
