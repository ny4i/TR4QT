# TR4QT Source Files

# Create object library for core components (enables testing without full app linking)
# This library contains radio control, logging, and other testable components
add_library(tr4qt_core OBJECT
    # Radio control (K4Radio, HamlibRadio, etc.)
    radio/RadioInterface.h
    radio/RadioInterface.cpp
    radio/K4Radio.cpp
    radio/K4Radio.h
    radio/HamlibRadio.cpp
    radio/HamlibRadio.h
    radio/IcomRadio.cpp
    radio/IC7760Radio.cpp
    radio/IC7760Radio.h
    radio/IC9700Radio.cpp
    radio/IC9700Radio.h
    radio/CWTiming.cpp
    radio/RadioFactory.cpp
    radio/RadioController.cpp
    radio/RadioPreflightHelper.cpp
    radio/HamRadioPrivileges.cpp
    radio/icomnetwork.cpp

    # Rotator control (PSTRotator, future: GS-232, etc.)
    rotator/IRotatorController.h
    rotator/PSTRotatorController.cpp
    rotator/PSTRotatorController.h
    rotator/RotatorFactory.cpp
    rotator/RotatorFactory.h

    # Core types
    core/Types.cpp
    core/Types.h
    core/Constants.h

    # Logging system
    logging/Logger.cpp
    logging/Logger.h
    logging/LogLevel.cpp
    logging/LogLevel.h
    logging/LogFormatter.cpp
    logging/LogFormatter.h
    logging/FileAppender.cpp
    logging/FileAppender.h
    logging/LogMacros.h
)

# Object library needs same includes/links as main app
target_include_directories(tr4qt_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAMLIB_INCLUDE_DIRS}
)

target_link_libraries(tr4qt_core PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::SerialPort
    ${HAMLIB_LIBRARIES}
)

