cmake_minimum_required(VERSION 3.21)

set(SMM_APP_VERSION "0.4.8" CACHE STRING "Application release version")
if(NOT SMM_APP_VERSION MATCHES "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)$")
    message(FATAL_ERROR "SMM_APP_VERSION must be a stable major.minor.patch version")
endif()

# C is enabled because vendored miniz is a C library; everything first-party is C++17.
project(sekiro_mod_manager VERSION ${SMM_APP_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_GUI "Build the Qt 6 / QML HuskarUI desktop application" ON)
option(SMM_WERROR "Treat compiler warnings as errors" OFF)
option(SMM_BUILD_TESTS "Build headless regression tests" OFF)

# /utf-8 is required because HuskarUI's sources contain CJK string literals.
# NOMINMAX keeps <windows.h> from defining min/max macros that break std::min.
if(MSVC)
    add_compile_options(/utf-8 /EHsc /Zc:__cplusplus)
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS WIN32_WINNT=0x0A00)
endif()

# Warning policy applies to first-party targets only; third-party targets opt out with /w.
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
#   src/core : pure C++17 static library (smm_core). Never links Qt, never prints.
#   src/gui  : native Qt 6 Quick / QML (HuskarUI) desktop app linking smm_core directly.
#   tests/   : core, GUI-headless, website, and worker test suites.
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/patch_huskarui.py")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/patch_huskarui.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_QUIET
    )
endif()

include(CTest)
if(SMM_BUILD_TESTS)
    set(BUILD_TESTING ON CACHE BOOL "" FORCE)
    enable_testing()
endif()

add_subdirectory(src/core)

if(BUILD_GUI)
    add_subdirectory(src/gui)
endif()

if(SMM_BUILD_TESTS OR BUILD_TESTING)
    foreach(test_name doctor_tests core_business_tests)
        add_executable(smm_${test_name} tests/core/${test_name}.cpp)
        target_link_libraries(smm_${test_name} PRIVATE smm_core smm::warnings)
        add_test(NAME smm_${test_name} COMMAND smm_${test_name})
    endforeach()
endif()

message(STATUS "sekiro_mod_manager ${PROJECT_VERSION}: BUILD_GUI=${BUILD_GUI} SMM_BUILD_TESTS=${SMM_BUILD_TESTS}")
