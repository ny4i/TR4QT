# TR4QT Source Files

set(SOURCES
    main.cpp
    core/Types.cpp
    utils/ADIFExporter.cpp
    utils/AppSettings.cpp
    utils/ArrlSectionHelper.cpp
    utils/CabrilloExporter.cpp
    utils/CountryFile.cpp
    utils/CountryFileDownloader.cpp
    utils/K4Discovery.cpp
    utils/DialogHelper.cpp
    utils/DXClusterListDownloader.cpp
    utils/SelectableMessageBox.cpp
    utils/GeographicUtils.cpp
    utils/LOTWUserDownloader.cpp
    utils/MapDataProvider.cpp
    utils/RadioEnumerator.cpp
    utils/ThemeManager.cpp
    radio/HamlibRadio.cpp
    radio/RadioController.cpp
    network/TelnetClient.cpp
    network/RadioInfo.cpp
    network/ContactInfo.cpp
    network/UdpBroadcaster.cpp
    network/UdpBroadcastManager.cpp
    network/WebServer.cpp
    data/Database.cpp
    data/GlobalDatabase.cpp
    data/QSORepository.cpp
    data/LOTWUserRepository.cpp
    data/SpotRepository.cpp
    data/BackupManager.cpp
    data/ExchangeMemoryRepository.cpp
    data/DXCCRepository.cpp
    exchanges/InitialExchangeManager.cpp
    exchanges/SmartExchangeParser.cpp
    importers/ADIFParser.cpp
    importers/ADIFFieldMapper.cpp
    importers/ADIFImporter.cpp
    cw/HamlibCWSender.cpp
    cw/CWSenderFactory.cpp
    contests/ARRLDXContest.cpp
    contests/ARRLRTTYRoundupContest.cpp
    contests/ARRLSweepstakesContest.cpp
    contests/ContestRegistry.cpp
    contests/CQWWContest.cpp
    contests/CQWPXContest.cpp
    contests/NAQPContest.cpp
    contests/WinterFieldDayContest.cpp
    contests/ARRLFieldDayContest.cpp
    contests/IARUHFContest.cpp
    logging/LogLevel.cpp
    logging/LogFormatter.cpp
    logging/FileAppender.cpp
    logging/Logger.cpp
    ui/MainWindow.cpp
    ui/NativeMapViewer.cpp
    ui/models/QSOTableModel.cpp
    ui/dialogs/RadioConfigDialog.cpp
    ui/dialogs/PreferencesDialog.cpp
    ui/dialogs/OperatorDialog.cpp
    ui/dialogs/ContestChooserDialog.cpp
    ui/dialogs/BackupRestoreDialog.cpp
    ui/dialogs/EditQSODialog.cpp
    ui/dialogs/ExportPreviewDialog.cpp
    ui/dialogs/SendMorseDialog.cpp
    ui/dialogs/MacroEditDialog.cpp
    ui/dialogs/ADIFImportDialog.cpp
    ui/widgets/BandSummaryGrid.cpp
    ui/widgets/BandMapWidget.cpp
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
    utils/CountryFile.h
    utils/CountryFileDownloader.h
    utils/K4Discovery.h
    utils/DialogHelper.h
    utils/DXClusterListDownloader.h
    utils/SelectableMessageBox.h
    utils/GeographicUtils.h
    utils/LOTWUserDownloader.h
    utils/RadioEnumerator.h
    utils/ThemeManager.h
    radio/RadioInterface.h
    radio/HamlibRadio.h
    radio/RadioController.h
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
    exchanges/InitialExchangeManager.h
    exchanges/SmartExchangeParser.h
    importers/ADIFParser.h
    importers/ADIFFieldMapper.h
    importers/ADIFImporter.h
    cw/CWSender.h
    cw/HamlibCWSender.h
    cw/CWSenderFactory.h
    contests/ARRLDXContest.h
    contests/ARRLRTTYRoundupContest.h
    contests/ARRLSweepstakesContest.h
    contests/ContestBase.h
    contests/ContestMetadata.h
    contests/ContestRegistry.h
    contests/CQWWContest.h
    contests/CQWPXContest.h
    contests/NAQPContest.h
    contests/WinterFieldDayContest.h
    contests/ARRLFieldDayContest.h
    contests/IARUHFContest.h
    logging/LogLevel.h
    logging/LogFormatter.h
    logging/FileAppender.h
    logging/Logger.h
    logging/LogMacros.h
    models/QSO.h
    models/StationInfo.h
    ui/MainWindow.h
    ui/models/QSOTableModel.h
    ui/dialogs/RadioConfigDialog.h
    ui/dialogs/PreferencesDialog.h
    ui/dialogs/OperatorDialog.h
    ui/dialogs/ContestChooserDialog.h
    ui/dialogs/BackupRestoreDialog.h
    ui/dialogs/EditQSODialog.h
    ui/dialogs/ExportPreviewDialog.h
    ui/dialogs/SendMorseDialog.h
    ui/dialogs/MacroEditDialog.h
    ui/dialogs/ADIFImportDialog.h
    ui/widgets/BandSummaryGrid.h
    ui/widgets/BandMapWidget.h
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
        MACOSX_BUNDLE_SHORT_VERSION_STRING "3.8.1"
        MACOSX_BUNDLE_BUNDLE_VERSION "3.8.1"
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

# Link Qt6 libraries
target_link_libraries(tr4qt PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::HttpServer
    Qt6::PrintSupport
    Qt6::Concurrent
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
