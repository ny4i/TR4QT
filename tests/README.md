# TR4QT Unit Tests

Comprehensive unit test suite for TR4QT using Qt Test framework.

## Overview

TR4QT uses the Qt Test framework for unit testing. Tests cover business logic, data validation, formatting, and type conversions. The test infrastructure is integrated with CMake's CTest for easy execution.

**Current Status: Phase 3 Infrastructure Tests Started**
- 7 test suites implemented
- 174 total test cases (all passing)
- Phase 1 (44 tests): LogFormatter, Types, QSO
- Phase 2 (108 tests): CountryFile, CQ WW, CQ WPX
- Phase 3 Infrastructure (22 tests): ThemeManager
- Estimated 40-45% code coverage

## Test Files

### test_logformatter.cpp (15 tests)
Tests TR4W-compatible log message formatting.

**Coverage:**
- `formatTimestamp()`: Date/time formatting with milliseconds
- `formatThreadId()`: Thread ID bracketing
- `format()`: Complete TR4W pattern formatting
- Edge cases: empty messages, special characters, long messages

**Example:**
```cpp
void TestLogFormatter::testFormat_Complete() {
    QDateTime dt(QDate(2025, 12, 24), QTime(18, 14, 2, 968), Qt::UTC);
    QString result = LogFormatter::format(dt, 2650, threadId, LogLevel::Info,
                                         "TR4WDebugLog", "DecimalSeparator = .");
    // Expected: "24 Dec 2025 18:14:02.968 2650 [10252] info TR4WDebugLog - DecimalSeparator = .\n"
}
```

### test_types.cpp (11 tests)
Tests enum/string conversions for Band, Mode, and Continent types.

**Coverage:**
- `bandToString()` / `stringToBand()`: All 14 bands
- `modeToString()` / `stringToMode()`: All 14 modes
- `continentToString()`: All 6 continents
- Round-trip validation (band → string → band = identity)
- Invalid input handling

**Example:**
```cpp
void TestTypes::testBandRoundTrip() {
    QCOMPARE(stringToBand(bandToString(BandType::Band20M)), BandType::Band20M);
    // Verifies bidirectional conversion works correctly
}
```

### test_qso.cpp (18 tests)
Tests QSO model validation and duplicate checking.

**Coverage:**
- `normalizeCallsign()`: Uppercase conversion and trimming
- `isPersisted()`: Database persistence checking (id >= 0)
- `isValid()`: Required field validation (callsign, timestamp, band, mode)
- `getDupeKey()`: Unique key generation for duplicate detection

**Example:**
```cpp
void TestQSO::testGetDupeKey_DifferentBand() {
    QSO qso1, qso2;
    qso1.callsign = qso2.callsign = "W1AW";
    qso1.band = BandType::Band20M;
    qso2.band = BandType::Band40M;  // Different band
    qso1.mode = qso2.mode = ModeType::CW;

    QVERIFY(qso1.getDupeKey() != qso2.getDupeKey());  // Not a dupe
}
```

### test_countryfile.cpp (38 tests - Phase 2)
Tests CTY.DAT file parsing and callsign lookup functionality.

**Coverage:**
- `loadFromFile()`: Country file loading and parsing
- `stripPortable()`: Portable indicator removal (/P, /M, /MM, /QRP, etc.)
- `extractWPXPrefix()`: WPX prefix extraction (W1AW → W1)
- `lookup()`: Callsign lookup with exact matches, longest prefix matching
- Zone overrides (CQ and ITU zones)
- Case insensitivity
- Country data validation

**Example:**
```cpp
void TestCountryFile::testLookup_SimpleUS() {
    CountryData result = m_countryFile.lookup("W1AW");
    QVERIFY(result.isValid());
    QCOMPARE(result.name, QString("United States"));
    QCOMPARE(result.primaryPrefix, QString("K"));
    QCOMPARE(result.cqZone, 5);
    QCOMPARE(result.ituZone, 8);
    QCOMPARE(result.continent, Continent::NA);
}
```

