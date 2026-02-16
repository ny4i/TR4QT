cmake_minimum_required(VERSION 3.16)
project(tr4qt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS deployment target - support macOS 11.0 (Big Sur) and later
# This ensures compatibility with Sonoma (14.x) and earlier versions
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)

    # Set up proper rpath for app bundle
    # This ensures Qt frameworks and libraries load from the bundled Frameworks directory
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
    # Use FALSE so macdeployqt can find the original Homebrew paths
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

# Qt6 setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Sql
    Network
    HttpServer
    PrintSupport
    Concurrent
    SerialPort
    Svg
    Xml
    Multimedia
    Test
    Quick
    QuickWidgets
    Qml
    ShaderTools
)

# Find Qt6 private modules for QRhi (required for WaterfallRhiItem panadapter)
# GuiPrivate may not be available on all platforms (e.g., Debian without qt6-base-private-dev)
find_package(Qt6 COMPONENTS GuiPrivate QUIET)
if(NOT Qt6GuiPrivate_FOUND)
    message(STATUS "Qt6::GuiPrivate not found - panadapter will be disabled")
endif()

# qtkeychain - Secure credential storage (macOS Keychain, Windows Credential Manager)
# Built from source via FetchContent for zero deployment overhead (static link)
include(FetchContent)

# qtkeychain 0.15.0 has a bug: it uses /utf-8 (MSVC-only flag) unconditionally on Windows.
# MinGW interprets /utf-8 as a filename, causing "linker input file not found" error.
# We patch the CMakeLists.txt during fetch to wrap /utf-8 in if(MSVC).
if(MINGW)
    set(_QTKEYCHAIN_PATCH_COMMAND
        ${CMAKE_COMMAND} -E env
        ${CMAKE_COMMAND} -DFILE=<SOURCE_DIR>/qtkeychain/CMakeLists.txt
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_qtkeychain.cmake
    )
else()
    set(_QTKEYCHAIN_PATCH_COMMAND "")
endif()

FetchContent_Declare(qtkeychain
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
    GIT_TAG 0.15.0
    PATCH_COMMAND ${_QTKEYCHAIN_PATCH_COMMAND}
)
# Save and override BUILD_SHARED_LIBS for qtkeychain only (static link = no deployment overhead)
set(_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
set(BUILD_TEST_APPLICATION OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(qtkeychain)
set(BUILD_SHARED_LIBS ${_SAVED_BUILD_SHARED_LIBS})

# Hamlib configuration
# HAMLIB_ROOT can be passed via -DHAMLIB_ROOT=path or will use platform defaults
if (NOT DEFINED HAMLIB_ROOT)
    if (WIN32)
        set(HAMLIB_ROOT "C:/projects/hamlib")
    elseif(APPLE)
        set(HAMLIB_ROOT "$ENV{HOME}/projects/hamlib")
    else()
        # Linux: check system location first, then user location
        if (EXISTS "/usr/include/hamlib")
            set(HAMLIB_ROOT "/usr")
        else()
            set(HAMLIB_ROOT "$ENV{HOME}/projects/hamlib")
        endif()
    endif()
endif()

# Set include directory
if (EXISTS "${HAMLIB_ROOT}/include/hamlib")
    set(HAMLIB_INCLUDE_DIRS "${HAMLIB_ROOT}/include")
elseif (EXISTS "${HAMLIB_ROOT}/include")
    set(HAMLIB_INCLUDE_DIRS "${HAMLIB_ROOT}/include")
endif()

# Find the library file based on platform
if (WIN32)
    # Windows: prefer .dll.a (import library), fallback to .lib
    if (EXISTS "${HAMLIB_ROOT}/lib/gcc/libhamlib.dll.a")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/gcc/libhamlib.dll.a")
    elseif (EXISTS "${HAMLIB_ROOT}/lib/libhamlib.dll.a")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/libhamlib.dll.a")
    elseif (EXISTS "${HAMLIB_ROOT}/lib/hamlib.lib")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/hamlib.lib")
    endif()
elseif(APPLE)
    # macOS: .dylib
    # Check installed location first, then build location
    if (EXISTS "${HAMLIB_ROOT}/hamlib_install/lib/libhamlib.dylib")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/hamlib_install/lib/libhamlib.dylib")
        set(HAMLIB_INCLUDE_DIRS "${HAMLIB_ROOT}/hamlib_install/include")
    elseif (EXISTS "${HAMLIB_ROOT}/lib/libhamlib.dylib")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/libhamlib.dylib")
    endif()
else()
    # Linux: .so or system library
    if (EXISTS "${HAMLIB_ROOT}/lib/libhamlib.so")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/libhamlib.so")
    elseif (EXISTS "${HAMLIB_ROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/libhamlib.so")
        set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/libhamlib.so")
    else()
        # Fallback: let the linker find it
        set(HAMLIB_LIBRARIES "hamlib")
    endif()
endif()

message(STATUS "HAMLIB_ROOT: ${HAMLIB_ROOT}")
message(STATUS "HAMLIB_INCLUDE_DIRS: ${HAMLIB_INCLUDE_DIRS}")
message(STATUS "HAMLIB_LIBRARIES: ${HAMLIB_LIBRARIES}")



# Check if Hamlib is built
if(NOT EXISTS "${HAMLIB_LIBRARIES}")
    message(WARNING "Hamlib not found at ${HAMLIB_LIBRARIES}")
    message(WARNING "Please build/install Hamlib for this platform and toolchain.")
endif()

# Add source directory
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Add tools directory
add_subdirectory(tools)

# Installation rules
install(TARGETS tr4qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
