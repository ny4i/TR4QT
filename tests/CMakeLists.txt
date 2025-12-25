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

add_tr4qt_test(test_geographicutils
    test_geographicutils.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/CountryFile.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)
