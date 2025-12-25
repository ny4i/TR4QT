cmake_minimum_required(VERSION 3.16)
project(tr4qt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Sql
    Network
    PrintSupport
    Test
)

# Use Hamlib from source build
set(HAMLIB_ROOT "$ENV{HOME}/projects/Hamlib/hamlib_install")
set(HAMLIB_INCLUDE_DIRS "${HAMLIB_ROOT}/include")
set(HAMLIB_LIBRARIES "${HAMLIB_ROOT}/lib/libhamlib.dylib")

# Check if Hamlib is built
if(NOT EXISTS "${HAMLIB_LIBRARIES}")
    message(WARNING "Hamlib not found at ${HAMLIB_LIBRARIES}")
    message(WARNING "Please build Hamlib first:")
    message(WARNING "  cd ~/projects/Hamlib/build && make && make install")
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
