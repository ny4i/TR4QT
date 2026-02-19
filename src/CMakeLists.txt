# TR4QT Source Files

# Check Qt version and GuiPrivate availability for panadapter support
# QQuickRhiItem requires Qt 6.6+ AND Qt6::GuiPrivate (private headers)
if(Qt6_VERSION VERSION_GREATER_EQUAL "6.6.0" AND Qt6GuiPrivate_FOUND)
    set(PANADAPTER_SUPPORTED TRUE)
    message(STATUS "Panadapter: ENABLED (Qt ${Qt6_VERSION} >= 6.6.0, GuiPrivate found)")
elseif(NOT Qt6_VERSION VERSION_GREATER_EQUAL "6.6.0")
    set(PANADAPTER_SUPPORTED FALSE)
    message(STATUS "Panadapter: DISABLED (Qt ${Qt6_VERSION} < 6.6.0, QQuickRhiItem not available)")
else()
    set(PANADAPTER_SUPPORTED FALSE)
    message(STATUS "Panadapter: DISABLED (Qt6::GuiPrivate not found - install qt6-base-private-dev)")
endif()

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

    # Rotator control (PSTRotator, Hamlib, etc.)
    rotator/IRotatorController.h
    rotator/PSTRotatorController.cpp
    rotator/PSTRotatorController.h
    rotator/HamlibRotator.cpp
    rotator/HamlibRotator.h
    rotator/RotatorFactory.cpp
    rotator/RotatorFactory.h

    # Amplifier control (KPA1500, Hamlib, etc.)
    amplifiers/IAmplifierController.h
    amplifiers/HamlibAmplifier.cpp
    amplifiers/HamlibAmplifier.h
    amplifiers/KPA1500Direct.cpp
    amplifiers/KPA1500Direct.h
    amplifiers/AmplifierFactory.cpp
    amplifiers/AmplifierFactory.h

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
    utils/FontManager.cpp
    utils/PlatformUtils.cpp
    utils/ReconnectionManager.cpp
    utils/CredentialStore.cpp
    utils/PerformanceProfiler.cpp
    # Radio files now in tr4qt_core object library
    network/TelnetClient.cpp
    network/RadioInfo.cpp
    network/ContactInfo.cpp
    network/UdpBroadcaster.cpp
    network/UdpBroadcastManager.cpp
    network/WebServer.cpp
    network/WebServerContext.cpp
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
    cw/KeyerCWSender.cpp
    cw/DtrRtsCWSender.cpp
    cw/MorseEncoder.cpp
    cw/CWSenderFactory.cpp
    cw/CWTemplateEngine.cpp
    keyers/KeyerController.cpp
    keyers/KeyerFactory.cpp
    keyers/WinKeyerDevice.cpp
    keyers/HaliKeySerialDevice.cpp
    keyers/HaliKeyMidiDevice.cpp
    keyers/IambicKeyer.cpp
    audio/SidetoneGenerator.cpp
    3rdparty/rtmidi/RtMidi.cpp
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
    controllers/BandSwitchingManager.cpp
    controllers/KPA1500UdpPoller.cpp
    controllers/AmplifierController.cpp
    controllers/RotatorController.cpp
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
    services/SpotProcessorWorker.cpp
    services/DXLabPathfinder.cpp
    services/QSOQueryService.cpp
    services/RotatorService.cpp
    services/AmplifierService.cpp
    services/MaintenanceService.cpp
    services/InputHandlerService.cpp
    services/QSOSearchService.cpp
    services/CWService.cpp
    services/StatusNotifier.cpp
    ui/MainWindow.cpp
    ui/NativeMapViewer.cpp
    ui/models/QSOTableModel.cpp
    ui/controllers/PlaceholderActions.cpp
    ui/managers/SettingsManager.cpp
    ui/managers/MenuManager.cpp
    ui/managers/WindowManager.cpp
    ui/helpers/WindowActivationHelper.cpp
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
    ui/dialogs/QSOSearchDialog.cpp
    ui/dialogs/GraylineMapDialog.cpp
    ui/dialogs/SO2RConfigDialog.cpp
    ui/dialogs/RadioEditDialog.cpp
    ui/dialogs/CWOutputEditDialog.cpp
    ui/dialogs/KeyerSetupDialog.cpp
    ui/widgets/BandSummaryGrid.cpp
    ui/widgets/BandMapWidget.cpp
    ui/widgets/CivAddressWidget.cpp
    ui/widgets/RadioControlWidget.cpp
    ui/widgets/SMeterWidget.cpp
    ui/widgets/MultiplierWidget.cpp
    ui/widgets/DXClusterWindow.cpp
    ui/statistics/StatisticsWindow.cpp
    ui/statistics/RateCalculator.cpp
    ui/statistics/RateDisplayWidget.cpp
    ui/statistics/BandSummaryModel.cpp
    ui/widgets/NeedsDisplayWidget.cpp
    ui/widgets/QSOSearchPanel.cpp
    ui/widgets/SvgPanelWidget.cpp
    ui/windows/AmplifierControlWindow.cpp
    ui/panels/IAmplifierPanelController.h
    ui/panels/KPA1500PanelController.cpp
    ui/panels/AmplifierPanelFactory.cpp
    ui/plots/qcustomplot.cpp
    3rdparty/miniz/miniz.cpp
)

