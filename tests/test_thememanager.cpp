#include <QTest>
#include <QSignalSpy>
#include <QSettings>
#include "../src/utils/ThemeManager.h"

using namespace TR4QT;

/**
 * Unit tests for ThemeManager
 * Tests theme switching, color retrieval, and custom colors
 */
class TestThemeManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Singleton tests
    void testSingleton_SameInstance();

    // Theme tests
    void testDefaultTheme_IsTR4W();
    void testSetTheme_ChangesCurrentTheme();
    void testSetTheme_EmitsSignal();
    void testThemeName_AllThemes();

    // Color retrieval tests
    void testColor_TR4WDefault_VfoBackground();
    void testColor_DarkMode_VfoBackground();
    void testColor_HighContrast_VfoBackground();
    void testColorName_ReturnsHexString();

    // ColorRole naming tests
    void testColorRoleName_AllRoles();

    // Custom color tests
    void testSetCustomColor_AutoSwitchesToCustomTheme();
    void testSetCustomColor_EmitsSignal();
    void testCustomColor_ReturnsSetColor();
    void testHasCustomColor_DetectsCustomColors();
    void testClearCustomColors_RemovesAll();
    void testColor_CustomTheme_UsesCustomColor();
    void testColor_CustomTheme_FallsBackToDefault();

    // Save/Load tests
    void testSaveLoad_PreservesTheme();
    void testSaveLoad_PreservesCustomColors();
    void testLoad_InvalidTheme_FallsBackToDefault();

private:
    void clearSettings();
};

void TestThemeManager::initTestCase() {
    // Clear any existing settings before all tests
    clearSettings();
}

void TestThemeManager::cleanupTestCase() {
    // Clean up after all tests
    clearSettings();
}

void TestThemeManager::init() {
    // Reset to default theme before each test
    ThemeManager::instance().setTheme(ThemeType::TR4WDefault);
    ThemeManager::instance().clearCustomColors();
}

void TestThemeManager::cleanup() {
    // Clean up after each test
    clearSettings();
}

void TestThemeManager::clearSettings() {
    QSettings settings;
    settings.beginGroup("Theme");
    settings.remove("");  // Remove all keys in Theme group
    settings.endGroup();
}

// Singleton tests

void TestThemeManager::testSingleton_SameInstance() {
    ThemeManager& instance1 = ThemeManager::instance();
    ThemeManager& instance2 = ThemeManager::instance();

    QCOMPARE(&instance1, &instance2);  // Same memory address
}

// Theme tests

void TestThemeManager::testDefaultTheme_IsTR4W() {
    ThemeManager& theme = ThemeManager::instance();
    QCOMPARE(theme.currentTheme(), ThemeType::TR4WDefault);
}

void TestThemeManager::testSetTheme_ChangesCurrentTheme() {
    ThemeManager& theme = ThemeManager::instance();

    theme.setTheme(ThemeType::DarkMode);
    QCOMPARE(theme.currentTheme(), ThemeType::DarkMode);

    theme.setTheme(ThemeType::HighContrast);
    QCOMPARE(theme.currentTheme(), ThemeType::HighContrast);

    theme.setTheme(ThemeType::Custom);
    QCOMPARE(theme.currentTheme(), ThemeType::Custom);
}

void TestThemeManager::testSetTheme_EmitsSignal() {
    ThemeManager& theme = ThemeManager::instance();
    QSignalSpy spy(&theme, &ThemeManager::themeChanged);

    theme.setTheme(ThemeType::DarkMode);

    QCOMPARE(spy.count(), 1);  // Signal emitted once
}

void TestThemeManager::testThemeName_AllThemes() {
    QCOMPARE(ThemeManager::themeName(ThemeType::TR4WDefault), QString("TR4W Default"));
    QCOMPARE(ThemeManager::themeName(ThemeType::DarkMode), QString("Dark Mode"));
    QCOMPARE(ThemeManager::themeName(ThemeType::HighContrast), QString("High Contrast"));
    QCOMPARE(ThemeManager::themeName(ThemeType::Custom), QString("Custom"));
}

// Color retrieval tests

void TestThemeManager::testColor_TR4WDefault_VfoBackground() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::TR4WDefault);

    QColor vfoColor = theme.color(ColorRole::VfoBackground);
    QCOMPARE(vfoColor, QColor("#00FFFF"));  // Cyan
}

void TestThemeManager::testColor_DarkMode_VfoBackground() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::DarkMode);

    QColor vfoColor = theme.color(ColorRole::VfoBackground);
    QCOMPARE(vfoColor, QColor("#006666"));  // Dark cyan
}

void TestThemeManager::testColor_HighContrast_VfoBackground() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::HighContrast);

    QColor vfoColor = theme.color(ColorRole::VfoBackground);
    QCOMPARE(vfoColor, QColor(Qt::black));  // Black
}

void TestThemeManager::testColorName_ReturnsHexString() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::TR4WDefault);

    QString colorName = theme.colorName(ColorRole::VfoBackground);
    QCOMPARE(colorName, QString("#00ffff"));  // Lowercase hex
}

// ColorRole naming tests

