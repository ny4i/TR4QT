# TR4QT Source Files

set(SOURCES
    main.cpp
    core/Types.cpp
    utils/CountryFile.cpp
    utils/CountryFileDownloader.cpp
    utils/AppSettings.cpp
    utils/ThemeManager.cpp
    utils/RadioEnumerator.cpp
    utils/ADIFExporter.cpp
    utils/CabrilloExporter.cpp
    radio/HamlibRadio.cpp
    radio/RadioController.cpp
    network/TelnetClient.cpp
    network/RadioInfo.cpp
    network/ContactInfo.cpp
    network/UdpBroadcaster.cpp
    network/UdpBroadcastManager.cpp
    data/Database.cpp
    data/QSORepository.cpp
    contests/ContestRegistry.cpp
    contests/CQWWContest.cpp
    contests/CQWPXContest.cpp
    contests/WinterFieldDayContest.cpp
    logging/LogLevel.cpp
    logging/LogFormatter.cpp
    logging/FileAppender.cpp
    logging/Logger.cpp
    ui/MainWindow.cpp
    ui/models/QSOTableModel.cpp
    ui/dialogs/RadioConfigDialog.cpp
    ui/dialogs/PreferencesDialog.cpp
    ui/dialogs/ContestChooserDialog.cpp
    ui/widgets/BandSummaryGrid.cpp
    ui/widgets/BandMapWidget.cpp
    ui/widgets/RadioControlWidget.cpp
    ui/widgets/MultiplierWidget.cpp
    ui/widgets/DXClusterWindow.cpp
)

set(HEADERS
    core/Types.h
    core/Constants.h
    utils/CountryFile.h
    utils/CountryFileDownloader.h
    utils/AppSettings.h
    utils/ThemeManager.h
    utils/RadioEnumerator.h
    utils/ADIFExporter.h
    utils/CabrilloExporter.h
    radio/RadioInterface.h
    radio/HamlibRadio.h
    radio/RadioController.h
    network/TelnetClient.h
    network/RadioInfo.h
    network/ContactInfo.h
    network/UdpBroadcaster.h
    network/UdpBroadcastManager.h
    data/Database.h
    data/QSORepository.h
    contests/ContestBase.h
    contests/ContestMetadata.h
    contests/ContestRegistry.h
    contests/CQWWContest.h
    contests/CQWPXContest.h
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
    ui/dialogs/ContestChooserDialog.h
    ui/widgets/BandSummaryGrid.h
    ui/widgets/BandMapWidget.h
    ui/widgets/RadioControlWidget.h
    ui/widgets/MultiplierWidget.h
    ui/widgets/DXClusterWindow.h
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
