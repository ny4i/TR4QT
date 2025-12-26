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
    utils/DXClusterListDownloader.cpp
    utils/SelectableMessageBox.cpp
    utils/GeographicUtils.cpp
    utils/LOTWUserDownloader.cpp
    utils/RadioEnumerator.cpp
    utils/ThemeManager.cpp
    radio/HamlibRadio.cpp
    radio/RadioController.cpp
    network/TelnetClient.cpp
    network/RadioInfo.cpp
    network/ContactInfo.cpp
    network/UdpBroadcaster.cpp
    network/UdpBroadcastManager.cpp
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
    contests/ARRLDXContest.cpp
    contests/ARRLRTTYRoundupContest.cpp
    contests/ARRLSweepstakesContest.cpp
    contests/ContestRegistry.cpp
    contests/CQWWContest.cpp
    contests/CQWPXContest.cpp
    contests/NAQPContest.cpp
    contests/WinterFieldDayContest.cpp
    logging/LogLevel.cpp
    logging/LogFormatter.cpp
    logging/FileAppender.cpp
    logging/Logger.cpp
    ui/MainWindow.cpp
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
    ui/widgets/BandSummaryGrid.cpp
    ui/widgets/BandMapWidget.cpp
    ui/widgets/RadioControlWidget.cpp
    ui/widgets/SMeterWidget.cpp
    ui/widgets/MultiplierWidget.cpp
    ui/widgets/DXClusterWindow.cpp
    ui/widgets/StatisticsWindow.cpp
    ui/plots/qcustomplot.cpp
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
    ui/widgets/BandSummaryGrid.h
    ui/widgets/BandMapWidget.h
    ui/widgets/RadioControlWidget.h
    ui/widgets/SMeterWidget.h
    ui/widgets/MultiplierWidget.h
    ui/widgets/DXClusterWindow.h
    ui/widgets/StatisticsWindow.h
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
    set_target_properties(tr4qt PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "TR4QT"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.tr4qt.TR4QT"
        MACOSX_BUNDLE_ICON_FILE "tr4qt.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "2.13.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "2.13.0"
        RESOURCE "${MACOSX_ICON_FILE}"
    )
else()
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
    Qt6::PrintSupport
    Qt6::Concurrent
)

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
