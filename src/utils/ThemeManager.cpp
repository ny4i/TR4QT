#include "ThemeManager.h"
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

        // UI Colors
        case ColorRole::PrimaryText: return "Primary Text";
        case ColorRole::SecondaryText: return "Secondary Text";
        case ColorRole::HoverHighlight: return "Hover Highlight";
        case ColorRole::BorderColor: return "Border Color";

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
    QSettings settings;

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
    QSettings settings;

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

            // UI Colors
            colors[ColorRole::PrimaryText] = QColor(Qt::black);
            colors[ColorRole::SecondaryText] = QColor(Qt::darkGray);
            colors[ColorRole::HoverHighlight] = QColor("#e0e0e0");
            colors[ColorRole::BorderColor] = QColor("#cccccc");
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

            // UI Colors
            colors[ColorRole::PrimaryText] = QColor("#e0e0e0");  // Light gray
            colors[ColorRole::SecondaryText] = QColor("#808080");  // Gray
            colors[ColorRole::HoverHighlight] = QColor("#3c3c3c");  // Lighter dark gray
            colors[ColorRole::BorderColor] = QColor("#555555");  // Medium gray
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

            // UI Colors
            colors[ColorRole::PrimaryText] = QColor(Qt::black);
            colors[ColorRole::SecondaryText] = QColor("#404040");  // Dark gray
            colors[ColorRole::HoverHighlight] = QColor("#d0d0d0");  // Light gray
            colors[ColorRole::BorderColor] = QColor(Qt::black);
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
        case ColorRole::PrimaryText: return "PrimaryText";
        case ColorRole::SecondaryText: return "SecondaryText";
        case ColorRole::HoverHighlight: return "HoverHighlight";
        case ColorRole::BorderColor: return "BorderColor";
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
    if (str == "PrimaryText") return ColorRole::PrimaryText;
    if (str == "SecondaryText") return ColorRole::SecondaryText;
    if (str == "HoverHighlight") return ColorRole::HoverHighlight;
    if (str == "BorderColor") return ColorRole::BorderColor;

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