set(SOURCES
    main.cpp
    utils/ADIFExporter.cpp
    utils/AppSettings.cpp
    utils/ArrlSectionHelper.cpp
    utils/CabrilloExporter.cpp
    utils/CallsignValidator.cpp
    utils/CountryFile.cpp
    utils/CountryFileDownloader.cpp
    utils/SCPDownloader.cpp
    utils/SCPCallsignExtractor.cpp
    utils/SCPMatcher.cpp
    utils/K4Discovery.cpp
    utils/IcomDiscovery.cpp
    utils/DialogHelper.cpp
    utils/DXClusterListDownloader.cpp
    utils/SelectableMessageBox.cpp
    utils/GeographicUtils.cpp
    utils/LOTWUserDownloader.cpp
    utils/MapDataProvider.cpp
    utils/PathManager.cpp
    utils/RadioEnumerator.cpp
    utils/ThemeManager.cpp
    utils/PerformanceProfiler.cpp
    # Radio files now in tr4qt_core object library
    network/TelnetClient.cpp
    network/RadioInfo.cpp
    network/ContactInfo.cpp
    network/UdpBroadcaster.cpp
    network/UdpBroadcastManager.cpp
    network/WebServer.cpp
    data/Database.cpp
    data/GlobalDatabase.cpp
    data/QSORepository.cpp
    data/ContestRepository.cpp
    data/LOTWUserRepository.cpp
    data/SpotRepository.cpp
    data/BackupManager.cpp
    data/ExchangeMemoryRepository.cpp
    data/DXCCRepository.cpp
    data/SCPRepository.cpp
    exchanges/InitialExchangeManager.cpp
    exchanges/SmartExchangeParser.cpp
    importers/ADIFParser.cpp
    importers/ADIFFieldMapper.cpp
    importers/ADIFImporter.cpp
    cw/HamlibCWSender.cpp
    cw/CWSenderFactory.cpp
    cw/CWTemplateEngine.cpp
    contests/QSOPartyContestBase.cpp
    contests/ARRLDXContest.cpp
    contests/ARRLDXBase.cpp
    contests/ARRLDXCWContest.cpp
    contests/ARRLDXPhoneContest.cpp
    contests/ARRLRTTYRoundupContest.cpp
    contests/ARRLSweepstakesContest.cpp
    contests/ARRLSSBase.cpp
    contests/ARRLSSCWContest.cpp
    contests/ARRLSSSSBContest.cpp
    contests/ContestRegistry.cpp
    contests/ContestMetadata.cpp
    contests/CQWWContest.cpp
    contests/CQWWBase.cpp
    contests/CQWWCWContest.cpp
    contests/CQWWSSBContest.cpp
    contests/CQWPXContest.cpp
    contests/CQWPXBase.cpp
    contests/CQWPXCWContest.cpp
    contests/CQWPXSSBContest.cpp
    contests/FloridaQSOPartyContest.cpp
    contests/GeneralLoggingContest.cpp
    contests/NAQPBase.cpp
    contests/NAQPCWContest.cpp
    contests/NAQPSSBContest.cpp
    contests/NAQPRTTYContest.cpp
    contests/WinterFieldDayContest.cpp
    contests/ARRLFieldDayContest.cpp
    contests/IARUHFContest.cpp
    # Logging files now in tr4qt_core object library
    controllers/QSOLogger.cpp
    controllers/DataIntegrityManager.cpp
    controllers/ContestManager.cpp
    controllers/ContestService.cpp
    controllers/ImportExportManager.cpp
    controllers/RadioManager.cpp
    controllers/DownloadManager.cpp
    controllers/CWMessageManager.cpp
    controllers/BandSwitchingManager.cpp
    controllers/KPA1500UdpPoller.cpp
    commands/CommandDispatcher.cpp
    services/QSOPersistenceService.cpp
    services/ExchangeMemoryService.cpp
    services/QSOLoggingCoordinator.cpp
    services/QSOLoggingService.cpp
    services/StationInfoService.cpp
    services/ScoreCalculationService.cpp
    services/LogExportService.cpp
    services/FrequencyInputService.cpp
    services/SpotProcessingService.cpp
    services/QSOQueryService.cpp
    services/RotatorService.cpp
    ui/MainWindow.cpp
    ui/NativeMapViewer.cpp
    ui/models/QSOTableModel.cpp
    ui/controllers/PlaceholderActions.cpp
    ui/managers/SettingsManager.cpp
    ui/managers/MenuManager.cpp
    ui/managers/WindowManager.cpp
    ui/dialogs/PreferencesDialog.cpp
    ui/dialogs/OperatorDialog.cpp
    ui/dialogs/ContestChooserDialog.cpp
    ui/dialogs/BackupRestoreDialog.cpp
    ui/dialogs/EditQSODialog.cpp
    ui/dialogs/ExportPreviewDialog.cpp
    ui/dialogs/SendMorseDialog.cpp
    ui/dialogs/CWMessageEditorDialog.cpp
    ui/dialogs/MacroEditDialog.cpp
    ui/dialogs/FunctionKeysWindow.cpp
    ui/dialogs/ADIFImportDialog.cpp
    ui/dialogs/GraylineMapDialog.cpp
    ui/widgets/BandSummaryGrid.cpp
    ui/widgets/BandMapWidget.cpp
    ui/widgets/CivAddressWidget.cpp
    ui/widgets/RadioControlWidget.cpp
    ui/widgets/SMeterWidget.cpp
    ui/widgets/MultiplierWidget.cpp
    ui/widgets/DXClusterWindow.cpp
    ui/widgets/StatisticsWindow.cpp
    ui/widgets/NeedsDisplayWidget.cpp
    ui/plots/qcustomplot.cpp
    3rdparty/miniz/miniz.cpp
)