# Conditionally add panadapter sources (requires Qt 6.6+)
if(PANADAPTER_SUPPORTED)
    list(APPEND SOURCES
        ui/windows/PanadapterWindow.cpp
        panadapter/PanadapterDataModel.cpp
        panadapter/renderers/QmlPanadapterRenderer.cpp
        panadapter/WaterfallImageProvider.cpp
        panadapter/WaterfallRhiItem.cpp
        panadapter/SpectrumRhiItem.cpp
        radio/K4PanadapterReader.cpp
    )
endif()

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
    utils/FontManager.h
    utils/PlatformUtils.h
    utils/ReconnectionManager.h
    utils/CredentialStore.h
    # Radio headers now in tr4qt_core object library (removed to prevent duplicate MOC)
    network/TelnetClient.h
    network/RadioInfo.h
    network/ContactInfo.h
    network/UdpBroadcaster.h
    network/UdpBroadcastManager.h
    network/WebServer.h
    network/WebServerContext.h
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
    cw/KeyerCWSender.h
    cw/DtrRtsCWSender.h
    cw/MorseTable.h
    cw/MorseEncoder.h
    cw/CWSenderFactory.h
    cw/CWTemplateEngine.h
    keyers/ICWKeyerDevice.h
    keyers/KeyerConfig.h
    keyers/KeyerController.h
    keyers/KeyerFactory.h
    keyers/WinKeyerDevice.h
    keyers/HaliKeySerialDevice.h
    keyers/HaliKeyMidiDevice.h
    keyers/IambicKeyer.h
    audio/SidetoneGenerator.h
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
    controllers/BandSwitchingManager.h
    controllers/AmplifierController.h
    controllers/RotatorController.h
    models/QSO.h
    models/StationInfo.h
    ui/MainWindow.h
    ui/models/QSOTableModel.h
    ui/managers/SettingsManager.h
    ui/managers/MenuManager.h
    ui/managers/WindowManager.h
    ui/helpers/WindowActivationHelper.h
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
    ui/dialogs/QSOSearchDialog.h
    ui/dialogs/SO2RConfigDialog.h
    ui/dialogs/RadioEditDialog.h
    ui/dialogs/CWOutputEditDialog.h
    cw/CWOutputProfile.h
    ui/dialogs/KeyerSetupDialog.h
    ui/widgets/BandSummaryGrid.h
    ui/widgets/BandMapWidget.h
    ui/widgets/CivAddressWidget.h
    ui/widgets/RadioControlWidget.h
    ui/widgets/SMeterWidget.h
    ui/widgets/MultiplierWidget.h
    ui/widgets/DXClusterWindow.h
    ui/statistics/StatisticsWindow.h
    ui/statistics/RateCalculator.h
    ui/statistics/RateDisplayWidget.h
    ui/statistics/BandSummaryModel.h
    ui/widgets/NeedsDisplayWidget.h
    ui/widgets/QSOSearchPanel.h
    ui/widgets/SvgPanelWidget.h
    ui/windows/AmplifierControlWindow.h
    ui/panels/IAmplifierPanelController.h
    ui/panels/KPA1500PanelController.h
    ui/panels/AmplifierPanelFactory.h
    ui/plots/qcustomplot.h
)

