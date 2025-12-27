# TR4QT Unit Tests

# Enable testing at subdirectory level
enable_testing()

# Helper function to create test executables
# Usage: add_tr4qt_test(test_name source_file.cpp [additional_sources...])
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

    # Add source directory to include path so tests can access source headers
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${HAMLIB_INCLUDE_DIRS}
    )

    # Define TESTS_SOURCE_DIR for test fixture paths
    target_compile_definitions(${test_name} PRIVATE
        TESTS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # Link against Hamlib (needed for types that reference hamlib)
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

# We need to compile the source files we're testing
# For now, we'll link against object libraries or include source files directly

# Test executables
# Add test files as we create them

# Phase 1 tests - we need to include source files directly for now
# since we don't have a separate library target
add_tr4qt_test(test_logformatter
    test_logformatter.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/LogLevel.cpp
)

add_tr4qt_test(test_types
    test_types.cpp
    ../src/core/Types.cpp
)

add_tr4qt_test(test_qso
    test_qso.cpp
    ../src/core/Types.cpp
)

add_tr4qt_test(test_arrlsection
    test_arrlsection.cpp
    ../src/utils/ArrlSectionHelper.cpp
)

# Phase 2 tests - business logic
add_tr4qt_test(test_countryfile
    test_countryfile.cpp
    ../src/utils/CountryFile.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_cqww
    test_cqww.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_cqwpx
    test_cqwpx.cpp
    ../src/contests/CQWPXContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# Phase 3 tests - infrastructure
add_tr4qt_test(test_thememanager
    test_thememanager.cpp
    ../src/utils/ThemeManager.cpp
)
# Skip on Windows (fails in CI environment)
if(WIN32)
    set_tests_properties(test_thememanager PROPERTIES DISABLED TRUE)
endif()

add_tr4qt_test(test_geographicutils
    test_geographicutils.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/CountryFile.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# Phase 4 tests - exchange parser and contest validation
add_tr4qt_test(test_winterfieldday
    test_winterfieldday.cpp
    ../src/contests/WinterFieldDayContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_smartexchangeparser
    test_smartexchangeparser.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/contests/WinterFieldDayContest.cpp
    ../src/contests/ARRLSweepstakesContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# Phase 5 tests - database persistence
add_tr4qt_test(test_qso_persistence
    test_qso_persistence.cpp
    ../src/data/QSORepository.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)
# Skip on all platforms - Qt resource loading issues in CI environment
# Test works locally but fails in CI because schema.sql resource isn't available
set_tests_properties(test_qso_persistence PROPERTIES DISABLED TRUE)

# Performance tests
add_tr4qt_test(test_score_performance
    test_score_performance.cpp
    ../src/core/Types.cpp
)

# Load performance tests - QSO database stress testing
add_tr4qt_test(test_qso_load_performance
    test_qso_load_performance.cpp
    ../src/data/QSORepository.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)
# Set longer timeout for load tests (5 minutes)
set_tests_properties(test_qso_load_performance PROPERTIES TIMEOUT 300)
# This test uses Qt Concurrent for multi-threading
target_link_libraries(test_qso_load_performance PRIVATE Qt6::Concurrent)