### test_cqww.cpp (33 tests - Phase 2)
Tests CQ World Wide DX Contest rules and scoring.

**Coverage:**
- Exchange validation (RST + CQ Zone 1-40)
- Exchange parsing
- QSO point calculation:
  - Same continent: 1 point
  - Different continent: 3 points (CW), 2 points (SSB)
  - W/VE special rule: 2 points
- Total score formula: QSO points × (Countries + Zones)
- Multiplier tracking (Countries and CQ Zones, per-band)
- Contest metadata and factory methods

**Example:**
```cpp
void TestCQWW::testCalculatePoints_CW_WVE_Rule() {
    CQWWContest contest(ModeType::CW);
    StationInfo myStation;
    myStation.country = "United States";

    QSO qso;
    qso.dxccEntity = "Canada";

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 2);  // W/VE working each other = 2 points
}
```

### test_cqwpx.cpp (37 tests - Phase 2)
Tests CQ WPX (Worked All Prefixes) Contest rules and scoring.

**Coverage:**
- Exchange validation (RST + Serial Number 1-9999)
- Exchange parsing and formatting
- Prefix extraction from callsigns (W1AW → W1, DL1ABC → DL1)
- Portable callsign handling (W1/G3XYZ → W1)
- QSO point calculation:
  - Same continent: 1 point
  - Different continent: 3 points (CW), 2 points (SSB)
  - 160m and 10m: Double points
- Total score formula: QSO points × Total Prefixes
- Multiplier tracking (Prefixes, all-band scope)
- Serial number formatting (zero-padded)

**Example:**
```cpp
void TestCQWPX::testCalculatePoints_CW_160m_Double() {
    CQWPXContest contest(ModeType::CW);
    StationInfo myStation;
    myStation.continent = "NA";

    QSO qso;
    qso.continent = "AS";
    qso.band = BandType::Band160M;  // 160m = double points

    int points = contest.calculateQSOPoints(qso, myStation);
    QCOMPARE(points, 6);  // (3 × 2) = 6 points
}
```

### test_thememanager.cpp (22 tests - Phase 3)
Tests ThemeManager singleton for color customization system.

**Coverage:**
- `instance()`: Singleton pattern verification
- `setTheme()`: Theme switching (TR4W Default, Dark Mode, High Contrast, Custom)
- `color()`: Color retrieval for all 17 ColorRoles across all themes
- `setCustomColor()`: Custom color setting and auto-switch to Custom theme
- `customColor()`: Custom color retrieval
- `hasCustomColor()`: Custom color detection
- `clearCustomColors()`: Bulk removal of custom colors
- `saveToSettings()`: Persistence to QSettings
- `loadFromSettings()`: Loading from QSettings with invalid data handling
- `themeChanged()` signal emission

**Example:**
```cpp
void TestThemeManager::testColor_TR4WDefault_VfoBackground() {
    ThemeManager& theme = ThemeManager::instance();
    theme.setTheme(ThemeType::TR4WDefault);

    QColor vfoColor = theme.color(ColorRole::VfoBackground);
    QCOMPARE(vfoColor, QColor("#00FFFF"));  // Cyan
}
```

## Running Tests

### Run All Tests

```bash
cd /Users/toms/projects/TR4QT
cmake -S . -B build
cd build
make
ctest
```

**Expected output:**
```
Test project /Users/toms/projects/TR4QT/build
    Start 1: test_logformatter
1/7 Test #1: test_logformatter ................   Passed    0.10 sec
    Start 2: test_types
2/7 Test #2: test_types .......................   Passed    0.10 sec
    Start 3: test_qso
3/7 Test #3: test_qso .........................   Passed    0.10 sec
    Start 4: test_countryfile
4/7 Test #4: test_countryfile .................   Passed    0.12 sec
    Start 5: test_cqww
5/7 Test #5: test_cqww ........................   Passed    0.11 sec
    Start 6: test_cqwpx
6/7 Test #6: test_cqwpx .......................   Passed    0.12 sec
    Start 7: test_thememanager
7/7 Test #7: test_thememanager ................   Passed    0.12 sec

100% tests passed, 0 tests failed out of 7
Total Test time (real) =   1.40 sec
```