void TestThemeManager::testColorRoleName_AllRoles() {
    // Display Colors
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::VfoBackground), QString("VFO Background"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::VfoText), QString("VFO Text"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::WindowBackground), QString("Window Background"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::TextDisplayBackground), QString("Text Display Background"));

    // Status Colors
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::ConnectedStatus), QString("Connected Status"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::DisconnectedStatus), QString("Disconnected Status"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::FrozenIndicator), QString("Frozen Indicator"));

    // Functional Colors
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::DupeText), QString("Duplicate Text"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::NewMultiplierBackground), QString("New Multiplier Background"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::WorkedStationText), QString("Worked Station Text"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::MultiplierText), QString("Multiplier Text"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::NeededMultiplierBackground), QString("Needed Multiplier Background"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::ConfirmedMultiplierBackground), QString("Confirmed Multiplier Background"));

    // UI Colors
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::PrimaryText), QString("Primary Text"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::SecondaryText), QString("Secondary Text"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::HoverHighlight), QString("Hover Highlight"));
    QCOMPARE(ThemeManager::colorRoleName(ColorRole::BorderColor), QString("Border Color"));
}

// Custom color tests

void TestThemeManager::testSetCustomColor_AutoSwitchesToCustomTheme() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::TR4WDefault);

    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));

    QCOMPARE(theme.currentTheme(), ThemeType::Custom);
}

void TestThemeManager::testSetCustomColor_EmitsSignal() {
    ThemeManager& theme = ThemeManager::instance();
    QSignalSpy spy(&theme, &ThemeManager::themeChanged);

    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));

    QVERIFY(spy.count() >= 1);  // At least one signal (maybe two if theme also switched)
}

void TestThemeManager::testCustomColor_ReturnsSetColor() {
    ThemeManager& theme = ThemeManager::instance();

    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));

    QColor customColor = theme.customColor(ColorRole::VfoBackground);
    QCOMPARE(customColor, QColor(Qt::magenta));
}

void TestThemeManager::testHasCustomColor_DetectsCustomColors() {
    ThemeManager& theme = ThemeManager::instance();

    QVERIFY(!theme.hasCustomColor(ColorRole::VfoBackground));

    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));

    QVERIFY(theme.hasCustomColor(ColorRole::VfoBackground));
}

void TestThemeManager::testClearCustomColors_RemovesAll() {
    ThemeManager& theme = ThemeManager::instance();

    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));
    theme.setCustomColor(ColorRole::DupeText, QColor(Qt::yellow));

    QVERIFY(theme.hasCustomColor(ColorRole::VfoBackground));
    QVERIFY(theme.hasCustomColor(ColorRole::DupeText));

    theme.clearCustomColors();

    QVERIFY(!theme.hasCustomColor(ColorRole::VfoBackground));
    QVERIFY(!theme.hasCustomColor(ColorRole::DupeText));
}

void TestThemeManager::testColor_CustomTheme_UsesCustomColor() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::Custom);

    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));

    QColor color = theme.color(ColorRole::VfoBackground);
    QCOMPARE(color, QColor(Qt::magenta));
}

void TestThemeManager::testColor_CustomTheme_FallsBackToDefault() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::Custom);

    // Don't set custom color for DupeText
    QColor color = theme.color(ColorRole::DupeText);

    // Should fall back to TR4W Default (red)
    QCOMPARE(color, QColor(Qt::red));
}

// Save/Load tests

void TestThemeManager::testSaveLoad_PreservesTheme() {
    ThemeManager& theme = ThemeManager::instance();

    // Set to Dark Mode and save
    theme.setTheme(ThemeType::DarkMode);
    theme.saveToSettings();

    // Reset to default
    theme.setTheme(ThemeType::TR4WDefault);
    QCOMPARE(theme.currentTheme(), ThemeType::TR4WDefault);

    // Load from settings
    theme.loadFromSettings();
    QCOMPARE(theme.currentTheme(), ThemeType::DarkMode);
}

void TestThemeManager::testSaveLoad_PreservesCustomColors() {
    ThemeManager& theme = ThemeManager::instance();

    // Set custom colors and save
    theme.setTheme(ThemeType::Custom);
    theme.setCustomColor(ColorRole::VfoBackground, QColor(Qt::magenta));
    theme.setCustomColor(ColorRole::DupeText, QColor(Qt::yellow));
    theme.saveToSettings();

    // Clear custom colors
    theme.clearCustomColors();
    QVERIFY(!theme.hasCustomColor(ColorRole::VfoBackground));

    // Load from settings
    theme.loadFromSettings();
    QCOMPARE(theme.currentTheme(), ThemeType::Custom);
    QCOMPARE(theme.customColor(ColorRole::VfoBackground), QColor(Qt::magenta));
    QCOMPARE(theme.customColor(ColorRole::DupeText), QColor(Qt::yellow));
}

void TestThemeManager::testLoad_InvalidTheme_FallsBackToDefault() {
    // Manually write invalid theme to settings
    QSettings settings;
    settings.setValue("Theme/currentTheme", "InvalidTheme");

    ThemeManager& theme = ThemeManager::instance();
    theme.loadFromSettings();

    QCOMPARE(theme.currentTheme(), ThemeType::TR4WDefault);  // Fallback
}

QTEST_MAIN(TestThemeManager)
#include "test_thememanager.moc"