set(HEADERS
    core/Types.h
    core/Constants.h
    utils/ADIFExporter.h
    utils/AppSettings.h
    utils/ArrlSectionHelper.h
    utils/CabrilloExporter.h
    utils/CallsignValidator.h
    utils/CountryFile.h
    utils/CountryFileDownloader.h
    utils/SCPDownloader.h
    utils/SCPCallsignExtractor.h
    utils/SCPMatcher.h
    utils/K4Discovery.h
    utils/IcomDiscovery.h
    utils/DialogHelper.h
    utils/DXClusterListDownloader.h
    utils/SelectableMessageBox.h
    utils/GeographicUtils.h
    utils/LOTWUserDownloader.h
    utils/PathManager.h
    utils/RadioEnumerator.h
    utils/ThemeManager.h
    # Radio headers now in tr4qt_core object library (removed to prevent duplicate MOC)
    network/TelnetClient.h
    network/RadioInfo.h
    network/ContactInfo.h
    network/UdpBroadcaster.h
    network/UdpBroadcastManager.h
    network/WebServer.h
    data/Database.h
    data/GlobalDatabase.h
    data/QSORepository.h
    data/LOTWUserRepository.h
    data/SpotRepository.h
    data/BackupManager.h
    data/ExchangeMemoryRepository.h
    data/DXCCRepository.h
    data/SCPRepository.h
    exchanges/InitialExchangeManager.h
    exchanges/SmartExchangeParser.h
    importers/ADIFParser.h
    importers/ADIFFieldMapper.h
    importers/ADIFImporter.h
    cw/CWSender.h
    cw/HamlibCWSender.h
    cw/CWSenderFactory.h
    cw/CWTemplateEngine.h
    contests/ARRLDXContest.h
    contests/ARRLRTTYRoundupContest.h
    contests/ARRLSweepstakesContest.h
    contests/ContestBase.h
    contests/QSOPartyContestBase.h
    contests/ContestMetadata.h
    contests/ContestRegistry.h
    contests/CQWWContest.h
    contests/CQWPXContest.h
    contests/FloridaQSOPartyContest.h
    contests/GeneralLoggingContest.h
    contests/NAQPContest.h
    contests/WinterFieldDayContest.h
    contests/ARRLFieldDayContest.h
    contests/IARUHFContest.h
    # Logging headers now in tr4qt_core object library (removed to prevent duplicate MOC)
    controllers/QSOLogger.h
    controllers/DataIntegrityManager.h
    controllers/ContestManager.h
    controllers/ImportExportManager.h
    controllers/RadioManager.h
    controllers/DownloadManager.h
    controllers/CWMessageManager.h
    controllers/BandSwitchingManager.h
    models/QSO.h
    models/StationInfo.h
    ui/MainWindow.h
    ui/models/QSOTableModel.h
    ui/managers/SettingsManager.h
    ui/managers/MenuManager.h
    ui/managers/WindowManager.h
    ui/dialogs/PreferencesDialog.h
    ui/dialogs/OperatorDialog.h
    ui/dialogs/ContestChooserDialog.h
    ui/dialogs/BackupRestoreDialog.h
    ui/dialogs/EditQSODialog.h
    ui/dialogs/ExportPreviewDialog.h
    ui/dialogs/SendMorseDialog.h
    ui/dialogs/CWMessageEditorDialog.h
    ui/dialogs/MacroEditDialog.h
    ui/dialogs/FunctionKeysWindow.h
    ui/dialogs/ADIFImportDialog.h
    ui/widgets/BandSummaryGrid.h
    ui/widgets/BandMapWidget.h
    ui/widgets/CivAddressWidget.h
    ui/widgets/RadioControlWidget.h
    ui/widgets/SMeterWidget.h
    ui/widgets/MultiplierWidget.h
    ui/widgets/DXClusterWindow.h
    ui/widgets/StatisticsWindow.h
    ui/widgets/NeedsDisplayWidget.h
    ui/plots/qcustomplot.h
)

# Set icon file for macOS
set(MACOSX_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/icons/tr4qt.icns")

# Create executable
if(APPLE)
    add_executable(tr4qt MACOSX_BUNDLE
        ${SOURCES}
        ${HEADERS}
        ../resources/resources.qrc
        ${MACOSX_ICON_FILE}
    )

    # Set macOS bundle properties
    # IMPORTANT: Update version here AND in src/core/Constants.h AND installer/tr4qt.nsi
    set_target_properties(tr4qt PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "TR4QT"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.tr4qt.TR4QT"
        MACOSX_BUNDLE_ICON_FILE "tr4qt.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "3.38.37"
        MACOSX_BUNDLE_BUNDLE_VERSION "3.38.37"
        RESOURCE "${MACOSX_ICON_FILE}"
    )
elseif(WIN32)
    # Windows: Build as GUI application (no console window)
    # Include resource file for version information and manifest
    add_executable(tr4qt WIN32
        ${SOURCES}
        ${HEADERS}
        ../resources/resources.qrc
        ../resources/tr4qt.rc
    )
else()
    # Linux: Standard executable
    add_executable(tr4qt
        ${SOURCES}
        ${HEADERS}
        ../resources/resources.qrc
    )
endif()

# Link Qt6 libraries and tr4qt_core object library
target_link_libraries(tr4qt PRIVATE
    tr4qt_core  # Core components (radio, logging, types)
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::HttpServer
    Qt6::PrintSupport
    Qt6::Concurrent
    Qt6::SerialPort
)

# Note: Qt6::EntryPoint is not needed for console applications
# Windows builds as console app due to Qt 6.10.1 MinGW compatibility issues

# Link Hamlib (will be used when we add radio support)
target_include_directories(tr4qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAMLIB_INCLUDE_DIRS}
)

target_link_libraries(tr4qt PRIVATE
    ${HAMLIB_LIBRARIES}
)

# Set include directories
target_include_directories(tr4qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Suppress deprecation warnings for third-party qcustomplot.cpp
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set_source_files_properties(ui/plots/qcustomplot.cpp PROPERTIES
        COMPILE_FLAGS "-Wno-deprecated-declarations"
    )
endif()

# macOS: Automatically bundle Qt frameworks and fix library paths after build
# Skip in CI environments - GitHub Actions has its own bundling process
if(APPLE AND NOT DEFINED ENV{CI})
    add_custom_command(TARGET tr4qt POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/macos-bundle.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running macOS app bundling script..."
        VERBATIM
    )
endif()
