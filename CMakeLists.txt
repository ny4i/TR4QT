cmake_minimum_required(VERSION 3.16)
project(tr4qt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    PrintSupport
    Concurrent
    Test
)


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
    if (EXISTS "${HAMLIB_ROOT}/lib/libhamlib.dylib")
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