# Conditionally add panadapter headers (requires Qt 6.6+)
if(PANADAPTER_SUPPORTED)
    list(APPEND HEADERS
        ui/windows/PanadapterWindow.h
        panadapter/PanadapterTypes.h
        panadapter/PanadapterDataModel.h
        panadapter/IPanadapterRenderer.h
        panadapter/renderers/QmlPanadapterRenderer.h
        panadapter/WaterfallImageProvider.h
        panadapter/WaterfallRhiItem.h
        panadapter/SpectrumRhiItem.h
        radio/K4PanadapterReader.h
    )
endif()

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
        MACOSX_BUNDLE_SHORT_VERSION_STRING "3.45.78"
        MACOSX_BUNDLE_BUNDLE_VERSION "3.45.78"
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

# Compile GLSL shaders for waterfall QRhi rendering (only if panadapter supported)
if(PANADAPTER_SUPPORTED)
    # qt_add_shaders generates .qsb files for all supported backends (Metal, Vulkan, D3D, GLSL)
    qt_add_shaders(tr4qt "waterfall_shaders"
        PREFIX "/"
        BASE "${CMAKE_SOURCE_DIR}/resources"
        FILES
            ${CMAKE_SOURCE_DIR}/resources/shaders/waterfall.vert
            ${CMAKE_SOURCE_DIR}/resources/shaders/waterfall.frag
            ${CMAKE_SOURCE_DIR}/resources/shaders/spectrum.vert
            ${CMAKE_SOURCE_DIR}/resources/shaders/spectrum.frag
    )
endif()

# Link Qt6 libraries, tr4qt_core object library, and qtkeychain
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
    Qt6::Svg
    Qt6::Xml
    Qt6::Multimedia
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Qml
    qt6keychain  # Secure credential storage (macOS Keychain, Windows Credential Manager)
)

# Conditionally link Qt6::GuiPrivate for panadapter (QRhi requires it)
if(PANADAPTER_SUPPORTED)
    target_link_libraries(tr4qt PRIVATE Qt6::GuiPrivate)
    target_compile_definitions(tr4qt PRIVATE PANADAPTER_ENABLED=1)
endif()

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

# macOS: SystemConfiguration framework for PlatformUtils (NetBIOS name lookup)
if(APPLE)
    target_link_libraries(tr4qt PRIVATE "-framework SystemConfiguration")
endif()

# RtMidi platform-specific configuration (for HaliKey MIDI support)
if(APPLE)
    target_compile_definitions(tr4qt PRIVATE __MACOSX_CORE__)
    target_link_libraries(tr4qt PRIVATE
        "-framework CoreMIDI"
        "-framework CoreAudio"
        "-framework CoreFoundation"
    )
elseif(WIN32)
    target_compile_definitions(tr4qt PRIVATE __WINDOWS_MM__)
    target_link_libraries(tr4qt PRIVATE winmm setupapi user32)
else()
    target_compile_definitions(tr4qt PRIVATE __LINUX_ALSA__)
    # Linux: libasound2-dev required for ALSA MIDI
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            target_include_directories(tr4qt PRIVATE ${ALSA_INCLUDE_DIRS})
            target_link_libraries(tr4qt PRIVATE ${ALSA_LIBRARIES})
        else()
            target_link_libraries(tr4qt PRIVATE asound)
        endif()
    else()
        target_link_libraries(tr4qt PRIVATE asound)
    endif()
endif()

# Set include directories
target_include_directories(tr4qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${qtkeychain_SOURCE_DIR}
    ${qtkeychain_BINARY_DIR}
)