### Run Tests with Verbose Output

```bash
ctest -V                    # Verbose CTest output
ctest --output-on-failure   # Show output only on failures
```

### Run Specific Test Suite

```bash
ctest -R test_logformatter    # Run only LogFormatter tests
ctest -R test_types           # Run only Types tests
ctest -R test_qso             # Run only QSO tests
ctest -R test_countryfile     # Run only CountryFile tests
ctest -R test_cqww            # Run only CQ WW tests
ctest -R test_cqwpx           # Run only CQ WPX tests
ctest -R test_thememanager    # Run only ThemeManager tests
```

### Run Individual Test Executable

```bash
./tests/test_logformatter      # Direct execution
./tests/test_logformatter -v2  # Verbose Qt Test output
```

### Qt Test Options

```bash
./tests/test_logformatter -functions              # List all test functions
./tests/test_logformatter testFormat_Complete     # Run single test function
./tests/test_logformatter -o results.txt,txt      # Save results to file
```

## Test Structure

All tests follow Qt Test framework conventions:

```cpp
#include <QTest>

class TestClassName : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();        // Called once before all tests
    void cleanupTestCase();     // Called once after all tests
    void init();                // Called before each test function
    void cleanup();             // Called after each test function

    // Test functions - must be private slots
    void testSomething();
    void testSomethingElse();
};

// Test implementations using QCOMPARE/QVERIFY
void TestClassName::testSomething() {
    QCOMPARE(actual, expected);  // Shows both values on failure
    QVERIFY(condition);          // Boolean assertion
}

QTEST_MAIN(TestClassName)
#include "test_classname.moc"  // Required for Qt meta-object
```

## Adding New Tests

### 1. Create Test File

Create `tests/test_newfeature.cpp`:

```cpp
#include <QTest>
#include "../src/module/Feature.h"

using namespace TR4QT;

class TestNewFeature : public QObject {
    Q_OBJECT

private slots:
    void testBasicFunctionality();
    void testEdgeCases();
};

void TestNewFeature::testBasicFunctionality() {
    // Arrange
    Feature feature;

    // Act
    bool result = feature.doSomething();

    // Assert
    QCOMPARE(result, true);
}

void TestNewFeature::testEdgeCases() {
    Feature feature;
    QVERIFY(!feature.doSomethingInvalid());
}

QTEST_MAIN(TestNewFeature)
#include "test_newfeature.moc"
```

### 2. Update CMakeLists.txt

Add to `/Users/toms/projects/TR4QT/tests/CMakeLists.txt`:

```cmake
add_tr4qt_test(test_newfeature
    test_newfeature.cpp
    ../src/module/Feature.cpp      # Include implementation files
    ../src/module/Dependency.cpp   # Add any dependencies
)
```

### 3. Build and Run

```bash
cd build
make
ctest -R test_newfeature
```

## Best Practices

### Test Naming

Use descriptive names that explain what's being tested:

```cpp
void testFunctionName_WhenCondition_ExpectedResult();

// Examples:
void testBandToString_Given160m_ReturnsString160m();
void testIsValid_WhenMissingCallsign_ReturnsFalse();
void testGetDupeKey_SameContact_ReturnsSameKey();
```

### AAA Pattern

Structure tests with Arrange, Act, Assert:

```cpp
void TestClass::testSomething() {
    // Arrange - set up test data
    QSO qso;
    qso.callsign = "W1AW";
    qso.band = BandType::Band20M;

    // Act - execute the code under test
    QString dupeKey = qso.getDupeKey();

    // Assert - verify the result
    QVERIFY(dupeKey.contains("W1AW"));
    QVERIFY(dupeKey.contains("20"));
}
```

### Use QCOMPARE Over QVERIFY

```cpp
// Preferred - shows both values on failure
QCOMPARE(actual, expected);

// Avoid - only shows "false" on failure
QVERIFY(actual == expected);
```

### Test Both Success and Failure Paths

