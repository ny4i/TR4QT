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
    Test
)

# Optional: EntryPoint for Windows GUI applications (Qt 6.10+)
find_package(Qt6 COMPONENTS EntryPoint QUIET)


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

# Installation rules
install(TARGETS tr4qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
