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

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QMap>
#include <QString>

namespace TR4QT {

/**
 * Color roles for semantic color naming
 * Each role represents a specific UI element that can be themed
 */
enum class ColorRole {
    // Display Colors
    VfoBackground,              // VFO display background (default: cyan #00FFFF)
    VfoText,                    // VFO text (default: black)
    WindowBackground,           // Main window background (default: #f0f0f0)
    TextDisplayBackground,      // Text displays like logs (default: white)

    // Status Colors
    ConnectedStatus,            // Radio/cluster connected (default: green)
    DisconnectedStatus,         // Radio/cluster disconnected (default: red)
    FrozenIndicator,            // Freeze button when active (default: pink #FFB6C1)

    // Functional Colors
    DupeText,                   // Duplicate QSO text (default: red)
    NewMultiplierBackground,    // New multiplier highlight (default: light green #90EE90)
    WorkedStationText,          // Already worked stations (default: gray #A0A0A0)
    MultiplierText,             // Multiplier spots (default: blue)
    LotwUserText,               // LOTW user indicator (default: dark green #006400)
    NeededMultiplierBackground, // Needed multiplier (default: light yellow #FFFFC8)
    ConfirmedMultiplierBackground, // Confirmed multiplier (default: light green #90EE90)

    // Spot Aging Colors
    NewSpotText,                // New spot text <60s (default: dark green #006400)
    NewSpotBackground,          // New spot background (default: light cyan #E0FFFF)
    AgingSpotText,              // Aging spot text last 2 min (default: dark orange #FF8C00)
    AgingSpotBackground,        // Aging spot background (default: light yellow #FFFACD)

    // UI Colors
    PrimaryText,                // Main text color (default: black)
    SecondaryText,              // Secondary/disabled text (default: dark gray)
    HoverHighlight,             // Hover effect (default: #e0e0e0)
    BorderColor,                // Widget borders (default: #ccc)

    // Validation State Colors
    ValidationValidBorder,      // Valid input border (default: green #00aa00)
    ValidationValidBackground,  // Valid input background (default: light green #f0fff0)
    ValidationWarningBorder,    // Warning input border (default: orange #ffaa00)
    ValidationWarningBackground,// Warning input background (default: light yellow #fffef0)
    ValidationErrorBorder,      // Error input border (default: red #ff0000)
    ValidationErrorBackground,  // Error input background (default: light red #fff0f0)

    // Status and Highlight Colors
    WarningText,                // Warning status text (default: orange #ff6600)
    SCPMatchText,               // Super Check Partial matches (default: blue #0066cc)
    StatusFlashBackground,      // Disconnected status flash background (default: red #ff0000)
    StatusFlashText,            // Disconnected status flash text (default: white #ffffff)
    StatusFlashBorder,          // Disconnected status flash border (default: dark red #aa0000)

    // Map Sidebar Colors
    SectionHeaderText,          // Map sidebar section titles (default: gold #FFD700)
    SidebarBackground,          // Map sidebar background (default: #2C3E50)
    SidebarListBackground,      // Map worked list background (default: #34495E)
    CompletionText,             // Completion percentage text (default: #2ECC71)
    MapButtonBackground,        // Map button background (default: #3498DB)
    MapButtonHover,             // Map button hover (default: #2980B9)

    // Toggle Button State Colors
    ButtonCheckedBackground,    // Toggle on background (default: #4CAF50)
    ButtonCheckedBorder,        // Toggle on border (default: #2E7D32)
    ButtonCheckedHover,         // Toggle on hover (default: #45A049)
    ButtonUncheckedBackground,  // Toggle off background (default: #E0E0E0)
    ButtonUncheckedBorder,      // Toggle off border (default: #808080)
    ButtonHoverBackground,      // Toggle off hover (default: #D0D0D0)

    // Amplifier-specific Colors
    AmplifierSuccessBackground, // Amp connect button background (default: #006600)
    AmplifierSuccessHover,      // Amp connect button hover (default: #008800)
    LcdDisplayText,             // Amplifier LCD text color (default: #000032)
    SvgPanelBackground,         // SVG panel fallback background (default: #2a2a2a)

