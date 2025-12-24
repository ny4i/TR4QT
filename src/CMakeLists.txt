# TR4QT Source Files

set(SOURCES
    main.cpp
    core/Types.cpp
    utils/CountryFile.cpp
    utils/CountryFileDownloader.cpp
    utils/AppSettings.cpp
    utils/RadioEnumerator.cpp
    utils/ADIFExporter.cpp
    utils/CabrilloExporter.cpp
    radio/HamlibRadio.cpp
    radio/RadioController.cpp
    data/Database.cpp
    data/QSORepository.cpp
    contests/CQWWContest.cpp
    contests/CQWPXContest.cpp
    contests/WinterFieldDayContest.cpp
    ui/MainWindow.cpp
    ui/models/QSOTableModel.cpp
    ui/dialogs/RadioConfigDialog.cpp
    ui/dialogs/PreferencesDialog.cpp
    ui/dialogs/ContestChooserDialog.cpp
    ui/widgets/BandSummaryGrid.cpp
    ui/widgets/BandMapWidget.cpp
    ui/widgets/RadioControlWidget.cpp
    ui/widgets/MultiplierWidget.cpp
)

set(HEADERS
    core/Types.h
    core/Constants.h
    utils/CountryFile.h
    utils/CountryFileDownloader.h
    utils/AppSettings.h
    utils/RadioEnumerator.h
    utils/ADIFExporter.h
    utils/CabrilloExporter.h
    radio/RadioInterface.h
    radio/HamlibRadio.h
    radio/RadioController.h
    data/Database.h
    data/QSORepository.h
    contests/ContestBase.h
    contests/CQWWContest.h
    contests/CQWPXContest.h
    contests/WinterFieldDayContest.h
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
)

# Create executable
add_executable(tr4qt
    ${SOURCES}
    ${HEADERS}
)

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
