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
        Qt6::Concurrent
        qt6keychain  # Secure credential storage (needed by AppSettings)
    )

    # Add source directory to include path so tests can access source headers
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${HAMLIB_INCLUDE_DIRS}
        ${qtkeychain_SOURCE_DIR}
        ${qtkeychain_BINARY_DIR}
    )

    # Define TESTS_SOURCE_DIR for test fixture paths
    target_compile_definitions(${test_name} PRIVATE
        TESTS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # Link against Hamlib (needed for types that reference hamlib)
    target_link_libraries(${test_name} PRIVATE
        ${HAMLIB_LIBRARIES}
    )

    # macOS: SystemConfiguration framework for PlatformUtils (NetBIOS name lookup)
    if(APPLE)
        target_link_libraries(${test_name} PRIVATE "-framework SystemConfiguration")
    endif()

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

add_tr4qt_test(test_command_dispatcher
    test_command_dispatcher.cpp
    ../src/commands/CommandDispatcher.cpp
)

add_tr4qt_test(test_frequency_input_service
    test_frequency_input_service.cpp
    ../src/services/FrequencyInputService.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_dxlab_pathfinder
    test_dxlab_pathfinder.cpp
    ../src/services/DXLabPathfinder.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_spot_collector_service
    test_spot_collector_service.cpp
    ../src/services/SpotCollectorService.cpp
    ../src/services/DXLabPathfinder.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_spot_processor_worker
    test_spot_processor_worker.cpp
    ../src/services/SpotProcessorWorker.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/Database.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_spot_processing_service
    test_spot_processing_service.cpp
    ../src/services/SpotProcessingService.cpp
    ../src/data/LOTWUserRepository.cpp
    ../src/data/Database.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/DXCCRepository.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_station_info_service
    test_station_info_service.cpp
    ../src/services/StationInfoService.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/ThemeManager.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_log_export_service
    test_log_export_service.cpp
)

add_tr4qt_test(test_qso_persistence_service
    test_qso_persistence_service.cpp
    ../src/services/QSOPersistenceService.cpp
    ../src/data/QSORepository.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_exchange_memory_service
    test_exchange_memory_service.cpp
    ../src/services/ExchangeMemoryService.cpp
    ../src/data/ExchangeMemoryRepository.cpp
    ../src/data/Database.cpp
    ../src/exchanges/InitialExchangeManager.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/contests/WinterFieldDayContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_qso_logging_coordinator
    test_qso_logging_coordinator.cpp
    ../src/services/QSOLoggingCoordinator.cpp
    ../src/network/UdpBroadcaster.cpp
    ../src/network/UdpBroadcastManager.cpp
    ../src/network/RadioInfo.cpp
    ../src/network/ContactInfo.cpp
    ../src/data/BackupManager.cpp
    ../src/data/Database.cpp
    ../src/controllers/DataIntegrityManager.cpp
    ../src/data/QSORepository.cpp
    ../src/ui/models/QSOTableModel.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/PlatformUtils.cpp
    ../src/utils/ThemeManager.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_qso_logging_service
    test_qso_logging_service.cpp
    ../src/services/QSOLoggingService.cpp
    ../src/services/QSOPersistenceService.cpp
    ../src/services/ExchangeMemoryService.cpp
    ../src/services/QSOLoggingCoordinator.cpp
    ../src/controllers/QSOLogger.cpp
    ../src/network/UdpBroadcaster.cpp
    ../src/network/UdpBroadcastManager.cpp
    ../src/network/RadioInfo.cpp
    ../src/network/ContactInfo.cpp
    ../src/data/BackupManager.cpp
    ../src/data/Database.cpp
    ../src/data/QSORepository.cpp
    ../src/data/ExchangeMemoryRepository.cpp
    ../src/controllers/DataIntegrityManager.cpp
    ../src/ui/models/QSOTableModel.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/CQWWBase.cpp
    ../src/contests/CQWPXContest.cpp
    ../src/contests/CQWPXBase.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/utils/ADIFExporter.cpp
    ../src/utils/CallsignValidator.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/PlatformUtils.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/utils/ThemeManager.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/exchanges/InitialExchangeManager.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_arrlsection
    test_arrlsection.cpp
    ../src/utils/ArrlSectionHelper.cpp
)

# Phase 2 tests - business logic
add_tr4qt_test(test_countryfile
    test_countryfile.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
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

add_tr4qt_test(test_flqp
    test_flqp.cpp
    ../src/contests/QSOPartyContestBase.cpp
    ../src/contests/FloridaQSOPartyContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
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

add_tr4qt_test(test_smeter
    test_smeter.cpp
    ../src/ui/widgets/SMeterWidget.cpp
    ../src/utils/ThemeManager.cpp
)

add_tr4qt_test(test_geographicutils
    test_geographicutils.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/PathManager.cpp
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
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# Phase 1 CLAUDE.md compliance tests - written BEFORE extraction
add_tr4qt_test(test_contest_exchange_update
    test_contest_exchange_update.cpp
    ../src/contests/WinterFieldDayContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/controllers/ContestManager.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/data/Database.cpp
    ../src/data/QSORepository.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_duplicate_checking
    test_duplicate_checking.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/CQWPXContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/data/Database.cpp
    ../src/data/QSORepository.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_integrity_checking
    test_integrity_checking.cpp
    ../src/data/Database.cpp
    ../src/data/QSORepository.cpp
    ../src/ui/models/QSOTableModel.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/ThemeManager.cpp
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
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

add_tr4qt_test(test_naqp
    test_naqp.cpp
    ../src/contests/NAQPSSBContest.cpp
    ../src/contests/NAQPBase.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# Contest Config Fields tests (Contest Wizard dynamic configuration)
add_tr4qt_test(test_contest_config_fields
    test_contest_config_fields.cpp
    ../src/contests/NAQPSSBContest.cpp
    ../src/contests/NAQPCWContest.cpp
    ../src/contests/NAQPBase.cpp
    ../src/contests/WinterFieldDayContest.cpp
    ../src/contests/ARRLSSCWContest.cpp
    ../src/contests/ARRLSSSSBContest.cpp
    ../src/contests/ARRLSSBase.cpp
    ../src/contests/ARRLSweepstakesContest.cpp
    ../src/contests/CQWWCWContest.cpp
    ../src/contests/CQWWBase.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
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

# ADIF import tests
add_tr4qt_test(test_adif_import
    test_adif_import.cpp
    ../src/importers/ADIFParser.cpp
    ../src/importers/ADIFFieldMapper.cpp
    ../src/importers/ADIFImporter.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# ADIF mapping demo (shows field mapping for first record)
add_tr4qt_test(test_adif_mapping_demo
    test_adif_mapping_demo.cpp
    ../src/importers/ADIFParser.cpp
    ../src/importers/ADIFFieldMapper.cpp
    ../src/importers/ADIFImporter.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# QSO Logging tests (QSOLogger class)
add_tr4qt_test(test_qso_logging
    test_qso_logging.cpp
    ../src/controllers/QSOLogger.cpp
    ../src/utils/CallsignValidator.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/ArrlSectionHelper.cpp
    ../src/exchanges/SmartExchangeParser.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/Database.cpp
    ../src/data/DXCCRepository.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# Data Integrity tests (DataIntegrityManager class)
add_tr4qt_test(test_data_integrity
    test_data_integrity.cpp
    ../src/controllers/DataIntegrityManager.cpp
    ../src/data/QSORepository.cpp
    ../src/data/Database.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/PathManager.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# K4Radio Simulator Integration tests (Direct K4 protocol implementation)
# Tests K4Radio's direct Elecraft K4 protocol using Hamlib's K4 simulator
# Note: Requires Hamlib K4 simulator built at ../hamlib/build/simelecraftk4
#       Test will SKIP gracefully if simulator not found (optional integration test)
add_executable(test_k4radio_simulator
    test_k4radio_simulator.cpp
    TcpToPtyBridge.h  # Glue code: bridges TCP (K4Radio) to PTY (simulator)
)

# Link against tr4qt_core object library (provides K4Radio, logging, types)
target_link_libraries(test_k4radio_simulator PRIVATE
    tr4qt_core  # Object library with K4Radio, RadioInterface, Logger, Types
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
    ${HAMLIB_LIBRARIES}
)

# Add include directories
target_include_directories(test_k4radio_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${HAMLIB_INCLUDE_DIRS}
)

# Register with CTest
add_test(NAME test_k4radio_simulator COMMAND test_k4radio_simulator)

# Set longer timeout for simulator integration tests (startup overhead)
set_tests_properties(test_k4radio_simulator PROPERTIES TIMEOUT 60)

# FloatingDate calculation tests (contest date scheduling)
add_tr4qt_test(test_floating_dates
    test_floating_dates.cpp
    ../src/contests/ContestMetadata.cpp
    ../src/core/Types.cpp
)

# Window persistence tests (validates window state restoration on startup)
add_tr4qt_test(test_window_persistence
    test_window_persistence.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/FontManager.cpp
    ../src/utils/PathManager.cpp
    ../src/ui/managers/SettingsManager.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# StatusNotifier tests (pure function: formatMessage + styleForEvent)
add_tr4qt_test(test_status_notifier
    test_status_notifier.cpp
    ../src/services/StatusNotifier.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# QSOQueryService tests (worked callsigns, bands, time windows, rate)
add_tr4qt_test(test_qso_query_service
    test_qso_query_service.cpp
    ../src/services/QSOQueryService.cpp
    ../src/services/StationInfoService.cpp
    ../src/utils/CountryFile.cpp
    ../src/utils/GeographicUtils.cpp
    ../src/utils/ThemeManager.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/data/DXCCRepository.cpp
    ../src/data/GlobalDatabase.cpp
    ../src/data/Database.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# ScoreCalculationService tests (scoring, multipliers, band stats)
add_tr4qt_test(test_score_calculation_service
    test_score_calculation_service.cpp
    ../src/services/ScoreCalculationService.cpp
    ../src/contests/CQWWContest.cpp
    ../src/contests/CQWWCWContest.cpp
    ../src/contests/CQWWBase.cpp
    ../src/contests/CQWPXContest.cpp
    ../src/contests/CQWPXCWContest.cpp
    ../src/contests/CQWPXBase.cpp
    ../src/contests/ContestRegistry.cpp
    ../src/contests/ContestMetadata.cpp
    ../src/core/Types.cpp
    ../src/logging/Logger.cpp
    ../src/logging/LogLevel.cpp
    ../src/logging/LogFormatter.cpp
    ../src/logging/FileAppender.cpp
)

# CWService tests (adjustWPM clamping, preconditions, signals)
# Uses tr4qt_core for RadioController (heavy transitive deps)
add_executable(test_cw_service
    test_cw_service.cpp
    ../src/services/CWService.cpp
    ../src/services/CWService.h
    ../src/cw/CWTemplateEngine.cpp
    ../src/cw/CWSenderFactory.cpp
    ../src/cw/CWSender.h
    ../src/cw/HamlibCWSender.cpp
    ../src/cw/HamlibCWSender.h
    ../src/cw/KeyerCWSender.cpp
    ../src/cw/KeyerCWSender.h
    ../src/cw/DtrRtsCWSender.cpp
    ../src/cw/DtrRtsCWSender.h
    ../src/cw/MorseEncoder.cpp
    ../src/keyers/KeyerController.cpp
    ../src/keyers/KeyerController.h
    ../src/keyers/KeyerFactory.cpp
    ../src/keyers/ICWKeyerDevice.h
    ../src/keyers/IambicKeyer.cpp
    ../src/keyers/IambicKeyer.h
    ../src/keyers/WinKeyerDevice.cpp
    ../src/keyers/WinKeyerDevice.h
    ../src/keyers/HaliKeySerialDevice.cpp
    ../src/keyers/HaliKeySerialDevice.h
    ../src/keyers/HaliKeyMidiDevice.cpp
    ../src/keyers/HaliKeyMidiDevice.h
    ../src/3rdparty/rtmidi/RtMidi.cpp
    ../src/utils/AppSettings.cpp
    ../src/utils/CredentialStore.cpp
    ../src/utils/PathManager.cpp
    ../src/utils/PerformanceProfiler.cpp
    ../src/network/UdpBroadcaster.cpp
    ../src/network/UdpBroadcaster.h
    ../src/network/ContactInfo.cpp
    ../src/network/RadioInfo.cpp
)

target_link_libraries(test_cw_service PRIVATE
    tr4qt_core
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::SerialPort
    ${HAMLIB_LIBRARIES}
    qt6keychain
)

target_include_directories(test_cw_service PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${HAMLIB_INCLUDE_DIRS}
    ${qtkeychain_SOURCE_DIR}
    ${qtkeychain_BINARY_DIR}
)

# macOS: SystemConfiguration framework for PlatformUtils
if(APPLE)
    target_link_libraries(test_cw_service PRIVATE "-framework SystemConfiguration")
endif()

# RtMidi needs CoreMIDI on macOS, ALSA on Linux
if(APPLE)
    target_link_libraries(test_cw_service PRIVATE "-framework CoreMIDI" "-framework CoreAudio" "-framework CoreFoundation")
    target_compile_definitions(test_cw_service PRIVATE __MACOSX_CORE__)
elseif(UNIX)
    target_link_libraries(test_cw_service PRIVATE asound)
    target_compile_definitions(test_cw_service PRIVATE __LINUX_ALSA__)
elseif(WIN32)
    target_link_libraries(test_cw_service PRIVATE winmm)
    target_compile_definitions(test_cw_service PRIVATE __WINDOWS_MM__)
endif()

# Enable AUTOMOC for CW/keyer headers with Q_OBJECT
set_target_properties(test_cw_service PROPERTIES AUTOMOC ON)

add_test(NAME test_cw_service COMMAND test_cw_service)
set_tests_properties(test_cw_service PROPERTIES TIMEOUT 30)

# Rotator service tests (Issue #60 - PSTRotator support)
# Tests rotator control system with mock rotator
add_executable(test_rotator_service
    test_rotator_service.cpp
)

# Link against tr4qt_core object library (provides rotator classes)
target_link_libraries(test_rotator_service PRIVATE
    tr4qt_core  # Object library with rotator classes, logging, types
    Qt6::Test
    Qt6::Core
    Qt6::Network
)

# Add include directories
target_include_directories(test_rotator_service PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link additional source files needed by RotatorService
# Note: RotatorController runs device in worker thread (Issue #69)
target_sources(test_rotator_service PRIVATE
    ../src/services/RotatorService.cpp
    ../src/controllers/RotatorController.cpp
)

# Register with CTest
add_test(NAME test_rotator_service COMMAND test_rotator_service)

# Set test properties
set_tests_properties(test_rotator_service PROPERTIES TIMEOUT 30)
