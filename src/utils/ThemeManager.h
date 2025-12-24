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
    NeededMultiplierBackground, // Needed multiplier (default: light yellow #FFFFC8)
    ConfirmedMultiplierBackground, // Confirmed multiplier (default: light green #90EE90)

    // UI Colors
    PrimaryText,                // Main text color (default: black)
    SecondaryText,              // Secondary/disabled text (default: dark gray)
    HoverHighlight,             // Hover effect (default: #e0e0e0)
    BorderColor                 // Widget borders (default: #ccc)
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
