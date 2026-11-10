add_executable(smm
    src/main.cpp
    src/cli_parser.cpp
    src/json_output.cpp
    src/table.cpp
    src/terminal.cpp
    src/resolve.cpp
    src/commands/cmd_mods.cpp
    src/commands/cmd_deploy.cpp
    src/commands/cmd_import.cpp
    src/commands/cmd_doctor.cpp
    src/commands/cmd_presets.cpp
    src/commands/cmd_export.cpp
)

target_include_directories(smm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(smm PRIVATE SMM_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(smm PRIVATE smm_core smm::warnings)
set_target_properties(smm PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# The CLI runs elevated on every start, matching smm_gui (see src/gui/CMakeLists.txt).
#
# Why a linker switch instead of a checked-in .manifest: link.exe already synthesises a manifest
# containing <requestedExecutionLevel level="asInvoker"/>, so feeding a second manifest through
# /MANIFESTINPUT makes the manifest tool merge two requestedExecutionLevel elements and fail the
# link (c1010001: "Values of attribute \"level\" not equal in different manifest snippets",
# surfaced as LNK1327). /MANIFESTUAC rewrites that single element in place.
#
# Consequence worth knowing before you debug "smm does nothing": a process started by
# CreateProcess (cmd, PowerShell, VS Code tasks, Python subprocess) CANNOT raise UAC - the call
# fails with ERROR_ELEVATION_REQUIRED (740) and no prompt appears. UAC only appears for shell
# launches (Explorer, `start smm ...`, ShellExecuteEx with the runas verb) or when the parent is
# already elevated. An already-elevated console therefore still runs `smm` directly.
if(WIN32 AND MSVC)
    target_link_options(smm PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
endif()

# Keep the CLI alias stable for build scripts.
add_executable(smm-cli ALIAS smm)