# Suppress deprecation warnings for third-party code
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set_source_files_properties(ui/plots/qcustomplot.cpp PROPERTIES
        COMPILE_FLAGS "-Wno-deprecated-declarations"
    )
    set_source_files_properties(3rdparty/rtmidi/RtMidi.cpp PROPERTIES
        COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-unused-parameter"
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

    # Re-sign the entire app bundle after bundling frameworks.
    # macOS Tahoe (26.x) rejects loading frameworks whose ad-hoc signature
    # differs from the main binary's (SIGABRT: "different Team IDs").
    # Using Developer ID keeps a stable identity so macOS Keychain doesn't
    # prompt for credentials on every rebuild.
    add_custom_command(TARGET tr4qt POST_BUILD
        COMMAND codesign --force --deep --sign "Developer ID Application" $<TARGET_BUNDLE_DIR:tr4qt>
        COMMENT "Re-signing app bundle (macOS Tahoe Team ID fix)"
        VERBATIM
    )
endif()

# ============================================================================
# TR4QT Headless Server (tr4qt_server)
# ============================================================================
# A headless binary that exposes the WebServer API without MainWindow.
# Validates architecture modularity and enables automated testing.

# Server sources - headless components only (no UI)
set(SERVER_SOURCES
    server/main_server.cpp
    # Qt Resources (schema.sql, etc.)
    ../resources/resources.qrc
    # Utils (no UI dependencies)
    utils/AppSettings.cpp
    utils/ArrlSectionHelper.cpp
    utils/CallsignValidator.cpp
    utils/CountryFile.cpp
    utils/CredentialStore.cpp
    utils/GeographicUtils.cpp
    utils/PathManager.cpp
    utils/PlatformUtils.cpp
    utils/ADIFExporter.cpp
    utils/CabrilloExporter.cpp
    # Network
    network/WebServer.cpp
    network/WebServerContext.cpp
    network/UdpBroadcaster.cpp
    network/UdpBroadcastManager.cpp
    network/RadioInfo.cpp
    network/ContactInfo.cpp
    # Data layer
    data/Database.cpp
    data/GlobalDatabase.cpp
    data/DXCCRepository.cpp
    data/QSORepository.cpp
    data/ContestRepository.cpp
    data/ExchangeMemoryRepository.cpp
    data/BackupManager.cpp
    # Exchanges
    exchanges/InitialExchangeManager.cpp
    exchanges/SmartExchangeParser.cpp
    # Contests (all contest types for full compatibility)
    contests/ContestRegistry.cpp
    contests/ContestMetadata.cpp
    contests/QSOPartyContestBase.cpp
    contests/CQWWContest.cpp
    contests/CQWWBase.cpp
    contests/CQWWCWContest.cpp
    contests/CQWWSSBContest.cpp
    contests/CQWPXContest.cpp
    contests/CQWPXBase.cpp
    contests/CQWPXCWContest.cpp
    contests/CQWPXSSBContest.cpp
    contests/ARRLDXContest.cpp
    contests/ARRLDXBase.cpp
    contests/ARRLDXCWContest.cpp
    contests/ARRLDXPhoneContest.cpp
    contests/ARRLRTTYRoundupContest.cpp
    contests/ARRLSweepstakesContest.cpp
    contests/ARRLSSBase.cpp
    contests/ARRLSSCWContest.cpp
    contests/ARRLSSSSBContest.cpp
    contests/NAQPBase.cpp
    contests/NAQPCWContest.cpp
    contests/NAQPSSBContest.cpp
    contests/NAQPRTTYContest.cpp
    contests/WinterFieldDayContest.cpp
    contests/ARRLFieldDayContest.cpp
    contests/FloridaQSOPartyContest.cpp
    contests/GeneralLoggingContest.cpp
    contests/IARUHFContest.cpp
    # Controllers (headless-safe)
    controllers/QSOLogger.cpp
    controllers/ContestManager.cpp
    controllers/DataIntegrityManager.cpp
    # Note: RadioManager excluded - uses DialogHelper (QtWidgets)
    # Services (headless-safe)
    services/QSOPersistenceService.cpp
    services/ExchangeMemoryService.cpp
    services/QSOLoggingCoordinator.cpp
    services/QSOLoggingService.cpp
    services/ScoreCalculationService.cpp
)

# Create headless server executable
add_executable(tr4qt_server ${SERVER_SOURCES})

# Server links to core library and Qt (no Widgets!)
target_link_libraries(tr4qt_server PRIVATE
    tr4qt_core  # Radio, logging, types
    Qt6::Core
    Qt6::Sql
    Qt6::Network
    Qt6::HttpServer
    Qt6::SerialPort
    ${HAMLIB_LIBRARIES}
    qt6keychain  # Secure credential storage
)

# Server include directories
target_include_directories(tr4qt_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAMLIB_INCLUDE_DIRS}
    ${qtkeychain_SOURCE_DIR}
    ${qtkeychain_BINARY_DIR}
)