```cpp
void testIsValid_ValidQSO();      // Happy path
void testIsValid_MissingField();  // Error handling
void testIsValid_EmptyInput();    // Edge case
```

### Keep Tests Isolated

- Tests should not depend on each other
- Use `init()`/`cleanup()` for per-test setup/teardown
- Use `initTestCase()`/`cleanupTestCase()` for one-time setup
- Avoid shared state between tests

## CMake Integration

The test infrastructure uses a helper function to simplify test creation:

```cmake
function(add_tr4qt_test test_name)
    # Create executable from all provided source files
    add_executable(${test_name} ${ARGN})

    # Link against Qt Test and other required Qt modules
    target_link_libraries(${test_name} PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
    )

    # Add source directory to include path
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${HAMLIB_INCLUDE_DIRS}
    )

    # Link against Hamlib
    target_link_libraries(${test_name} PRIVATE
        ${HAMLIB_LIBRARIES}
    )

    # Register test with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})

    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30  # 30 second timeout per test
    )
endfunction()
```

## Future Phases

### Phase 3: Data Layer Tests (Planned)
- `test_qsorepository.cpp`: Database operations with in-memory SQLite (~30 tests)
- `test_adifexporter.cpp`: ADIF format validation (~25 tests)
- `test_cabrilloexporter.cpp`: Cabrillo format validation (~20 tests)

**Target**: 50%+ code coverage

## Coverage Goals

| Phase | Test Files | Test Cases | Est. Coverage |
|-------|------------|------------|---------------|
| Phase 1 (Complete) | 3 | 44 | 15-20% |
| **Phase 2** (Current) | 6 | 152 | 35-40% |
| Phase 3 (Planned) | 8+ | ~225+ | 50%+ |

## Troubleshooting

### Tests Fail to Build

**Problem**: Missing Qt6::Test component
```
CMake Error: Could not find a package configuration file provided by "Qt6Test"
```

**Solution**: Ensure Qt6 Test module is installed
```bash
# macOS with Homebrew
brew install qt@6

# Verify Qt6::Test is available
cmake -S . -B build
```

### Test Execution Timeout

**Problem**: Test hangs or exceeds 30 second timeout

**Solution**: Check for infinite loops or blocking operations. Increase timeout in CMakeLists.txt:
```cmake
set_tests_properties(${test_name} PROPERTIES
    TIMEOUT 60  # Increase to 60 seconds
)
```

### MOC File Not Found

**Problem**: `fatal error: 'test_name.moc' file not found`

**Solution**: Ensure you have `#include "test_name.moc"` at the end of your test file and the class uses `Q_OBJECT` macro.

### Private Method Testing

If you need to test private methods, consider:
1. Make them protected and subclass for testing
2. Make them public if they have well-defined behavior worth testing directly
3. Test them indirectly through public API

## References

- **Qt Test Documentation**: https://doc.qt.io/qt-6/qtest-overview.html
- **CTest Documentation**: https://cmake.org/cmake/help/latest/manual/ctest.1.html
- **Implementation Plan**: `/Users/toms/.claude/plans/composed-twirling-quilt.md`
- **TR4QT Source**: `/Users/toms/projects/TR4QT/src/`

## Contributing

When adding new features to TR4QT:

1. **Write tests first** (TDD approach recommended)
2. **Ensure all existing tests pass** before committing
3. **Aim for 80%+ coverage** on new code
4. **Follow naming conventions** for consistency
5. **Document test purpose** in comments if non-obvious

## Success Metrics

✓ All tests pass on first run
✓ Tests run in < 5 seconds total
✓ Tests are isolated (no inter-test dependencies)
✓ QCOMPARE provides useful failure messages
✓ Easy to add new tests (template provided)
✓ CI/CD ready (future integration)

---

**Last Updated**: 2025-12-24
**Phase**: 3 (Infrastructure Tests Started)
**Status**: All tests passing (174/174)
**Test Suites**: 7 (LogFormatter, Types, QSO, CountryFile, CQ WW, CQ WPX, ThemeManager)
