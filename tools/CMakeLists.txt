# TR4QT Tools
#
# Utility programs for testing and development

# Manual test program for PSTRotator with physical hardware
add_executable(rotator_test_manual
    rotator_test_manual.cpp
)

# Link against tr4qt_core object library (provides rotator classes)
target_link_libraries(rotator_test_manual PRIVATE
    tr4qt_core  # Object library with rotator classes, logging, types
    Qt6::Core
    Qt6::Network
)

# Add include directories
target_include_directories(rotator_test_manual PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
