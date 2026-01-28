# Patch qtkeychain CMakeLists.txt to fix MinGW build
# Bug: qtkeychain 0.15.0 uses /utf-8 (MSVC-only flag) unconditionally on Windows
# Fix: Wrap /utf-8 in if(MSVC) so MinGW doesn't try to use it

if(NOT DEFINED FILE)
    message(FATAL_ERROR "FILE variable must be set")
endif()

file(READ "${FILE}" content)

# Replace:
#     add_definitions( /utf-8 -DUNICODE )
# With:
#     if(MSVC)
#         add_definitions( /utf-8 )
#     endif()
#     add_definitions( -DUNICODE )
string(REPLACE
    "add_definitions( /utf-8 -DUNICODE )"
    "if(MSVC)\n        add_definitions( /utf-8 )\n    endif()\n    add_definitions( -DUNICODE )"
    content "${content}"
)

file(WRITE "${FILE}" "${content}")
message(STATUS "Patched qtkeychain CMakeLists.txt for MinGW compatibility")
