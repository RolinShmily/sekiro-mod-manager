cmake_minimum_required(VERSION 3.20)
# C is enabled because the vendored miniz is a C library; everything first-party is C++17.
project(sekiro_mod_manager VERSION 0.3.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_CLI "Build the smm command line tool" ON)
option(BUILD_GUI "Build the Qt/QML HuskarUI GUI" ON)
option(SMM_WERROR "Treat compiler warnings as errors" OFF)

# Language and definition settings that must hold everywhere, third-party code included.
#
# /utf-8 is not a style choice: HuskarUI's sources contain CJK string literals, and without it
# MSVC reads them in the system code page and mangles them. NOMINMAX keeps <windows.h> from
# defining min/max macros that break std::min, and WIN32_WINNT gates the Win32 APIs we call.
if(MSVC)
    add_compile_options(/utf-8 /EHsc /Zc:__cplusplus)
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS WIN32_WINNT=0x0A00)
endif()

# Warning policy applies to OUR code only, so it lives on an interface target our targets opt into.
# add_compile_options() is directory-scoped and inherited by every subdirectory added afterwards,
# which silently put vendor code under /W4: a clean build then printed ~100 unused-parameter
# warnings from HuskarUI and buried anything we actually wrote. Third-party targets get /W0
# at the point they are declared instead - see third_party/CMakeLists.txt and src/gui.
add_library(smm_warnings INTERFACE)
add_library(smm::warnings ALIAS smm_warnings)
if(MSVC)
    target_compile_options(smm_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(smm_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()
if(SMM_WERROR)
    target_compile_options(smm_warnings INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/WX>
                                                 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>)
endif()

# ---------------------------------------------------------------------------
# Architecture:
#
#   smm_core (static library)
#      ^              ^
#      |              |
#   smm_cli        smm_gui
#
#   src/core : pure C++17 business logic. Never links Qt, never prints.
#   src/cli  : standalone CLI tool and machine readable (--json) API.
#   src/gui  : native Qt 6 Quick / QML (HuskarUI) GUI linking smm_core directly
#              for zero-overhead native performance.
# ---------------------------------------------------------------------------
# Automatically apply compatibility & stability patches to third-party submodules if present
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/patch_huskarui.py")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/patch_huskarui.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_QUIET
    )
endif()

add_subdirectory(third_party)
add_subdirectory(src/core)

if(BUILD_CLI)
    add_subdirectory(src/cli)
endif()

if(BUILD_GUI)
    add_subdirectory(src/gui)
endif()

message(STATUS "sekiro_mod_manager configured: BUILD_CLI=${BUILD_CLI} BUILD_GUI=${BUILD_GUI}")