    // Map Colors (QSO count gradient)
    MapBackground,              // Map background (default: light blue #E8F4F8)
    MapNotWorked,               // 0 QSOs (default: gray #CCCCCC)
    MapFirstContact,            // 1 QSO (default: blue #3498DB)
    MapSecondContact,           // 2 QSOs (default: red #E74C3C)
    MapFew,                     // 3-9 QSOs (default: dark green #0D5D0D)
    MapSome,                    // 10-19 QSOs (default: green #137A13)
    MapMany,                    // 20-49 QSOs (default: medium green #1A9E1A)
    MapManyMore,                // 50-99 QSOs (default: bright green #2ECC71)
    MapHundreds,                // 100-199 QSOs (default: light green #5ED68F)
    MapHundredsMore,            // 200-499 QSOs (default: lighter green #8EE0AD)
    MapThousands                // 500+ QSOs (default: very light green #C8F0DC)
};

/**
 * Pre-defined theme types
 */
enum class ThemeType {
    TR4WDefault,    // TR4W-compatible theme (current TR4QT appearance)
    DarkMode,       // Dark theme for low-light environments
    HighContrast,   // High contrast theme for accessibility
    Custom          // User-defined custom theme
};

/**
 * Theme Manager - Centralized color management
 *
 * Singleton class that manages application colors through themes.
 * Provides TR4W-compatible default theme plus Dark Mode and High Contrast.
 * Supports custom per-element color customization.
 *
 * Usage:
 *   auto& theme = ThemeManager::instance();
 *   QColor vfoColor = theme.color(ColorRole::VfoBackground);
 *
 * Connect to themeChanged() signal to update UI when theme changes:
 *   connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
 *           this, &MyWidget::applyTheme);
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    /**
     * Get singleton instance
     */
    static ThemeManager& instance();

    /**
     * Get current theme type
     */
    ThemeType currentTheme() const { return m_currentTheme; }

    /**
     * Set active theme
     * Emits themeChanged() signal
     */
    void setTheme(ThemeType theme);

    /**
     * Get color for a specific role
     * Returns custom color if set (for Custom theme),
     * otherwise returns theme default
     */
    QColor color(ColorRole role) const;

    /**
     * Get color name (for CSS/stylesheets)
     */
    QString colorName(ColorRole role) const;

    /**
     * Set custom color for a role (Custom theme only)
     * Automatically switches to Custom theme
     * Emits themeChanged() signal
     */
    void setCustomColor(ColorRole role, const QColor& color);

    /**
     * Get custom color for a role (returns invalid QColor if not set)
     */
    QColor customColor(ColorRole role) const;

    /**
     * Check if a custom color is set for a role
     */
    bool hasCustomColor(ColorRole role) const;

    /**
     * Clear all custom colors
     */
    void clearCustomColors();

    /**
     * Get human-readable name for a color role
     */
    static QString colorRoleName(ColorRole role);

    /**
     * Get human-readable name for a theme type
     */
    static QString themeName(ThemeType theme);

    /**
     * Save current theme and custom colors to settings
     */
    void saveToSettings();

    /**
     * Load theme and custom colors from settings
     */
    void loadFromSettings();

signals:
    /**
     * Emitted when theme changes or custom colors are modified
     * Widgets should connect to this signal and update their appearance
     */
    void themeChanged();

private:
    ThemeManager();
    ~ThemeManager() override = default;

    // Prevent copying
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    /**
     * Get theme color definitions
     * Returns map of ColorRole -> QColor for a given theme
     */
    QMap<ColorRole, QColor> getThemeColors(ThemeType theme) const;

    /**
     * Convert ColorRole enum to string for settings storage
     */
    static QString colorRoleToString(ColorRole role);

    /**
     * Convert string to ColorRole enum
     */
    static ColorRole stringToColorRole(const QString& str, bool* ok = nullptr);

    /**
     * Convert ThemeType enum to string for settings storage
     */
    static QString themeTypeToString(ThemeType theme);

    /**
     * Convert string to ThemeType enum
     */
    static ThemeType stringToThemeType(const QString& str, bool* ok = nullptr);

    ThemeType m_currentTheme{ThemeType::TR4WDefault};
    QMap<ColorRole, QColor> m_customColors;
};

} // namespace TR4QT

#endif // THEMEMANAGER_H
