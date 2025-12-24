# TR4QT Unit Tests

Comprehensive unit test suite for TR4QT using Qt Test framework.

## Overview

TR4QT uses the Qt Test framework for unit testing. Tests cover business logic, data validation, formatting, and type conversions. The test infrastructure is integrated with CMake's CTest for easy execution.

**Current Status: Phase 1 Complete**
- 3 test suites implemented
- 44 total test cases
- All tests passing
- Estimated 15-20% code coverage

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
1/3 Test #1: test_logformatter ................   Passed    0.10 sec
    Start 2: test_types
2/3 Test #2: test_types .......................   Passed    0.10 sec
    Start 3: test_qso
3/3 Test #3: test_qso .........................   Passed    0.10 sec

100% tests passed, 0 tests failed out of 3
Total Test time (real) =   0.30 sec
```

### Run Tests with Verbose Output

```bash
ctest -V                    # Verbose CTest output
ctest --output-on-failure   # Show output only on failures
```

### Run Specific Test Suite

```bash
ctest -R test_logformatter  # Run only LogFormatter tests
ctest -R test_types         # Run only Types tests
ctest -R test_qso           # Run only QSO tests
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

### Phase 2: Business Logic Tests (Planned)
- `test_countryfile.cpp`: Country file parsing and callsign lookup (~40 tests)
- `test_cqww.cpp`: CQ WW contest scoring and validation (~30 tests)
- `test_cqwpx.cpp`: CQ WPX prefix extraction and scoring (~25 tests)

**Target**: 35-40% code coverage

### Phase 3: Data Layer Tests (Planned)
- `test_qsorepository.cpp`: Database operations with in-memory SQLite (~30 tests)
- `test_adifexporter.cpp`: ADIF format validation (~25 tests)
- `test_cabrilloexporter.cpp`: Cabrillo format validation (~20 tests)

**Target**: 50%+ code coverage

## Coverage Goals

| Phase | Test Files | Test Cases | Est. Coverage |
|-------|------------|------------|---------------|
| **Phase 1** (Current) | 3 | 44 | 15-20% |
| Phase 2 (Planned) | 6 | ~170 | 35-40% |
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
**Phase**: 1 (Infrastructure Complete)
**Status**: All tests passing (44/44)
